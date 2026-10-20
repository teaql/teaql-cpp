#include "../generated_constants_temporal_required/include/cpp_constants_temporal_required.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <cstdlib>
#include <iostream>
#include <sqlite3.h>

using namespace teaql;

static Json database_view(Memory r, sqlite3* database) {
    Json::Object output(r);
    for (auto name : {"platform_data", "priority_data", "label_data", "teaql_id_space"}) {
        std::string sql = "SELECT * FROM " + std::string(name) + " ORDER BY " +
                          (std::string_view(name) == "teaql_id_space" ? "type_name" : "id");
        sqlite3_stmt* raw = nullptr;
        if (sqlite3_prepare_v2(database, sql.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
        Json::Array rows(r);
        int status = SQLITE_OK;
        while ((status = sqlite3_step(statement.get())) == SQLITE_ROW) {
            Json::Object row(r);
            for (int index = 0; index < sqlite3_column_count(statement.get()); ++index) {
                Json value(r);
                switch (sqlite3_column_type(statement.get(), index)) {
                case SQLITE_NULL:
                    break;
                case SQLITE_INTEGER:
                    value = Json(
                        r, static_cast<std::int64_t>(sqlite3_column_int64(statement.get(), index)));
                    break;
                case SQLITE_FLOAT:
                    value = Json(r, sqlite3_column_double(statement.get(), index));
                    break;
                case SQLITE_TEXT:
                    value = Json(r,
                                 std::string_view(reinterpret_cast<const char*>(
                                                      sqlite3_column_text(statement.get(), index)),
                                                  static_cast<std::size_t>(sqlite3_column_bytes(
                                                      statement.get(), index))));
                    break;
                default:
                    throw std::runtime_error("Unexpected temporal fixture SQLite value");
                }
                row.emplace(sqlite3_column_name(statement.get(), index), std::move(value));
            }
            rows.emplace_back(row);
        }
        if (status != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        output.emplace(name, Json(r, rows));
    }
    return Json(r, output);
}

static std::int64_t signed_number(const Json& value) {
    if (auto* signed_value = std::get_if<std::int64_t>(&value.data)) {
        return *signed_value;
    }
    return static_cast<std::int64_t>(std::get<std::uint64_t>(value.data));
}

static Json observe(GlobalContext& global, const Json& input) {
    namespace domain = cpp_constants_temporal_required;
    auto r = global.memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto business_millis = signed_number(field(input, "business_millis"));
    auto create = [&](UserContext& user_context, std::string_view mode) {
        auto& module = mode == "plain"      ? domain::module(user_context)
                       : mode == "checker"  ? domain::moduleWithCheckers(user_context)
                       : mode == "behavior" ? domain::moduleWithBehaviors(user_context)
                                            : domain::moduleWithBehaviorsAndCheckers(user_context);
        module.apply_to(user_context);
        use_sqlite_provider(user_context, global, transport);
        auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
        user_context.set_internal_id_generator(user_context, ids);
        auto dialect = user_context.require_resource<SqliteDialect>();
        auto& state = user_context.make<SqlExecutorState>();
        auto executor = user_context.make_ref<domain::DataServiceExecutor>(
            domain::require_value(dialect).get(), transport, module.metadata, state);
        user_context.register_executor(executor);
        auto seconds = business_millis / 1000;
        auto remainder = business_millis % 1000;
        if (remainder < 0) {
            --seconds;
            remainder += 1000;
        }
        auto& clock = user_context.make<FixedBusinessClock>(
            UtcDateTime::from_timestamp(seconds, static_cast<std::uint32_t>(remainder * 1000000))
                .value());
        user_context.set_business_clock(user_context, clock);
        user_context.disable_sql_log();
    };
    auto warmup = global.make_user_context();
    create(*warmup, "combined");
    const auto& initial = warmup->ensure_schema().wait();
    if (!initial) {
        Json::Object output(r);
        output.emplace("wall_millis", Json(r, Timestamp::now().as_millis()));
        output.emplace("warmup_error", Json(r, initial.error().to_string(r)));
        return Json(r, output);
    }
    warmup.reset();
    auto state = text(field(input, "state"));
    std::string_view fixture_sql;
    if (state == "empty") {
        fixture_sql = "DELETE FROM label_data;DELETE FROM priority_data;DELETE FROM platform_data;";
    } else if (state == "corrupt") {
        fixture_sql =
            "UPDATE priority_data SET name='stale',instant=0,clock='stale',optional_clock=NULL,created_at=0,updated_at=0 WHERE id=1;";
    } else if (state == "missing") {
        fixture_sql = "DELETE FROM label_data WHERE id=32;DELETE FROM priority_data WHERE id=22;";
    } else if (state == "deleted") {
        fixture_sql = "UPDATE priority_data SET version=-1 WHERE id=1;";
    } else if (state == "extra") {
        fixture_sql =
            "INSERT INTO label_data(id,code,platform,priority,version) VALUES(99,'CUSTOM',1,1,7);";
    } else if (state != "existing") {
        throw std::runtime_error("Unknown temporal fixture state");
    }
    if (sqlite3_exec(connection.native_handle(),
                     std::string(fixture_sql).c_str(),
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    Json::Array rounds(r);
    auto count = std::get<std::uint64_t>(field(input, "rounds").data);
    for (std::uint64_t index = 0; index < count; ++index) {
        auto wall =
            signed_number(field(input, "wall_millis")) +
            static_cast<std::int64_t>(index + 1) * signed_number(field(input, "tick_per_round"));
        if (setenv("TEAQL_TEST_SAMPLE_WALL_MILLIS", std::to_string(wall).c_str(), 1) != 0) {
            throw std::runtime_error("Cannot configure fixture wall clock");
        }
        auto user_context = global.make_user_context();
        create(*user_context, text(field(input, "module")));
        if (std::get<bool>(field(input, "logging").data)) {
            user_context->enable_all_sql_log();
        }
        auto before = database_view(r, connection.native_handle());
        const auto& result = user_context->ensure_schema().wait();
        Json::Object round(r);
        round.emplace("before", std::move(before));
        round.emplace("after", database_view(r, connection.native_handle()));
        round.emplace("error", result ? Json(r) : Json(r, result.error().to_string(r)));
        auto root = user_context->require_active_root("Platform");
        round.emplace("root", root ? Json(r, root.value().get().id) : Json(r));
        auto business_time = user_context->business_time();
        round.emplace("business_millis",
                      Json(r,
                           business_time.timestamp() * 1000 +
                               business_time.timestamp_subsec_nanos() / 1000000));
        Json::Array evidence(r), logs(r);
        for (const auto& value : user_context->last_fix_evidence()) {
            Json::Object item(r);
            item.emplace("entity", Json(r, value.entity_type));
            item.emplace("path", Json(r, value.model_path));
            item.emplace("source", Json(r, fix_evidence_source_name(value.source)));
            item.emplace("label", Json(r, value.source_label));
            evidence.emplace_back(item);
        }
        for (const auto& entry : user_context->sql_logs()) {
            Json::Object item(r);
            item.emplace("sql", Json(r, entry.sql));
            item.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
            item.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
            item.emplace("audit_as", entry.audit_reason ? Json(r, *entry.audit_reason) : Json(r));
            Json::Array parameters(r);
            for (const auto& parameter : entry.params) {
                parameters.emplace_back(debug_string(r, parameter));
            }
            item.emplace("params", Json(r, parameters));
            logs.emplace_back(item);
        }
        round.emplace("evidence", Json(r, evidence));
        round.emplace("logs", Json(r, logs));
        rounds.emplace_back(round);
        user_context.reset();
    }
    if (setenv("TEAQL_TEST_SAMPLE_WALL_MILLIS",
               std::to_string(signed_number(field(input, "wall_millis"))).c_str(),
               1) != 0) {
        throw std::runtime_error("Cannot restore fixture wall clock");
    }
    Json::Object output(r);
    output.emplace("wall_millis", Json(r, Timestamp::now().as_millis()));
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
                throw std::runtime_error("Invalid temporal bootstrap fixture input");
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
