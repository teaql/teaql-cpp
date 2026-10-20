#include "../generated_query_field_branches/include/cpp_query_branches.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <sstream>
using namespace cpp_query_branches;

static void initialize(UserContext& user_context) {
    auto& module = moduleWithBehaviorsAndCheckers(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<cpp_query_branches::DataServiceExecutor>(
        require_value(dialect).get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) +
                       "/tests/support/query_field_branches_sql.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    if (!file ||
        sqlite3_exec(connection.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    user_context.with_active_root("Tenant", 1);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    auto query = std::get<bool>(field(input, "minimal").data) ? Q.entriesMinimal() : Q.entries();
    auto op = text(field(input, "operation"));
    auto alias = text(field(input, "alias"));
    for (int n = 0; n < (std::get<bool>(field(input, "repeat").data) ? 2 : 1); ++n) {
        if (op == "select") {
            query.selectRatioWithFunction(AggregateFunction::Sum);
        } else if (op == "select_as") {
            query.selectRatioAsWithFunction(alias, AggregateFunction::Sum);
        } else if (op == "std") {
            query.standardDeviationRatio();
        } else if (op == "std_as") {
            query.standardDeviationRatioAs(alias);
        } else if (op == "pop") {
            query.squareRootOfPopulationStandardDeviationRatio();
        } else if (op == "pop_as") {
            query.squareRootOfPopulationStandardDeviationRatioAs(alias);
        } else if (op == "var") {
            query.sampleVarianceRatio();
        } else if (op == "var_as") {
            query.sampleVarianceRatioAs(alias);
        } else if (op == "var_pop") {
            query.samplePopulationVarianceRatio();
        } else if (op == "var_pop_as") {
            query.samplePopulationVarianceRatioAs(alias);
        } else if (op == "reference_value") {
            query.filterByTenant(Value(r, std::get<std::uint64_t>(field(input, "id").data)));
        } else {
            query.filterByTenant(std::get<std::uint64_t>(field(input, "id").data));
        }
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "query", debug_string(r, query.query()));
    initialize(user_context);
    const auto& result = std::move(query)
                             .comment("inspect numeric and reference fields")
                             .purpose("qualify numeric and reference SQL")
                             .executeForRows(user_context)
                             .wait();
    Json::Array rows(r);
    Json error(r);
    if (result) {
        for (const auto& row : result.value().data) {
            Json::Array columns(r);
            for (auto key : {std::string_view("id"),
                             std::string_view("version"),
                             std::string_view("ratio"),
                             std::string_view("tenant"),
                             alias}) {
                auto* item = row.get(key);
                columns.emplace_back(item ? Json(r, debug_string(r, *item)) : Json(r));
            }
            rows.emplace_back(columns);
        }
    } else {
        error = Json(r, result.error().to_string(r));
    }
    detail::model_put(output, "rows", Json(r, rows));
    detail::model_put(output, "error", error);
    return output;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Extension retained User memory");
            }
        }
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
