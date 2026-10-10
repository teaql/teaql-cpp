#include <teaql/core/query.hpp>
#include <teaql/core/detail/text.hpp>
#include <charconv>

namespace teaql {
namespace {
template <class T>
void copy_items(Vector<T>& target, const Vector<T>& source) {
    target.reserve(source.size());
    for (const auto& item : source) {
        target.emplace_back(item);
    }
}

void set_text(std::optional<String>& target, Memory r, std::string_view text) {
    String copy(text, r);
    target.emplace(std::move(copy));
}

void clone_optional(std::optional<String>& target, Memory r, const std::optional<String>& source) {
    if (source) {
        target.emplace(*source, r);
    }
}

template <class T>
void clone_optional(std::optional<T>& target, Memory r, const std::optional<T>& source) {
    if (source) {
        target.emplace(std::allocator_arg, Allocator(r), *source);
    }
}

bool blank_namespace(std::string_view value) {
    detail::trim_start(value);
    return value.empty();
}

}

NamedExpr::NamedExpr(std::allocator_arg_t, Allocator a, std::string_view name, const Expr& value)
    : alias(name, a.resource()), expr(std::allocator_arg, a, value) {}

NamedExpr::NamedExpr(std::allocator_arg_t, Allocator a, const NamedExpr& other)
    : NamedExpr(std::allocator_arg, a, other.alias, other.expr) {}

OrderBy::OrderBy(std::allocator_arg_t, Allocator a, std::string_view name, SortDirection dir)
    : field(name, a.resource()), direction(dir) {}

OrderBy::OrderBy(std::allocator_arg_t, Allocator a, const Expr& value, SortDirection dir)
    : field(a.resource()), expr(std::in_place, std::allocator_arg, a, value), direction(dir) {}

OrderBy::OrderBy(std::allocator_arg_t, Allocator a, const OrderBy& other)
    : field(other.field, a.resource()), direction(other.direction) {
    clone_optional(expr, a.resource(), other.expr);
}

OrderBy OrderBy::asc(Memory r, std::string_view field) {
    return OrderBy(std::allocator_arg, Allocator(r), field, SortDirection::Asc);
}

OrderBy OrderBy::desc(Memory r, std::string_view field) {
    return OrderBy(std::allocator_arg, Allocator(r), field, SortDirection::Desc);
}

OrderBy OrderBy::asc_expr(const Expr& expr) {
    return OrderBy(std::allocator_arg, Allocator(expr.memory_resource()), expr, SortDirection::Asc);
}

OrderBy OrderBy::desc_expr(const Expr& expr) {
    return OrderBy(
        std::allocator_arg, Allocator(expr.memory_resource()), expr, SortDirection::Desc);
}

OrderBy OrderBy::asc_gbk(Memory r, std::string_view field) {
    return asc_expr(Expr::gbk(Expr::column(r, field)));
}

OrderBy OrderBy::desc_gbk(Memory r, std::string_view field) {
    return desc_expr(Expr::gbk(Expr::column(r, field)));
}

Aggregate::Aggregate(std::allocator_arg_t,
                     Allocator a,
                     AggregateFunction fn,
                     std::string_view name,
                     std::string_view output)
    : function(fn), field(name, a.resource()), alias(output, a.resource()) {}

Aggregate::Aggregate(std::allocator_arg_t, Allocator a, const Aggregate& other)
    : Aggregate(std::allocator_arg, a, other.function, other.field, other.alias) {}

Aggregate Aggregate::count(Memory r, std::string_view alias) {
    return count_field(r, "*", alias);
}

RelationLoad::RelationLoad(std::allocator_arg_t, Allocator a, std::string_view name)
    : name(name, a.resource()) {}

RelationLoad::RelationLoad(std::allocator_arg_t,
                           Allocator a,
                           std::string_view name,
                           const SelectQuery& value)
    : name(name, a.resource()), query(detail::make_box<SelectQuery>(a.resource(), value)) {}

RelationLoad::RelationLoad(std::allocator_arg_t, Allocator a, const RelationLoad& other)
    : RelationLoad(std::allocator_arg, a, other.name) {
    if (other.query) {
        query = detail::make_box<SelectQuery>(a.resource(), *other.query);
    }
}

RelationLoad::RelationLoad(std::allocator_arg_t, Allocator a, RelationLoad&& other)
    : RelationLoad(detail::move_to(a.resource(), std::move(other))) {}

RelationLoad::RelationLoad(RelationLoad&&) noexcept = default;

RelationLoad& RelationLoad::operator=(RelationLoad&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

RelationLoad::~RelationLoad() = default;

bool RelationLoad::operator==(const RelationLoad& other) const {
    return name == other.name && bool(query) == bool(other.query) &&
           (!query || *query == *other.query);
}

RawSqlProjection::RawSqlProjection(std::allocator_arg_t,
                                   Allocator a,
                                   std::string_view name,
                                   std::string_view sql)
    : property_name(name, a.resource()), raw_sql_segment(sql, a.resource()) {}

RawSqlProjection::RawSqlProjection(std::allocator_arg_t, Allocator a, const RawSqlProjection& other)
    : RawSqlProjection(std::allocator_arg, a, other.property_name, other.raw_sql_segment) {}

ContinuousPageFetchOptions::ContinuousPageFetchOptions(std::allocator_arg_t,
                                                       Allocator a,
                                                       std::string_view name,
                                                       std::uint64_t ttl)
    : namespace_name(name, a.resource()), ttl_seconds(ttl) {
    if (blank_namespace(name)) {
        throw std::invalid_argument("continuous page namespace must not be empty");
    }
    if (!ttl) {
        throw std::invalid_argument("continuous page ttl_seconds must be positive");
    }
}

ContinuousPageFetchOptions::ContinuousPageFetchOptions(std::allocator_arg_t,
                                                       Allocator a,
                                                       const ContinuousPageFetchOptions& other)
    : namespace_name(other.namespace_name, a.resource()), ttl_seconds(other.ttl_seconds) {}

IdSetPaginationOptions::IdSetPaginationOptions(std::allocator_arg_t,
                                               Allocator a,
                                               std::string_view name,
                                               std::uint64_t ttl,
                                               std::uint64_t maximum)
    : namespace_name(name, a.resource()), ttl_seconds(ttl), max_ids(maximum) {
    if (blank_namespace(name)) {
        throw std::invalid_argument("ID set pagination namespace must not be empty");
    }
    if (!ttl) {
        throw std::invalid_argument("ID set pagination ttl_seconds must be positive");
    }
    if (!maximum) {
        throw std::invalid_argument("ID set pagination max_ids must be positive");
    }
}

IdSetPaginationOptions::IdSetPaginationOptions(std::allocator_arg_t,
                                               Allocator a,
                                               const IdSetPaginationOptions& other)
    : namespace_name(other.namespace_name, a.resource()), ttl_seconds(other.ttl_seconds),
      max_ids(other.max_ids) {}

SelectQuery::SelectQuery(std::allocator_arg_t, Allocator a, std::string_view name)
    : entity(name, a.resource()), projection(a.resource()), expr_projection(a.resource()),
      order_by(a.resource()), aggregates(a.resource()), group_by_fields(a.resource()),
      relations(a.resource()), trace_chain(a.resource()), raw_sql_search_criteria(a.resource()),
      dynamic_properties(a.resource()), raw_projections(a.resource()),
      object_group_bys(a.resource()), child_enhancements(a.resource()) {}

SelectQuery::SelectQuery(std::allocator_arg_t, Allocator a, const SelectQuery& other)
    : SelectQuery(std::allocator_arg, a, other.entity) {
    hard_limit_value = other.hard_limit_value;
    slice = other.slice;
    top_n_probe_parent_threshold_value = other.top_n_probe_parent_threshold_value;
    aggregation_cache = other.aggregation_cache;
    stream_config = other.stream_config;
    copy_items(projection, other.projection);
    copy_items(expr_projection, other.expr_projection);
    copy_items(order_by, other.order_by);
    copy_items(aggregates, other.aggregates);
    copy_items(group_by_fields, other.group_by_fields);
    copy_items(relations, other.relations);
    copy_items(trace_chain, other.trace_chain);
    copy_items(raw_sql_search_criteria, other.raw_sql_search_criteria);
    copy_items(dynamic_properties, other.dynamic_properties);
    copy_items(raw_projections, other.raw_projections);
    copy_items(object_group_bys, other.object_group_bys);
    copy_items(child_enhancements, other.child_enhancements);
    clone_optional(search_text, a.resource(), other.search_text);
    clone_optional(predicate, a.resource(), other.predicate);
    clone_optional(having_predicate, a.resource(), other.having_predicate);
    clone_optional(partition_field, a.resource(), other.partition_field);
    clone_optional(comment_text, a.resource(), other.comment_text);
    clone_optional(raw_sql_text, a.resource(), other.raw_sql_text);
    clone_optional(continuous_page_fetch, a.resource(), other.continuous_page_fetch);
    clone_optional(id_set_pagination, a.resource(), other.id_set_pagination);
}

SelectQuery::SelectQuery(std::allocator_arg_t, Allocator a, SelectQuery&& other)
    : SelectQuery(detail::move_to(a.resource(), std::move(other))) {}

SelectQuery::SelectQuery(SelectQuery&&) noexcept = default;

SelectQuery& SelectQuery::operator=(SelectQuery&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

SelectQuery::~SelectQuery() = default;
bool SelectQuery::operator==(const SelectQuery&) const = default;

SelectQuery SelectQuery::clone(Memory r) const {
    return SelectQuery(std::allocator_arg, Allocator(r), *this);
}

SelectQuery& SelectQuery::projects(std::initializer_list<std::string_view> names) {
    for (auto name : names) {
        project(name);
    }
    return *this;
}

SelectQuery& SelectQuery::project_expr(std::string_view name, const Expr& expr) {
    expr_projection.emplace_back(name, expr);
    return *this;
}

SelectQuery& SelectQuery::project_raw(std::string_view name, std::string_view sql) {
    raw_projections.emplace_back(name, sql);
    return *this;
}

SelectQuery& SelectQuery::dynamic_property_raw(std::string_view name, std::string_view sql) {
    dynamic_properties.emplace_back(name, sql);
    return *this;
}

SelectQuery& SelectQuery::search_with_text(std::string_view text) {
    set_text(search_text, memory_resource(), text);
    return *this;
}

SelectQuery& SelectQuery::add_order(const OrderBy& value) {
    order_by.emplace_back(value);
    return *this;
}

SelectQuery& SelectQuery::order(std::string_view field, SortDirection dir) {
    order_by.emplace_back(field, dir);
    return *this;
}

SelectQuery& SelectQuery::order_asc(std::string_view field) {
    return order(field);
}

SelectQuery& SelectQuery::order_desc(std::string_view field) {
    return order(field, SortDirection::Desc);
}

SelectQuery& SelectQuery::order_expr_asc(const Expr& expr) {
    order_by.emplace_back(expr, SortDirection::Asc);
    return *this;
}

SelectQuery& SelectQuery::order_expr_desc(const Expr& expr) {
    order_by.emplace_back(expr, SortDirection::Desc);
    return *this;
}

SelectQuery& SelectQuery::order_gbk_asc(std::string_view field) {
    return order_expr_asc(Expr::gbk(Expr::column(memory_resource(), field)));
}

SelectQuery& SelectQuery::order_gbk_desc(std::string_view field) {
    return order_expr_desc(Expr::gbk(Expr::column(memory_resource(), field)));
}

SelectQuery& SelectQuery::group_by(std::string_view field) {
    group_by_fields.emplace_back(field);
    return *this;
}

SelectQuery& SelectQuery::aggregate(const Aggregate& value) {
    aggregates.emplace_back(value);
    return *this;
}

SelectQuery& SelectQuery::count(std::string_view alias) {
    return aggregate(Aggregate::count(memory_resource(), alias));
}

SelectQuery& SelectQuery::enable_aggregation_cache() {
    return enable_aggregation_cache_for(0);
}

SelectQuery& SelectQuery::enable_aggregation_cache_for(std::uint64_t millis) {
    aggregation_cache = AggregationCacheOptions::for_duration(millis);
    return *this;
}

SelectQuery& SelectQuery::propagate_aggregation_cache(std::uint64_t millis) {
    if (!aggregation_cache) {
        enable_aggregation_cache();
    }
    aggregation_cache->with_propagation(millis);
    return *this;
}

SelectQuery& SelectQuery::comment(std::string_view text) {
    String copy(text, memory_resource());
    trace_chain.emplace_back(TraceKind::Comment, entity, copy);
    comment_text.emplace(std::move(copy));
    return *this;
}

SelectQuery& SelectQuery::purpose(std::string_view text) {
    trace_chain.emplace_back(TraceKind::Purpose, entity, text);
    return *this;
}

SelectQuery& SelectQuery::raw_sql(std::string_view text) {
    set_text(raw_sql_text, memory_resource(), text);
    return *this;
}

SelectQuery& SelectQuery::add_raw_sql_search_criteria(std::string_view sql) {
    raw_sql_search_criteria.emplace_back(sql);
    return *this;
}

SelectQuery& SelectQuery::object_group_by(std::string_view name,
                                          std::string_view field,
                                          const SelectQuery& query) {
    object_group_bys.emplace_back(name, field, query);
    return *this;
}

SelectQuery& SelectQuery::child_enhancement(const SelectQuery& query) {
    child_enhancements.emplace_back(query);
    return *this;
}

SelectQuery& SelectQuery::relation(std::string_view name) {
    relations.emplace_back(name);
    return *this;
}

SelectQuery& SelectQuery::relation_query(std::string_view name, const SelectQuery& query) {
    relations.emplace_back(name, query);
    return *this;
}

SelectQuery& SelectQuery::limit(std::uint64_t value) {
    if (!slice) {
        slice = Slice{};
    }
    slice->limit = value;
    return *this;
}

SelectQuery& SelectQuery::offset(std::uint64_t value) {
    if (!slice) {
        slice = Slice{};
    }
    slice->offset = value;
    return *this;
}

SelectQuery& SelectQuery::hard_limit(std::uint64_t value) {
    if (!value) {
        throw std::invalid_argument("hard_limit must be positive");
    }
    hard_limit_value = value;
    return *this;
}

SelectQuery& SelectQuery::page(std::uint64_t offset_value, std::uint64_t limit_value) {
    return offset(offset_value).limit(limit_value);
}

void SelectQuery::apply_list_limit(std::uint64_t ceiling) {
    if (!slice) {
        slice = Slice{};
    }
    if (slice->limit && *slice->limit > ceiling) {
        String message("QUERY_HARD_LIMIT_EXCEEDED: requested limit ", memory_resource());
        auto append = [&](std::uint64_t value) {
            char digits[20];
            auto result = std::to_chars(digits, digits + sizeof(digits), value);
            message.append(digits, result.ptr);
        };
        append(*slice->limit);
        message += " exceeds hard limit ";
        append(ceiling);
        throw std::invalid_argument(message.c_str());
    }
    if (!slice->limit) {
        slice->limit = ceiling;
    }
    for (auto& relation : relations) {
        if (relation.query) {
            relation.query->apply_list_limit(10'000);
        }
    }
    for (auto& child : child_enhancements) {
        child.apply_list_limit(10'000);
    }
}

SelectQuery SelectQuery::prepare_for_list() const {
    auto result = clone(memory_resource());
    result.apply_list_limit(hard_limit_value);
    return result;
}

SelectQuery& SelectQuery::optimize_for_continuous_page_fetch() {
    return optimize_for_continuous_page_fetch_with("default",
                                                   ContinuousPageFetchOptions::DEFAULT_TTL_SECONDS);
}

SelectQuery& SelectQuery::optimize_for_continuous_page_fetch_with(std::string_view name,
                                                                  std::uint64_t ttl) {
    ContinuousPageFetchOptions copy(std::allocator_arg, Allocator(memory_resource()), name, ttl);
    continuous_page_fetch.emplace(std::move(copy));
    return *this;
}

SelectQuery& SelectQuery::optimize_pagination_with_id_set() {
    return optimize_pagination_with_id_set_config("default",
                                                  IdSetPaginationOptions::DEFAULT_TTL_SECONDS,
                                                  IdSetPaginationOptions::DEFAULT_MAX_IDS);
}

SelectQuery& SelectQuery::optimize_pagination_with_id_set_config(std::string_view name,
                                                                 std::uint64_t ttl,
                                                                 std::uint64_t maximum) {
    IdSetPaginationOptions copy(
        std::allocator_arg, Allocator(memory_resource()), name, ttl, maximum);
    id_set_pagination.emplace(std::move(copy));
    return *this;
}

SelectQuery& SelectQuery::partition_by(std::string_view field) {
    set_text(partition_field, memory_resource(), field);
    return *this;
}

SelectQuery& SelectQuery::top_n_probe_parent_threshold(std::size_t value) {
    top_n_probe_parent_threshold_value = value;
    return *this;
}

SelectQuery& SelectQuery::stream(std::size_t size) {
    stream_config = StreamConfig{size};
    return *this;
}

SelectQuery& SelectQuery::stream_default() {
    stream_config = StreamConfig{};
    return *this;
}

ObjectGroupBy::ObjectGroupBy(std::allocator_arg_t,
                             Allocator a,
                             std::string_view name,
                             std::string_view field,
                             const SelectQuery& value)
    : property_name(name, a.resource()), storage_field(field, a.resource()),
      query(std::allocator_arg, a, value) {}

ObjectGroupBy::ObjectGroupBy(std::allocator_arg_t, Allocator a, const ObjectGroupBy& other)
    : ObjectGroupBy(std::allocator_arg, a, other.property_name, other.storage_field, other.query) {}

RelationAggregate::RelationAggregate(std::allocator_arg_t,
                                     Allocator a,
                                     std::string_view name,
                                     std::string_view output,
                                     const SelectQuery& value,
                                     bool single)
    : relation_name(name, a.resource()), alias(output, a.resource()),
      query(std::allocator_arg, a, value), single_result(single) {}

RelationAggregate::RelationAggregate(std::allocator_arg_t,
                                     Allocator a,
                                     const RelationAggregate& other)
    : RelationAggregate(std::allocator_arg,
                        a,
                        other.relation_name,
                        other.alias,
                        other.query,
                        other.single_result) {}

SelectQuery& SelectQuery::filter(const Expr& value) {
    auto copy = value.clone(memory_resource());
    predicate.emplace(std::move(copy));
    return *this;
}

SelectQuery& SelectQuery::and_filter(const Expr& value) {
    auto copy = predicate ? predicate->and_expr(value) : value.clone(memory_resource());
    predicate.emplace(std::move(copy));
    return *this;
}

SelectQuery& SelectQuery::or_filter(const Expr& value) {
    auto copy = predicate ? predicate->or_expr(value) : value.clone(memory_resource());
    predicate.emplace(std::move(copy));
    return *this;
}

SelectQuery& SelectQuery::having(const Expr& value) {
    auto copy = value.clone(memory_resource());
    having_predicate.emplace(std::move(copy));
    return *this;
}

SelectQuery& SelectQuery::and_having(const Expr& value) {
    auto copy =
        having_predicate ? having_predicate->and_expr(value) : value.clone(memory_resource());
    having_predicate.emplace(std::move(copy));
    return *this;
}

SelectQuery& SelectQuery::or_having(const Expr& value) {
    auto copy =
        having_predicate ? having_predicate->or_expr(value) : value.clone(memory_resource());
    having_predicate.emplace(std::move(copy));
    return *this;
}

Aggregate Aggregate::count_field(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::Count, field, alias);
}

SelectQuery& SelectQuery::count_field(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::count_field(memory_resource(), field, alias));
}

Aggregate Aggregate::sum(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::Sum, field, alias);
}

SelectQuery& SelectQuery::sum(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::sum(memory_resource(), field, alias));
}

Aggregate Aggregate::avg(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::Avg, field, alias);
}

