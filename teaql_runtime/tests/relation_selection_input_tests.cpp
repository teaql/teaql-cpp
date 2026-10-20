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
concept SelectRelationInput = requires(Parent& parent, const Child& child) {
    parent.selectRelation("customer", child);
    std::move(parent).selectRelation("customer", child);
};

using ParentQuery = cpp_dynamic_json::OrderRequest<false>::Query;
using ChildQuery = cpp_relations::SchoolRequest<false>::Query;
static_assert(SelectRelationInput<ParentQuery, ChildQuery>);
static_assert(SelectRelationInput<ParentQuery, request::QuerySelection>);
static_assert(SelectRelationInput<ParentQuery, SelectQuery>);
static_assert(!SelectRelationInput<ParentQuery, cpp_relations::SchoolRequest<true>::Query>);

static void lifetime() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    auto other_Q = user_context->queries<cpp_relations::query_factory>();
    auto parent = Q.ordersMinimal();
    auto child =
        other_Q.schoolsMinimal()
            .comment("long relation child intent retained through explicit destination copy")
            .rawSqlFilter("1=1");
    parent.selectCustomerWith(child);
    {
        auto captured = debug_string(user_context->memory_resource(), parent.query());
        child.rawSqlFilter("1=0");
        check(debug_string(user_context->memory_resource(), parent.query()) == captured,
              "Relation selection captures an independent snapshot");
    }
    {
        auto foreign = global.make_user_context();
        auto foreign_Q = foreign->queries<cpp_relations::query_factory>();
        auto foreign_child = foreign_Q.schoolsMinimal();
        bool rejected = false;
        try {
            parent.selectCustomerWith(foreign_child);
        } catch (const ReferenceError&) {
            rejected = true;
        }
        check(rejected, "Foreign Context request rejects");
        auto selection = foreign_child.selection();
        rejected = false;
        try {
            parent.selectCustomerWith(selection);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Foreign Context selection rejects");
        auto core = std::move(foreign_child.selection()).into_query();
        rejected = false;
        try {
            parent.selectCustomerWith(core);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Foreign Context core query rejects");
        auto& copied = user_context->make<request::QuerySelection>(selection);
        std::move(parent).selectCustomerWith(copied);
        auto& copied_core = user_context->make<SelectQuery>(core);
        std::move(parent).selectCustomerWith(copied_core);
    }
    auto retained = parent.copyToGlobal(*user_context);
    {
        auto local_child = Q.peopleMinimal();
        bool rejected = false;
        try {
            retained.selectCustomerWith(local_child);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "Target-specific overload also requires an explicit Global copy");
        auto global_child = local_child.copyToGlobal(*user_context);
        retained.selectCustomerWith(global_child);
    }
    user_context.reset();
    check(global.live_user_bytes() == 0, "Actual deletion returns all User blocks");
    bool expired = false;
    try {
        (void)parent.query();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "Ordinary query access expires after deletion");
    auto next_user_context = global.make_user_context();
    auto rebound = retained.rebindGlobal(*next_user_context);
    auto output = debug_string(next_user_context->memory_resource(), rebound.query());
    check(output.find("long relation child intent") != String::npos,
          "Explicit Global copy retains nested relation state");
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
