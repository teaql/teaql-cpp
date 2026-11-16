#include "compiled_scalar_storage.hpp"
#include "fixture_value.hpp"
#include <iostream>
#include <sqlite3.h>
#include <type_traits>

using namespace compiled_scalar_storage;

static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::declval<const Reading&>().value())>,
                             TEAQL_EXPECTED_VALUE_TYPE>);

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    require_value(serviceRuntimeFromConnection(user_context, connection));
    user_context.with_active_root("Gateway", 1);
    auto sql = text(field(input, "sql"));
    if (sqlite3_exec(user_context.get(connection).native_handle(),
                     std::string(sql).c_str(),
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error("Scalar SQLite fixture initialization failed");
    }
    auto Q = user_context.queries<query_factory>();
    auto query = Q.readings().runtimeQuery();
    query.orderAsc("id");
    const auto& result = std::move(query)
                             .comment("Read compiler-resolved scalar storage")
                             .purpose("Verify typed values through SQLite and expressions")
                             .executeForList(user_context)
                             .wait();
    if (!result) {
        throw std::runtime_error(std::string(result.error().to_string(r)));
    }
    Json::Array rows(r), logs(r);
    for (const auto& reading : result.value().data) {
        Json::Object row(r);
        row.emplace("getter", generated_detail::encode(r, reading.value()).to_json_value(r));
        row.emplace(
            "expression",
            generated_detail::encode(r, E::reading(reading).getValue().eval()).to_json_value(r));
        EntityJsonTraversal traversal(std::allocator_arg, Allocator(r));
        auto json = reading.borrowed_json(r, traversal);
        if (!json) {
            throw std::runtime_error("Scalar JSON traversal failed");
        }
        row.emplace("json", std::move(*json));
        rows.emplace_back(std::move(row));
    }
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        log.emplace("count", entry.result_count ? Json(r, *entry.result_count) : Json(r));
        logs.emplace_back(std::move(log));
    }
    Json::Object output(r);
    output.emplace("rows", Json(r, std::move(rows)));
    output.emplace("sql", Json(r, std::move(logs)));
    return Json(r, std::move(output));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        {
            auto r = user_context->memory_resource();
            auto input = Json::parse_result(r, line);
            std::cout << observe(*user_context, require_value(input)).dump(r) << '\n';
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Scalar consumer retained UserContext memory");
        }
    }
}
