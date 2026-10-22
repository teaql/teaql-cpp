#include "../../tests/support/save_preparation_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <iostream>
#include <array>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static EntityDescriptor descriptor(Memory r, std::string_view name) {
    EntityDescriptor out(std::allocator_arg, Allocator(r), name);
    out.property("id", DataType::U64).id();
    out.property("name", DataType::Text);
    return out;
}

static void install(UserContext& user) {
    auto r = user.memory_resource();
    auto& module = user.make<RuntimeModule>();
    auto parent = descriptor(r, "Parent");
    parent.relation("kids", "Child").local("id").foreign("parent_id");
    auto child = descriptor(r, "Child");
    child.relation("leaves", "Leaf").local("id").foreign("parent_id");
    module.descriptor(std::move(parent))
        .descriptor(std::move(child))
        .descriptor(descriptor(r, "Leaf"))
        .apply_to(user);
}

static void extraction_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& model = user->make<save_fixture::Model>();
    model.values.fields.emplace(String("id", r), Value(r, std::uint64_t{1}));
    auto& leaf = user->make<Record>();
    leaf.emplace(String("id", r), Value(r, std::uint64_t{3}));
    leaf.emplace(String("name", r),
                 Value(r, "Long nested value whose characters must survive the source Context"));
    auto& child = user->make<Record>();
    child.emplace(String("id", r), Value(r, std::uint64_t{2}));
    child.emplace(String("leaves", r), Value(r, leaf));
    model.values.fields.emplace(String("kids", r), Value(r, child));
    model.dirty.emplace(r);
    model.dirty->emplace("name");
    model.comment.emplace("Handwritten audited comment retained explicitly", r);
    auto& graph = user->make<Result<GraphNode, RuntimeError>>(
        graph_node_from_entity(*user, detail::clone_in(r, model)));
    check(graph.is_ok() && graph.value()
                                   .relations.at(String("kids", r))
                                   .front()
                                   .relations.at(String("leaves", r))
                                   .size() == 1,
          "public typed extraction recursively builds metadata-controlled relations");
    auto copy = user->make_global<GraphNode>(graph.value());
    auto locations = user->make_global<detail::LedgerObjectLocations>(
        detail::ledger_object_locations(r, graph.value()));
    user.reset();
    auto next = global.make_user_context();
    auto& retained = next->get(next->rebind_global(copy));
    auto& deepest =
        retained.relations.find("kids")->second.front().relations.find("leaves")->second.front();
    check(deepest.values.fields.find("name")->second.try_text()->starts_with("Long nested"),
          "all nested data is explicitly copied into Global before source destruction");
    auto& paths = next->get(next->rebind_global(locations));
    EntityKey key(std::allocator_arg,
                  Allocator(next->memory_resource()),
                  "Leaf",
                  Value(next->memory_resource(), std::uint64_t{3}));
    check(paths.at(key).model_path(next->memory_resource()) == "kids[0].leaves[0]",
          "retained ledger locations preserve model paths");
}

static void preflight_and_partial_failure() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& registry = user->make<InMemoryCheckerRegistry>();
    auto& parent = user->make<save_fixture::Checker>("Parent", "fill");
    auto& child = user->make<save_fixture::Checker>("Child", "error");
    registry.register_checker(*user, parent);
    registry.register_checker(*user, child);
    user->set_checker_registry(*user, registry);
    auto& root = user->make<EntityRuntimeState>();
    auto& node = user->make<GraphNode>("Parent");
    node.value("id", std::uint64_t{1});
    node.dirty_fields.emplace(r);
    node.dirty_fields->emplace("name");
    GraphNode first(std::allocator_arg, Allocator(r), "Child");
    first.value("id", std::uint64_t{2});
    node.relation("kids", first);
    node.relation("kids", first);
    auto result = detail::preflight_graph(*user, node, ObjectLocation::root(r), &root);
    check(!result && parent.calls.size() == 1 && child.calls.size() == 1 &&
              child.calls[0] == "kids[0]",
          "preflight stops on first child failure after completing parent fix");
    check(!node.values.fields.contains(check_object_status_field) &&
              !node.values.fields.contains("_dirty_fields"),
          "normal check errors clear preflight markers");
    EntityKey key(std::allocator_arg, Allocator(r), "Parent", Value(r, std::uint64_t{1}));
    auto changed = root.get(key, "name");
    check(changed && changed->try_text() == "fixed" &&
              result.error().check_results()[0].location.instance_path(r) == "/kids/0/name",
          "parent fix remains in the ledger and child diagnostics retain location");
}

