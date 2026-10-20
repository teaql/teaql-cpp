#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include <teaql/core/request_debug.hpp>
#include <iostream>

using namespace cpp_dynamic_json;

template <class Range>
static String sequence_debug(Memory resource, const Range& values) {
    String output(resource);
    DebugWriter writer(output, {});
    writer.sequence(values, DefaultDebugElement{});
    return output;
}

template <class Request, class Child>
static Json
observe_request(UserContext& user_context, Request query, Child child, const Json& input) {
    auto r = user_context.memory_resource();
    auto text_value = text(field(input, "text"));
    for (const auto& item : std::get<Json::Array>(field(input, "operations").data)) {
        auto operation = text(item);
        if (operation == "options") {
            query.comment(text_value)
                .rawSql(text_value)
                .rawSqlFilter(text_value)
                .createPropertyAs("_computed", text_value);
            if constexpr (requires {
                              query.selectNameUnsafeRaw(
                                  request::UnsafeRawSqlSegment::trusted(r, text_value));
                          }) {
                query.selectNameUnsafeRaw(request::UnsafeRawSqlSegment::trusted(r, text_value));
            } else {
                query.selectCodeUnsafeRaw(request::UnsafeRawSqlSegment::trusted(r, text_value));
            }
        } else if (operation == "child") {
            query.enhanceChild(child);
        } else if (operation == "relation") {
            if constexpr (requires { query.selectOrdersWith(child); }) {
                query.selectOrdersWith(child);
            } else {
                query.selectCustomerWith(child);
            }
        } else if (operation == "filter") {
            if constexpr (requires { query.withOrdersMatching(child); }) {
                query.withOrdersMatching(child);
            } else {
                query.withCustomerMatching(child);
            }
        } else if (operation == "analysis") {
            auto Q = user_context.queries<query_factory>();
            if constexpr (requires {
                              query.groupByPlatformWith(Q.platforms());
                              query.statsFromOrdersAs("_stats", Q.orders());
                          }) {
                auto group = Q.platforms().comment(text_value);
                query.groupByPlatformWith(group)
                    .facetByPlatformAsWithOptions("_facet", group, false)
                    .statsFromOrdersAs("_stats", Q.orders().comment(text_value));
            } else {
                auto group = Q.people().comment(text_value);
                query.groupByCustomerWith(group)
                    .facetByCustomerAsWithOptions("_facet", group, false)
                    .statsFromLinesAs("_stats", Q.lines().comment(text_value));
            }
        } else if (operation == "shape") {
            query.groupBy("id")
                .aggregateWithFunction("id", "_count", AggregateFunction::Count)
                .pageOffset(3, 7);
        }
    }
    Json::Object output(r);
    auto core = query.coreQuery();
    output.emplace(String("core", r), Json(r, debug_string(r, core)));
    auto options = query.queryOptions();
    output.emplace(String("options", r), Json(r, debug_string(r, options)));
    output.emplace(String("relations", r), Json(r, sequence_debug(r, query.relationSelections())));
    output.emplace(String("filters", r), Json(r, sequence_debug(r, query.relationFilters())));
    output.emplace(String("children", r), Json(r, sequence_debug(r, query.childEnhancements())));
    auto raw_source = query;
    auto raw = std::move(raw_source).intoQuery();
    auto selected = query.selection();
    auto complete = std::move(selected).into_query();
    auto selection = query.selection();
    output.emplace(String("raw", r), Json(r, debug_string(r, raw)));
    output.emplace(String("complete", r), Json(r, debug_string(r, complete)));
    output.emplace(String("selection", r), Json(r, debug_string(r, selection)));
    return Json(r, output);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto minimal = std::get<bool>(field(input, "minimal").data);
    auto operand = text(field(input, "text"));
    if (text(field(input, "entity")) == "person") {
        return observe_request(user_context,
                               minimal ? Q.peopleMinimal() : Q.people(),
                               Q.orders().comment(operand).selectCode().withCodeIs(operand),
                               input);
    }
    return observe_request(user_context,
                           minimal ? Q.ordersMinimal() : Q.orders(),
                           Q.people().comment(operand).selectName().whoseNameIs(operand),
                           input);
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
            if (global.live_user_bytes() != 0) {
                throw std::logic_error("Request state observer retained User data");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
