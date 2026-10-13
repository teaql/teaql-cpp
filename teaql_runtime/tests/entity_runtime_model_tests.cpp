#include <teaql/runtime.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void lazy_slot_and_complete_metadata() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    EntityRuntimeState original(std::allocator_arg, Allocator(r));
    EntityRuntimeState independent(std::allocator_arg, Allocator(r));
    EntityKey key(std::allocator_arg, Allocator(r), "Item", Value(r, std::uint64_t{7}));
    auto before = user_context->allocation_stats().issued_objects;
    check(original == original && !(original == independent),
          "unbound states have distinct identities; self comparison stays reflexive");
    check(!original.get(key, "missing") && !original.pop_change_set() &&
              original.changed_field_names(key).empty() && original.new_keys().empty() &&
              original.deleted_keys().empty(),
          "unbound readers and empty pop do not materialize the ledger");
    original.clear_committed();
    original.clear_current_change_set();
    auto empty = debug_string(r, original);
    check(empty.find("LazyLedgerSlot { shared: OnceLock(<uninit>) }") != empty.npos &&
              user_context->allocation_stats().issued_objects == before,
          "empty diagnostics and reads allocate no managed ledger slot");
    auto copy = detail::clone_in(r, original);
    check(user_context->allocation_stats().issued_objects == before + 1 && original == copy &&
              !detail::EntityRuntimeAccess::has_mutation_context(copy),
          "first Clone publishes exactly one shared empty binding without a ledger");
    auto second_copy = detail::clone_in(r, original);
    check(user_context->allocation_stats().issued_objects == before + 1 && copy == second_copy,
          "further Clones reuse that binding");
    copy.set(key, "name", Value(r, "native"));
    copy.set_entity_comment(key, "entity comment");
    check(original.get(key, "name")->try_text() == "native" &&
              original.get_entity_comment(key) == "entity comment",
          "mutation through a cloned binding reaches its siblings");
    const std::array fields{DynamicFieldDefinitions::Field{"extension", DataType::Text}};
    auto schema = DynamicFieldDefinitions::from_fields(r, "Item", "revision", fields);
    check(bool(schema), "dynamic metadata fixture validates");
    auto& definitions = user_context->make<DynamicFieldDefinitions>(std::move(schema).value());
    DynamicFieldValues::Values initial(r);
    auto payload = DynamicFieldValues::from_values(r, definitions, initial);
    check(bool(payload), "dynamic payload validates");
    auto& loaded = user_context->make<DynamicFieldValues>(std::move(payload).value());
    check(bool(loaded.assign("extension", Value(r, "old extension"))),
          "dynamic original value is loaded");
    original.install_loaded_dynamic_fields(loaded);
    check(bool(original.update_dynamic_field(key, "extension", Value(r, "new extension"))),
          "production mutation records dynamic provenance");
    auto output = debug_string(r, original);
    check(output.find("dynamic_changes: Some(") != output.npos &&
              output.find("dynamic_metadata: Some(DynamicMutationMetadata") != output.npos &&
              output.find("entity_comments: {") != output.npos &&
              output.find("old extension") != output.npos &&
              output.find("dynamic_fields: Some(DynamicFieldValues") != output.npos,
          "diagnostics traverse real dynamic changes, schemas, originals and per-entity comments");
}

static void retained_graph_and_cached_identity() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto root = user->make_ref<EntityRuntimeState>();
    {
        auto empty = debug_string(owner, user->get(root));
        check(empty.find("inner: LazyLedgerSlot { shared: OnceLock(<uninit>) }") != empty.npos &&
                  !detail::EntityRuntimeAccess::has_mutation_context(user->get(root)),
              "diagnostics preserve deferred ledger construction");
    }
    auto& key = user->make<EntityKey>("Item", Value(owner, std::uint64_t{1}));
    key.id = Value(owner, "changed public field");
    auto& payload = user->make<String>(1024, 'p');
    user->get(root).set(key, "value", Value(owner, payload));
    user->get(root).set_original_version(key, 7);
    auto& builder = user->make<EntityGraphBuilder>();
    builder.install(1, String(payload, owner));
    builder.install_relation_option<String>("Item", 1, "child", String(payload, owner));
    auto frozen = user->get(root).freeze_graph(std::move(builder));
    check(bool(frozen), "the fixture publishes the production graph once");
    auto kept_root = user->make_global<EntityRuntimeState>(user->get(root));
    auto kept_key = user->make_global<EntityKey>(key);
    auto error = user->make_ref<LedgerCompositionError>(
        LedgerCompositionError::Kind::ConflictingOriginalVersion, "Item", "1", 4, 5);
    auto kept_error = user->make_global<LedgerCompositionError>(user->get(error));
    {
        auto diagnostic = debug_string(owner, key);
        check(diagnostic.find("id: Text(\"changed public field\")") != diagnostic.npos &&
                  diagnostic.find("id_key: U64(1)") != diagnostic.npos,
              "derived diagnostics preserve the cached identity after public id mutation");
        check(key.debug(owner) == diagnostic, "compact helper delegates to the structural writer");
        auto weak = detail::EntityRuntimeAccess::fresh_with_weak_graph(owner, user->get(root));
        auto borrowed = debug_string(owner, weak);
        auto promoted = detail::clone_in(owner, weak);
        check(borrowed.find("graph: Weak((Weak))") != borrowed.npos &&
                  debug_string(owner, promoted).find("graph: Strong(OnceLock(FrozenEntityGraph") !=
                      String::npos,
              "diagnostic borrowing preserves weak graph roles; actual Clone promotes them");
    }
    user.reset();
    check(global.live_user_bytes() == 0, "the complete original graph User is actually deleted");
    auto next = global.make_user_context();
    bool stale = false;
    try {
        (void)next->get(root);
    } catch (const ReferenceError&) {
        stale = true;
    }
    check(stale, "stale graph references reject before graph or ledger access");
    const auto& retained = next->get(next->rebind_global(kept_root));
    const auto& retained_key = next->get(next->rebind_global(kept_key));
    const auto& retained_error = next->get(next->rebind_global(kept_error));
    auto value = retained.get(retained_key, "value");
    const auto* entity = retained.resolve_entity<String>(1);
    auto relation = retained.relation_option<String>("Item", 1, "child");
    auto output = debug_string(next->memory_resource(), retained, DebugOptions{.pretty = true});
    check(
        value && value->try_text()->size() == 1024 && entity && entity->size() == 1024 &&
            entity->get_allocator().resource() == global.memory_resource() && relation.value() &&
            relation.value()->get_allocator().resource() == global.memory_resource() &&
            retained_key.memory_resource() == global.memory_resource() &&
            retained.get_original_version(retained_key) == 7 &&
            output.get_allocator().resource() == next->memory_resource() &&
            output.find("OriginalVersions") != output.npos &&
            output.find("FrozenEntityGraph") != output.npos,
        "Global graph/ledger/key copies and complete diagnostics survive source User destruction");
    check(retained_error.source == 5 && retained_error.error_source() == nullptr &&
              retained_error.entity.get_allocator().resource() == global.memory_resource(),
          "the conflict source payload and empty Error cause remain separate responsibilities");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        lazy_slot_and_complete_metadata();
        retained_graph_and_cached_identity();
        std::cout << "Entity-runtime model contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
