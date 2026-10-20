#include "../generator/parity/query_alias_contracts/cpp/include/cpp_query_aliases.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sqlite3.h>
using namespace cpp_query_aliases;

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    auto& sqlite = user_context.get(connection);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/query_alias_sql.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    if (!file ||
        sqlite3_exec(sqlite.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(sqlite.native_handle()));
    }
    require_value(serviceRuntimeFromConnection(user_context, connection));
    user_context.with_active_root("Platform", 1);
    auto Q = user_context.queries<query_factory>();
    auto query = std::get<bool>(field(input, "minimal").data) ? Q.recordsMinimal() : Q.records();
    auto name = text(field(input, "field"));
    auto op = text(field(input, "operation"));
    auto* alias = std::get_if<String>(&field(input, "alias").data);
    for (int repeat = 0; repeat < (std::get<bool>(field(input, "repeat").data) ? 2 : 1); ++repeat) {
        if (name == "class" && op == "count") {
            if (alias) {
                query.countModelClassAs(*alias);
            } else {
                query.countModelClass();
            }
        } else if (name == "class" && op == "sum") {
            if (alias) {
                query.sumModelClassAs(*alias);
            } else {
                query.sumModelClass();
            }
        } else if (name == "class" && op == "avg") {
            if (alias) {
                query.avgModelClassAs(*alias);
            } else {
                query.avgModelClass();
            }
        } else if (name == "class" && op == "min") {
            if (alias) {
                query.minModelClassAs(*alias);
            } else {
                query.minModelClass();
            }
        } else if (name == "class" && op == "max") {
            if (alias) {
                query.maxModelClassAs(*alias);
            } else {
                query.maxModelClass();
            }
        } else if (name == "class" && op == "std") {
            if (alias) {
                query.standardDeviationModelClassAs(*alias);
            } else {
                query.standardDeviationModelClass();
            }
        } else if (name == "class" && op == "pop") {
            if (alias) {
                query.squareRootOfPopulationStandardDeviationModelClassAs(*alias);
            } else {
                query.squareRootOfPopulationStandardDeviationModelClass();
            }
        } else if (name == "class" && op == "var") {
            if (alias) {
                query.sampleVarianceModelClassAs(*alias);
            } else {
                query.sampleVarianceModelClass();
            }
        } else if (name == "class" && op == "var_pop") {
            if (alias) {
                query.samplePopulationVarianceModelClassAs(*alias);
            } else {
                query.samplePopulationVarianceModelClass();
            }
        } else if (name == "operator" && op == "count") {
            if (alias) {
                query.countModelOperatorAs(*alias);
            } else {
                query.countModelOperator();
            }
        } else if (name == "operator" && op == "sum") {
            if (alias) {
                query.sumModelOperatorAs(*alias);
            } else {
                query.sumModelOperator();
            }
        } else if (name == "operator" && op == "avg") {
            if (alias) {
                query.avgModelOperatorAs(*alias);
            } else {
                query.avgModelOperator();
            }
        } else if (name == "operator" && op == "min") {
            if (alias) {
                query.minModelOperatorAs(*alias);
            } else {
                query.minModelOperator();
            }
        } else if (name == "operator" && op == "max") {
            if (alias) {
                query.maxModelOperatorAs(*alias);
            } else {
                query.maxModelOperator();
            }
        } else if (name == "operator" && op == "std") {
            if (alias) {
                query.standardDeviationModelOperatorAs(*alias);
            } else {
                query.standardDeviationModelOperator();
            }
        } else if (name == "operator" && op == "pop") {
            if (alias) {
                query.squareRootOfPopulationStandardDeviationModelOperatorAs(*alias);
            } else {
                query.squareRootOfPopulationStandardDeviationModelOperator();
            }
        } else if (name == "operator" && op == "var") {
            if (alias) {
                query.sampleVarianceModelOperatorAs(*alias);
            } else {
                query.sampleVarianceModelOperator();
            }
        } else if (name == "operator" && op == "var_pop") {
            if (alias) {
                query.samplePopulationVarianceModelOperatorAs(*alias);
            } else {
                query.samplePopulationVarianceModelOperator();
            }
        } else if (name == "template" && op == "count") {
            if (alias) {
                query.countModelTemplateAs(*alias);
            } else {
                query.countModelTemplate();
            }
        } else if (name == "template" && op == "sum") {
            if (alias) {
                query.sumModelTemplateAs(*alias);
            } else {
                query.sumModelTemplate();
            }
        } else if (name == "template" && op == "avg") {
            if (alias) {
                query.avgModelTemplateAs(*alias);
            } else {
                query.avgModelTemplate();
            }
        } else if (name == "template" && op == "min") {
            if (alias) {
                query.minModelTemplateAs(*alias);
            } else {
                query.minModelTemplate();
            }
        } else if (name == "template" && op == "max") {
            if (alias) {
                query.maxModelTemplateAs(*alias);
            } else {
                query.maxModelTemplate();
            }
        } else if (name == "template" && op == "std") {
            if (alias) {
                query.standardDeviationModelTemplateAs(*alias);
            } else {
                query.standardDeviationModelTemplate();
            }
        } else if (name == "template" && op == "pop") {
            if (alias) {
                query.squareRootOfPopulationStandardDeviationModelTemplateAs(*alias);
            } else {
                query.squareRootOfPopulationStandardDeviationModelTemplate();
            }
        } else if (name == "template" && op == "var") {
            if (alias) {
                query.sampleVarianceModelTemplateAs(*alias);
            } else {
                query.sampleVarianceModelTemplate();
            }
        } else if (name == "template" && op == "var_pop") {
            if (alias) {
                query.samplePopulationVarianceModelTemplateAs(*alias);
            } else {
                query.samplePopulationVarianceModelTemplate();
            }
        } else if (name == "type" && op == "count") {
            if (alias) {
                query.countTypeAs(*alias);
            } else {
                query.countType();
            }
        } else if (name == "type" && op == "sum") {
            if (alias) {
                query.sumTypeAs(*alias);
            } else {
                query.sumType();
            }
        } else if (name == "type" && op == "avg") {
            if (alias) {
                query.avgTypeAs(*alias);
            } else {
                query.avgType();
            }
        } else if (name == "type" && op == "min") {
            if (alias) {
                query.minTypeAs(*alias);
            } else {
                query.minType();
            }
        } else if (name == "type" && op == "max") {
            if (alias) {
                query.maxTypeAs(*alias);
            } else {
                query.maxType();
            }
        } else if (name == "type" && op == "std") {
            if (alias) {
                query.standardDeviationTypeAs(*alias);
            } else {
                query.standardDeviationType();
            }
        } else if (name == "type" && op == "pop") {
            if (alias) {
                query.squareRootOfPopulationStandardDeviationTypeAs(*alias);
            } else {
                query.squareRootOfPopulationStandardDeviationType();
            }
        } else if (name == "type" && op == "var") {
            if (alias) {
                query.sampleVarianceTypeAs(*alias);
            } else {
                query.sampleVarianceType();
            }
        } else if (name == "type" && op == "var_pop") {
            if (alias) {
                query.samplePopulationVarianceTypeAs(*alias);
            } else {
                query.samplePopulationVarianceType();
            }
        } else if (name == "match" && op == "count") {
            if (alias) {
                query.countMatchAs(*alias);
            } else {
                query.countMatch();
            }
        } else if (name == "match" && op == "sum") {
            if (alias) {
                query.sumMatchAs(*alias);
            } else {
                query.sumMatch();
            }
        } else if (name == "match" && op == "avg") {
            if (alias) {
                query.avgMatchAs(*alias);
            } else {
                query.avgMatch();
            }
        } else if (name == "match" && op == "min") {
            if (alias) {
                query.minMatchAs(*alias);
            } else {
                query.minMatch();
            }
        } else if (name == "match" && op == "max") {
            if (alias) {
                query.maxMatchAs(*alias);
            } else {
                query.maxMatch();
            }
        } else if (name == "match" && op == "std") {
            if (alias) {
                query.standardDeviationMatchAs(*alias);
            } else {
                query.standardDeviationMatch();
            }
        } else if (name == "match" && op == "pop") {
            if (alias) {
                query.squareRootOfPopulationStandardDeviationMatchAs(*alias);
            } else {
                query.squareRootOfPopulationStandardDeviationMatch();
            }
        } else if (name == "match" && op == "var") {
            if (alias) {
                query.sampleVarianceMatchAs(*alias);
            } else {
                query.sampleVarianceMatch();
            }
        } else if (name == "match" && op == "var_pop") {
            if (alias) {
                query.samplePopulationVarianceMatchAs(*alias);
            } else {
                query.samplePopulationVarianceMatch();
            }
        } else if (name == "ordinary" && op == "count") {
            if (alias) {
                query.countOrdinaryAs(*alias);
            } else {
                query.countOrdinary();
            }
        } else if (name == "ordinary" && op == "sum") {
            if (alias) {
                query.sumOrdinaryAs(*alias);
            } else {
                query.sumOrdinary();
            }
        } else if (name == "ordinary" && op == "avg") {
            if (alias) {
                query.avgOrdinaryAs(*alias);
            } else {
                query.avgOrdinary();
            }
        } else if (name == "ordinary" && op == "min") {
            if (alias) {
                query.minOrdinaryAs(*alias);
            } else {
                query.minOrdinary();
            }
        } else if (name == "ordinary" && op == "max") {
            if (alias) {
                query.maxOrdinaryAs(*alias);
            } else {
                query.maxOrdinary();
            }
        } else if (name == "ordinary" && op == "std") {
            if (alias) {
                query.standardDeviationOrdinaryAs(*alias);
            } else {
                query.standardDeviationOrdinary();
            }
        } else if (name == "ordinary" && op == "pop") {
            if (alias) {
                query.squareRootOfPopulationStandardDeviationOrdinaryAs(*alias);
            } else {
                query.squareRootOfPopulationStandardDeviationOrdinary();
            }
        } else if (name == "ordinary" && op == "var") {
            if (alias) {
                query.sampleVarianceOrdinaryAs(*alias);
            } else {
                query.sampleVarianceOrdinary();
            }
        } else if (name == "ordinary" && op == "var_pop") {
            if (alias) {
                query.samplePopulationVarianceOrdinaryAs(*alias);
            } else {
                query.samplePopulationVarianceOrdinary();
            }
        } else if (name == "namespace" && op == "count") {
            if (alias) {
                query.countModelNamespaceAs(*alias);
            } else {
                query.countModelNamespace();
            }
        } else {
            throw std::logic_error("unknown alias fixture");
        }
    }
    Json::Object output(r);
    output.emplace("query", Json(r, debug_string(r, query.query())));
    auto request = std::move(query)
                       .comment("inspect generated aggregate aliases")
                       .purpose("preserve authentic result keys");
    const auto& result = request.executeForRows(user_context).wait();
    Json::Array rows(r);
    Json error(r);
    if (result) {
        for (const auto& row : result.value().data) {
            Json::Object values(r);
            for (const auto& [key, value] : row.iter()) {
                values.insert_or_assign(String(key, r), Json(r, debug_string(r, value)));
            }
            rows.emplace_back(values);
        }
    } else {
        error = Json(r, result.error().to_string(r));
    }
    output.emplace("rows", Json(r, rows));
    output.emplace("error", std::move(error));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("invalid alias fixture");
                }
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("alias fixture retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
