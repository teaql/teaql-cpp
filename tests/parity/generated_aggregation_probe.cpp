#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include "../support/dynamic_json_runtime_fixture.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

static Json observe_nullable(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    auto query = std::get<bool>(field(input, "minimal").data) ? Q.profilesMinimal() : Q.profiles();
    for (const auto& operation : std::get<Json::Array>(field(input, "ops").data)) {
        auto name = text(field(operation, "op"));
        auto alias = optional(operation, "alias") ? text(field(operation, "alias")) : "metric";
        if (name == "count") {
            query.count();
        } else if (name == "count_as") {
            query.countAs(alias);
        } else if (name == "empty") {
            query.filter(Expr::lt(r, "id", std::int64_t(0)));
        } else if (name == "all_null") {
            query.filter(Expr::is_null(r, "note"));
        } else if (name == "with_deleted") {
            query.withDeletedRows();
        } else if (name == "unlimited") {
            query.unlimited();
        } else if (name == "count_note") {
            query.countNote();
        } else if (name == "count_note_as") {
            query.countNoteAs(alias);
        } else if (name == "sum_note") {
            query.sumNote();
        } else if (name == "sum_note_as") {
            query.sumNoteAs(alias);
        } else if (name == "avg_note") {
            query.avgNote();
        } else if (name == "avg_note_as") {
            query.avgNoteAs(alias);
        } else if (name == "min_note") {
            query.minNote();
        } else if (name == "min_note_as") {
            query.minNoteAs(alias);
        } else if (name == "max_note") {
            query.maxNote();
        } else if (name == "max_note_as") {
            query.maxNoteAs(alias);
        } else if (name == "group_by_note") {
            query.groupByNote();
        } else if (name == "group_by_note_as") {
            query.groupByNoteAs(alias);
        } else if (name == "group_by_note_with_function") {
            query.groupByNoteWithFunction(alias, AggregateFunction::Count);
        } else {
            throw std::runtime_error("unknown nullable operation");
        }
    }
    Json::Object out(r);
    out.emplace(String("query", r), Json(r, debug_string(r, query.query())));
    if (auto* execute = optional(input, "execute"); execute && std::get<bool>(execute->data)) {
        teaql::test::initialize_dynamic_json_database(user_context);
        const auto& result = std::move(query)
                                 .comment("inspect aggregation")
                                 .purpose("qualify aggregation")
                                 .executeForRows(user_context)
                                 .wait();
        Json::Array rows(r);
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array values(r);
                for (const auto& column : std::get<Json::Array>(field(input, "columns").data)) {
                    auto* value = row.get(text(column));
                    values.emplace_back(value ? Json(r, debug_string(r, *value)) : Json(r));
                }
                rows.emplace_back(values);
            }
            out.emplace(String("error", r), Json(r));
        } else {
            out.emplace(String("error", r), Json(r, result.error().to_string(r)));
        }
        out.emplace(String("rows", r), Json(r, rows));
    }
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    if (auto* entity = optional(input, "entity"); entity && text(*entity) == "profile") {
        return observe_nullable(user_context, input);
    }
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    auto query = std::get<bool>(field(input, "minimal").data) ? Q.ordersMinimal() : Q.orders();
    for (const auto& operation : std::get<Json::Array>(field(input, "ops").data)) {
        auto name = text(field(operation, "op"));
        auto column = optional(operation, "field") ? text(field(operation, "field")) : "priority";
        auto alias = optional(operation, "alias") ? text(field(operation, "alias")) : "metric";
        auto duration =
            request::dynamic_json_u64_field(std::get<Json::Object>(operation.data), "duration")
                .value_or(0);
        auto kind = optional(operation, "function") ? text(field(operation, "function")) : "Count";
        AggregateFunction function = AggregateFunction::Count;
        if (kind == "Count") {
            function = AggregateFunction::Count;
        }
        if (kind == "Sum") {
            function = AggregateFunction::Sum;
        }
        if (kind == "Avg") {
            function = AggregateFunction::Avg;
        }
        if (kind == "Min") {
            function = AggregateFunction::Min;
        }
        if (kind == "Max") {
            function = AggregateFunction::Max;
        }
        if (kind == "Stddev") {
            function = AggregateFunction::Stddev;
        }
        if (kind == "StddevPop") {
            function = AggregateFunction::StddevPop;
        }
        if (kind == "VarSamp") {
            function = AggregateFunction::VarSamp;
        }
        if (kind == "VarPop") {
            function = AggregateFunction::VarPop;
        }
        if (kind == "BitAnd") {
            function = AggregateFunction::BitAnd;
        }
        if (kind == "BitOr") {
            function = AggregateFunction::BitOr;
        }
        if (kind == "BitXor") {
            function = AggregateFunction::BitXor;
        }
        if (name == "group") {
            query.groupBy(column);
        } else if (name == "count") {
            query.count();
        } else if (name == "count_as") {
            query.countAs(alias);
        } else if (name == "aggregate_count") {
            query.aggregateCount(alias);
        } else if (name == "count_field") {
            query.aggregateCountField(column, alias);
        } else if (name == "function") {
            query.aggregateWithFunction(column, alias, function);
        } else if (name == "cache") {
            query.enableAggregationCache();
        } else if (name == "cache_for") {
            query.enableAggregationCacheFor(duration);
        } else if (name == "propagate") {
            query.propagateAggregationCache(duration);
        } else if (name == "empty") {
            query.filter(Expr::lt(r, "id", std::int64_t(0)));
        } else if (name == "with_deleted") {
            query.withDeletedRows();
        } else if (name == "unlimited") {
            query.unlimited();
        } else if (name == "aggregate_sum") {
            query.aggregateSum(column, alias);
        } else if (name == "aggregate_avg") {
            query.aggregateAvg(column, alias);
        } else if (name == "aggregate_min") {
            query.aggregateMin(column, alias);
        } else if (name == "aggregate_max") {
            query.aggregateMax(column, alias);
        } else if (name == "aggregate_stddev") {
            query.aggregateStddev(column, alias);
        } else if (name == "aggregate_stddev_pop") {
            query.aggregateStddevPop(column, alias);
        } else if (name == "aggregate_var_samp") {
            query.aggregateVarSamp(column, alias);
        } else if (name == "aggregate_var_pop") {
            query.aggregateVarPop(column, alias);
        } else if (name == "aggregate_bit_and") {
            query.aggregateBitAnd(column, alias);
        } else if (name == "aggregate_bit_or") {
            query.aggregateBitOr(column, alias);
        } else if (name == "aggregate_bit_xor") {
            query.aggregateBitXor(column, alias);
        } else if (name == "group_by_id") {
            query.groupById();
        } else if (name == "group_by_id_as") {
            query.groupByIdAs(alias);
        } else if (name == "group_by_id_with_function") {
            query.groupByIdWithFunction(alias, function);
        } else if (name == "count_id") {
            query.countId();
        } else if (name == "count_id_as") {
            query.countIdAs(alias);
        } else if (name == "sum_id") {
            query.sumId();
        } else if (name == "sum_id_as") {
            query.sumIdAs(alias);
        } else if (name == "avg_id") {
            query.avgId();
        } else if (name == "avg_id_as") {
            query.avgIdAs(alias);
        } else if (name == "min_id") {
            query.minId();
        } else if (name == "min_id_as") {
            query.minIdAs(alias);
        } else if (name == "max_id") {
            query.maxId();
        } else if (name == "max_id_as") {
            query.maxIdAs(alias);
        } else if (name == "group_by_version") {
            query.groupByVersion();
        } else if (name == "group_by_version_as") {
            query.groupByVersionAs(alias);
        } else if (name == "group_by_version_with_function") {
            query.groupByVersionWithFunction(alias, function);
        } else if (name == "count_version") {
            query.countVersion();
        } else if (name == "count_version_as") {
            query.countVersionAs(alias);
        } else if (name == "sum_version") {
            query.sumVersion();
        } else if (name == "sum_version_as") {
            query.sumVersionAs(alias);
        } else if (name == "avg_version") {
            query.avgVersion();
        } else if (name == "avg_version_as") {
            query.avgVersionAs(alias);
        } else if (name == "min_version") {
            query.minVersion();
        } else if (name == "min_version_as") {
            query.minVersionAs(alias);
        } else if (name == "max_version") {
            query.maxVersion();
        } else if (name == "max_version_as") {
            query.maxVersionAs(alias);
        } else if (name == "group_by_priority") {
            query.groupByPriority();
        } else if (name == "group_by_priority_as") {
            query.groupByPriorityAs(alias);
        } else if (name == "group_by_priority_with_function") {
            query.groupByPriorityWithFunction(alias, function);
        } else if (name == "count_priority") {
            query.countPriority();
        } else if (name == "count_priority_as") {
            query.countPriorityAs(alias);
        } else if (name == "sum_priority") {
            query.sumPriority();
        } else if (name == "sum_priority_as") {
            query.sumPriorityAs(alias);
        } else if (name == "avg_priority") {
            query.avgPriority();
        } else if (name == "avg_priority_as") {
            query.avgPriorityAs(alias);
        } else if (name == "min_priority") {
            query.minPriority();
        } else if (name == "min_priority_as") {
            query.minPriorityAs(alias);
        } else if (name == "max_priority") {
            query.maxPriority();
        } else if (name == "max_priority_as") {
            query.maxPriorityAs(alias);
        } else if (name == "group_by_amount") {
            query.groupByAmount();
        } else if (name == "group_by_amount_as") {
            query.groupByAmountAs(alias);
        } else if (name == "group_by_amount_with_function") {
            query.groupByAmountWithFunction(alias, function);
        } else if (name == "count_amount") {
            query.countAmount();
        } else if (name == "count_amount_as") {
            query.countAmountAs(alias);
        } else if (name == "sum_amount") {
            query.sumAmount();
        } else if (name == "sum_amount_as") {
            query.sumAmountAs(alias);
        } else if (name == "avg_amount") {
            query.avgAmount();
        } else if (name == "avg_amount_as") {
            query.avgAmountAs(alias);
        } else if (name == "min_amount") {
            query.minAmount();
        } else if (name == "min_amount_as") {
            query.minAmountAs(alias);
        } else if (name == "max_amount") {
            query.maxAmount();
        } else if (name == "max_amount_as") {
            query.maxAmountAs(alias);
        } else if (name == "group_by_code") {
            query.groupByCode();
        } else if (name == "group_by_code_as") {
            query.groupByCodeAs(alias);
        } else if (name == "group_by_code_with_function") {
            query.groupByCodeWithFunction(alias, function);
        } else if (name == "count_code") {
            query.countCode();
        } else if (name == "count_code_as") {
            query.countCodeAs(alias);
        } else if (name == "sum_code") {
            query.sumCode();
        } else if (name == "sum_code_as") {
            query.sumCodeAs(alias);
        } else if (name == "avg_code") {
            query.avgCode();
        } else if (name == "avg_code_as") {
            query.avgCodeAs(alias);
        } else if (name == "min_code") {
            query.minCode();
        } else if (name == "min_code_as") {
            query.minCodeAs(alias);
        } else if (name == "max_code") {
            query.maxCode();
        } else if (name == "max_code_as") {
            query.maxCodeAs(alias);
        } else if (name == "group_by_customer") {
            query.groupByCustomer();
        } else if (name == "group_by_customer_as") {
            query.groupByCustomerAs(alias);
        } else if (name == "group_by_customer_with_function") {
            query.groupByCustomerWithFunction(alias, function);
        } else if (name == "count_customer") {
            query.countCustomer();
        } else if (name == "count_customer_as") {
            query.countCustomerAs(alias);
        } else if (name == "standard_deviation_amount") {
            query.standardDeviationAmount();
        } else if (name == "standard_deviation_amount_as") {
            query.standardDeviationAmountAs(alias);
        } else if (name == "square_root_of_population_standard_deviation_amount") {
            query.squareRootOfPopulationStandardDeviationAmount();
        } else if (name == "square_root_of_population_standard_deviation_amount_as") {
            query.squareRootOfPopulationStandardDeviationAmountAs(alias);
        } else if (name == "sample_variance_amount") {
            query.sampleVarianceAmount();
        } else if (name == "sample_variance_amount_as") {
            query.sampleVarianceAmountAs(alias);
        } else if (name == "sample_population_variance_amount") {
            query.samplePopulationVarianceAmount();
        } else if (name == "sample_population_variance_amount_as") {
            query.samplePopulationVarianceAmountAs(alias);
        } else if (name == "standard_deviation_priority") {
            query.standardDeviationPriority();
        } else if (name == "standard_deviation_priority_as") {
            query.standardDeviationPriorityAs(alias);
        } else if (name == "square_root_of_population_standard_deviation_priority") {
            query.squareRootOfPopulationStandardDeviationPriority();
        } else if (name == "square_root_of_population_standard_deviation_priority_as") {
            query.squareRootOfPopulationStandardDeviationPriorityAs(alias);
        } else if (name == "sample_variance_priority") {
            query.sampleVariancePriority();
        } else if (name == "sample_variance_priority_as") {
            query.sampleVariancePriorityAs(alias);
        } else if (name == "sample_population_variance_priority") {
            query.samplePopulationVariancePriority();
        } else if (name == "sample_population_variance_priority_as") {
            query.samplePopulationVariancePriorityAs(alias);
        } else {
            throw std::runtime_error("unknown operation");
        }
    }
    Json::Object out(r);
    out.emplace(String("query", r), Json(r, debug_string(r, query.query())));
    if (auto* execute = optional(input, "execute"); execute && std::get<bool>(execute->data)) {
        teaql::test::initialize_dynamic_json_database(user_context);
        const auto& result = std::move(query)
                                 .comment("inspect aggregation")
                                 .purpose("qualify aggregation")
                                 .executeForRows(user_context)
                                 .wait();
        Json::Array rows(r);
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array values(r);
                for (const auto& column : std::get<Json::Array>(field(input, "columns").data)) {
                    auto* value = row.get(text(column));
                    values.emplace_back(value ? Json(r, debug_string(r, *value)) : Json(r));
                }
                rows.emplace_back(values);
            }
            out.emplace(String("error", r), Json(r));
        } else {
            out.emplace(String("error", r), Json(r, result.error().to_string(r)));
        }
        out.emplace(String("rows", r), Json(r, rows));
    }
    return Json(r, out);
}

int main() {
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        try {
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        } catch (const std::exception& error) {
            Json::Object out(r);
            out.emplace(String("panic", r), Json(r, error.what()));
            std::cout << Json(r, out).dump(r) << '\n';
        }
    }
}
