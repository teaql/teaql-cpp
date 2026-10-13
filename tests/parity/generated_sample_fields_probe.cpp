#include "../generated_sample_fields_runtime/include/cpp_sample_runtime.hpp"
#include "../generated_sample_fields_temporal/include/cpp_sample_temporal.hpp"
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <iostream>
using namespace teaql;

static constexpr std::string_view schema =
    "CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);CREATE TABLE sample_record_data(id INTEGER PRIMARY KEY,platform INTEGER,label TEXT,quantity INTEGER,zero_bound INTEGER,negative_max INTEGER,signed_max INTEGER,wide_max INTEGER,negative_bound TEXT,amount NUMERIC(19,7),height REAL,optional_count INTEGER,active BOOLEAN,disabled BOOLEAN,optional_active BOOLEAN,day DATE,optional_day DATE,recorded_at INTEGER,optional_recorded_at INTEGER,clock_text TEXT,optional_clock_text TEXT,ratio REAL,payload TEXT,version INTEGER);CREATE TABLE sample_link_data(id INTEGER PRIMARY KEY,first_record INTEGER,second_record INTEGER,name TEXT,version INTEGER);";

template <class Executor, class Maker>
static sqlite3* configure(UserContext& user_context, Maker maker, bool root, bool temporal) {
    auto r = user_context.memory_resource();
    auto& module = maker(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
    user_context.set_internal_id_generator(user_context, ids);
    auto dialect = user_context.require_resource<SqliteDialect>();
    if (!dialect) {
        throw std::runtime_error(std::string(dialect.error().to_string(r)));
    }
    auto& state = user_context.make<SqlExecutorState>();
    auto executor =
        user_context.make_ref<Executor>(dialect.value().get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    user_context.with_active_root("Platform", 1);
    auto* database = connection.native_handle();
    std::string definition(schema);
    if (temporal) {
        definition.erase(definition.find(",payload TEXT"),
                         std::string_view(",payload TEXT").size());
    }
    if (sqlite3_exec(database, definition.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK ||
        (root && sqlite3_exec(database,
                              "INSERT INTO platform_data VALUES(1,'existing root',1);",
                              nullptr,
                              nullptr,
                              nullptr) != SQLITE_OK)) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return database;
}

static Json rows(Memory r, sqlite3* database, std::string_view table) {
    std::string query = "SELECT * FROM " + std::string(table) + " ORDER BY id";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database, query.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array out(r);
    int status;
    while ((status = sqlite3_step(raw)) == SQLITE_ROW) {
        Json::Object row(r);
        for (int index = 0; index < sqlite3_column_count(raw); ++index) {
            Json value(r);
            switch (sqlite3_column_type(raw, index)) {
            case SQLITE_INTEGER:
                value = Json(r, static_cast<std::int64_t>(sqlite3_column_int64(raw, index)));
                break;
            case SQLITE_FLOAT:
                value = Json(r, sqlite3_column_double(raw, index));
                break;
            case SQLITE_TEXT:
                value = Json(r,
                             std::string_view(
                                 reinterpret_cast<const char*>(sqlite3_column_text(raw, index))));
                break;
            case SQLITE_NULL:
                break;
            default:
                throw std::runtime_error("Unexpected sample storage");
            }
            row.emplace(String(sqlite3_column_name(raw, index), r), std::move(value));
        }
        out.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    bool temporal = text(field(input, "kind")) == "temporal";
    bool root = std::get<bool>(field(input, "root").data);
    auto wall = Timestamp::now().millis;
    auto* database =
        temporal
            ? configure<cpp_sample_temporal::DataServiceExecutor>(
                  user_context, cpp_sample_temporal::moduleWithBehaviorsAndCheckers, root, true)
            : configure<cpp_sample_runtime::DataServiceExecutor>(
                  user_context, cpp_sample_runtime::moduleWithBehaviorsAndCheckers, root, false);
    auto scale_text = text(field(input, "scale"));
    auto scale = scale_text == "Tiny"    ? SampleDataScale::Tiny
                 : scale_text == "Small" ? SampleDataScale::Small
                                         : SampleDataScale::Medium;
    auto seed = std::get<std::uint64_t>(field(input, "seed").data);
    Json::Array rounds(r);
    for (std::uint64_t index = 0; index < std::get<std::uint64_t>(field(input, "rounds").data);
         ++index) {
        auto& operation = temporal
                              ? cpp_sample_temporal::generateSampleData(user_context, {scale, seed})
                              : cpp_sample_runtime::generateSampleData(user_context, {scale, seed});
        const auto& result = operation.wait();
        Json::Object report(r);
        if (result) {
            Json::Object generated(r);
            for (const auto& [name, count] : result.value().generated) {
                generated.emplace(String(name, r), Json(r, std::uint64_t(count)));
            }
            Json::Array skipped(r);
            for (const auto& skip : result.value().skipped) {
                Json::Object item(r);
                item.emplace(String("entity", r), Json(r, skip.entity));
                item.emplace(String("reason", r), Json(r, skip.reason));
                skipped.emplace_back(item);
            }
            report.emplace(String("generated", r), Json(r, generated));
            report.emplace(String("skipped", r), Json(r, skipped));
            report.emplace(String("error", r), Json(r));
        } else {
            report.emplace(String("error", r), Json(r, result.error().to_string(r)));
        }
        Json::Object round(r);
        round.emplace(String("report", r), Json(r, report));
        round.emplace(String("records", r), rows(r, database, "sample_record_data"));
        round.emplace(String("links", r), rows(r, database, "sample_link_data"));
        rounds.emplace_back(round);
    }
    Json::Object result(r);
    result.emplace(String("wall_millis", r), Json(r, wall));
    result.emplace(String("rounds", r), Json(r, rounds));
    return Json(r, result);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            ContextLimits limits;
            limits.user.max_bytes = 1024ull * 1024 * 1024;
            limits.max_live_user_bytes = limits.user.max_bytes;
            GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid sample fields input");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
