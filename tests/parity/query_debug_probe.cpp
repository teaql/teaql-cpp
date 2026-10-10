#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json evaluate(Memory owner, const Json& input) {
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    auto n = number<std::uint64_t>(field(input, "number"));
    auto op = BinaryOp(std::get<std::uint64_t>(field(input, "op").data));
    auto function = ExprFunction(std::get<std::uint64_t>(field(input, "function").data));
    auto aggregate_function =
        AggregateFunction(std::get<std::uint64_t>(field(input, "aggregate").data));
    auto trace_kind = TraceKind(std::get<std::uint64_t>(field(input, "trace").data));
    auto direction = flags & 1 ? SortDirection::Desc : SortDirection::Asc;
    auto column = Expr::column(owner, "field\n雪");
    auto literal = Expr::value(owner, value(owner, field(input, "value")));
    auto binary = Expr::binary(column.clone(owner), op, literal.clone(owner));
    SelectQuery child(std::allocator_arg, Allocator(owner), "Child");
    child.projects({"id", "value"});
    child.filter(binary);
    child.slice = Slice{2, 1};
    EntityDescriptor entity(std::allocator_arg, Allocator(owner), "Child");
    entity.property("id", DataType::U64).id();
    entity.property("secret", DataType::Text).with_max_length(0);
    RelationDescriptor metadata(std::allocator_arg, Allocator(owner), "owner", "Owner");
    metadata.detached();
    entity.relation(metadata).mask_fields({"secret"}).audit_max_length(0);
    Vector<Expr> args(owner);
    args.emplace_back(column.clone(owner));
    args.emplace_back(literal.clone(owner));
    Vector<Expr> expressions(owner);
    expressions.emplace_back(column.clone(owner));
    expressions.emplace_back(literal.clone(owner));
    expressions.emplace_back(Expr::function(owner, function, args));
    expressions.emplace_back(binary.clone(owner));
    Expr subquery(owner, Expr::Kind::SubQuery);
    subquery.op = op;
    subquery.args.emplace_back(column.clone(owner));
    subquery.subquery_data = detail::make_box<SubQueryExpr>(owner, entity, child);
    expressions.emplace_back(std::move(subquery));
    Expr between(owner, Expr::Kind::Between);
    between.args.emplace_back(column.clone(owner));
    between.args.emplace_back(literal.clone(owner));
    between.args.emplace_back(Expr::value(owner, std::int64_t{9}));
    expressions.emplace_back(std::move(between));
    for (auto kind : {Expr::Kind::IsNull, Expr::Kind::IsNotNull}) {
        Expr unary(owner, kind);
        unary.args.emplace_back(column.clone(owner));
        expressions.emplace_back(std::move(unary));
    }
    Expr and_expr(owner, Expr::Kind::And);
    and_expr.args.emplace_back(binary.clone(owner));
    and_expr.args.emplace_back(literal.clone(owner));
    expressions.emplace_back(std::move(and_expr));
    expressions.emplace_back(Expr(owner, Expr::Kind::Or));
    Expr not_expr(owner, Expr::Kind::Not);
    not_expr.args.emplace_back(binary.clone(owner));
    expressions.emplace_back(std::move(not_expr));
    NamedExpr named(std::allocator_arg, Allocator(owner), std::string_view("alias\0", 6), binary);
    auto order = flags & 1 ? OrderBy(std::allocator_arg, Allocator(owner), binary, direction)
                           : OrderBy(std::allocator_arg, Allocator(owner), "field", direction);
    Aggregate aggregate(
        std::allocator_arg, Allocator(owner), aggregate_function, "amount", "total");
    Slice slice{flags & 2 ? std::optional(n) : std::nullopt, n};
    auto relation = flags & 2
                        ? RelationLoad(std::allocator_arg, Allocator(owner), "children", child)
                        : RelationLoad(std::allocator_arg, Allocator(owner), "children");
    RelationAggregate relation_aggregate(
        std::allocator_arg, Allocator(owner), "children", "count", child, bool(flags & 1));
    RawSqlProjection raw(std::allocator_arg, Allocator(owner), "derived", "sensitive raw SQL\n");
    ObjectGroupBy group(std::allocator_arg, Allocator(owner), "group", "group_id", child);
    AggregationCacheOptions cache{bool(flags & 1), n, bool(flags & 2), n};
    StreamConfig stream{flags & 1 ? 0u : 1000u};
    ContinuousPageFetchOptions continuous(
        std::allocator_arg, Allocator(owner), "namespace", std::max(n, std::uint64_t{1}));
    IdSetPaginationOptions id_set(std::allocator_arg,
                                  Allocator(owner),
                                  "namespace",
                                  std::max(n, std::uint64_t{1}),
                                  std::max(n, std::uint64_t{1}));
    TraceNode trace(std::allocator_arg,
                    Allocator(owner),
                    trace_kind,
                    "Order",
                    "private comment\n雪",
                    flags & 2 ? std::optional(n) : std::nullopt);
    SelectQuery query(std::allocator_arg, Allocator(owner), "Order\n雪");
    if (flags & 4) {
        query.hard_limit_value = n;
        query.projects({"id", "id", ""});
        query.expr_projection.emplace_back(named);
        query.search_text.emplace("", owner);
        query.filter(expressions[4]);
        query.having(expressions[5]);
        query.order_by.emplace_back(order);
        query.slice = slice;
        query.partition_field.emplace("partition", owner);
        query.top_n_probe_parent_threshold_value = 0;
        query.aggregates.emplace_back(aggregate);
        query.group_by_fields.emplace_back("status");
        query.relations.emplace_back(relation);
        query.aggregation_cache = cache;
        query.comment_text.emplace("", owner);
        query.trace_chain.emplace_back(trace);
        query.raw_sql_text.emplace("", owner);
        query.raw_sql_search_criteria.emplace_back("raw = 1");
        query.dynamic_properties.emplace_back(raw);
        query.raw_projections.emplace_back(raw);
        query.object_group_bys.emplace_back(group);
        query.child_enhancements.emplace_back(child);
        query.stream_config = stream;
        query.continuous_page_fetch.emplace(std::allocator_arg, Allocator(owner), continuous);
        query.id_set_pagination.emplace(std::allocator_arg, Allocator(owner), id_set);
    }
    Json::Object output(owner);
    Json::Array expr_outputs(owner);
    for (const auto& expr : expressions) {
        expr_outputs.emplace_back(formatted_debug(owner, expr));
    }
    output.emplace("expressions", Json(owner, std::move(expr_outputs)));
    auto observe = [&](std::string_view name, const auto& item) {
        output.emplace(name, formatted_debug(owner, item));
    };
    observe("query", query);
    observe("named", named);
    observe("order", order);
    observe("aggregate", aggregate);
    observe("slice", slice);
    observe("relation", relation);
    observe("relation_aggregate", relation_aggregate);
    observe("raw", raw);
    observe("group", group);
    observe("cache", cache);
    observe("stream", stream);
    observe("continuous", continuous);
    observe("id_set", id_set);
    observe("trace", trace);
    if (std::get<bool>(field(input, "enums").data)) {
        Json::Object enums(owner);
        auto enum_cases = [&]<class T>(std::string_view name, unsigned count) {
            Json::Array values(owner);
            for (unsigned index = 0; index < count; ++index) {
                values.emplace_back(formatted_debug(owner, T(index)));
            }
            enums.emplace(name, Json(owner, std::move(values)));
        };
        enum_cases.operator()<BinaryOp>("binary", 12);
        enum_cases.operator()<ExprFunction>("functions", 14);
        enum_cases.operator()<AggregateFunction>("aggregate", 12);
        enum_cases.operator()<SortDirection>("direction", 2);
        enum_cases.operator()<TraceKind>("trace", 9);
        output.emplace("enums", Json(owner, std::move(enums)));
    }
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid query Debug fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
