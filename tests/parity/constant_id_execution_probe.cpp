#include "constant_id_fixture.hpp"
#include "fixture_value.hpp"
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

static Json observe(GlobalContext& global, const Json& input, Memory r) {
    auto& connection = global.make_resource<SqliteConnection>();
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto* database = connection.native_handle();
    auto create = [&] {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        auto mode = text(field(input, "module"));
        auto& module = mode == "plain"     ? constant_id_fixture::module(user_context)
                       : mode == "checker" ? constant_id_fixture::moduleWithCheckers(user_context)
                       : mode == "behavior"
                           ? constant_id_fixture::moduleWithBehaviors(user_context)
                           : constant_id_fixture::moduleWithBehaviorsAndCheckers(user_context);
        module.apply_to(user_context);
        use_sqlite_provider(user_context, global, transport);
        auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
        user_context.set_internal_id_generator(user_context, ids);
        auto& cache = user_context.make<SqlExecutorState>();
        auto executor = user_context.make_ref<constant_id_fixture::ServiceRuntimeExecutor>(
            user_context.require_resource<SqliteDialect>().value().get(),
            transport,
            module.metadata,
            cache);
        user_context.register_executor(executor);
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
                fixture_sql(database,
                            "UPDATE status_data SET code='STALE',version=3 WHERE code='BASIC'");
            }
            if (state == "missing") {
                fixture_sql(database, "DELETE FROM status_data WHERE code='BASIC'");
            }
            if (state == "deleted") {
                fixture_sql(database, "UPDATE status_data SET version=-1 WHERE code='BASIC'");
            }
            if (state == "extra") {
                fixture_sql(
                    database,
                    "INSERT INTO status_data(id,platform,code,version) VALUES(777,1,'EXTRA',4)");
            }
        }
    }
    Json::Array rounds(r);
    auto count = std::get<std::uint64_t>(field(input, "rounds").data);
    for (unsigned index = 0; index < count; ++index) {
        Json::Object round(r);
        round.emplace("before", snapshot(r, database));
        auto user_context = create();
        const auto& result = user_context->ensure_schema().wait();
        round.emplace("error", result ? Json(r) : Json(r, result.error().to_string(r)));
        auto root = user_context->require_active_root("Platform");
        round.emplace("root",
                      root ? Json(r, root.value().get().id) : Json(r, root.error().to_string(r)));
        round.emplace("after", snapshot(r, database));
        user_context.reset();
        rounds.emplace_back(round);
    }
    Json::Object out(r);
    out.emplace("warmup_error", std::move(warmup_error));
    out.emplace("rounds", Json(r, rounds));
    return Json(r, out);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto fixture_context = global.make_user_context();
            auto r = fixture_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("Invalid constant ID execution input");
            }
            std::cout << observe(global, *input, r).dump(r) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
