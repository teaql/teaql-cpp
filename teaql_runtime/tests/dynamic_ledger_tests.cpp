#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/detail/data_service/entity_service.hpp>
#include "../../tests/support/runtime_mutation_fixture.hpp"
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static EntityKey key(Memory r, std::string_view entity = "Item") {
    return EntityKey(std::allocator_arg, Allocator(r), entity, Value(r, std::uint64_t{7}));
}

static DynamicFieldDefinitions definitions(Memory r, std::uint64_t executor = 0) {
    const std::array fields{DynamicFieldDefinitions::Field{"name", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto result = std::move(DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields)).value();
    return executor ? result.with_storage_binding(r, "profile", executor) : std::move(result);
}

static void install(UserContext& user_context,
                    EntityRuntimeState& state,
                    std::string_view name = "original",
                    std::uint64_t executor = 0) {
    auto r = user_context.memory_resource();
    auto& schema = user_context.make<DynamicFieldDefinitions>(definitions(r, executor));
    Value::Object values(r);
    values.emplace(String("name", r), Value(r, name));
    auto loaded = DynamicFieldValues::from_values(r, schema, values);
    state.install_loaded_dynamic_fields(loaded.value());
}

static void mutation_and_frames() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& state = user_context->make<EntityRuntimeState>();
    auto identity = key(r);
    auto missing = state.update_dynamic_field(identity, "name", Value(r, "new"));
    check(!missing && missing.error().code == "DYNAMIC_FIELD_DEFINITIONS_MISSING" &&
              !detail::EntityRuntimeAccess::has_mutation_context(state),
          "missing carrier rejects without creating ledger");
    install(*user_context, state);
    check(!state.has_pending_dynamic_mutations() &&
              !detail::EntityRuntimeAccess::has_mutation_context(state),
          "hydration is not mutation");
    auto invalid = state.update_dynamic_field(identity, "name", Value(r, true));
    auto wrong = state.delete_dynamic_field(key(r, "Other"), "name");
    check(!invalid && !wrong && wrong.error().code == "DYNAMIC_FIELD_OWNER_MISMATCH" &&
              !state.has_pending_dynamic_mutations(),
          "validation does not fabricate intent");
    auto first = state.update_dynamic_field(identity, "name", Value(r, "next"));
    check(first && !first.value() && state.has_pending_dynamic_mutations(),
          "existing name assignment reports unchanged availability");
    auto second = state.update_dynamic_field(identity, "name", Value(r, nullptr));
    auto count = state.update_dynamic_field(identity, "count", Value(r, 0));
    check(second && !second.value() && count && count.value(),
          "NULL is loaded, newly selected zero changes availability");
    auto changes = state.current_change_set();
    check(changes.changes().empty() && !changes.is_empty() &&
              changes.field_names(identity).contains("#name") && !state.get(identity, "#name"),
          "durable changes remain separate from native mutations");
    const auto* originals = detail::EntityRuntimeAccess::dynamic_originals(changes, identity);
    check(originals && originals->find("name")->second->try_text() == "original" &&
              !originals->find("count")->second,
          "first originals preserve loaded value versus absence");
    check(changes.dynamic_changes()->find(identity)->second.find("name")->second.value()->is_null(),
          "latest Set(NULL) retains explicit persistence intent");
    auto& copy = user_context->make<EntityRuntimeState>(state);
    check(copy == state &&
              &copy.loaded_dynamic_fields()->values() != &state.loaded_dynamic_fields()->values(),
          "Clone shares ledger but isolates dynamic row values");
    check(bool(copy.update_dynamic_field(identity, "name", Value(r, "private copy"))) &&
              copy != state &&
              state.loaded_dynamic_fields()->field("name").value().state() ==
                  DynamicFieldState::Null,
          "view payloads remain independent after shared-ledger mutation");
    state.push_change_set();
    check(bool(state.delete_dynamic_field(identity, "name")),
          "inner delete records separate intent");
    auto inner = state.current_change_set();
    check(inner.dynamic_changes()->find(identity)->second.find("name")->second.is_delete() &&
              detail::EntityRuntimeAccess::dynamic_originals(inner, identity)
                  ->find("name")
                  ->second->is_null(),
          "inner frame snapshots actual NULL instead of outer pending value");
    auto popped = state.pop_change_set();
    check(popped && state.has_pending_dynamic_mutations(), "pop retains outer intent");
    state.clear_current_change_set();
    check(!state.has_pending_dynamic_mutations() &&
              !state.loaded_dynamic_fields()->field("name").value().is_loaded(),
          "clearing intent does not roll back view data");
    changes.clear_entity(identity);
    check(changes.is_empty() && changes.dynamic_changes()->empty() &&
              !detail::EntityRuntimeAccess::dynamic_originals(changes, identity),
          "entity clearing removes mutation and provenance together");
    auto fresh = EntityRuntimeState::fresh_with_shared_graph(r, state);
    check(!fresh.loaded_dynamic_fields(), "fresh graph view carries no loaded extensions");
    auto rebound = fresh.with_loaded_view_from(state);
    check(rebound.loaded_dynamic_fields() && !rebound.has_pending_dynamic_mutations(),
          "loaded view rebind keeps receiver ledger ownership");
    state.clear_committed();
    check(state.loaded_dynamic_fields() && !state.has_pending_dynamic_mutations(),
          "commit clearing retains loaded view");
}

