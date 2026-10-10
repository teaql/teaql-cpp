#pragma once
#include <teaql/core/query.hpp>
#include <teaql/core/list.hpp>
#include <teaql/memory/clone.hpp>

namespace teaql {
class DebugWriter;
}

namespace teaql::request {
inline constexpr std::string_view COUNT_ALIAS = "count", TYPE_FIELD = "internal_type",
                                  TYPE_GROUP_FIELD = "type_group";
enum class FieldOperator {
    Equal,
    NotEqual,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,
    Between,
    In,
    NotIn,
    Contain,
    NotContain,
    BeginWith,
    NotBeginWith,
    EndWith,
    NotEndWith,
    SoundsLike,
    IsNull,
    IsNotNull
};

template <class T>
struct DateRange {
    using allocator_type = Allocator;
    T start, end;

    DateRange(std::allocator_arg_t, Allocator a, const T& start, const T& end)
        requires detail::domain_copyable<T>
        : start(detail::clone_in(a.resource(), start)), end(detail::clone_in(a.resource(), end)),
          resource_(a.resource()) {}

    DateRange(std::allocator_arg_t, Allocator a, T&& start, T&& end)
        : start(detail::move_in(a.resource(), std::move(start))),
          end(detail::move_in(a.resource(), std::move(end))), resource_(a.resource()) {}

    DateRange(std::allocator_arg_t, Allocator a, const DateRange& other)
        requires detail::domain_copyable<T>
        : DateRange(std::allocator_arg, a, other.start, other.end) {}

    DateRange(std::allocator_arg_t, Allocator a, DateRange&& other)
        : DateRange(detail::move_to(a.resource(), std::move(other))) {}

    DateRange(const DateRange&) = delete;
    DateRange(DateRange&&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

    Memory memory_resource() const noexcept {
        return resource_;
    }

    bool operator==(const DateRange& other) const
        requires std::equality_comparable<T>
    {
        return start == other.start && end == other.end;
    }

private:
    Memory resource_;
};
template <class T>
concept EntityReference =
    std::same_as<std::remove_cvref_t<T>, Value> ||
    std::same_as<std::remove_cvref_t<T>, std::uint64_t> || requires(T&& value) {
        { std::forward<T>(value).entity_id_value() } -> std::same_as<Value>;
    };

template <EntityReference T>
Value entity_id_value(Memory r, T&& value) {
    if constexpr (std::same_as<std::remove_cvref_t<T>, Value> ||
                  std::same_as<std::remove_cvref_t<T>, std::uint64_t>) {
        return Value(r, std::forward<T>(value));
    } else {
        return Value(r, std::forward<T>(value).entity_id_value());
    }
}

class UnsafeRawSqlSegment {
public:
    using allocator_type = Allocator;

    static UnsafeRawSqlSegment trusted(Memory r, std::string_view sql) {
        return UnsafeRawSqlSegment(r, sql);
    }

    UnsafeRawSqlSegment(std::allocator_arg_t, Allocator a, const UnsafeRawSqlSegment& other)
        : sql_(other.sql_, a.resource()) {}

    UnsafeRawSqlSegment(const UnsafeRawSqlSegment&) = delete;
    UnsafeRawSqlSegment(UnsafeRawSqlSegment&&) noexcept = default;

    String into_sql() && {
        return std::move(sql_);
    }

    Memory memory_resource() const {
        return sql_.get_allocator().resource();
    }

    bool operator==(const UnsafeRawSqlSegment&) const = default;

private:
    friend void write_debug(DebugWriter&, const UnsafeRawSqlSegment&);

    UnsafeRawSqlSegment(Memory r, std::string_view sql) : sql_(sql, r) {}

    String sql_;
};
struct QuerySelection;
struct RelationSelection;
struct RelationFilter;
struct RawDynamicProperty;
struct RawProjection;
struct RelationAggregate;
struct FacetRequest;
struct ObjectGroupBy;

struct QueryOptions {
    using allocator_type = Allocator;
    std::optional<String> comment, raw_sql;
    Vector<String> raw_sql_search_criteria;
    Vector<RawDynamicProperty> dynamic_properties;
    Vector<RawProjection> raw_projections;
    Vector<RelationAggregate> relation_aggregates;
    Vector<ObjectGroupBy> object_group_bys;
    Vector<FacetRequest> facets;
    QueryOptions(std::allocator_arg_t, Allocator);
    QueryOptions(std::allocator_arg_t, Allocator, const QueryOptions&);
    QueryOptions(std::allocator_arg_t, Allocator, QueryOptions&&);
    QueryOptions(const QueryOptions&) = delete;
    QueryOptions(QueryOptions&&) noexcept;
    QueryOptions& operator=(QueryOptions&&);
    ~QueryOptions();

