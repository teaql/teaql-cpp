#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include "fixture_sql.hpp"
#include <sqlite3.h>
#include <iostream>

struct CachedPartitionSchema final : SchemaProvider {
    const EntityDescriptor& entity;

    explicit CachedPartitionSchema(const EntityDescriptor& entity) : entity(entity) {}

    const EntityDescriptor* get_entity(std::string_view name) const override {
        return name == entity.name ? &entity : nullptr;
    }
};

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& provider = user_context.make_resource<SqliteMutationExecutor>(connection);
    if (sqlite3_exec(connection.native_handle(),
                     text(field(input, "seed_sql")).data(),
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    auto& descriptor = user_context.make<EntityDescriptor>(entity(r, field(input, "entity")));
    auto& schema = user_context.make_resource<CachedPartitionSchema>(descriptor);
    auto& dialect = user_context.make_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto& executor = user_context.make<SqlDataServiceExecutor<MutationExecutorError>>(
        dialect, provider, schema, state);
    Json::Array results(r);
    for (const auto& value : items(input, "queries")) {
        auto request_query = query(r, value);
        request_query.entity = descriptor.name;
        auto intent =
            QueryIntent::create(r, "Inspect cached grouped rows", "Verify SQLite HAVING rebinding");
        QueryRequest request(std::allocator_arg, Allocator(r), request_query, intent.value());
        const auto& result = executor.query(r, std::move(request)).wait();
        auto output = detail::model_json(r);
        if (!result) {
            detail::model_put(output, "error", result.error().to_string(r));
        } else {
            Json::Array rows(r), params(r), policies(r);
            for (const auto& row : result.value().rows) {
                rows.emplace_back(compact_row_to_json_value(r, row));
            }
            for (const auto& param : result.value().metadata.params) {
                params.emplace_back(param.to_json_value(r));
            }
            for (auto policy : result.value().metadata.sql_log.parameter_policies) {
                policies.emplace_back(static_cast<unsigned>(policy));
            }
            detail::model_put(output, "rows", rows);
            detail::model_put(output, "params", params);
            detail::model_put(output, "policies", policies);
            const auto& sql = result.value().metadata.parameterized_query;
            detail::model_put(output, "sql", sql ? Json(r, *sql) : Json(r));
        }
        results.emplace_back(std::move(output));
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "results", results);
    return output;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid cached partition input");
        }
        std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
