#include <teaql/core/detail/query_debug.hpp>
#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Json full_expression(Memory owner, const Json& input) {
    auto column = text(field(input, "column"));
    auto pattern = text(field(input, "pattern"));
    auto lower = value(owner, field(input, "lower"));
    auto upper = value(owner, field(input, "upper"));
    auto base = Expr::value(owner, lower);
    Json::Object out(owner);
    auto record = [&](std::string_view label, const Expr& expr) {
        Json::Object state(owner);
        state.emplace("debug", Json(owner, detail::debug_expr(owner, expr)));
        state.emplace("self", Json(owner, expr == expr));
        state.emplace("clone", Json(owner, expr == expr.clone(owner)));
        out.emplace(String(label, owner), Json(owner, std::move(state)));
    };
    record("column", Expr::column(owner, column));
    record("value", Expr::value(owner, lower));
    record("eq", Expr::eq(owner, column, lower));
    record("ne", Expr::ne(owner, column, lower));
    record("gt", Expr::gt(owner, column, lower));
    record("gte", Expr::gte(owner, column, lower));
    record("lt", Expr::lt(owner, column, lower));
    record("lte", Expr::lte(owner, column, lower));
    record("sound_like", Expr::sound_like(owner, column, lower));
    record("between", Expr::between(owner, column, lower, upper));
    record("between_scalar", Expr::between(owner, column, INT64_MIN, UINT64_MAX));
    record("between_text", Expr::between(owner, column, pattern, ""));
    record("like", Expr::like(owner, column, pattern));
    record("not_like", Expr::not_like(owner, column, pattern));
    record("contain", Expr::contain(owner, column, pattern));
    record("not_contain", Expr::not_contain(owner, column, pattern));
    record("begin_with", Expr::begin_with(owner, column, pattern));
    record("not_begin_with", Expr::not_begin_with(owner, column, pattern));
    record("end_with", Expr::end_with(owner, column, pattern));
    record("not_end_with", Expr::not_end_with(owner, column, pattern));
    record("is_null", Expr::is_null(owner, column));
    record("is_not_null", Expr::is_not_null(owner, column));
    record("count_all", Expr::count_all(owner));
    using Unary = Expr (*)(const Expr&);
    const std::pair<std::string_view, Unary> unary[]{{"soundex", Expr::soundex},
                                                     {"gbk", Expr::gbk},
                                                     {"count_expr", Expr::count_expr},
                                                     {"sum_expr", Expr::sum_expr},
                                                     {"avg_expr", Expr::avg_expr},
                                                     {"min_expr", Expr::min_expr},
                                                     {"max_expr", Expr::max_expr},
                                                     {"stddev_expr", Expr::stddev_expr},
                                                     {"stddev_pop_expr", Expr::stddev_pop_expr},
                                                     {"var_samp_expr", Expr::var_samp_expr},
                                                     {"var_pop_expr", Expr::var_pop_expr},
                                                     {"bit_and_expr", Expr::bit_and_expr},
                                                     {"bit_or_expr", Expr::bit_or_expr},
                                                     {"bit_xor_expr", Expr::bit_xor_expr}};
    for (const auto& [label, function] : unary) {
        record(label, function(base));
    }
    record("negate", Expr::negate(base));
    Vector<Expr> parts(owner);
    parts.emplace_back(base);
    parts.emplace_back(Expr::is_null(owner, column));
    parts.emplace_back(base);
    auto nested = Expr::all_of(owner, parts);
    record("and_expr_equal", base.and_expr(base));
    record("or_expr_equal", base.or_expr(base));
    record("and_expr_nested_right", base.and_expr(nested));
    record("and_expr_existing", nested.and_expr(base));
    record("or_expr_nested_right", base.or_expr(nested));
    Vector<Expr> or_parts(owner);
    or_parts.emplace_back(base);
    or_parts.emplace_back(Expr::is_null(owner, column));
    record("or_expr_existing", Expr::any_of(owner, or_parts).or_expr(base));
    EntityDescriptor entity(std::allocator_arg, Allocator(owner), "Child");
    SelectQuery query(std::allocator_arg, Allocator(owner), "Child");
    query.project("obsolete").comment(pattern);
    if (std::get<bool>(field(input, "filter").data)) {
        query.filter(Expr::eq(owner, "name", lower));
    }
    for (unsigned index = 0; index < 12; ++index) {
        auto op = BinaryOp(index);
        record("binary_" + std::to_string(index),
               Expr::binary(base.clone(owner), op, Expr::column(owner, column)));
        record("columns_" + std::to_string(index),
               Expr::compare_columns(owner, column, op, "other"));
        record("subquery_" + std::to_string(index),
               Expr::subquery(base.clone(owner), op, entity, query, column));
    }
    record("in_subquery", Expr::in_subquery(owner, "id", entity, query, column));
    record("not_in_subquery", Expr::not_in_subquery(owner, "id", entity, query, column));
    out.emplace("threshold", Json(owner, std::uint64_t(Expr::LARGE_IN_THRESHOLD)));
    return Json(owner, std::move(out));
}

