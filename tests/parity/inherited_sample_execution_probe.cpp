#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <sqlite3.h>
#include <iostream>
using namespace cpp_inheritance;

static Json table(Memory r, sqlite3* database, std::string_view name) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database,
                           "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?1",
                           -1,
                           &raw,
                           nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> exists(raw, sqlite3_finalize);
    sqlite3_bind_text(raw, 1, name.data(), static_cast<int>(name.size()), SQLITE_TRANSIENT);
    if (sqlite3_step(raw) != SQLITE_ROW) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    if (!sqlite3_column_int64(raw, 0)) {
        return Json(r);
    }
    exists.reset();
    String query("SELECT * FROM ", r);
    query += name;
    query += " ORDER BY 1";
    if (sqlite3_prepare_v2(database, query.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array rows(r);
    int status;
    while ((status = sqlite3_step(raw)) == SQLITE_ROW) {
        Json::Object row(r);
        for (int index = 0; index < sqlite3_column_count(raw); ++index) {
            Json value(r);
            switch (sqlite3_column_type(raw, index)) {
            case SQLITE_NULL:
                break;
            case SQLITE_INTEGER:
                value = Json(r, static_cast<std::int64_t>(sqlite3_column_int64(raw, index)));
                break;
            case SQLITE_FLOAT:
                value = Json(r, sqlite3_column_double(raw, index));
                break;
            default:
                value = Json(
                    r,
                    std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(raw, index)),
                                     static_cast<std::size_t>(sqlite3_column_bytes(raw, index))));
                break;
            }
            row.emplace(String(sqlite3_column_name(raw, index), r), std::move(value));
        }
        rows.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, rows);
}

static void execute(sqlite3* database, std::string_view query) {
    std::string text(query);
    if (sqlite3_exec(database, text.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json snapshot(Memory r, sqlite3* database) {
    auto output = detail::model_json(r);
    for (auto pair : {std::pair{"platform", "platform_data"},
                      std::pair{"school", "school_data"},
                      std::pair{"academy", "academy_data"},
                      std::pair{"research", "research_academy_data"},
                      std::pair{"ids", "teaql_id_space"}}) {
        detail::model_put(output, pair.first, table(r, database, pair.second));
    }
    return output;
}

static String trace_string(Memory r, const TraceChain& trace) {
    String result(r);
    DebugWriter writer(result, {});
    writer.sequence(trace, DefaultDebugElement{});
    return result;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto mode = text(field(input, "module"));
    auto& runtime_module = mode == "plain"      ? module(user_context)
                           : mode == "checker"  ? moduleWithCheckers(user_context)
                           : mode == "behavior" ? moduleWithBehaviors(user_context)
                                                : moduleWithBehaviorsAndCheckers(user_context);
    runtime_module.apply_to(user_context);
    user_context.set_metadata(user_context, runtime_module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
    user_context.set_internal_id_generator(user_context, ids);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<cpp_inheritance::DataServiceExecutor>(
        require_value(dialect).get(), transport, runtime_module.metadata, state);
    user_context.register_executor(executor);
    user_context.disable_sql_log();
    const auto& schema = user_context.ensure_schema().wait();
    if (!schema) {
        auto output = detail::model_json(r);
        detail::model_put(output, "schema_error", schema.error().to_string(r));
        return output;
    }
    auto* database = connection.native_handle();
    auto root = text(field(input, "root"));
    if (root == "missing") {
        execute(database, "DELETE FROM platform_data;");
    }
    if (root == "modified") {
        execute(database, "UPDATE platform_data SET name='application root',version=4 WHERE id=1;");
    }
    if (auto* fault = optional(input, "fault")) {
        auto name = text(*fault);
        if (name != "school" && name != "academy" && name != "research_academy") {
            throw std::logic_error("Invalid sample fault table");
        }
        std::string query =
            "CREATE TRIGGER sample_failure BEFORE INSERT ON " + std::string(name) +
            "_data WHEN NEW.id=3 BEGIN SELECT RAISE(ABORT,'sample fixture write failed'); END;";
        execute(database, query);
    }
    user_context.enable_all_sql_log();
    Json::Array rounds(r);
    for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(input, "rounds").data); ++i) {
        user_context.clear_sql_logs();
        auto before = snapshot(r, database);
        auto scale =
            text(field(input, "scale")) == "Small" ? SampleDataScale::Small : SampleDataScale::Tiny;
        const auto& result =
            generateSampleData(user_context,
                               {scale, std::get<std::uint64_t>(field(input, "seed").data)})
                .wait();
        auto report = detail::model_json(r);
        if (result) {
            Json::Object counts(r);
            for (const auto& [name, count] : result.value().generated) {
                counts.emplace(String(name, r), Json(r, std::uint64_t(count)));
            }
            Json::Array skipped(r);
            for (const auto& skip : result.value().skipped) {
                auto item = detail::model_json(r);
                detail::model_put(item, "entity", skip.entity);
                detail::model_put(item, "reason", skip.reason);
                skipped.emplace_back(item);
            }
            detail::model_put(report, "generated", Json(r, counts));
            detail::model_put(report, "skipped", Json(r, skipped));
            detail::model_put(report, "error", Json(r));
        } else {
            detail::model_put(report, "error", result.error().to_string(r));
        }
        Json::Array logs(r);
        for (const auto& e : user_context.sql_logs()) {
            auto item = detail::model_json(r);
            detail::model_put(item, "context", debug_string(r, e.log_context));
            detail::model_put(item, "operation", debug_string(r, e.operation));
            detail::model_put(item, "sql", e.sql);
            Json::Array params(r);
            for (const auto& v : e.params) {
                params.emplace_back(debug_string(r, v));
            }
            detail::model_put(item, "params", Json(r, params));
            detail::model_put(item, "comment", e.comment ? Json(r, *e.comment) : Json(r));
            detail::model_put(item, "purpose", e.purpose ? Json(r, *e.purpose) : Json(r));
            detail::model_put(
                item, "audit_reason", e.audit_reason ? Json(r, *e.audit_reason) : Json(r));
            detail::model_put(item, "trace", trace_string(r, e.trace_path));
            detail::model_put(item, "debug_sql", e.debug_sql);
            detail::model_put(item, "pretty_sql", e.pretty_sql);
            detail::model_put(item,
                              "result_count",
                              e.result_count ? Json(r, std::uint64_t(*e.result_count)) : Json(r));
            detail::model_put(
                item, "result_type", e.result_type ? Json(r, *e.result_type) : Json(r));
            detail::model_put(
                item, "affected_rows", e.affected_rows ? Json(r, *e.affected_rows) : Json(r));
            detail::model_put(item, "summary", e.result_summary);
            logs.emplace_back(item);
        }
        auto round = detail::model_json(r);
        detail::model_put(round, "before", before);
        detail::model_put(round, "report", report);
        detail::model_put(round, "after", snapshot(r, database));
        detail::model_put(round, "logs", Json(r, logs));
        rounds.emplace_back(round);
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "schema_error", Json(r));
    detail::model_put(output, "rounds", Json(r, rounds));
    return output;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            ContextLimits limits;
            limits.user.max_bytes = 512ull * 1024 * 1024;
            limits.max_live_user_bytes = limits.user.max_bytes;
            GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::logic_error("Sample observer retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