static void ledger_discovery_and_merge() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& root = user->make<EntityRuntimeState>();
    EntityKey child(std::allocator_arg, Allocator(r), "Child", Value(r, std::uint64_t{2}));
    EntityKey leaf(std::allocator_arg, Allocator(r), "Leaf", Value(r, std::uint64_t{3}));
    root.set(child, "parent_id", Value(r, std::uint64_t{1}));
    root.set(child, "name", Value(r, "child fix"));
    root.set_original_version(child, 5);
    root.set(leaf, "parent_id", Value(r, std::uint64_t{2}));
    root.mark_as_new(leaf);
    auto& graph = user->make<GraphNode>("Parent");
    graph.value("id", std::uint64_t{1});
    EntityRuntimeState::Keys visited(r);
    visited.emplace("Parent", Value(r, std::uint64_t{1}));
    check(bool(detail::hydrate_ledger_relations(*user, root, graph, visited)) &&
              visited.size() == 3,
          "ledger changes discover nested children with one shared visited set");
    auto& found = graph.relations.find("kids")->second.front();
    check(found.operation == GraphOperation::Upsert &&
              found.relations.find("leaves")->second.front().operation == GraphOperation::Create,
          "original version and new-key roles select the source graph operations");
    auto& target = user->make<EntityRuntimeState>();
    found.dirty_fields.emplace(r);
    found.dirty_fields->emplace("name");
    found.original_values.emplace(r);
    found.original_values->fields.emplace(String("version", r), Value(r, std::int64_t{5}));
    check(bool(detail::merge_relation_mutations_into_root(target, graph)) &&
              target.get(child, "name") && target.is_new(leaf) &&
              target.get_original_version(child) == 5,
          "relation merge preserves dirty fields, versions and create roles");
    GraphNode broken(std::allocator_arg, Allocator(r), "Child");
    graph.relation("z", broken);
    auto error = detail::merge_relation_mutations_into_root(target, graph);
    check(!error && target.get(child, "name"),
          "later malformed child does not roll back already merged ledger state");
}

struct IndexedSavedModel : EntityDefaults<IndexedSavedModel> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    CompactRow row;
    eval::LoadState load_state;
    unsigned installer_calls = 0;
    bool installed = false;

    IndexedSavedModel(std::allocator_arg_t, Allocator a, CompactRow input)
        : row(std::allocator_arg, a, std::move(input)),
          load_state(row.indexed_load_state(
              detail::ContextArena::from_memory(a.resource()).loaded_snapshots(),
              field_layout(a.resource()))) {}

    IndexedSavedModel(std::allocator_arg_t, Allocator a, const IndexedSavedModel& other)
        : row(std::allocator_arg, a, other.row),
          load_state(std::allocator_arg, a, other.load_state),
          installer_calls(other.installer_calls), installed(other.installed) {}

    IndexedSavedModel(const IndexedSavedModel&) = delete;
    IndexedSavedModel(IndexedSavedModel&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.property("id", DataType::U64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        return descriptor;
    }

    static Result<FieldLayout, String> create_field_layout(Memory r) {
        const std::array<FieldLayout::Index, 3> indexes{{{"id", 0}, {"version", 1}, {"name", 2}}};
        const std::array<FieldLayout::Mapping, 3> mappings{
            {{"id", "id", "id"}, {"version", "version", "version"}, {"name", "name", "name"}}};
        const std::array<std::string_view, 3> members{"id", "version", "name"};
        return FieldLayout::from_generated(
            r, ENTITY_NAME, "saved-dynamic-observer-v1", indexes, mappings, {}, members);
    }

    static const FieldLayout& field_layout(Memory r) {
        return detail::ContextArena::from_memory(r).field_layout<IndexedSavedModel>();
    }

    static Result<IndexedSavedModel, EntityError> from_compact_row(Memory r, CompactRow row) {
        return Result<IndexedSavedModel, EntityError>::ok(
            r, IndexedSavedModel(std::allocator_arg, Allocator(r), std::move(row)));
    }

    MutationValues into_values() && {
        MutationValues values(memory_resource());
        values.fields = row.into_map(memory_resource());
        return values;
    }

    std::optional<LoadedSnapshot::Borrow> loaded_state_snapshot() const {
        const auto* snapshot = load_state.indexed_snapshot();
        return snapshot ? std::optional(*snapshot) : std::nullopt;
    }

    Result<std::monostate, EntityError>
    install_loaded_dynamic_fields(const DynamicFieldValues&, LoadedSnapshot::Borrow snapshot) {
        ++installer_calls;
        auto r = memory_resource();
        if (row.get("name")->try_text() == std::string_view("reject")) {
            return Result<std::monostate, EntityError>::err(
                r,
                EntityError(std::allocator_arg,
                            Allocator(r),
                            ENTITY_NAME,
                            "installer rejected saved dynamic fields"));
        }
        check(snapshot.identity() == loaded_state_snapshot()->identity(),
              "empty overlay retains snapshot identity");
        installed = true;
        return Result<std::monostate, EntityError>::ok(r, {});
    }
};

