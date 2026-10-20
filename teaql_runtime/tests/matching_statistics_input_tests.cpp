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
concept MatchingStatisticsInput =
    requires(Parent& parent, const Child& child, const EntityDescriptor& expected) {
        parent.filterRelation("customer_id", "id", expected, child, false, "customer");
        std::move(parent).filterRelation("customer_id", "id", expected, child, true, "customer");
        parent.aggregateRelationField(
            "orders", "_metric", child, "amount", "sum_amount", AggregateFunction::Sum);
        std::move(parent).aggregateRelationField(
            "orders", "_metric", child, "amount", "sum_amount", AggregateFunction::Sum);
    };
using ParentQuery = cpp_dynamic_json::OrderRequest<false>::Query;
static_assert(MatchingStatisticsInput<ParentQuery, cpp_relations::SchoolRequest<false>::Query>);
static_assert(MatchingStatisticsInput<ParentQuery, request::QuerySelection>);
static_assert(MatchingStatisticsInput<ParentQuery, SelectQuery>);
static_assert(!MatchingStatisticsInput<ParentQuery, cpp_relations::SchoolRequest<true>::Query>);

static void lifetime() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    auto other_Q = user_context->queries<cpp_relations::query_factory>();
    auto parent = Q.ordersMinimal().runtimeQuery();
    auto child = other_Q.schoolsMinimal()
                     .comment("long matching child intent retained explicitly")
                     .rawSqlFilter("1=1");
    auto capture = [](auto& query, const auto& value) {
        auto expected = cpp_dynamic_json::Person::entity_descriptor(query.memory_resource());
        query.filterRelation("customer_id", "id", expected, value, false, "customer");
        query.groupByObject("customer", "customer_id", value);
        std::move(query).aggregateRelationField(
            "orders", "_metric", value, "amount", "sum_amount", AggregateFunction::Sum);
    };
    capture(parent, child.runtimeQuery());
    {
        auto before = debug_string(user_context->memory_resource(), parent.query());
        child.rawSqlFilter("1=0");
        check(before == debug_string(user_context->memory_resource(), parent.query()),
              "Matching/statistics snapshot is independent");
        check(parent.relationFilters().front().query.entity == "School",
              "Incoming query identity is retained separately from expected target");
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
            auto expected = cpp_dynamic_json::Person::entity_descriptor(parent.memory_resource());
            unsigned rejected = 0;
            try {
                parent.filterRelation("customer_id", "id", expected, value, false, "customer");
            } catch (const Error&) {
                ++rejected;
            }
            try {
                parent.aggregateRelationField(
                    "orders", "_metric", value, "amount", "sum_amount", AggregateFunction::Sum);
            } catch (const Error&) {
                ++rejected;
            }
            check(rejected == 2, "Both routes reject foreign wrappers and values");
        };
        rejects(foreign_child.runtimeQuery());
        rejects(selection);
        rejects(core);
        auto& copied = user_context->make<request::QuerySelection>(selection);
        capture(parent, copied);
        auto& copied_core = user_context->make<SelectQuery>(core);
        capture(parent, copied_core);
    }
    auto retained = parent.copyToGlobal(*user_context);
    {
        bool rejected = false;
        try {
            capture(retained, child.runtimeQuery());
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Global parent requires an explicit destination copy");
        auto global_child = child.copyToGlobal(*user_context);
        capture(retained, global_child.runtimeQuery());
    }
    user_context.reset();
    check(global.live_user_bytes() == 0, "Real deletion returns all User blocks");
    bool expired = false;
    try {
        (void)parent.query();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "Ordinary query access expires");
    auto next_user_context = global.make_user_context();
    auto rebound = retained.rebindGlobal(*next_user_context);
    auto output = debug_string(next_user_context->memory_resource(), rebound.query());
    check(output.find("long matching child intent") != String::npos,
          "Explicit Global retention survives deletion");
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
