#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include "fixture_sql.hpp"
#include <sqlite3.h>
#include <iostream>

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& provider = user_context.make_resource<SqliteMutationExecutor>(connection);
    auto seed = text(field(input, "seed_sql"));
    if (sqlite3_exec(connection.native_handle(), seed.data(), nullptr, nullptr, nullptr) !=
        SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    auto descriptor = entity(r, field(input, "entity"));
    auto request = query(r, field(input, "query"));
    auto output = detail::model_json(r);
    SqliteDialect dialect;
    try {
        auto compiled = dialect.compile_select(r, descriptor, request);
        auto rows = provider.fetch_all_compact(r, compiled);
        if (!rows) {
            throw std::runtime_error(std::string(rows.error().to_string(r)));
        }
        detail::model_put(output, "sql", compiled.sql);
        Json::Array params(r), policies(r), values(r);
        for (const auto& value : compiled.params) {
            params.emplace_back(value.to_json_value(r));
        }
        for (auto policy : compiled.log_context.parameter_policies) {
            policies.emplace_back(static_cast<unsigned>(policy));
        }
        for (const auto& row : rows.value()) {
            values.emplace_back(compact_row_to_json_value(r, row));
        }
        detail::model_put(output, "params", params);
        detail::model_put(output, "policies", policies);
        detail::model_put(output, "rows", values);
    } catch (const SqlCompileError& error) {
        detail::model_put(output, "compile_error", error.to_string(r));
    }
    return output;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid grouped partition fixture");
        }
        std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
