#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace scalar_candidate_fixture;

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& module = moduleWithBehaviorsAndCheckers(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<scalar_candidate_fixture::DataServiceExecutor>(
        require_value(dialect).get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    if (sqlite3_exec(connection.native_handle(), CANDIDATE_SQL, nullptr, nullptr, nullptr) !=
        SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    user_context.with_active_root("Platform", 1);
    user_context.disable_sql_log();
    if (std::get<bool>(field(input, "logging").data)) {
        user_context.enable_select_sql_log();
    }
    auto Q = user_context.queries<query_factory>();
    auto request = std::get<bool>(field(input, "minimal").data) ? Q.recordsMinimal() : Q.records();
    for (const auto& step : std::get<Json::Array>(field(input, "steps").data)) {
        candidate_filter(
            request,
            static_cast<std::size_t>(std::get<std::uint64_t>(field(step, "index").data)),
            std::get<bool>(field(step, "negative").data));
    }
    request.orderByIdAsc();
    Json::Object out(r);
    out.emplace("filter",
                request.query().predicate ? Json(r, debug_string(r, *request.query().predicate))
                                          : Json(r));
    auto executable = std::move(request)
                          .comment("what: qualify scalar candidates")
                          .purpose("why: compare source runtime candidates");
    const auto& result = executable.executeForList(user_context).wait();
    Json::Array rows(r);
    Json error(r);
    if (result) {
        for (const auto& entity : result.value().data) {
            Json::Object row(r);
            row.emplace("id", Json(r, entity.id()));
            row.emplace("version", Json(r, entity.version()));
            EntityJsonTraversal traversal(std::allocator_arg, Allocator(r));
            auto json = entity.borrowed_json(r, traversal);
            row.emplace("json", json ? Json(r, *json) : Json(r));
            rows.emplace_back(row);
        }
    } else {
        error = Json(r, result.error().to_string(r));
    }
    out.emplace("rows", Json(r, rows));
    out.emplace("error", std::move(error));
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        Json::Array parameters(r);
        for (const auto& value : entry.params) {
            parameters.emplace_back(debug_string(r, value));
        }
        log.emplace("params", Json(r, parameters));
        logs.emplace_back(log);
    }
    out.emplace("logs", Json(r, logs));
    return Json(r, out);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid scalar candidate input");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