static void composition_and_retention() {
    GlobalContext global;
    EntityRuntimeState* retained = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto identity = key(r);
        auto& receiver = user_context->make<EntityRuntimeState>();
        auto& source = user_context->make<EntityRuntimeState>();
        install(*user_context, receiver, "receiver original", 1);
        install(*user_context, source, "source original", 1);
        receiver.set_comment("receiving reason");
        source.set_comment("source reason");
        check(bool(receiver.update_dynamic_field(identity, "name", Value(r, "receiver pending"))) &&
                  bool(source.update_dynamic_field(identity, "name", Value(r, "source pending"))),
              "compose fixtures mutate");
        receiver.adopt_mutations_from(source);
        auto changes = receiver.current_change_set();
        check(!detail::EntityRuntimeAccess::first_composition_error(receiver) &&
                  changes.dynamic_changes()
                          ->find(identity)
                          ->second.find("name")
                          ->second.value()
                          ->try_text() == "source pending" &&
                  detail::EntityRuntimeAccess::dynamic_originals(changes, identity)
                          ->find("name")
                          ->second->try_text() == "receiver original" &&
                  receiver.get_comment() == "receiving reason",
              "composition imports pending operand but receiver original/reason win");
        check(receiver.loaded_dynamic_fields()->field("name").value().value()->try_text() ==
                  "receiver pending",
              "composition does not overwrite receiving view data");
        auto& incompatible = user_context->make<EntityRuntimeState>();
        install(*user_context, incompatible, "wrong original", 2);
        incompatible.set(identity, "native", Value(r, "must not import"));
        check(bool(incompatible.update_dynamic_field(identity, "name", Value(r, "wrong pending"))),
              "incompatible source mutates");
        receiver.adopt_mutations_from(incompatible);
        auto error = detail::EntityRuntimeAccess::first_composition_error(receiver);
        check(error && error->kind == LedgerCompositionError::Kind::ConflictingDynamicDefinitions &&
                  error->to_string(r) ==
                      "cannot compose incompatible dynamic definitions for Item" &&
                  !receiver.get(identity, "native"),
              "all dynamic definitions validate before importing native intent");
        retained = &global.make<EntityRuntimeState>(receiver);
    }
    auto identity = key(global.memory_resource());
    auto changes = retained->current_change_set();
    check(
        retained->loaded_dynamic_fields()->field("name").value().value()->try_text() ==
                "receiver pending" &&
            detail::EntityRuntimeAccess::dynamic_originals(changes, identity)
                    ->find("name")
                    ->second->try_text() == "receiver original" &&
            detail::EntityRuntimeAccess::dynamic_definitions(changes, identity)
                    ->memory_resource() == global.memory_resource(),
        "explicit Global copy retains values, originals and definitions after actual User deletion");
}

static void dynamic_privacy() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& store = user_context->make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    store.register_entity(std::move(descriptor));
    user_context->set_metadata(*user_context, store);
    auto& state = user_context->make<EntityRuntimeState>();
    install(*user_context, state, "original private value");
    check(bool(state.update_dynamic_field(key(r), "name", Value(r, "pending private value"))),
          "dynamic privacy fixture mutates");
    auto privacy = detail::entity_ledger_privacy(*user_context, state);
    check(bool(privacy), "dynamic-only keys require mapped entity descriptor");
    Vector<String> safe(r), debug(r);
    privacy.value().extend_secrets(false, safe);
    privacy.value().extend_secrets(true, debug);
    auto contains = [](const Vector<String>& terms, std::string_view expected) {
        return std::any_of(terms.begin(), terms.end(), [&](const auto& term) {
            return term == expected;
        });
    };
    check(contains(safe, "original private value") && contains(safe, "pending private value") &&
              contains(debug, "original private value") && contains(debug, "pending private value"),
          "unmapped durable operands/originals use Unknown privacy even in debug logs");
    auto& missing = user_context->make<EntityRuntimeState>();
    install(*user_context, missing);
    auto other = key(r, "Unknown");
    missing.set(other, "native", Value(r, 1));
    check(!detail::entity_ledger_privacy(*user_context, missing),
          "descriptor rejection remains before protected writes");
}

static void unsupported_storage_rejects_before_io() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& store = user_context->make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("native", DataType::Text);
    store.register_entity(std::move(descriptor));
    user_context->set_metadata(*user_context, store);
    auto& state = user_context->make<mutation_fixture::State>();
    auto& executor = user_context->make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        *user_context, "Item", *user_context, executor);
    auto& root = user_context->make<EntityRuntimeState>();
    install(*user_context, root);
    root.set_comment("Explicit dynamic save request");
    root.set(key(r), "native", Value(r, "must not write partially"));
    check(bool(root.update_dynamic_field(key(r), "name", Value(r, "pending"))),
          "storage fixture mutates");
    auto& locations = user_context->make<detail::LedgerObjectLocations>();
    auto& result =
        detail::EntityDataServiceAccess::execute_ledger_plan_internal(service, root, locations)
            .wait();
    check(
        !result && result.error().runtime_error().kind() == RuntimeError::Kind::DynamicField &&
            result.error().runtime_error().dynamic_field_error().code ==
                "DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED" &&
            result.error().runtime_error().to_string(r) ==
                "DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED: save" &&
            state.queries.empty() && state.requests.empty() &&
            root.has_pending_dynamic_mutations() && root.get(key(r), "native"),
        "missing transaction-bound dynamic storage rejects before any read/write and retains all intent");
    auto copied = detail::clone_in(global.memory_resource(), result.error().runtime_error());
    check(copied.dynamic_field_error().memory_resource() == global.memory_resource(),
          "typed dynamic error propagates destination allocator");
}

int main() {
    try {
        mutation_and_frames();
        composition_and_retention();
        dynamic_privacy();
        unsupported_storage_rejects_before_io();
        std::cout << "Dynamic ledger contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
