#pragma once
#include <teaql/core/expr.hpp>
#include <teaql/core/meta.hpp>
#include <teaql/core/row.hpp>
#include <teaql/core/trace.hpp>
#include <stdexcept>

namespace teaql {
enum class SortDirection { Asc, Desc };

struct NamedExpr {
    using allocator_type = Allocator;
    String alias;
    Expr expr;
    NamedExpr(std::allocator_arg_t, Allocator, std::string_view, const Expr&);
    NamedExpr(std::allocator_arg_t, Allocator, const NamedExpr&);

    NamedExpr(std::allocator_arg_t, Allocator a, NamedExpr&& other)
        : NamedExpr(detail::move_to(a.resource(), std::move(other))) {}

    NamedExpr(NamedExpr&&) noexcept = default;

    NamedExpr& operator=(NamedExpr&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return alias.get_allocator().resource();
    }

    bool operator==(const NamedExpr&) const = default;
};

struct OrderBy {
    using allocator_type = Allocator;
    String field;
    std::optional<Expr> expr;
    SortDirection direction;
    OrderBy(std::allocator_arg_t, Allocator, std::string_view, SortDirection);
    OrderBy(std::allocator_arg_t, Allocator, const Expr&, SortDirection);
    OrderBy(std::allocator_arg_t, Allocator, const OrderBy&);

    OrderBy(std::allocator_arg_t, Allocator a, OrderBy&& other)
        : OrderBy(detail::move_to(a.resource(), std::move(other))) {}

    OrderBy(OrderBy&&) noexcept = default;

    OrderBy& operator=(OrderBy&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return field.get_allocator().resource();
    }

    static OrderBy asc(Memory, std::string_view);
    static OrderBy desc(Memory, std::string_view);
    static OrderBy asc_expr(const Expr&);
    static OrderBy desc_expr(const Expr&);
    static OrderBy asc_gbk(Memory, std::string_view);
    static OrderBy desc_gbk(Memory, std::string_view);
    bool operator==(const OrderBy&) const = default;
};
enum class AggregateFunction {
    Count,
    Sum,
    Avg,
    Min,
    Max,
    Stddev,
    StddevPop,
    VarSamp,
    VarPop,
    BitAnd,
    BitOr,
    BitXor
};

struct Aggregate {
    using allocator_type = Allocator;
    AggregateFunction function;
    String field, alias;
    Aggregate(
        std::allocator_arg_t, Allocator, AggregateFunction, std::string_view, std::string_view);
    Aggregate(std::allocator_arg_t, Allocator, const Aggregate&);

    Aggregate(std::allocator_arg_t, Allocator a, Aggregate&& other)
        : Aggregate(detail::move_to(a.resource(), std::move(other))) {}

    Aggregate(Aggregate&&) noexcept = default;

    Aggregate& operator=(Aggregate&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return field.get_allocator().resource();
    }

    static Aggregate count(Memory, std::string_view);
    static Aggregate count_field(Memory, std::string_view, std::string_view);
    static Aggregate sum(Memory, std::string_view, std::string_view);
    static Aggregate avg(Memory, std::string_view, std::string_view);
    static Aggregate min(Memory, std::string_view, std::string_view);
    static Aggregate max(Memory, std::string_view, std::string_view);
    static Aggregate stddev(Memory, std::string_view, std::string_view);
    static Aggregate stddev_pop(Memory, std::string_view, std::string_view);
    static Aggregate var_samp(Memory, std::string_view, std::string_view);
    static Aggregate var_pop(Memory, std::string_view, std::string_view);
    static Aggregate bit_and(Memory, std::string_view, std::string_view);
    static Aggregate bit_or(Memory, std::string_view, std::string_view);
    static Aggregate bit_xor(Memory, std::string_view, std::string_view);
    bool operator==(const Aggregate&) const = default;
};

struct Slice {
    std::optional<std::uint64_t> limit;
    std::uint64_t offset = 0;
    bool operator==(const Slice&) const = default;
};

struct RelationLoad {
    using allocator_type = Allocator;
    String name;
    detail::ScopedBox<SelectQuery> query;
    RelationLoad(std::allocator_arg_t, Allocator, std::string_view);
    RelationLoad(std::allocator_arg_t, Allocator, std::string_view, const SelectQuery&);
    RelationLoad(std::allocator_arg_t, Allocator, const RelationLoad&);
    RelationLoad(std::allocator_arg_t, Allocator, RelationLoad&&);
    RelationLoad(RelationLoad&&) noexcept;
    RelationLoad& operator=(RelationLoad&&);
    ~RelationLoad();

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    bool operator==(const RelationLoad&) const;
};

struct RawSqlProjection {
    using allocator_type = Allocator;
    String property_name, raw_sql_segment;
    RawSqlProjection(std::allocator_arg_t, Allocator, std::string_view, std::string_view);
    RawSqlProjection(std::allocator_arg_t, Allocator, const RawSqlProjection&);

