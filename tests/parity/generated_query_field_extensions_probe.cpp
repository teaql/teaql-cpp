#include "../generated_query_field_branches/include/cpp_query_branches.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace cpp_query_branches;

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
