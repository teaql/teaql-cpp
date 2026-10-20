#pragma once
#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <fstream>
#include <sstream>
#include <sqlite3.h>

namespace teaql::test {
using namespace cpp_dynamic_json;

inline SqliteConnection& initialize_dynamic_json_database(UserContext& user_context) {
    auto& module = moduleWithBehaviorsAndCheckers(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<cpp_dynamic_json::DataServiceExecutor>(
        require_value(dialect).get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    for (const auto* fixture : {"dynamic_json_schema.sql", "dynamic_json_dataset.sql"}) {
        std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/" + fixture);
        std::ostringstream sql;
        sql << file.rdbuf();
        if (!file ||
            sqlite3_exec(
                connection.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
                SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
        }
    }
    user_context.with_active_root("Platform", 1);
    return connection;
}

}
