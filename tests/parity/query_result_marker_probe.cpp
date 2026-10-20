#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include <teaql/core/request_debug.hpp>
#include <iostream>

using namespace cpp_dynamic_json;

struct NotAnEntity {};

template <class Range>
static String sequence_debug(Memory r, const Range& values) {
    String output(r);
    DebugWriter writer(output, {});
    writer.sequence(values, DefaultDebugElement{});
    return output;
}

template <class Marker, class Request, class Child>
static Json inspect(Request source, Child source_child, const Json& input) {
    auto query = source.template returnType<Marker>();
    auto child = source_child.template returnType<NotAnEntity>();
    auto r = query.memory_resource();
    auto operand = text(field(input, "text"));
    auto shape = text(field(input, "shape"));
    if (shape == "options" || shape == "combined") {
        query.comment(operand).rawSql(operand).rawSqlFilter(operand).createPropertyAs("_computed",
                                                                                      operand);
    }
    if (shape == "child" || shape == "combined") {
        child.comment(operand);
        query.enhanceChild(child).matchingAnyOf(child);
        if constexpr (requires { query.selectOrdersWith(child); }) {
            query.selectOrdersWith(child);
        } else {
            query.selectCustomerWith(child);
        }
    }
    Json::Object output(r);
    auto bare = query.coreQuery();
    auto options = query.queryOptions();
    output.emplace("bare", Json(r, debug_string(r, bare)));
    output.emplace("options", Json(r, debug_string(r, options)));
    output.emplace("relations", Json(r, sequence_debug(r, query.relationSelections())));
    output.emplace("filters", Json(r, sequence_debug(r, query.relationFilters())));
    output.emplace("children", Json(r, sequence_debug(r, query.childEnhancements())));
    auto copy = query;
    auto raw = std::move(copy).intoQuery();
    auto selection = query.selection();
    auto complete = std::move(selection).into_query();
    output.emplace("raw", Json(r, debug_string(r, raw)));
    output.emplace("complete", Json(r, debug_string(r, complete)));
    output.emplace("selection", Json(r, debug_string(r, query.selection())));
    return Json(r, output);
}

template <class Marker>
static Json observe(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    bool minimal = std::get<bool>(field(input, "minimal").data);
    if (text(field(input, "entity")) == "person") {
        return inspect<Marker>(minimal ? Q.peopleMinimal() : Q.people(), Q.orders(), input);
    }
    return inspect<Marker>(minimal ? Q.ordersMinimal() : Q.orders(), Q.people(), input);
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                auto marker = text(field(*input, "marker"));
                auto output = marker == "integer" ? observe<std::uint64_t>(*user_context, *input)
                              : marker == "tuple"
                                  ? observe<std::pair<std::int64_t, String>>(*user_context, *input)
                                  : observe<NotAnEntity>(*user_context, *input);
                std::cout << output.dump(user_context->memory_resource()) << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::logic_error("Marker request retained User data");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
