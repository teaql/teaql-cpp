#include "../generated_query_field_branches/include/cpp_query_branches.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <sstream>
using namespace cpp_query_branches;

static void initialize(UserContext& user_context) {
    auto& module = moduleWithBehaviorsAndCheckers(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<cpp_query_branches::DataServiceExecutor>(
        require_value(dialect).get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) +
                       "/tests/support/query_field_branches_sql.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    if (!file ||
        sqlite3_exec(connection.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    user_context.with_active_root("Tenant", 1);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    auto query = std::get<bool>(field(input, "minimal").data) ? Q.entriesMinimal() : Q.entries();
    auto output = detail::model_json(r);
    try {
        for (const auto& source : std::get<Json::Array>(field(input, "filters").data)) {
            query.filterWithJson(text(source));
        }
    } catch (const GeneratedDynamicJsonFormatError& error) {
        detail::model_put(output, "panic", error.to_string(r));
        return output;
    } catch (const std::exception& error) {
        detail::model_put(output, "panic", std::string_view(error.what()));
        return output;
    }
    detail::model_put(output, "query", debug_string(r, query.query()));
    initialize(user_context);
    auto executable = std::move(query)
                          .comment("inspect scalar JSON criteria")
                          .purpose("qualify scalar JSON execution");
    Json::Array rows(r);
    Json error(r), scalar(r);
    auto mode = text(field(input, "mode"));
    if (mode == "count") {
        const auto& result = executable.executeForCount(user_context).wait();
        if (result) {
            scalar = Json(r, result.value());
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } else if (mode == "exists") {
        const auto& result = executable.executeForExists(user_context).wait();
        if (result) {
            scalar = Json(r, result.value());
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } else {
        const auto& result = executable.executeForRows(user_context).wait();
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array columns(r);
                for (auto key : {"id",
                                 "version",
                                 "tenant_id",
                                 "label",
                                 "optional_label",
                                 "notes",
                                 "secret",
                                 "active",
                                 "optional_active",
                                 "amount",
                                 "optional_amount",
                                 "height",
                                 "ratio",
                                 "counter",
                                 "day",
                                 "optional_day",
                                 "moment",
                                 "optional_moment"}) {
                    auto* item = row.get(key);
                    columns.emplace_back(item ? Json(r, debug_string(r, *item)) : Json(r));
                }
                rows.emplace_back(columns);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    }
    detail::model_put(output, "rows", Json(r, rows));
    detail::model_put(output, "error", error);
    detail::model_put(output, "scalar", scalar);
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        Json::Array params(r);
        for (const auto& p : entry.params) {
            params.emplace_back(debug_string(r, p));
        }
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("params", Json(r, params));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        logs.emplace_back(log);
    }
    detail::model_put(output, "sql", Json(r, logs));
    return output;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Extension retained User memory");
            }
        }
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
