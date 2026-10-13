#include "../generated_candidates/include/cpp_candidates.hpp"
#include "fixture_value.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace cpp_candidates;

static void sql(sqlite3* database, const std::string& statement) {
    if (sqlite3_exec(database, statement.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json rows(Memory r, sqlite3* database, const std::string& query) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database, query.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array result(r);
    int status = SQLITE_OK;
    while ((status = sqlite3_step(statement.get())) == SQLITE_ROW) {
        Json::Array row(r);
        for (int index = 0; index < sqlite3_column_count(statement.get()); ++index) {
            if (sqlite3_column_type(statement.get(), index) == SQLITE_NULL) {
                row.emplace_back();
            } else if (sqlite3_column_type(statement.get(), index) == SQLITE_INTEGER) {
                row.emplace_back(
                    static_cast<std::int64_t>(sqlite3_column_int64(statement.get(), index)));
            } else {
                row.emplace_back(std::string_view(
                    reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), index))));
            }
        }
        result.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, result);
}

static Json database_view(Memory r, sqlite3* database) {
    Json::Object out(r);
    out.emplace(String("platform_data", r),
                rows(r, database, "SELECT id,name,version FROM platform_data ORDER BY id"));
    out.emplace(String("order_status_data", r),
                rows(r,
                     database,
                     "SELECT id,name,code,platform,version FROM order_status_data ORDER BY id"));
    auto existence =
        rows(r,
             database,
             "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='teaql_id_space'");
    bool exists =
        std::get<std::uint64_t>(
            std::get<Json::Array>(std::get<Json::Array>(existence.data)[0].data)[0].data) != 0;
    out.emplace(String("ids", r),
                exists
                    ? rows(r,
                           database,
                           "SELECT type_name,current_level FROM teaql_id_space ORDER BY type_name")
                    : Json(r, Json::Array(r)));
    return Json(r, out);
}

struct FailureCounter {
    unsigned attempts = 0, failures = 0;
};

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto executor = configureRuntime(user_context);
    auto* database = user_context.get(executor).transport.connection().native_handle();
    auto state = text(field(input, "state"));
    std::string definition =
        "CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT NOT NULL,version INTEGER NOT NULL);CREATE TABLE order_status_data(id INTEGER PRIMARY KEY,name TEXT NOT NULL,code TEXT NOT NULL,platform INTEGER NOT NULL,version INTEGER NOT NULL,FOREIGN KEY(platform) REFERENCES platform_data(id));";
    if (state == "null_fields") {
        for (auto position = definition.find(" NOT NULL"); position != std::string::npos;
             position = definition.find(" NOT NULL")) {
            definition.erase(position, 9);
        }
    }
    sql(database, definition);
    if (state != "empty") {
        sql(database,
            "INSERT INTO platform_data VALUES(1,'existing root',7);INSERT INTO order_status_data VALUES(11,'Draft order','DRAFT',1,3),(22,'In progress order','IN_PROGRESS',1,4),(2104194,'Completed order','DONE',1,5);");
    }
    if (state == "corrupt") {
        sql(database, "UPDATE order_status_data SET name='stale',code='OLD' WHERE id=11;");
    } else if (state == "missing") {
        sql(database, "DELETE FROM order_status_data WHERE id=22;");
    } else if (state == "deleted") {
        sql(database, "UPDATE order_status_data SET version=-3 WHERE id=11;");
    } else if (state == "foreign_root") {
        sql(database,
            "INSERT INTO platform_data VALUES(2,'other',3);UPDATE order_status_data SET platform=2 WHERE id=11;");
    } else if (state == "extra") {
        sql(database, "INSERT INTO order_status_data VALUES(999,'Custom','CUSTOM',1,8);");
    } else if (state == "null_fields") {
        sql(database, "UPDATE order_status_data SET name=NULL,code=NULL WHERE id=11;");
    } else if (state == "root_mismatch") {
        user_context.with_active_root("Platform", std::uint64_t{42});
    }
    auto& counter = user_context.make<FailureCounter>();
    counter.failures =
        static_cast<unsigned>(std::get<std::uint64_t>(field(input, "failures").data));
    if (sqlite3_create_function_v2(
            database,
            "bootstrap_failure",
            0,
            SQLITE_UTF8,
            &counter,
            [](sqlite3_context* callback, int, sqlite3_value**) {
                auto* value = static_cast<FailureCounter*>(sqlite3_user_data(callback));
                sqlite3_result_int(callback, value->attempts++ < value->failures ? 1 : 0);
            },
            nullptr,
            nullptr,
            nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    auto fault = text(field(input, "fault"));
    auto message = fault == "primary"     ? "UNIQUE constraint failed: order_status_data.id"
                   : fault == "secondary" ? "UNIQUE constraint failed: order_status_data.code"
                                          : "fixture bootstrap write failed";
    if (counter.failures) {
        sql(database,
            "CREATE TRIGGER insert_failure BEFORE INSERT ON order_status_data WHEN NEW.id=11 AND bootstrap_failure()!=0 BEGIN SELECT RAISE(ABORT,'" +
                std::string(message) +
                "'); END;CREATE TRIGGER update_failure BEFORE UPDATE ON order_status_data WHEN NEW.id=11 AND bootstrap_failure()!=0 BEGIN SELECT RAISE(ABORT,'" +
                std::string(message) + "'); END;");
    }
    Json::Array rounds(r);
    auto count = std::get<std::uint64_t>(field(input, "rounds").data);
    for (std::uint64_t index = 0; index < count; ++index) {
        auto before = database_view(r, database);
        const auto& result = user_context.ensure_schema().wait();
        Json::Object round(r);
        round.emplace(String("before", r), std::move(before));
        round.emplace(String("after", r), database_view(r, database));
        round.emplace(String("error", r), result ? Json(r) : Json(r, result.error().to_string(r)));
        round.emplace(String("attempts", r), Json(r, std::uint64_t(counter.attempts)));
        auto root = user_context.require_active_root("Platform");
        round.emplace(String("root", r), root ? Json(r, root.value().get().id) : Json(r));
        rounds.emplace_back(round);
    }
    return Json(r, rounds);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid bootstrap input");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