    Memory memory_resource() const {
        return raw_sql_search_criteria.get_allocator().resource();
    }

    bool operator==(const QueryOptions&) const;
};

struct QuerySelection {
    using allocator_type = Allocator;
    SelectQuery query;
    Vector<RelationSelection> relation_selections;
    Vector<RelationFilter> relation_filters;
    Vector<QuerySelection> child_enhancements;
    QueryOptions query_options;
    QuerySelection(std::allocator_arg_t, Allocator, const SelectQuery&);
    QuerySelection(std::allocator_arg_t, Allocator, const QuerySelection&);
    QuerySelection(std::allocator_arg_t, Allocator, QuerySelection&&);
    QuerySelection(const QuerySelection&) = delete;
    QuerySelection(QuerySelection&&) noexcept;
    QuerySelection& operator=(QuerySelection&&);
    ~QuerySelection();

    Memory memory_resource() const {
        return query.memory_resource();
    }

    bool operator==(const QuerySelection&) const;
    SelectQuery into_query() &&;
};

struct RelationSelection {
    using allocator_type = Allocator;
    String name;
    SelectQuery query;
    Vector<RelationSelection> relation_selections;
    Vector<RelationFilter> relation_filters;
    Vector<QuerySelection> child_enhancements;
    QueryOptions query_options;
    RelationSelection(std::allocator_arg_t, Allocator, std::string_view, const QuerySelection&);
    RelationSelection(std::allocator_arg_t, Allocator, std::string_view, const SelectQuery&);
    RelationSelection(std::allocator_arg_t, Allocator, const RelationSelection&);
    RelationSelection(std::allocator_arg_t, Allocator, RelationSelection&&);
    RelationSelection(const RelationSelection&) = delete;
    RelationSelection(RelationSelection&&) noexcept;
    RelationSelection& operator=(RelationSelection&&);
    ~RelationSelection();

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    bool operator==(const RelationSelection&) const;
    SelectQuery into_query() &&;
};

struct RelationFilter {
    using allocator_type = Allocator;
    String name;
    SelectQuery query;
    Vector<RelationSelection> relation_selections;
    Vector<RelationFilter> relation_filters;
    Vector<QuerySelection> child_enhancements;
    QueryOptions query_options;
    RelationFilter(std::allocator_arg_t, Allocator, std::string_view, const QuerySelection&);
    RelationFilter(std::allocator_arg_t, Allocator, std::string_view, const SelectQuery&);
    RelationFilter(std::allocator_arg_t, Allocator, const RelationFilter&);
    RelationFilter(std::allocator_arg_t, Allocator, RelationFilter&&);
    RelationFilter(const RelationFilter&) = delete;
    RelationFilter(RelationFilter&&) noexcept;
    RelationFilter& operator=(RelationFilter&&);
    ~RelationFilter();

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    bool operator==(const RelationFilter&) const;
};

struct RawDynamicProperty {
    using allocator_type = Allocator;
    String property_name, raw_sql_segment;
    RawDynamicProperty(std::allocator_arg_t, Allocator, std::string_view, UnsafeRawSqlSegment);
    RawDynamicProperty(std::allocator_arg_t, Allocator, const RawDynamicProperty&);
    RawDynamicProperty(std::allocator_arg_t, Allocator, RawDynamicProperty&&);
    RawDynamicProperty(const RawDynamicProperty&) = delete;
    RawDynamicProperty(RawDynamicProperty&&) noexcept;
    RawDynamicProperty& operator=(RawDynamicProperty&&);
    ~RawDynamicProperty();

    Memory memory_resource() const {
        return property_name.get_allocator().resource();
    }

    bool operator==(const RawDynamicProperty&) const;
};

struct RawProjection {
    using allocator_type = Allocator;
    String property_name, raw_sql_segment;
    RawProjection(std::allocator_arg_t, Allocator, std::string_view, UnsafeRawSqlSegment);
    RawProjection(std::allocator_arg_t, Allocator, const RawProjection&);
    RawProjection(std::allocator_arg_t, Allocator, RawProjection&&);
    RawProjection(const RawProjection&) = delete;
    RawProjection(RawProjection&&) noexcept;
    RawProjection& operator=(RawProjection&&);
    ~RawProjection();

    Memory memory_resource() const {
        return property_name.get_allocator().resource();
    }

