#include <teaql/runtime/entity_save.hpp>
#include <teaql/runtime/dynamic_fields.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct ObservingChecker final : Checker {
    using allocator_type = Allocator;
    String name;
    mutable Vector<String> paths;
    mutable Vector<Record> inputs;
    bool deny = false;
    bool fix = false;

    ObservingChecker(std::allocator_arg_t, Allocator a, std::string_view name)
        : name(name, a.resource()), paths(a.resource()), inputs(a.resource()) {}

    std::string_view entity() const override {
        return name;
    }

    void check_and_fix(const UserContext& user_context,
                       EntityValues& values,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        auto r = user_context.memory_resource();
        paths.emplace_back(location.model_path(r));
        inputs.emplace_back(values.fields);
        if (deny) {
            results.emplace_back(
                CheckResult::required(r, detail::clone_in(r, location).member("name")));
        }
        if (fix) {
            values.fields.insert_or_assign(String("name", r), Value(r, "fixed"));
            FixEvidence evidence(std::allocator_arg,
                                 Allocator(r),
                                 name,
                                 detail::clone_in(r, location).member("name").model_path(r),
                                 FixEvidenceSource::Context,
                                 "pending-dynamic-fixture");
            user_context.record_fix_evidence(evidence);
        }
    }
};

static EntityRuntimeState&
changed(UserContext& user_context, std::string_view name, std::uint64_t id, Value parent) {
    auto r = user_context.memory_resource();
    auto& state = user_context.make<EntityRuntimeState>();
    Record record(r);
    record.emplace(String("id", r), Value(r, std::int64_t(id)));
    record.emplace(String("version", r), Value(r, std::int64_t(3)));
    record.emplace(String("name", r), Value(r, "original"));
    record.emplace(String("parent_id", r), std::move(parent));
    state.set_original_compact_row(name, CompactRow(std::allocator_arg, Allocator(r), record));
    const std::array fields{DynamicFieldDefinitions::Field{"score", DataType::I64}};
    auto& definition = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, name, "v1", fields)).value());
    Value::Object original(r);
    original.emplace(String("score", r), Value(r, std::int64_t(4)));
    auto value = DynamicFieldValues::from_values(r, definition, original);
    state.install_loaded_dynamic_fields(value.value());
    EntityKey key(std::allocator_arg, Allocator(r), name, Value(r, id));
    check(bool(state.update_dynamic_field(key, "score", Value(r, std::int64_t(5)))),
          "fixture records extension-only edit");
    return state;
}

static void relation_and_unrepresented_checks(bool deny) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    for (auto name : {"Parent", "Child", "Extra"}) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), name);
        descriptor.property("id", DataType::U64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        descriptor.property("parent_id", DataType::U64);
        if (std::string_view(name) == "Parent") {
            auto& relation = descriptor.relation("children", "Child");
            relation.many = true;
            relation.local_key = "id";
            relation.foreign_key = "parent_id";
        }
        metadata.register_entity(std::move(descriptor));
    }
    user_context->set_metadata(*user_context, metadata);
    auto& registry = user_context->make<InMemoryCheckerRegistry>();
    auto& child_checker = user_context->make_resource<ObservingChecker>("Child");
    auto& extra_checker = user_context->make_resource<ObservingChecker>("Extra");
    child_checker.fix = true;
    extra_checker.fix = !deny;
    extra_checker.deny = deny;
    registry.register_checker(*user_context, child_checker);
    registry.register_checker(*user_context, extra_checker);
    user_context->set_checker_registry(*user_context, registry);
    auto& root = user_context->make<EntityRuntimeState>();
    auto& child = changed(*user_context, "Child", 10, Value(r, std::int64_t(1)));
    auto& extra = changed(*user_context, "Extra", 20, Value(r, std::uint64_t(999)));
    check(bool(detail::EntityRuntimeAccess::compose(root, child)), "compose related child intent");
    check(bool(detail::EntityRuntimeAccess::compose(root, extra)),
          "compose unrepresented extra intent");
    GraphNode node(std::allocator_arg, Allocator(r), "Parent");
    node.value("id", std::uint64_t(1)).value("name", "parent").with_comment("check pending graph");
    EntityRuntimeState::Keys visited(r);
    visited.emplace("Parent", Value(r, std::uint64_t(1)));
    check(bool(detail::hydrate_ledger_relations(*user_context, root, node, visited)),
          "hydrate dynamic-only related child");
    check(node.relations.at(String("children", r)).size() == 1 &&
              node.relations.at(String("children", r))[0].original_values &&
              node.relations.at(String("children", r))[0].dirty_fields->contains("#score"),
          "relation discovery uses original signed FK and retains checker metadata");
    check(bool(detail::preflight_graph(*user_context, node, ObjectLocation::root(r), &root)),
          "preflight represented graph");
    auto result = detail::preflight_pending_dynamic_entities(*user_context, root, node);
    check(bool(result) == !deny && child_checker.paths.size() == 1 &&
              extra_checker.paths.size() == 1,
          "represented child and external dynamic key each run checker once");
    check(child_checker.paths.front() == "children[0]" && extra_checker.paths.front().empty(),
          "represented child uses graph path; unrepresented key uses source root path");
    EntityKey child_key(std::allocator_arg, Allocator(r), "Child", Value(r, std::uint64_t(10)));
    EntityKey extra_key(std::allocator_arg, Allocator(r), "Extra", Value(r, std::uint64_t(20)));
    check(root.get(child_key, "name")->try_text() == "fixed", "related Fix enters original ledger");
    auto loaded = extra_checker.inputs.front().at(String("_loaded_fields", r)).as_list();
    check(loaded && loaded->size() == 4,
          "external checker receives reconstructed native loaded fields");
    if (deny) {
        check(result.error().kind() == RuntimeError::Kind::Check && !root.get(extra_key, "name"),
              "checker denial leaves external native intent untouched");
    } else {
        check(root.get(extra_key, "name")->try_text() == "fixed",
              "external Fix enters original ledger");
    }
    auto plan = detail::ledger_policy_plan(*user_context, node, root);
    check(plan.operations.size() == 2 && plan.operations[0].changed_values.contains("#score") &&
              plan.operations[1].changed_values.contains("#score"),
          "policy includes every dynamic-only key and operation operand");
}