static Json query_state(Memory owner, const SelectQuery& query) {
    Json::Object state(owner);
    state.emplace("debug", Json(owner, detail::debug_query(owner, query)));
    state.emplace("self", Json(owner, query == query));
    state.emplace("clone", Json(owner, query == query.clone(owner)));
    return Json(owner, std::move(state));
}

static Json query_builders(Memory owner, const Json& input) {
    auto field_name = text(field(input, "column"));
    auto pattern = text(field(input, "pattern"));
    auto base = Expr::eq(owner, field_name, value(owner, field(input, "value")));
    SelectQuery child(std::allocator_arg, Allocator(owner), "Child");
    child.filter(base).page(3, 5).hard_limit(2);
    SelectQuery ignored(std::allocator_arg, Allocator(owner), "Ignored");
    ignored.limit(20'000);
    SelectQuery query(std::allocator_arg,
                      Allocator(owner),
                      std::string_view("Order雪\0", sizeof("Order雪\0") - 1));
    Json::Array states(owner);
    states.emplace_back(query_state(owner, query));
    query.project(field_name);
    states.emplace_back(query_state(owner, query));
    query.project_expr("expression", base);
    states.emplace_back(query_state(owner, query));
    query.project_raw("raw", "coalesce(value, 0)");
    states.emplace_back(query_state(owner, query));
    query.dynamic_property_raw("dynamic", pattern);
    states.emplace_back(query_state(owner, query));
    query.search_with_text("");
    states.emplace_back(query_state(owner, query));
    query.search_with_text(pattern);
    states.emplace_back(query_state(owner, query));
    query.filter(base);
    states.emplace_back(query_state(owner, query));
    query.and_filter(base);
    states.emplace_back(query_state(owner, query));
    query.or_filter(Expr::is_null(owner, field_name));
    states.emplace_back(query_state(owner, query));
    query.having(base);
    states.emplace_back(query_state(owner, query));
    query.and_having(base);
    states.emplace_back(query_state(owner, query));
    query.or_having(Expr::is_null(owner, field_name));
    states.emplace_back(query_state(owner, query));
    query.add_order(OrderBy(std::allocator_arg, Allocator(owner), field_name, SortDirection::Desc));
    states.emplace_back(query_state(owner, query));
    query.order_asc(field_name);
    states.emplace_back(query_state(owner, query));
    query.order_desc(field_name);
    states.emplace_back(query_state(owner, query));
    query.order_expr_asc(base);
    states.emplace_back(query_state(owner, query));
    query.order_expr_desc(base);
    states.emplace_back(query_state(owner, query));
    query.order_gbk_asc(field_name);
    states.emplace_back(query_state(owner, query));
    query.order_gbk_desc(field_name);
    states.emplace_back(query_state(owner, query));
    query.group_by(field_name);
    states.emplace_back(query_state(owner, query));
    query.aggregate(
        Aggregate(std::allocator_arg, Allocator(owner), AggregateFunction::Sum, field_name, "sum"));
    states.emplace_back(query_state(owner, query));
    query.count("count");
    states.emplace_back(query_state(owner, query));
    query.count_field(field_name, "count_field");
    states.emplace_back(query_state(owner, query));
    query.sum(field_name, "sum");
    states.emplace_back(query_state(owner, query));
    query.avg(field_name, "avg");
    states.emplace_back(query_state(owner, query));
    query.min(field_name, "min");
    states.emplace_back(query_state(owner, query));
    query.max(field_name, "max");
    states.emplace_back(query_state(owner, query));
    query.stddev(field_name, "stddev");
    states.emplace_back(query_state(owner, query));
    query.stddev_pop(field_name, "stddev_pop");
    states.emplace_back(query_state(owner, query));
    query.var_samp(field_name, "var_samp");
    states.emplace_back(query_state(owner, query));
    query.var_pop(field_name, "var_pop");
    states.emplace_back(query_state(owner, query));
    query.bit_and(field_name, "bit_and");
    states.emplace_back(query_state(owner, query));
    query.bit_or(field_name, "bit_or");
    states.emplace_back(query_state(owner, query));
    query.bit_xor(field_name, "bit_xor");
    states.emplace_back(query_state(owner, query));
    query.propagate_aggregation_cache(UINT64_MAX);
    states.emplace_back(query_state(owner, query));
    query.enable_aggregation_cache();
    states.emplace_back(query_state(owner, query));
    query.enable_aggregation_cache_for(7);
    states.emplace_back(query_state(owner, query));
    query.comment("");
    states.emplace_back(query_state(owner, query));
    query.comment(pattern);
    states.emplace_back(query_state(owner, query));
    query.raw_sql(pattern);
    states.emplace_back(query_state(owner, query));
    query.add_raw_sql_search_criteria(pattern);
    states.emplace_back(query_state(owner, query));
    query.object_group_by("object", field_name, ignored);
    states.emplace_back(query_state(owner, query));
    query.child_enhancement(child);
    states.emplace_back(query_state(owner, query));
    query.relation("absent");
    states.emplace_back(query_state(owner, query));
    query.relation_query("configured", child);
    states.emplace_back(query_state(owner, query));
    query.offset(UINT64_MAX);
    states.emplace_back(query_state(owner, query));
    query.limit(0);
    states.emplace_back(query_state(owner, query));
    query.page(9, 7);
    states.emplace_back(query_state(owner, query));
    query.hard_limit(20);
    states.emplace_back(query_state(owner, query));
    query.optimize_for_continuous_page_fetch();
    states.emplace_back(query_state(owner, query));
    query.optimize_for_continuous_page_fetch_with(" ns雪 ", UINT64_MAX);
    states.emplace_back(query_state(owner, query));
    query.optimize_pagination_with_id_set();
    states.emplace_back(query_state(owner, query));
    query.optimize_pagination_with_id_set_config(" ns雪 ", UINT64_MAX, UINT64_MAX);
    states.emplace_back(query_state(owner, query));
    query.partition_by(field_name);
    states.emplace_back(query_state(owner, query));
    query.top_n_probe_parent_threshold(0);
    states.emplace_back(query_state(owner, query));
    query.stream(0);
    states.emplace_back(query_state(owner, query));
    query.stream_default();
    states.emplace_back(query_state(owner, query));
    auto prepared = query.prepare_for_list();
    auto oversized = query.clone(owner);
    oversized.limit(21);
    auto nested_error = query.clone(owner);
    nested_error.relations[1].query->limit(10'001);
    auto error_text = [&](const SelectQuery& invalid) {
        try {
            (void)invalid.prepare_for_list();
        } catch (const std::invalid_argument& error) {
            return String(error.what(), owner);
        }
        throw std::runtime_error("Source-rejected list query was accepted");
    };
    Json::Object result(owner);
    result.emplace("states", Json(owner, std::move(states)));
    result.emplace("prepared", query_state(owner, prepared));
    result.emplace("outer_error", Json(owner, error_text(oversized)));
    result.emplace("nested_error", Json(owner, error_text(nested_error)));
    return Json(owner, std::move(result));
}

static Json query_helpers(Memory owner, const Json& input) {
    auto field_name = text(field(input, "column"));
    auto pattern = text(field(input, "pattern"));
    auto base = Expr::eq(owner, field_name, value(owner, field(input, "value")));
    SelectQuery child(std::allocator_arg, Allocator(owner), "Child");
    child.filter(base);
    Json::Object out(owner);
    auto record = [&](std::string_view label, const auto& item) {
        Json::Object state(owner);
        state.emplace("debug", Json(owner, debug_string(owner, item)));
        state.emplace("self", Json(owner, item == item));
        state.emplace("clone", Json(owner, item == detail::clone_in(owner, item)));
        out.emplace(String(label, owner), Json(owner, std::move(state)));
    };
    record("named", NamedExpr(std::allocator_arg, Allocator(owner), pattern, base));
    record("order_new",
           OrderBy(std::allocator_arg, Allocator(owner), field_name, SortDirection::Desc));
    record("order_expr", OrderBy(std::allocator_arg, Allocator(owner), base, SortDirection::Asc));
    record("order_asc", OrderBy::asc(owner, field_name));
    record("order_desc", OrderBy::desc(owner, field_name));
    record("order_asc_expr", OrderBy::asc_expr(base));
    record("order_desc_expr", OrderBy::desc_expr(base));
    record("order_asc_gbk", OrderBy::asc_gbk(owner, field_name));
    record("order_desc_gbk", OrderBy::desc_gbk(owner, field_name));
    record(
        "aggregate_new",
        Aggregate(
            std::allocator_arg, Allocator(owner), AggregateFunction::BitXor, field_name, pattern));
    record("aggregate_count", Aggregate::count(owner, pattern));
    record("relation_new", RelationLoad(std::allocator_arg, Allocator(owner), field_name));
    record("relation_query", RelationLoad(std::allocator_arg, Allocator(owner), field_name, child));
    record(
        "relation_aggregate",
        RelationAggregate(std::allocator_arg, Allocator(owner), field_name, pattern, child, true));
    record("raw", RawSqlProjection(std::allocator_arg, Allocator(owner), field_name, pattern));
    record("object_group",
           ObjectGroupBy(std::allocator_arg, Allocator(owner), field_name, pattern, child));
    record("cache_enabled", AggregationCacheOptions::for_duration(UINT64_MAX));
    record("cache_propagated",
           AggregationCacheOptions::for_duration(7).with_propagation(UINT64_MAX));
    record("stream_default", StreamConfig{});
    record("stream_zero", StreamConfig{0});
    record("continuous",
           ContinuousPageFetchOptions(std::allocator_arg, Allocator(owner), " ns雪 ", UINT64_MAX));
    record("id_set",
           IdSetPaginationOptions(
               std::allocator_arg, Allocator(owner), " ns雪 ", UINT64_MAX, UINT64_MAX));
    record("slice_empty", Slice{std::nullopt, UINT64_MAX});
    record("slice_zero", Slice{0, 0});
    record("aggregate_count_field", Aggregate::count_field(owner, field_name, pattern));
    record("aggregate_sum", Aggregate::sum(owner, field_name, pattern));
    record("aggregate_avg", Aggregate::avg(owner, field_name, pattern));
    record("aggregate_min", Aggregate::min(owner, field_name, pattern));
    record("aggregate_max", Aggregate::max(owner, field_name, pattern));
    record("aggregate_stddev", Aggregate::stddev(owner, field_name, pattern));
    record("aggregate_stddev_pop", Aggregate::stddev_pop(owner, field_name, pattern));
    record("aggregate_var_samp", Aggregate::var_samp(owner, field_name, pattern));
    record("aggregate_var_pop", Aggregate::var_pop(owner, field_name, pattern));
    record("aggregate_bit_and", Aggregate::bit_and(owner, field_name, pattern));
    record("aggregate_bit_or", Aggregate::bit_or(owner, field_name, pattern));
    record("aggregate_bit_xor", Aggregate::bit_xor(owner, field_name, pattern));
    Json::Array constants(owner);
    constants.emplace_back(ContinuousPageFetchOptions::DEFAULT_TTL_SECONDS);
    constants.emplace_back(IdSetPaginationOptions::DEFAULT_TTL_SECONDS);
    constants.emplace_back(IdSetPaginationOptions::DEFAULT_MAX_IDS);
    out.emplace("constants", Json(owner, std::move(constants)));
    return Json(owner, std::move(out));
}

static Json validate_query(Memory owner, const Json& input) {
    Json::Object out(owner);
    try {
        auto kind = text(field(input, "type"));
        auto name = text(field(input, "namespace"));
        auto ttl = number<std::uint64_t>(field(input, "ttl"));
        auto maximum = number<std::uint64_t>(field(input, "maximum"));
        if (kind == "continuous") {
            (void)ContinuousPageFetchOptions(std::allocator_arg, Allocator(owner), name, ttl);
        } else if (kind == "id_set") {
            (void)IdSetPaginationOptions(std::allocator_arg, Allocator(owner), name, ttl, maximum);
        } else {
            SelectQuery query(std::allocator_arg, Allocator(owner), "Order");
            query.hard_limit(maximum);
        }
        out.emplace("ok", Json(owner, true));
    } catch (const std::invalid_argument& error) {
        out.emplace("error", Json(owner, error.what()));
    }
    return Json(owner, std::move(out));
}

static Json observe(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    if (auto* kind = optional(input, "kind"); kind && text(*kind) == "validate_query") {
        return validate_query(owner, input);
    }
    if (auto* kind = optional(input, "kind"); kind && text(*kind) == "query_helpers") {
        return query_helpers(owner, input);
    }
    if (auto* kind = optional(input, "kind"); kind && text(*kind) == "query_builders") {
        return query_builders(owner, input);
    }
    if (auto* kind = optional(input, "kind"); kind && text(*kind) == "full_expression") {
        return full_expression(owner, input);
    }
    SelectQuery query(std::allocator_arg, Allocator(owner), text(field(input, "entity")));
    Json::Array states(owner);
    states.emplace_back(detail::debug_query(owner, query));
    for (const auto& names : std::get<Json::Array>(field(input, "projections").data)) {
        auto range =
            std::get<Json::Array>(names.data) | std::views::transform([](const Json& name) {
                return text(name);
            });
        query.projects(range);
        states.emplace_back(detail::debug_query(owner, query));
    }
    auto cache =
        AggregationCacheOptions::for_duration(number<std::uint64_t>(field(input, "expires")));
    cache.with_propagation(number<std::uint64_t>(field(input, "propagate")));
    query.aggregation_cache = cache;
    states.emplace_back(detail::debug_query(owner, query));
    Vector<Value> values(owner);
    Vector<Expr> parts(owner);
    for (const auto& value_input : std::get<Json::Array>(field(input, "values").data)) {
        values.emplace_back(value(owner, value_input));
        parts.emplace_back(Expr::eq(owner, "key", values.back()));
    }
    Json::Object out(owner);
    out.emplace("queries", Json(owner, std::move(states)));
    String cache_text("AggregationCacheOptions { enabled: true, cache_expired_millis: ", owner);
    cache_text += std::to_string(cache.cache_expired_millis);
    cache_text += ", propagate: true, propagate_cache_expired_millis: ";
    cache_text += std::to_string(cache.propagate_cache_expired_millis);
    cache_text += " }";
    out.emplace("cache", Json(owner, cache_text));
    auto expressions = parts | std::views::transform([](const Expr& item) -> const Expr& {
                           return item;
                       });
    auto literals = values | std::views::transform([](const Value& item) -> const Value& {
                        return item;
                    });
    out.emplace("function",
                Json(owner,
                     detail::debug_expr(owner,
                                        Expr::function(owner,
                                                       ExprFunction(std::get<std::uint64_t>(
                                                           field(input, "function").data)),
                                                       expressions))));
    out.emplace("and", Json(owner, detail::debug_expr(owner, Expr::all_of(owner, expressions))));
    out.emplace("or", Json(owner, detail::debug_expr(owner, Expr::any_of(owner, expressions))));
    out.emplace("in",
                Json(owner, detail::debug_expr(owner, Expr::in_list(owner, "key", literals))));
    out.emplace("not_in",
                Json(owner, detail::debug_expr(owner, Expr::not_in_list(owner, "key", literals))));
    out.emplace("large",
                Json(owner, detail::debug_expr(owner, Expr::in_large(owner, "key", literals))));
    out.emplace("not_large",
                Json(owner, detail::debug_expr(owner, Expr::not_in_large(owner, "key", literals))));
    return Json(owner, std::move(out));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid query API fixture");
            }
            std::cout << observe(*user, *input).dump(user->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