SelectQuery& SelectQuery::avg(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::avg(memory_resource(), field, alias));
}

Aggregate Aggregate::min(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::Min, field, alias);
}

SelectQuery& SelectQuery::min(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::min(memory_resource(), field, alias));
}

Aggregate Aggregate::max(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::Max, field, alias);
}

SelectQuery& SelectQuery::max(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::max(memory_resource(), field, alias));
}

Aggregate Aggregate::stddev(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::Stddev, field, alias);
}

SelectQuery& SelectQuery::stddev(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::stddev(memory_resource(), field, alias));
}

Aggregate Aggregate::stddev_pop(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::StddevPop, field, alias);
}

SelectQuery& SelectQuery::stddev_pop(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::stddev_pop(memory_resource(), field, alias));
}

Aggregate Aggregate::var_samp(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::VarSamp, field, alias);
}

SelectQuery& SelectQuery::var_samp(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::var_samp(memory_resource(), field, alias));
}

Aggregate Aggregate::var_pop(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::VarPop, field, alias);
}

SelectQuery& SelectQuery::var_pop(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::var_pop(memory_resource(), field, alias));
}

Aggregate Aggregate::bit_and(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::BitAnd, field, alias);
}

SelectQuery& SelectQuery::bit_and(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::bit_and(memory_resource(), field, alias));
}

Aggregate Aggregate::bit_or(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::BitOr, field, alias);
}

SelectQuery& SelectQuery::bit_or(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::bit_or(memory_resource(), field, alias));
}

Aggregate Aggregate::bit_xor(Memory r, std::string_view field, std::string_view alias) {
    return Aggregate(std::allocator_arg, Allocator(r), AggregateFunction::BitXor, field, alias);
}

SelectQuery& SelectQuery::bit_xor(std::string_view field, std::string_view alias) {
    return aggregate(Aggregate::bit_xor(memory_resource(), field, alias));
}
}