static void deleted_keys_and_null_relations() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
    parent.property("id", DataType::U64).id();
    auto& relation = parent.relation("children", "Child");
    relation.many = true;
    relation.foreign_key = "parent_id";
    metadata.register_entity(std::move(parent));
    EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
    child.property("id", DataType::U64).id();
    child.property("version", DataType::I64).version();
    metadata.register_entity(std::move(child));
    user_context->set_metadata(*user_context, metadata);
    auto& state = changed(*user_context, "Child", 10, Value(r));
    GraphNode empty_parent(std::allocator_arg, Allocator(r), "Parent");
    empty_parent.value("id", Value(r));
    EntityRuntimeState::Keys visited(r);
    check(bool(detail::hydrate_ledger_relations(*user_context, state, empty_parent, visited)) &&
              empty_parent.relations.at(String("children", r)).empty(),
          "two absent relation IDs never match");
    EntityKey key(std::allocator_arg, Allocator(r), "Child", Value(r, std::uint64_t(10)));
    state.mark_as_delete(key);
    GraphNode root(std::allocator_arg, Allocator(r), "Parent");
    root.value("id", std::uint64_t(1));
    check(bool(detail::preflight_pending_dynamic_entities(*user_context, state, root)),
          "deleted dynamic keys do not require update checker reconstruction");
}

struct RecheckingChecker final : Checker {
    using allocator_type = Allocator;
    EntityRuntimeState root;
    std::string_view action;
    mutable Vector<String> seen;

    RecheckingChecker(std::allocator_arg_t,
                      Allocator a,
                      const EntityRuntimeState& root,
                      std::string_view action)
        : root(std::allocator_arg, a, root), action(action), seen(a.resource()) {}

    std::string_view entity() const override {
        return "Order";
    }

    void check_and_fix(const UserContext& user_context,
                       EntityValues& values,
                       const ObjectLocation&,
                       CheckResults&) const override {
        auto r = user_context.memory_resource();
        auto id = *values.fields.at(String("id", r)).try_u64();
        String message(std::to_string(id), r);
        message += ':';
        message += check_object_status_name(check_object_status(values));
        seen.emplace_back(message);
        if (id == 1) {
            EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t{2}));
            if (action == "delete") {
                root.mark_as_delete(key);
            } else if (action == "new") {
                root.mark_as_new(key);
            }
        }
    }
};

static void pending_status_rechecks() {
    GlobalContext global;
    for (auto action : {"none", "delete", "new"}) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& root = user_context->make<EntityRuntimeState>();
        const std::array fields{DynamicFieldDefinitions::Field{"score", DataType::I64}};
        auto definitions = DynamicFieldDefinitions::from_fields(r, "Order", "1", fields);
        auto& owned = user_context->make<DynamicFieldDefinitions>(definitions.value());
        DynamicFieldValues::Values empty(r);
        auto values = DynamicFieldValues::from_values(r, owned, empty);
        root.install_loaded_dynamic_fields(values.value());
        for (std::uint64_t id : {1, 2}) {
            EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, id));
            check(bool(root.update_dynamic_field(key, "score", Value(r, std::int64_t{5}))),
                  "stage pending dynamic status control");
        }
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Order");
        descriptor.property("id", DataType::U64).id();
        metadata.register_entity(std::move(descriptor));
        user_context->set_metadata(*user_context, metadata);
        auto& checker = user_context->make_resource<RecheckingChecker>(root, action);
        auto& registry = user_context->make<InMemoryCheckerRegistry>();
        registry.register_checker(*user_context, checker);
        user_context->set_checker_registry(*user_context, registry);
        GraphNode node(std::allocator_arg, Allocator(r), "Parent");
        auto result = detail::preflight_pending_dynamic_entities(*user_context, root, node);
        check(bool(result), "pending dynamic status checks complete");
        check(checker.seen.size() == (std::string_view(action) == "delete" ? 1u : 2u) &&
                  checker.seen.front() == "1:update",
              "later deleted entities are skipped after an earlier Checker");
        if (checker.seen.size() == 2) {
            check(checker.seen[1] == (std::string_view(action) == "new" ? "2:create" : "2:update"),
                  "later entities use their current ledger creation status");
        }
    }
}

int main() {
    pending_status_rechecks();
    relation_and_unrepresented_checks(false);
    relation_and_unrepresented_checks(true);
    deleted_keys_and_null_relations();
    std::cout << "Pending dynamic preflight contracts passed\n";
}