    RawSqlProjection(std::allocator_arg_t, Allocator a, RawSqlProjection&& other)
        : RawSqlProjection(detail::move_to(a.resource(), std::move(other))) {}

    RawSqlProjection(RawSqlProjection&&) noexcept = default;

    RawSqlProjection& operator=(RawSqlProjection&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return property_name.get_allocator().resource();
    }

    bool operator==(const RawSqlProjection&) const = default;
};

struct AggregationCacheOptions {
    bool enabled = true;
    std::uint64_t cache_expired_millis = 0;
    bool propagate = false;
    std::uint64_t propagate_cache_expired_millis = 0;

    static AggregationCacheOptions for_duration(std::uint64_t millis) {
        return {true, millis, false, 0};
    }

    AggregationCacheOptions& with_propagation(std::uint64_t millis) {
        propagate = true;
        propagate_cache_expired_millis = millis;
        return *this;
    }

    bool operator==(const AggregationCacheOptions&) const = default;
};

struct StreamConfig {
    std::size_t chunk_size = 1000;
    bool operator==(const StreamConfig&) const = default;
};

struct ContinuousPageFetchOptions {
    using allocator_type = Allocator;
    static constexpr std::uint64_t DEFAULT_TTL_SECONDS = 600;
    String namespace_name;
    std::uint64_t ttl_seconds;
    ContinuousPageFetchOptions(std::allocator_arg_t, Allocator, std::string_view, std::uint64_t);
    ContinuousPageFetchOptions(std::allocator_arg_t, Allocator, const ContinuousPageFetchOptions&);

    ContinuousPageFetchOptions(std::allocator_arg_t,
                               Allocator a,
                               ContinuousPageFetchOptions&& other)
        : ContinuousPageFetchOptions(detail::move_to(a.resource(), std::move(other))) {}

    ContinuousPageFetchOptions(ContinuousPageFetchOptions&&) noexcept = default;

    ContinuousPageFetchOptions& operator=(ContinuousPageFetchOptions&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return namespace_name.get_allocator().resource();
    }

    bool operator==(const ContinuousPageFetchOptions&) const = default;
};

struct IdSetPaginationOptions {
    using allocator_type = Allocator;
    static constexpr std::uint64_t DEFAULT_TTL_SECONDS = 600;
    static constexpr std::uint64_t DEFAULT_MAX_IDS = 3'000'000;
    String namespace_name;
    std::uint64_t ttl_seconds, max_ids;
    IdSetPaginationOptions(
        std::allocator_arg_t, Allocator, std::string_view, std::uint64_t, std::uint64_t);
    IdSetPaginationOptions(std::allocator_arg_t, Allocator, const IdSetPaginationOptions&);

    IdSetPaginationOptions(std::allocator_arg_t, Allocator a, IdSetPaginationOptions&& other)
        : IdSetPaginationOptions(detail::move_to(a.resource(), std::move(other))) {}

    IdSetPaginationOptions(IdSetPaginationOptions&&) noexcept = default;

    IdSetPaginationOptions& operator=(IdSetPaginationOptions&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return namespace_name.get_allocator().resource();
    }

