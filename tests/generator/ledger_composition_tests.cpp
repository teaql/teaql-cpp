#include "parity/relation_identity_contracts/cpp/include/cpp_relation_identities.hpp"
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

using namespace cpp_relation_identities;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void explicit_composition_lifetimes() {
    GlobalContext global;
    auto connection = global.make_resource_ref<SqliteConnection>();
    auto first_user_context = global.make_user_context();
    auto second_user_context = global.make_user_context();
    auto* database =
        first_user_context->get(first_user_context->rebind_global(connection)).native_handle();
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/relation_identity_sql.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    check(bool(file) &&
              sqlite3_exec(database, sql.str().c_str(), nullptr, nullptr, nullptr) == SQLITE_OK,
          "Initialize the real generated-model SQLite fixture");
    require_value(serviceRuntimeFromConnection(*first_user_context,
                                               first_user_context->rebind_global(connection)));
    require_value(serviceRuntimeFromConnection(*second_user_context,
                                               second_user_context->rebind_global(connection)));
    first_user_context->with_active_root("Platform", 1);
    second_user_context->with_active_root("Platform", 1);
    Ref<Owner> retained;
    {
        auto Q = first_user_context->queries<query_factory>();
        auto next_Q = second_user_context->queries<query_factory>();
        auto load_owner = [&]() -> const Owner& {
            const auto& row = require_value(Q.owners()
                                                .withIdIs(10)
                                                .comment("Read composition receiver")
                                                .purpose("Review original version authority")
                                                .executeForOne(*first_user_context)
                                                .wait());
            check(bool(row), "Expected receiver row");
            return *row;
        };
        auto& receiver = first_user_context->make<Owner>(load_owner());
        receiver.updateName("Receiver intent");
        const auto& child = require_value(next_Q.entries()
                                              .withIdIs(20)
                                              .comment("Read separate Context participant")
                                              .purpose("Verify explicit destination copying")
                                              .executeForOne(*second_user_context)
                                              .wait());
        check(bool(child), "Expected participant row");
        auto& source = second_user_context->make<Entry>(*child);
        source.updateName("Participant text owned by its declared Context");
        auto expression = E::entry(source);
        auto r = first_user_context->memory_resource();
        const auto source_before = debug_string(r, source.root.current_change_set());
        auto logs_before = first_user_context->sql_logs().size();
        require_value(receiver.includePendingMutationsFrom(source));
        check(first_user_context->sql_logs().size() == logs_before,
              "Ledger composition does not execute SQL");
        check(debug_string(r, source.root.current_change_set()) == source_before,
              "The source retains its complete pending intent for retry");
        EntityKey source_key(
            std::allocator_arg, Allocator(r), "Entry", Value(r, std::uint64_t{20}));
        second_user_context.reset();
        bool expired = false;
        try {
            (void)expression.getName().unwrap();
        } catch (const ReferenceError&) {
            expired = true;
        }
        check(expired, "Composition does not extend an expired source Context");
        auto retained_text = receiver.root.get(source_key, "name");
        check(retained_text &&
                  retained_text->try_text() == "Participant text owned by its declared Context" &&
                  retained_text->memory_resource() == r,
              "Explicit ledger composition owns the merged nested value in its destination");
        auto original = detail::EntityRuntimeAccess::original_values_for(receiver.root, source_key);
        check(original && original->fields.at(String("name", r)).try_text() == "First" &&
                  original->fields.at(String("name", r)).memory_resource() == r &&
                  receiver.root.get_original_version(source_key) == 1,
              "Original snapshots and versions also belong to the receiving Context");
        check(sqlite3_exec(database,
                           "UPDATE owner_data SET version=2 WHERE id=10",
                           nullptr,
                           nullptr,
                           nullptr) == SQLITE_OK,
              "Create a real stale original-version boundary");
        auto& changed_version = first_user_context->make<Owner>(load_owner());
        changed_version.updateName("Conflicting source intent");
        const auto pending_before = debug_string(r, receiver.root.current_change_set());
        const auto& conflict = receiver.includePendingMutationsFrom(changed_version);
        check(!conflict &&
                  conflict.error().kind ==
                      LedgerCompositionError::Kind::ConflictingOriginalVersion &&
                  debug_string(r, receiver.root.current_change_set()) == pending_before,
              "A stale version rejects the complete merge before changing the receiver");
        check(sqlite3_exec(database,
                           "UPDATE owner_data SET version=1 WHERE id=10",
                           nullptr,
                           nullptr,
                           nullptr) == SQLITE_OK,
              "Remove the synthetic version-conflict fault before the independent commit control");
        retained = first_user_context->make_global<Owner>(receiver);
        retained_text.reset();
    }
    first_user_context.reset();
    check(global.live_user_bytes() == 0, "Both User arenas are returned after actual deletion");
    auto access_user_context = global.make_user_context();
    const auto& destination =
        access_user_context->get(access_user_context->rebind_global(retained));
    EntityKey key(std::allocator_arg,
                  Allocator(global.memory_resource()),
                  "Entry",
                  Value(global.memory_resource(), std::uint64_t{20}));
    auto global_text = destination.root.get(key, "name");
    check(global_text &&
              global_text->try_text() == "Participant text owned by its declared Context" &&
              global_text->memory_resource() == global.memory_resource(),
          "Explicit Global retention owns the composed ledger independently");
    require_value(serviceRuntimeFromConnection(*access_user_context,
                                               access_user_context->rebind_global(connection)));
    access_user_context->with_active_root("Platform", 1);
    auto& editable = access_user_context->make<Owner>(destination);
    const auto& saved = require_value(
        AuditedSaveExt<Owner>::save(
            std::move(editable).audit_as("Save copied intent after both source Contexts close"),
            *access_user_context)
            .wait());
    check(saved.name() == "Receiver intent" && saved.version() == 2,
          "A copied aggregate commits after all original Contexts are deleted");
    auto Q = access_user_context->queries<query_factory>();
    const auto& persisted =
        require_value(Q.entries()
                          .withIdIs(20)
                          .comment("Read the copied participant after commit")
                          .purpose("Verify actual persistence from independent retained intent")
                          .executeForOne(*access_user_context)
                          .wait());
    check(persisted && persisted->version() == 2 &&
              E::entry(*persisted).getName().unwrap() ==
                  "Participant text owned by its declared Context",
          "The participant's copied intent is persisted through the real provider");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        explicit_composition_lifetimes();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
