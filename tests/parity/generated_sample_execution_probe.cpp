#include "../support/sample_data_fixture.hpp"
#include "fixture_value.hpp"
#include <iostream>
using namespace cpp_relations;

static Json table_rows(Memory r, sqlite3* database, std::string_view table) {
    std::string query =
        "SELECT id,name,platform,version FROM " + std::string(table) + " ORDER BY id";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database, query.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array rows(r);
    int status;
    while ((status = sqlite3_step(raw)) == SQLITE_ROW) {
        Json::Array row(r);
        for (int column = 0; column < 4; ++column) {
            switch (sqlite3_column_type(raw, column)) {
            case SQLITE_NULL:
                row.emplace_back();
                break;
            case SQLITE_INTEGER:
                row.emplace_back(static_cast<std::int64_t>(sqlite3_column_int64(raw, column)));
                break;
            default:
                row.emplace_back(std::string_view(
                    reinterpret_cast<const char*>(sqlite3_column_text(raw, column))));
                break;
            }
        }
        rows.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, rows);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto* database = teaql::test::initialize_sample_database(
        user_context, std::get<bool>(field(input, "root").data));
    if (auto* target = optional(input, "fault_model")) {
        auto model = text(*target);
        if (model != "device" && model != "profile" && model != "school") {
            throw std::runtime_error("Invalid sample fault model");
        }
        auto fail_at = std::get<std::uint64_t>(field(input, "fail_at").data);
        std::string statement = "CREATE TRIGGER sample_failure BEFORE INSERT ON " +
                                std::string(model) +
                                "_data WHEN NEW.id=" + std::to_string(fail_at) +
                                " BEGIN SELECT RAISE(ABORT,'sample fixture write failed'); END;";
        if (sqlite3_exec(database, statement.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
    }
    if (auto* load_error = optional(input, "root_query_error");
        load_error && std::get<bool>(load_error->data)) {
        if (sqlite3_exec(database, "DROP TABLE platform_data;", nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
    }
    auto scale_text = text(field(input, "scale"));
    auto scale = scale_text == "Tiny"    ? SampleDataScale::Tiny
                 : scale_text == "Small" ? SampleDataScale::Small
                                         : SampleDataScale::Medium;
    auto seed = std::get<std::uint64_t>(field(input, "seed").data);
    Json::Array rounds(r);
    for (std::uint64_t index = 0; index < std::get<std::uint64_t>(field(input, "rounds").data);
         ++index) {
        const auto& result = generateSampleData(user_context, {scale, seed}).wait();
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
        Json::Object tables(r);
        for (auto table : {"device_data", "profile_data", "school_data"}) {
            tables.emplace(String(table, r), table_rows(r, database, table));
        }
        Json::Object round(r);
        round.emplace(String("report", r), Json(r, report));
        round.emplace(String("tables", r), Json(r, tables));
        rounds.emplace_back(round);
    }
    return Json(r, rounds);
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
                throw std::runtime_error("Invalid sample input");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