    bool operator==(const RawProjection&) const;
};

struct RelationAggregate {
    using allocator_type = Allocator;
    String relation_name, alias;
    QuerySelection query;
    bool single_result;
    RelationAggregate(std::allocator_arg_t,
                      Allocator,
                      std::string_view,
                      std::string_view,
                      const QuerySelection&,
                      bool);
    RelationAggregate(std::allocator_arg_t,
                      Allocator,
                      std::string_view,
                      std::string_view,
                      const SelectQuery&,
                      bool);
    RelationAggregate(std::allocator_arg_t, Allocator, const RelationAggregate&);
    RelationAggregate(std::allocator_arg_t, Allocator, RelationAggregate&&);
    RelationAggregate(const RelationAggregate&) = delete;
    RelationAggregate(RelationAggregate&&) noexcept;
    RelationAggregate& operator=(RelationAggregate&&);
    ~RelationAggregate();

    Memory memory_resource() const {
        return relation_name.get_allocator().resource();
    }

    bool operator==(const RelationAggregate&) const;
};

struct FacetRequest {
    using allocator_type = Allocator;
    String facet_name, relation_name;
    QuerySelection query;
    bool include_all_facets;
    FacetRequest(std::allocator_arg_t,
                 Allocator,
                 std::string_view,
                 std::string_view,
                 const QuerySelection&,
                 bool);
    FacetRequest(std::allocator_arg_t,
                 Allocator,
                 std::string_view,
                 std::string_view,
                 const SelectQuery&,
                 bool);
    FacetRequest(std::allocator_arg_t, Allocator, const FacetRequest&);
    FacetRequest(std::allocator_arg_t, Allocator, FacetRequest&&);
    FacetRequest(const FacetRequest&) = delete;
    FacetRequest(FacetRequest&&) noexcept;
    FacetRequest& operator=(FacetRequest&&);
    ~FacetRequest();

    Memory memory_resource() const {
        return facet_name.get_allocator().resource();
    }

    bool operator==(const FacetRequest&) const;
};

struct ObjectGroupBy {
    using allocator_type = Allocator;
    String property_name, storage_field;
    QuerySelection query;
    ObjectGroupBy(
        std::allocator_arg_t, Allocator, std::string_view, std::string_view, const QuerySelection&);
    ObjectGroupBy(
        std::allocator_arg_t, Allocator, std::string_view, std::string_view, const SelectQuery&);
    ObjectGroupBy(std::allocator_arg_t, Allocator, const ObjectGroupBy&);
    ObjectGroupBy(std::allocator_arg_t, Allocator, ObjectGroupBy&&);
    ObjectGroupBy(const ObjectGroupBy&) = delete;
    ObjectGroupBy(ObjectGroupBy&&) noexcept;
    ObjectGroupBy& operator=(ObjectGroupBy&&);
    ~ObjectGroupBy();

    Memory memory_resource() const {
        return property_name.get_allocator().resource();
    }

    bool operator==(const ObjectGroupBy&) const;
};

SelectQuery apply_relation_selections(SelectQuery, std::span<const RelationSelection>);
SelectQuery
apply_runtime_metadata(SelectQuery, const QueryOptions&, std::span<const QuerySelection>);
Vector<teaql::RelationAggregate> runtime_relation_aggregates(Memory, const QueryOptions&);
void merge_outer_filter_into_facet_aggregates(QuerySelection&, const SelectQuery&);

template <class T>
void attach_facets(SmartList<T>& rows,
                   const std::pmr::map<String, SmartList<CompactRow>, std::less<>>& facets) {
    for (const auto& [name, facet] : facets) {
        rows.with_facet(name, facet);
    }
}

Expr field_operator_expr(Memory, std::string_view, FieldOperator, std::span<const Value>);
Expr field_operator_column_expr(Memory, std::string_view, FieldOperator, std::string_view);
Value required_value(Memory, FieldOperator, std::span<const Value>, std::size_t);
String required_text(Memory, FieldOperator, std::span<const Value>, std::size_t);
std::optional<Expr> remove_default_live_filter(Memory, const std::optional<Expr>&);
std::optional<Expr> remove_filter_expr(Memory, const Expr&, const Expr&);
Value dynamic_json_value_to_teaql_value(Memory, const Json&);
Vector<Value> dynamic_json_values(Memory, const Json&);
FieldOperator dynamic_json_operator(const Json&);
Expr dynamic_json_filter_expr(Memory, std::string_view, const Json&);
std::optional<std::uint64_t> dynamic_json_u64_field(const Json::Object&, std::string_view);
}