    bool operator==(const IdSetPaginationOptions&) const = default;
};
struct ObjectGroupBy;

struct SelectQuery {
    using allocator_type = Allocator;
    std::uint64_t hard_limit_value = 10'000;
    String entity;
    Vector<String> projection;
    Vector<NamedExpr> expr_projection;
    std::optional<String> search_text;
    std::optional<Expr> predicate, having_predicate;
    Vector<OrderBy> order_by;
    std::optional<Slice> slice;
    std::optional<String> partition_field;
    std::optional<std::size_t> top_n_probe_parent_threshold_value;
    Vector<Aggregate> aggregates;
    Vector<String> group_by_fields;
    Vector<RelationLoad> relations;
    std::optional<AggregationCacheOptions> aggregation_cache;
    std::optional<String> comment_text;
    TraceChain trace_chain;
    std::optional<String> raw_sql_text;
    Vector<String> raw_sql_search_criteria;
    Vector<RawSqlProjection> dynamic_properties, raw_projections;
    Vector<ObjectGroupBy> object_group_bys;
    Vector<SelectQuery> child_enhancements;
    std::optional<StreamConfig> stream_config;
    std::optional<ContinuousPageFetchOptions> continuous_page_fetch;
    std::optional<IdSetPaginationOptions> id_set_pagination;

    SelectQuery(std::allocator_arg_t, Allocator, std::string_view);
    SelectQuery(std::allocator_arg_t, Allocator, const SelectQuery&);
    SelectQuery(std::allocator_arg_t, Allocator, SelectQuery&&);
    SelectQuery(const SelectQuery&) = delete;
    SelectQuery& operator=(const SelectQuery&) = delete;
    SelectQuery(SelectQuery&&) noexcept;
    SelectQuery& operator=(SelectQuery&&);
    ~SelectQuery();

    Memory memory_resource() const {
        return entity.get_allocator().resource();
    }

    SelectQuery clone(Memory) const;
    bool operator==(const SelectQuery&) const;

    SelectQuery& project(std::string_view field) {
        projection.emplace_back(field);
        return *this;
    }

    SelectQuery& projects(std::initializer_list<std::string_view>);

    template <std::ranges::input_range Names>
        requires std::convertible_to<std::ranges::range_reference_t<Names>, std::string_view>
    SelectQuery& projects(Names&& names) {
        Vector<String> snapshot(memory_resource());
        for (auto&& name : names) {
            snapshot.emplace_back(std::string_view(name));
        }
        projection.reserve(projection.size() + snapshot.size());
        for (auto& name : snapshot) {
            projection.emplace_back(std::move(name));
        }
        return *this;
    }

