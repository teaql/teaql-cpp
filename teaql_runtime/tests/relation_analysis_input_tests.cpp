#include "../../tests/generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "../../tests/generated_relations/include/cpp_relations.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

template <class Parent, class Child>
concept AnalysisInput = requires(Parent& parent, const Child& child) {
    parent.groupByObject("customer", "customer_id", child);
    std::move(parent).groupByObject("customer", "customer_id", child);
    parent.aggregateRelation("orders", "_metric", child, false);
    std::move(parent).aggregateRelation("orders", "_metric", child, false);
    parent.facetBy("customer", "_customers", child, false);
    std::move(parent).facetBy("customer", "_customers", child, false);
};
using ParentQuery = cpp_dynamic_json::OrderRequest<false>::Query;
using ChildQuery = cpp_relations::SchoolRequest<false>::Query;
static_assert(AnalysisInput<ParentQuery, ChildQuery>);
static_assert(AnalysisInput<ParentQuery, request::QuerySelection>);
static_assert(AnalysisInput<ParentQuery, SelectQuery>);
static_assert(!AnalysisInput<ParentQuery, cpp_relations::SchoolRequest<true>::Query>);

static void lifetime() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    auto other_Q = user_context->queries<cpp_relations::query_factory>();
    auto parent = Q.ordersMinimal().runtimeQuery();
    auto child = other_Q.schoolsMinimal()
                     .comment("long relation analysis intent for explicit retention")
                     .rawSqlFilter("1=1");
    auto capture = [](auto& query, const auto& selected) {
        query.groupByObject("customer", "customer_id", selected);
        query.aggregateRelation("orders", "_metric", selected, false);
        std::move(query).facetBy("customer", "_customers", selected, false);
    };
    capture(parent, child.runtimeQuery());
    {
        auto before = debug_string(user_context->memory_resource(), parent.query());
        child.rawSqlFilter("1=0");
        check(before == debug_string(user_context->memory_resource(), parent.query()),
              "Analysis snapshot is independent");
    }
    {
        auto foreign = global.make_user_context();
        auto foreign_Q = foreign->queries<cpp_relations::query_factory>();
        auto foreign_child = foreign_Q.schoolsMinimal();
        auto selection = foreign_child.selection();
        auto core = std::move(foreign_child.selection()).into_query();
        auto rejects = [&](const auto& value) {
            using Error = std::conditional_t<requires {
                value.selection();
            }, ReferenceError, std::invalid_argument>;
            unsigned failures = 0;
            try {
                parent.groupByObject("customer", "customer_id", value);
            } catch (const Error&) {
                ++failures;
            }
            try {
                parent.aggregateRelation("orders", "_metric", value, false);
            } catch (const Error&) {
                ++failures;
            }
            try {
                parent.facetBy("customer", "_customers", value);
            } catch (const Error&) {
                ++failures;
            }
            check(failures == 3, "Every analysis route rejects a foreign domain");
        };
        rejects(foreign_child.runtimeQuery());
        rejects(selection);
        rejects(core);
        auto& copy = user_context->make<request::QuerySelection>(selection);
        capture(parent, copy);
        auto& core_copy = user_context->make<SelectQuery>(core);
        capture(parent, core_copy);
    }
    auto retained = parent.copyToGlobal(*user_context);
    user_context.reset();
    check(global.live_user_bytes() == 0, "All User storage is reclaimed");
    bool expired = false;
    try {
        (void)parent.query();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "Analysis wrapper expires after deletion");
    auto next_user_context = global.make_user_context();
    auto rebound = retained.rebindGlobal(*next_user_context);
    auto output = debug_string(next_user_context->memory_resource(), rebound.query());
    check(output.find("long relation analysis intent") != String::npos,
          "Global copy retains nested intent and options");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        lifetime();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
