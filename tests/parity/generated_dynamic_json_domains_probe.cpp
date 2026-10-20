#include "../generated_candidates/include/cpp_candidates.hpp"
#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <sstream>
using namespace teaql;

template <class Executor>
static void initialize(UserContext& user_context, auto& module) {
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<Executor>(
        cpp_candidates::require_value(dialect).get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) +
                       "/tests/support/dynamic_json_domain_models.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    if (!file ||
        sqlite3_exec(connection.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
}

template <class Request>
static Json apply(UserContext& user_context, Request query, const Json& input) {
    auto r = user_context.memory_resource();
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
    auto executable = std::move(query)
                          .comment("inspect domain JSON criteria")
                          .purpose("qualify domain JSON execution");
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
                                 "name",
                                 "code",
                                 "phase",
                                 "tag",
                                 "status_id",
                                 "platform_id",
                                 "tenant_id",
                                 "receipt_number",
                                 "ticket_number"}) {
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

static Json observe(UserContext& user_context, const Json& input) {
    auto entity = text(field(input, "entity"));
    bool minimal = std::get<bool>(field(input, "minimal").data);
    if (entity == "receipt" || entity == "ticket") {
        auto& module = cpp_business_ids::moduleWithBehaviorsAndCheckers(user_context);
        initialize<cpp_business_ids::DataServiceExecutor>(user_context, module);
        user_context.with_active_root("Tenant", 1);
        auto Q = user_context.queries<cpp_business_ids::query_factory>();
        if (entity == "receipt") {
            return apply(user_context, minimal ? Q.receiptsMinimal() : Q.receipts(), input);
        }
        return apply(user_context, minimal ? Q.ticketsMinimal() : Q.tickets(), input);
    }
    auto& module = cpp_candidates::moduleWithBehaviorsAndCheckers(user_context);
    initialize<cpp_candidates::DataServiceExecutor>(user_context, module);
    user_context.with_active_root("Platform", 1);
    auto Q = user_context.queries<cpp_candidates::query_factory>();
    if (entity == "order_status") {
        return apply(user_context, minimal ? Q.orderStatusesMinimal() : Q.orderStatuses(), input);
    }
    if (entity == "person") {
        return apply(user_context, minimal ? Q.peopleMinimal() : Q.people(), input);
    }
    return apply(user_context, minimal ? Q.recordsMinimal() : Q.records(), input);
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
