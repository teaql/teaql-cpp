#pragma once
#include "../generated_relations/include/cpp_relations.hpp"
#include <sqlite3.h>

namespace teaql::test {
inline sqlite3* initialize_sample_database(UserContext& user_context, bool root) {
    using namespace cpp_relations;
    auto& module = moduleWithBehaviorsAndCheckers(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
    user_context.set_internal_id_generator(user_context, ids);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<cpp_relations::DataServiceExecutor>(
        require_value(dialect).get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    user_context.with_active_root("Platform", 1);
    auto* database = connection.native_handle();
    const char* definition =
        "CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);CREATE TABLE device_data(id INTEGER PRIMARY KEY,name TEXT,platform INTEGER,version INTEGER);CREATE TABLE profile_data(id INTEGER PRIMARY KEY,name TEXT,platform INTEGER,version INTEGER);CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,platform INTEGER,version INTEGER);";
    if (sqlite3_exec(database, definition, nullptr, nullptr, nullptr) != SQLITE_OK ||
        (root && sqlite3_exec(database,
                              "INSERT INTO platform_data VALUES(1,'existing root',1);",
                              nullptr,
                              nullptr,
                              nullptr) != SQLITE_OK)) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return database;
}
}
