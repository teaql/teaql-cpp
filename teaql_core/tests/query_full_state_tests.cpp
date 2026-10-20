#include "../../tests/support/query_full_state_fixture.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void domain(const SelectQuery&, Memory);

static void expression_domain(const Expr& expression, Memory owner) {
    check(expression.memory_resource() == owner && expression.literal.memory_resource() == owner,
          "Expression values must belong to the destination");
    for (const auto& argument : expression.args) {
        expression_domain(argument, owner);
    }
    if (expression.subquery_data) {
        check(expression.subquery_data->entity.memory_resource() == owner,
              "Expression descriptor ownership must propagate");
        domain(expression.subquery_data->query, owner);
    }
}

static void domain(const SelectQuery& query, Memory owner) {
    check(query.memory_resource() == owner, "Query owner must propagate");
    for (const auto* text : {&query.search_text,
                             &query.partition_field,
                             &query.comment_text,
                             &query.purpose_text,
                             &query.raw_sql_text}) {
        if (*text) {
            check((*text)->get_allocator().resource() == owner,
                  "Optional text owner must propagate");
        }
    }
    for (const auto* fields :
         {&query.projection, &query.group_by_fields, &query.raw_sql_search_criteria}) {
        check(fields->get_allocator().resource() == owner, "String-container owner must propagate");
        for (const auto& field : *fields) {
            check(field.get_allocator().resource() == owner, "Nested string owner must propagate");
        }
    }
    for (const auto* expression : {&query.predicate, &query.having_predicate}) {
        if (*expression) {
            expression_domain(**expression, owner);
        }
    }
    for (const auto& named : query.expr_projection) {
        check(named.memory_resource() == owner, "Named-expression owner must propagate");
        expression_domain(named.expr, owner);
    }
    for (const auto& order : query.order_by) {
        check(order.memory_resource() == owner, "Order owner must propagate");
        if (order.expr) {
            expression_domain(*order.expr, owner);
        }
    }
    for (const auto& aggregate : query.aggregates) {
        check(aggregate.memory_resource() == owner, "Aggregate owner must propagate");
    }
    for (const auto& relation : query.relations) {
        check(relation.memory_resource() == owner, "Relation owner must propagate");
        if (relation.query) {
            domain(*relation.query, owner);
        }
    }
    for (const auto& facet : query.facets) {
        check(facet.memory_resource() == owner && facet.query.memory_resource() == owner &&
                  facet.query.query_options.memory_resource() == owner,
              "Facet selection and options must use the destination");
        domain(facet.query.query, owner);
    }
    for (const auto& aggregate : query.relation_aggregates) {
        check(aggregate.relation_name.get_allocator().resource() == owner &&
                  aggregate.alias.get_allocator().resource() == owner,
              "Related metric names must use the destination");
        domain(aggregate.query, owner);
    }
    for (const auto& trace : query.trace_chain) {
        check(trace.entity_type.get_allocator().resource() == owner &&
                  trace.comment.get_allocator().resource() == owner,
              "Trace ownership must propagate");
    }
    for (const auto* projections : {&query.dynamic_properties, &query.raw_projections}) {
        for (const auto& projection : *projections) {
            check(projection.memory_resource() == owner, "Raw-projection owner must propagate");
        }
    }
    if (query.dynamic_property_definitions) {
        check(query.dynamic_property_definitions->memory_resource() == owner,
              "Readonly schema must use the explicit destination");
        for (const auto& [name, type] : query.dynamic_property_definitions->types()) {
            (void)type;
            check(name.get_allocator().resource() == owner, "Readonly schema key ownership");
        }
    }
    if (query.dynamic_field_selection) {
        check(query.dynamic_field_selection->memory_resource() == owner,
              "Persistent selection must use the explicit destination");
        for (const auto& [name, type] : query.dynamic_field_selection->selected_fields()) {
            (void)type;
            check(name.get_allocator().resource() == owner, "Persistent selection key ownership");
        }
    }
    for (const auto& group : query.object_group_bys) {
        check(group.property_name.get_allocator().resource() == owner &&
                  group.storage_field.get_allocator().resource() == owner,
              "Object-group names must use the destination");
        domain(group.query, owner);
    }
    for (const auto& child : query.child_enhancements) {
        domain(child, owner);
    }
    if (query.continuous_page_fetch) {
        check(query.continuous_page_fetch->memory_resource() == owner, "Seek hint ownership");
    }
    if (query.id_set_pagination) {
        check(query.id_set_pagination->memory_resource() == owner, "ID-set hint ownership");
    }
}

static void retained_state() {
    GlobalContext global;
    for (unsigned profile = 0; profile != 4; ++profile) {
        for (unsigned field = 0; field != 31; ++field) {
            Ref<SelectQuery> retained;
            String expected(global.memory_resource());
            {
                auto user_context = global.make_user_context();
                auto query = query_full_state_fixture::make(*user_context, profile);
                query_full_state_fixture::mutate(*user_context, query, field);
                expected = debug_string(global.memory_resource(), query);
                retained = user_context->make_global<SelectQuery>(query);
                const auto& copy = user_context->get(retained);
                domain(copy, global.memory_resource());
                check(copy == query, "An explicit destination copy preserves every field");
                if (query.dynamic_property_definitions) {
                    check(copy.dynamic_property_definitions.get() !=
                              query.dynamic_property_definitions.get(),
                          "A Global copy must not retain User schema storage");
                }
                if (query.dynamic_field_selection) {
                    check(copy.dynamic_field_selection.get() != query.dynamic_field_selection.get(),
                          "A Global copy must not retain User selection storage");
                }
            }
            auto next_user_context = global.make_user_context();
            const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
            domain(copy, global.memory_resource());
            check(debug_string(global.memory_resource(), copy) == expected,
                  "All retained fields remain readable after actual source User deletion");
        }
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        retained_state();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
