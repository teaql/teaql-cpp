#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;

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

static Json snapshot(Memory r, sqlite3* database) {
    Json::Object result(r);
    result.emplace("platform", table(r, database, "platform_data"));
    result.emplace("status", table(r, database, "status_data"));
    result.emplace("ids", table(r, database, "teaql_id_space"));
    return Json(r, result);
}

static void fixture_sql(sqlite3* database, const char* query) {
    if (sqlite3_exec(database, query, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json observe(GlobalContext& global, const Json& input) {
    namespace domain = constant_metadata_fixture;
    auto r = global.memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto* database = connection.native_handle();
    auto create = [&] {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        auto mode = text(field(input, "module"));
        auto& module = mode == "plain"      ? domain::module(user_context)
                       : mode == "checker"  ? domain::moduleWithCheckers(user_context)
                       : mode == "behavior" ? domain::moduleWithBehaviors(user_context)
                                            : domain::moduleWithBehaviorsAndCheckers(user_context);
        module.apply_to(user_context);
        use_sqlite_provider(user_context, global, transport);
        auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
        user_context.set_internal_id_generator(user_context, ids);
        auto& cache = user_context.make<SqlExecutorState>();
        auto executor = user_context.make_ref<domain::ServiceRuntimeExecutor>(
            user_context.require_resource<SqliteDialect>().value().get(),
            transport,
            module.metadata,
            cache);
        user_context.register_executor(executor);
        user_context.set_trace_id("constant-metadata");
        user_context.set_user_identifier("fixture");
        if (std::get<bool>(field(input, "logging").data)) {
            user_context.enable_all_sql_log();
        } else {
            user_context.disable_sql_log();
        }
        return owner;
    };
    auto state = text(field(input, "state"));
    Json warmup_error(r);
    if (state != "fresh") {
        auto user_context = create();
        const auto& result = user_context->ensure_schema().wait();
        if (!result) {
            warmup_error = Json(r, result.error().to_string(r));
        }
        user_context.reset();
        if (std::holds_alternative<std::monostate>(warmup_error.data)) {
            if (state == "drift") {
                fixture_sql(
                    database,
                    "UPDATE status_data SET name='Drift',code='STALE',version=3 WHERE id=(SELECT MIN(id) FROM status_data)");
            } else if (state == "missing") {
                fixture_sql(database,
                            "DELETE FROM status_data WHERE id=(SELECT MIN(id) FROM status_data)");
            } else if (state == "deleted") {
                fixture_sql(
                    database,
                    "UPDATE status_data SET version=-1 WHERE id=(SELECT MIN(id) FROM status_data)");
            } else if (state == "extra") {
                fixture_sql(
                    database,
                    "INSERT INTO status_data(id,name,code,version) VALUES(777,'Extra','EXTRA',4)");
            } else if (state != "existing") {
                throw std::runtime_error("Unknown constant metadata state");
            }
        }
    }
    Json::Array rounds(r);
    for (std::uint64_t index = 0; index < std::get<std::uint64_t>(field(input, "rounds").data);
         ++index) {
        Json::Object round(r);
        round.emplace("before", snapshot(r, database));
        auto user_context = create();
        const auto& result = user_context->ensure_schema().wait();
        round.emplace("error", result ? Json(r) : Json(r, result.error().to_string(r)));
        auto root = user_context->require_active_root("Platform");
        round.emplace("root",
                      root ? Json(r, root.value().get().id) : Json(r, root.error().to_string(r)));
        auto Q = user_context->queries<domain::query_factory>();
        const auto& queried = Q.statuses()
                                  .selectSelfFields()
                                  .orderByIdAsc()
                                  .comment("what: inspect constant metadata rows")
                                  .purpose("why: verify generated identifier and seed behavior")
                                  .executeForList(*user_context)
                                  .wait();
        round.emplace("query_error", queried ? Json(r) : Json(r, queried.error().to_string(r)));
        Json::Array values(r);
        if (queried) {
            for (const auto& entity : queried.value().data) {
                Json::Object row(r);
                row.emplace("id", Json(r, entity.id()));
                row.emplace("version", Json(r, entity.version()));
                row.emplace("name", Json(r, entity.name()));
                row.emplace("code", Json(r, entity.code()));
                row.emplace("expression", Json(r, domain::E::status(entity).getCode().unwrap()));
                values.emplace_back(row);
            }
        }
        round.emplace("values", Json(r, values));
        round.emplace("after", snapshot(r, database));
        Json::Array logs(r);
        for (const auto& entry : user_context->sql_logs()) {
            Json::Object log(r);
            log.emplace("sql", Json(r, entry.sql));
            log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
            log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
            log.emplace("audit_as", entry.audit_reason ? Json(r, *entry.audit_reason) : Json(r));
            Json::Array params(r);
            for (const auto& value : entry.params) {
                params.emplace_back(debug_string(r, value));
            }
            log.emplace("params", Json(r, params));
            logs.emplace_back(log);
        }
        round.emplace("logs", Json(r, logs));
        user_context.reset();
        round.emplace("after_close", snapshot(r, database));
        rounds.emplace_back(round);
    }
    Json::Object output(r);
    output.emplace("warmup_error", std::move(warmup_error));
    output.emplace("rounds", Json(r, rounds));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid constant metadata input");
            }
            std::cout << observe(global, *input).dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
