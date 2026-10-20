#pragma once
#include <teaql/core/request.hpp>
#include <teaql/runtime/context.hpp>
#include <array>

namespace query_full_state_fixture {
using namespace teaql;

inline std::uint64_t number(unsigned profile) {
    return std::array<std::uint64_t, 4>{1, 7, 10000, UINT64_MAX}.at(profile);
}

inline String label(Memory owner, unsigned profile) {
    if (profile == 2) {
        return String("state\0雪", 9, owner);
    }
    if (profile == 3) {
        return String(128, 'x', owner);
    }
    return String("state", owner);
}

inline const DynamicPropertyDefinitions& schema(UserContext& user_context, bool changed) {
    const std::array<DynamicPropertyDefinitions::Field, 1> fields{
        {{changed ? "_changed" : "_rank", changed ? DataType::Text : DataType::U64}}};
    auto result = DynamicPropertyDefinitions::from_fields(user_context.memory_resource(), fields);
    if (!result) {
        throw std::runtime_error("Invalid full-state schema fixture");
    }
    return user_context.make<DynamicPropertyDefinitions>(std::move(result).value());
}

inline DynamicFieldSelection selection(Memory owner, bool changed) {
    const std::array<DynamicFieldSelection::Field, 1> fields{
        {{changed ? "changed" : "note", changed ? DataType::Text : DataType::I64}}};
    auto result = DynamicFieldSelection::fields(owner, fields);
    if (!result) {
        throw std::runtime_error("Invalid full-state selection fixture");
    }
    return std::move(result).value();
}

inline SelectQuery make(UserContext& user_context, unsigned profile) {
    auto owner = user_context.memory_resource();
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
    if (profile == 0) {
        return query;
    }
    auto text = label(owner, profile);
    auto n = number(profile);
    query.entity = text;
    SelectQuery ignored(std::allocator_arg, Allocator(owner), "Ignored");
    ignored.limit(20001).hard_limit(1);
    SelectQuery child(std::allocator_arg, Allocator(owner), "Child");
    child.limit(10000).hard_limit(1);
    SelectQuery enhancement(std::allocator_arg, Allocator(owner), "Enhancement");
    enhancement.hard_limit(1);
    EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "Ignored");
    auto expression = Expr::in_subquery(owner, "id", descriptor, ignored, "id");
    query.hard_limit(n).project("id").project_expr("computed",
                                                   Expr::value(owner, std::uint64_t{7}));
    query.search_with_text(text).filter(expression).having(Expr::is_not_null(owner, "_rank"));
    query.order_expr_desc(Expr::column(owner, "id"));
    query.page(n, profile == 2 ? 0 : profile == 3 ? UINT64_MAX : 3);
    query.partition_by(text).top_n_probe_parent_threshold(profile == 2 ? 0 : 13);
    query.sum("amount", "total").group_by("owner");
    query.relation("plain").relation_query("child", child);
    query.facets.emplace_back("facet", "children", ignored, true);
    query.relation_aggregates.emplace_back("children", "related", ignored, false);
    query.enable_aggregation_cache_for(n).propagate_aggregation_cache(2);
    query.comment(text).purpose(text);
    query.trace_chain.emplace_back(TraceKind::Operation, text, text, std::uint64_t{0});
    query.raw_sql(text).add_raw_sql_search_criteria(text);
    query.dynamic_property_raw("_rank", "1");
    query.with_dynamic_property_definitions(schema(user_context, false));
    query.select_dynamic_fields(selection(owner, false));
    query.project_raw("raw", "42").object_group_by("object", "owner", ignored);
    query.child_enhancement(enhancement).stream(profile == 2 ? 0 : 1000);
    query.optimize_for_continuous_page_fetch_with(text, n);
    query.optimize_pagination_with_id_set_config(text, n, n);
    return query;
}

inline void mutate(UserContext& user_context, SelectQuery& query, unsigned field) {
    auto owner = query.memory_resource();
    SelectQuery child(std::allocator_arg, Allocator(owner), "Changed");
    child.limit(30000);
    auto changed = Expr::eq(owner, "changed", std::uint64_t{42});
    switch (field) {
    case 0:
        query.hard_limit_value = 12345;
        break;
    case 1:
        query.entity = "Changed";
        break;
    case 2:
        query.projection.emplace_back("changed");
        break;
    case 3:
        query.expr_projection.emplace_back("changed", changed);
        break;
    case 4:
        query.search_text.emplace("changed", owner);
        break;
    case 5:
        query.predicate.emplace(changed.clone(owner));
        break;
    case 6:
        query.having_predicate.emplace(changed.clone(owner));
        break;
    case 7:
        query.order_by.emplace_back("changed", SortDirection::Asc);
        break;
    case 8:
        query.slice = Slice{0, UINT64_MAX};
        break;
    case 9:
        query.partition_field.emplace("changed", owner);
        break;
    case 10:
        query.top_n_probe_parent_threshold_value = 42;
        break;
    case 11:
        query.aggregates.emplace_back(AggregateFunction::Count, "*", "changed");
        break;
    case 12:
        query.group_by_fields.emplace_back("changed");
        break;
    case 13:
        query.relations.emplace_back("changed");
        break;
    case 14:
        query.facets.emplace_back("changed", "changed", child, false);
        break;
    case 15:
        query.relation_aggregates.emplace_back("changed", "changed", child, true);
        break;
    case 16:
        query.aggregation_cache = AggregationCacheOptions{false, 42, true, 43};
        break;
    case 17:
        query.comment_text.emplace("changed", owner);
        break;
    case 18:
        query.purpose_text.emplace("changed", owner);
        break;
    case 19:
        query.trace_chain.emplace_back(TraceKind::Purpose, "changed", "changed");
        break;
    case 20:
        query.raw_sql_text.emplace("changed", owner);
        break;
    case 21:
        query.raw_sql_search_criteria.emplace_back("changed");
        break;
    case 22:
        query.dynamic_properties.emplace_back("_changed", "43");
        break;
    case 23:
        query.with_dynamic_property_definitions(schema(user_context, true));
        break;
    case 24:
        query.select_dynamic_fields(selection(owner, true));
        break;
    case 25:
        query.raw_projections.emplace_back("changed", "43");
        break;
    case 26:
        query.object_group_bys.emplace_back("changed", "changed", child);
        break;
    case 27:
        query.child_enhancements.emplace_back(child);
        break;
    case 28:
        query.stream_config = StreamConfig{42};
        break;
    case 29:
        query.continuous_page_fetch.emplace(std::allocator_arg, Allocator(owner), "changed", 42);
        break;
    case 30:
        query.id_set_pagination.emplace(std::allocator_arg, Allocator(owner), "changed", 42, 43);
        break;
    default:
        throw std::runtime_error("Unknown query field");
    }
}
}