    SelectQuery& project_expr(std::string_view, const Expr&);
    SelectQuery& project_raw(std::string_view, std::string_view);
    SelectQuery& dynamic_property_raw(std::string_view, std::string_view);
    SelectQuery& search_with_text(std::string_view);
    SelectQuery& filter(const Expr&);
    SelectQuery& and_filter(const Expr&);
    SelectQuery& or_filter(const Expr&);
    SelectQuery& having(const Expr&);
    SelectQuery& and_having(const Expr&);
    SelectQuery& or_having(const Expr&);
    SelectQuery& add_order(const OrderBy&);
    SelectQuery& order(std::string_view, SortDirection = SortDirection::Asc);
    SelectQuery& order_asc(std::string_view);
    SelectQuery& order_desc(std::string_view);
    SelectQuery& order_expr_asc(const Expr&);
    SelectQuery& order_expr_desc(const Expr&);
    SelectQuery& order_gbk_asc(std::string_view);
    SelectQuery& order_gbk_desc(std::string_view);
    SelectQuery& group_by(std::string_view);
    SelectQuery& aggregate(const Aggregate&);
    SelectQuery& count(std::string_view);
    SelectQuery& count_field(std::string_view, std::string_view);
    SelectQuery& sum(std::string_view, std::string_view);
    SelectQuery& avg(std::string_view, std::string_view);
    SelectQuery& min(std::string_view, std::string_view);
    SelectQuery& max(std::string_view, std::string_view);
    SelectQuery& stddev(std::string_view, std::string_view);
    SelectQuery& stddev_pop(std::string_view, std::string_view);
    SelectQuery& var_samp(std::string_view, std::string_view);
    SelectQuery& var_pop(std::string_view, std::string_view);
    SelectQuery& bit_and(std::string_view, std::string_view);
    SelectQuery& bit_or(std::string_view, std::string_view);
    SelectQuery& bit_xor(std::string_view, std::string_view);
    SelectQuery& enable_aggregation_cache();
    SelectQuery& enable_aggregation_cache_for(std::uint64_t);
    SelectQuery& propagate_aggregation_cache(std::uint64_t);
    SelectQuery& comment(std::string_view);
    SelectQuery& purpose(std::string_view); // Transitional low-level example adapter.
    SelectQuery& raw_sql(std::string_view);
    SelectQuery& add_raw_sql_search_criteria(std::string_view);
    SelectQuery& object_group_by(std::string_view, std::string_view, const SelectQuery&);
    SelectQuery& child_enhancement(const SelectQuery&);
    SelectQuery& relation(std::string_view);
    SelectQuery& relation_query(std::string_view, const SelectQuery&);
    SelectQuery& limit(std::uint64_t);
    SelectQuery& offset(std::uint64_t);
    SelectQuery& hard_limit(std::uint64_t);
    SelectQuery& page(std::uint64_t offset, std::uint64_t limit);
    SelectQuery prepare_for_list() const;
    SelectQuery& optimize_for_continuous_page_fetch();
    SelectQuery& optimize_for_continuous_page_fetch_with(std::string_view, std::uint64_t);
    SelectQuery& optimize_pagination_with_id_set();
    SelectQuery&
        optimize_pagination_with_id_set_config(std::string_view, std::uint64_t, std::uint64_t);
    SelectQuery& partition_by(std::string_view);
    SelectQuery& top_n_probe_parent_threshold(std::size_t);
    SelectQuery& stream(std::size_t);
    SelectQuery& stream_default();

private:
    void apply_list_limit(std::uint64_t);
};

struct RelationAggregate {
    using allocator_type = Allocator;
    String relation_name, alias;
    SelectQuery query;
    bool single_result;
    RelationAggregate(std::allocator_arg_t,
                      Allocator,
                      std::string_view,
                      std::string_view,
                      const SelectQuery&,
                      bool);
    RelationAggregate(std::allocator_arg_t, Allocator, const RelationAggregate&);

    RelationAggregate(std::allocator_arg_t, Allocator a, RelationAggregate&& other)
        : RelationAggregate(detail::move_to(a.resource(), std::move(other))) {}

    RelationAggregate(RelationAggregate&&) noexcept = default;

    RelationAggregate& operator=(RelationAggregate&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return relation_name.get_allocator().resource();
    }

    bool operator==(const RelationAggregate&) const = default;
};

struct ObjectGroupBy {
    using allocator_type = Allocator;
    String property_name, storage_field;
    SelectQuery query;
    ObjectGroupBy(
        std::allocator_arg_t, Allocator, std::string_view, std::string_view, const SelectQuery&);
    ObjectGroupBy(std::allocator_arg_t, Allocator, const ObjectGroupBy&);

    ObjectGroupBy(std::allocator_arg_t, Allocator a, ObjectGroupBy&& other)
        : ObjectGroupBy(detail::move_to(a.resource(), std::move(other))) {}

    ObjectGroupBy(ObjectGroupBy&&) noexcept = default;

    ObjectGroupBy& operator=(ObjectGroupBy&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return property_name.get_allocator().resource();
    }

    bool operator==(const ObjectGroupBy&) const = default;
};

// Storage of the core SubQuery expression, distinct from a runtime relation plan.
struct SubQueryExpr {
    using allocator_type = Allocator;
    EntityDescriptor entity;
    SelectQuery query;

    SubQueryExpr(std::allocator_arg_t, Allocator a, const EntityDescriptor& e, const SelectQuery& q)
        : entity(std::allocator_arg, a, e), query(std::allocator_arg, a, q) {}

    SubQueryExpr(std::allocator_arg_t, Allocator a, const SubQueryExpr& other)
        : SubQueryExpr(std::allocator_arg, a, other.entity, other.query) {}

    bool operator==(const SubQueryExpr&) const = default;
};

inline constexpr std::string_view PARTITION_RANK_PROPERTY = "__teaql_partition_rank";
}