static void saved_dynamic_installation_errors() {
    GlobalContext global;
    for (bool preserve : {false, true}) {
        for (unsigned scenario = 0; scenario < 3; ++scenario) {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            Record values(r);
            values.emplace(String("id", r), Value(r, std::uint64_t{7}));
            values.emplace(String("version", r), Value(r, std::int64_t{1}));
            values.emplace(String("name", r), Value(r, scenario == 1 ? "reject" : "accept"));
            CompactRow row(std::allocator_arg, Allocator(r), std::move(values));
            auto& root = user_context->make<EntityRuntimeState>();
            auto decoded = preserve ? IndexedSavedModel::from_compact_row_with_context(
                                          r, std::move(row), AnyRef(root))
                                    : IndexedSavedModel::from_compact_row(r, std::move(row));
            check(bool(decoded), "indexed saved entity decodes");
            auto& entity = decoded.value();
            auto definitions =
                DynamicFieldDefinitions::from_fields(r, scenario == 0 ? "Other" : "Order", "1", {});
            check(bool(definitions), "trusted empty definitions");
            auto& owned = user_context->make<DynamicFieldDefinitions>(definitions.value());
            DynamicFieldValues::Values empty(r);
            auto fields = DynamicFieldValues::from_values(r, owned, empty);
            check(bool(fields), "empty dynamic values");
            auto outcome = detail::install_saved_dynamic_fields(r, entity, &fields.value());
            check(entity.installer_calls == (scenario == 0 ? 0u : 1u),
                  "installer ordering matches source");
            if (scenario < 2) {
                const auto message =
                    scenario == 0
                        ? "graph write error: dynamic field owner disagrees with fixed entity layout"
                        : "graph write error: Order: installer rejected saved dynamic fields";
                check(!outcome && outcome.error().kind() == RuntimeError::Kind::Graph &&
                          outcome.error().to_string(r) == message,
                      "saved dynamic failure retains exact Graph error");
                check(!entity.installed, "failed installation does not report success");
            } else {
                check(bool(outcome) && entity.installed, "matching dynamic result is installed");
                for (auto name : {"id", "version", "name"}) {
                    check(entity.loaded_state_snapshot()->get().is_loaded(name),
                          "fixed field remains loaded");
                }
            }
        }
    }
}

static void saved_dynamic_snapshot_precondition() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& entity = user_context->make<save_fixture::Model>();
    check(!entity.loaded_state_snapshot(), "fixture has no indexed loaded snapshot");
    check(bool(detail::install_saved_dynamic_fields(r, entity, nullptr)),
          "absent dynamic result needs no indexed snapshot");
    auto definitions = DynamicFieldDefinitions::from_fields(r, "Parent", "1", {});
    check(bool(definitions), "empty trusted dynamic definitions are valid");
    auto& owned_definitions = user_context->make<DynamicFieldDefinitions>(definitions.value());
    DynamicFieldValues::Values values(r);
    auto fields = DynamicFieldValues::from_values(r, owned_definitions, values);
    check(bool(fields), "empty present dynamic result is valid");
    auto result = detail::install_saved_dynamic_fields(r, entity, &fields.value());
    check(!result && result.error().kind() == RuntimeError::Kind::Graph &&
              result.error().to_string(r) ==
                  "graph write error: indexed state missing from dynamic save result",
          "present dynamic result requires an indexed loaded snapshot even when empty");
}

int main() {
    auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        saved_dynamic_snapshot_precondition();
        saved_dynamic_installation_errors();
        extraction_and_retention();
        preflight_and_partial_failure();
        ledger_discovery_and_merge();
        std::pmr::set_default_resource(old);
        std::cout << "Save preparation/ownership contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(old);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
