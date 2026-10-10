#include <teaql/core/request_debug.hpp>
#include <teaql/core/detail/text.hpp>

namespace teaql::request {
namespace {
template <class T>
void copy_items(Vector<T>& dst, const Vector<T>& src) {
    for (const auto& value : src) {
        dst.emplace_back(value);
    }
}
}

QueryOptions::QueryOptions(std::allocator_arg_t, Allocator a, QueryOptions&& other)
    : QueryOptions(detail::move_to(a.resource(), std::move(other))) {}

QueryOptions::QueryOptions(QueryOptions&&) noexcept = default;

QueryOptions& QueryOptions::operator=(QueryOptions&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

QueryOptions::~QueryOptions() = default;
bool QueryOptions::operator==(const QueryOptions&) const = default;

QuerySelection::QuerySelection(std::allocator_arg_t, Allocator a, QuerySelection&& other)
    : QuerySelection(detail::move_to(a.resource(), std::move(other))) {}

QuerySelection::QuerySelection(QuerySelection&&) noexcept = default;

QuerySelection& QuerySelection::operator=(QuerySelection&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

QuerySelection::~QuerySelection() = default;
bool QuerySelection::operator==(const QuerySelection&) const = default;

RelationSelection::RelationSelection(std::allocator_arg_t, Allocator a, RelationSelection&& other)
    : RelationSelection(detail::move_to(a.resource(), std::move(other))) {}

RelationSelection::RelationSelection(RelationSelection&&) noexcept = default;

RelationSelection& RelationSelection::operator=(RelationSelection&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

RelationSelection::~RelationSelection() = default;
bool RelationSelection::operator==(const RelationSelection&) const = default;

RelationFilter::RelationFilter(std::allocator_arg_t, Allocator a, RelationFilter&& other)
    : RelationFilter(detail::move_to(a.resource(), std::move(other))) {}

RelationFilter::RelationFilter(RelationFilter&&) noexcept = default;

RelationFilter& RelationFilter::operator=(RelationFilter&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

RelationFilter::~RelationFilter() = default;
bool RelationFilter::operator==(const RelationFilter&) const = default;

RawDynamicProperty::RawDynamicProperty(std::allocator_arg_t,
                                       Allocator a,
                                       RawDynamicProperty&& other)
    : RawDynamicProperty(detail::move_to(a.resource(), std::move(other))) {}

RawDynamicProperty::RawDynamicProperty(RawDynamicProperty&&) noexcept = default;

RawDynamicProperty& RawDynamicProperty::operator=(RawDynamicProperty&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

RawDynamicProperty::~RawDynamicProperty() = default;
bool RawDynamicProperty::operator==(const RawDynamicProperty&) const = default;

RawProjection::RawProjection(std::allocator_arg_t, Allocator a, RawProjection&& other)
    : RawProjection(detail::move_to(a.resource(), std::move(other))) {}

RawProjection::RawProjection(RawProjection&&) noexcept = default;

RawProjection& RawProjection::operator=(RawProjection&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

RawProjection::~RawProjection() = default;
bool RawProjection::operator==(const RawProjection&) const = default;

RelationAggregate::RelationAggregate(std::allocator_arg_t, Allocator a, RelationAggregate&& other)
    : RelationAggregate(detail::move_to(a.resource(), std::move(other))) {}

RelationAggregate::RelationAggregate(RelationAggregate&&) noexcept = default;

RelationAggregate& RelationAggregate::operator=(RelationAggregate&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

RelationAggregate::~RelationAggregate() = default;
bool RelationAggregate::operator==(const RelationAggregate&) const = default;

FacetRequest::FacetRequest(std::allocator_arg_t, Allocator a, FacetRequest&& other)
    : FacetRequest(detail::move_to(a.resource(), std::move(other))) {}

FacetRequest::FacetRequest(FacetRequest&&) noexcept = default;

FacetRequest& FacetRequest::operator=(FacetRequest&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

FacetRequest::~FacetRequest() = default;
bool FacetRequest::operator==(const FacetRequest&) const = default;

ObjectGroupBy::ObjectGroupBy(std::allocator_arg_t, Allocator a, ObjectGroupBy&& other)
    : ObjectGroupBy(detail::move_to(a.resource(), std::move(other))) {}

ObjectGroupBy::ObjectGroupBy(ObjectGroupBy&&) noexcept = default;

ObjectGroupBy& ObjectGroupBy::operator=(ObjectGroupBy&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

ObjectGroupBy::~ObjectGroupBy() = default;
bool ObjectGroupBy::operator==(const ObjectGroupBy&) const = default;

QueryOptions::QueryOptions(std::allocator_arg_t, Allocator a)
    : raw_sql_search_criteria(a.resource()), dynamic_properties(a.resource()),
      raw_projections(a.resource()), relation_aggregates(a.resource()),
      object_group_bys(a.resource()), facets(a.resource()) {}

QueryOptions::QueryOptions(std::allocator_arg_t, Allocator a, const QueryOptions& other)
    : QueryOptions(std::allocator_arg, a) {
    if (other.comment) {
        comment.emplace(*other.comment, a.resource());
    }
    if (other.raw_sql) {
        raw_sql.emplace(*other.raw_sql, a.resource());
    }
    copy_items(raw_sql_search_criteria, other.raw_sql_search_criteria);
    copy_items(dynamic_properties, other.dynamic_properties);
    copy_items(raw_projections, other.raw_projections);
    copy_items(relation_aggregates, other.relation_aggregates);
    copy_items(object_group_bys, other.object_group_bys);
    copy_items(facets, other.facets);
}

QuerySelection::QuerySelection(std::allocator_arg_t, Allocator a, const SelectQuery& query)
    : query(std::allocator_arg, a, query), relation_selections(a.resource()),
      relation_filters(a.resource()), child_enhancements(a.resource()),
      query_options(std::allocator_arg, a) {}

QuerySelection::QuerySelection(std::allocator_arg_t, Allocator a, const QuerySelection& other)
    : query(std::allocator_arg, a, other.query), relation_selections(a.resource()),
      relation_filters(a.resource()), child_enhancements(a.resource()),
      query_options(std::allocator_arg, a, other.query_options) {
    copy_items(relation_selections, other.relation_selections);
    copy_items(relation_filters, other.relation_filters);
    copy_items(child_enhancements, other.child_enhancements);
}

RelationSelection::RelationSelection(std::allocator_arg_t,
                                     Allocator a,
                                     std::string_view name,
                                     const QuerySelection& selection)
    : name(name, a.resource()), query(std::allocator_arg, a, selection.query),
      relation_selections(a.resource()), relation_filters(a.resource()),
      child_enhancements(a.resource()),
      query_options(std::allocator_arg, a, selection.query_options) {
    copy_items(relation_selections, selection.relation_selections);
    copy_items(relation_filters, selection.relation_filters);
    copy_items(child_enhancements, selection.child_enhancements);
}

RelationSelection::RelationSelection(std::allocator_arg_t,
                                     Allocator a,
                                     std::string_view name,
                                     const SelectQuery& query)
    : RelationSelection(std::allocator_arg, a, name, QuerySelection(std::allocator_arg, a, query)) {
}

RelationSelection::RelationSelection(std::allocator_arg_t,
                                     Allocator a,
                                     const RelationSelection& other)
    : name(other.name, a.resource()), query(std::allocator_arg, a, other.query),
      relation_selections(a.resource()), relation_filters(a.resource()),
      child_enhancements(a.resource()), query_options(std::allocator_arg, a, other.query_options) {
    copy_items(relation_selections, other.relation_selections);
    copy_items(relation_filters, other.relation_filters);
    copy_items(child_enhancements, other.child_enhancements);
}

RelationFilter::RelationFilter(std::allocator_arg_t,
                               Allocator a,
                               std::string_view name,
                               const QuerySelection& selection)
    : name(name, a.resource()), query(std::allocator_arg, a, selection.query),
      relation_selections(a.resource()), relation_filters(a.resource()),
      child_enhancements(a.resource()),
      query_options(std::allocator_arg, a, selection.query_options) {
    copy_items(relation_selections, selection.relation_selections);
    copy_items(relation_filters, selection.relation_filters);
    copy_items(child_enhancements, selection.child_enhancements);
}

RelationFilter::RelationFilter(std::allocator_arg_t,
                               Allocator a,
                               std::string_view name,
                               const SelectQuery& query)
    : RelationFilter(std::allocator_arg, a, name, QuerySelection(std::allocator_arg, a, query)) {}

RelationFilter::RelationFilter(std::allocator_arg_t, Allocator a, const RelationFilter& other)
    : name(other.name, a.resource()), query(std::allocator_arg, a, other.query),
      relation_selections(a.resource()), relation_filters(a.resource()),
      child_enhancements(a.resource()), query_options(std::allocator_arg, a, other.query_options) {
    copy_items(relation_selections, other.relation_selections);
    copy_items(relation_filters, other.relation_filters);
    copy_items(child_enhancements, other.child_enhancements);
}

RawDynamicProperty::RawDynamicProperty(std::allocator_arg_t,
                                       Allocator a,
                                       std::string_view name,
                                       UnsafeRawSqlSegment raw)
    : property_name(name, a.resource()), raw_sql_segment(std::move(raw).into_sql(), a.resource()) {}

RawDynamicProperty::RawDynamicProperty(std::allocator_arg_t,
                                       Allocator a,
                                       const RawDynamicProperty& other)
    : property_name(other.property_name, a.resource()),
      raw_sql_segment(other.raw_sql_segment, a.resource()) {}

RawProjection::RawProjection(std::allocator_arg_t,
                             Allocator a,
                             std::string_view name,
                             UnsafeRawSqlSegment raw)
    : property_name(name, a.resource()), raw_sql_segment(std::move(raw).into_sql(), a.resource()) {}

RawProjection::RawProjection(std::allocator_arg_t, Allocator a, const RawProjection& other)
    : property_name(other.property_name, a.resource()),
      raw_sql_segment(other.raw_sql_segment, a.resource()) {}

RelationAggregate::RelationAggregate(std::allocator_arg_t,
                                     Allocator a,
                                     std::string_view first,
                                     std::string_view second,
                                     const QuerySelection& query,
                                     bool flag)
    : relation_name(first, a.resource()), alias(second, a.resource()),
      query(std::allocator_arg, a, query), single_result(flag) {}

RelationAggregate::RelationAggregate(std::allocator_arg_t,
                                     Allocator a,
                                     std::string_view first,
                                     std::string_view second,
                                     const SelectQuery& query,
                                     bool flag)
    : RelationAggregate(std::allocator_arg,
                        a,
                        first,
                        second,
                        QuerySelection(std::allocator_arg, a, query),
                        flag) {}

RelationAggregate::RelationAggregate(std::allocator_arg_t,
                                     Allocator a,
                                     const RelationAggregate& other)
    : RelationAggregate(std::allocator_arg,
                        a,
                        other.relation_name,
                        other.alias,
                        other.query,
                        other.single_result) {}

FacetRequest::FacetRequest(std::allocator_arg_t,
                           Allocator a,
                           std::string_view first,
                           std::string_view second,
                           const QuerySelection& query,
                           bool flag)
    : facet_name(first, a.resource()), relation_name(second, a.resource()),
      query(std::allocator_arg, a, query), include_all_facets(flag) {}

FacetRequest::FacetRequest(std::allocator_arg_t,
                           Allocator a,
                           std::string_view first,
                           std::string_view second,
                           const SelectQuery& query,
                           bool flag)
    : FacetRequest(std::allocator_arg,
                   a,
                   first,
                   second,
                   QuerySelection(std::allocator_arg, a, query),
                   flag) {}

FacetRequest::FacetRequest(std::allocator_arg_t, Allocator a, const FacetRequest& other)
    : FacetRequest(std::allocator_arg,
                   a,
                   other.facet_name,
                   other.relation_name,
                   other.query,
                   other.include_all_facets) {}

ObjectGroupBy::ObjectGroupBy(std::allocator_arg_t,
                             Allocator a,
                             std::string_view first,
                             std::string_view second,
                             const QuerySelection& query)
    : property_name(first, a.resource()), storage_field(second, a.resource()),
      query(std::allocator_arg, a, query) {}

ObjectGroupBy::ObjectGroupBy(std::allocator_arg_t,
                             Allocator a,
                             std::string_view first,
                             std::string_view second,
                             const SelectQuery& query)
    : ObjectGroupBy(
          std::allocator_arg, a, first, second, QuerySelection(std::allocator_arg, a, query)) {}

ObjectGroupBy::ObjectGroupBy(std::allocator_arg_t, Allocator a, const ObjectGroupBy& other)
    : ObjectGroupBy(std::allocator_arg, a, other.property_name, other.storage_field, other.query) {}

SelectQuery QuerySelection::into_query() && {
    auto selected = apply_relation_selections(std::move(query), relation_selections);
    return apply_runtime_metadata(std::move(selected), query_options, child_enhancements);
}

SelectQuery RelationSelection::into_query() && {
    auto selected = apply_relation_selections(std::move(query), relation_selections);
    return apply_runtime_metadata(std::move(selected), query_options, child_enhancements);
}

SelectQuery apply_relation_selections(SelectQuery query,
                                      std::span<const RelationSelection> selections) {
    auto a = Allocator(query.memory_resource());
    for (const auto& selection : selections) {
        auto copy = RelationSelection(std::allocator_arg, a, selection);
        auto nested = std::move(copy).into_query();
        query.relation_query(selection.name, nested);
    }
    return query;
}

SelectQuery apply_runtime_metadata(SelectQuery query,
                                   const QueryOptions& options,
                                   std::span<const QuerySelection> children) {
    auto r = query.memory_resource();
    auto a = Allocator(r);
    if (options.comment) {
        query.comment(*options.comment);
    }
    query.raw_sql_text.reset();
    if (options.raw_sql) {
        query.raw_sql_text.emplace(*options.raw_sql, r);
    }
    query.raw_sql_search_criteria.clear();
    copy_items(query.raw_sql_search_criteria, options.raw_sql_search_criteria);
    query.dynamic_properties.clear();
    for (const auto& p : options.dynamic_properties) {
        query.dynamic_properties.emplace_back(p.property_name, p.raw_sql_segment);
    }
    query.raw_projections.clear();
    for (const auto& p : options.raw_projections) {
        query.raw_projections.emplace_back(p.property_name, p.raw_sql_segment);
    }
    query.object_group_bys.clear();
    for (const auto& group : options.object_group_bys) {
        auto copy = QuerySelection(std::allocator_arg, a, group.query);
        query.object_group_bys.emplace_back(
            group.property_name, group.storage_field, std::move(copy).into_query());
    }
    query.child_enhancements.clear();
    for (const auto& child : children) {
        auto copy = QuerySelection(std::allocator_arg, a, child);
        query.child_enhancements.emplace_back(std::move(copy).into_query());
    }
    return query;
}

Vector<teaql::RelationAggregate> runtime_relation_aggregates(Memory r,
                                                             const QueryOptions& options) {
    Vector<teaql::RelationAggregate> output(r);
    for (const auto& aggregate : options.relation_aggregates) {
        auto copy = QuerySelection(std::allocator_arg, Allocator(r), aggregate.query);
        output.emplace_back(aggregate.relation_name,
                            aggregate.alias,
                            std::move(copy).into_query(),
                            aggregate.single_result);
    }
    return output;
}

void merge_outer_filter_into_facet_aggregates(QuerySelection& selection, const SelectQuery& outer) {
    if (!outer.predicate) {
        return;
    }
    for (auto& aggregate : selection.query_options.relation_aggregates) {
        if (aggregate.query.query.entity == outer.entity) {
            aggregate.query.query.and_filter(*outer.predicate);
        }
    }
}

Value required_value(Memory r, FieldOperator op, std::span<const Value> values, std::size_t index) {
    if (index >= values.size()) {
        auto message = debug_string(r, op);
        message += " requires value at index ";
        message += std::to_string(index);
        throw std::invalid_argument(std::string(message));
    }
    return Value(r, values[index]);
}

String required_text(Memory r, FieldOperator op, std::span<const Value> values, std::size_t index) {
    auto value = required_value(r, op, values, index);
    if (auto text = value.try_text()) {
        return String(*text, r);
    }
    auto message = debug_string(r, op);
    message += " requires text value, got ";
    message += debug_string(r, value);
    throw std::invalid_argument(std::string(message));
}

Expr field_operator_expr(Memory r,
                         std::string_view field,
                         FieldOperator op,
                         std::span<const Value> values) {
    switch (op) {
    case FieldOperator::Equal:
        return Expr::eq(r, field, required_value(r, op, values, 0));
    case FieldOperator::NotEqual:
        return Expr::ne(r, field, required_value(r, op, values, 0));
    case FieldOperator::GreaterThan:
        return Expr::gt(r, field, required_value(r, op, values, 0));
    case FieldOperator::GreaterThanOrEqual:
        return Expr::gte(r, field, required_value(r, op, values, 0));
    case FieldOperator::LessThan:
        return Expr::lt(r, field, required_value(r, op, values, 0));
    case FieldOperator::LessThanOrEqual:
        return Expr::lte(r, field, required_value(r, op, values, 0));
    case FieldOperator::Between: {
        auto lower = required_value(r, op, values, 0);
        auto upper = required_value(r, op, values, 1);
        return Expr::between(r, field, lower, upper);
    }
    case FieldOperator::In:
        return Expr::in_list(r, field, values);
    case FieldOperator::NotIn:
        return Expr::not_in_list(r, field, values);
    case FieldOperator::Contain:
        return Expr::contain(r, field, required_text(r, op, values, 0));
    case FieldOperator::NotContain:
        return Expr::not_contain(r, field, required_text(r, op, values, 0));
    case FieldOperator::BeginWith:
        return Expr::begin_with(r, field, required_text(r, op, values, 0));
    case FieldOperator::NotBeginWith:
        return Expr::not_begin_with(r, field, required_text(r, op, values, 0));
    case FieldOperator::EndWith:
        return Expr::end_with(r, field, required_text(r, op, values, 0));
    case FieldOperator::NotEndWith:
        return Expr::not_end_with(r, field, required_text(r, op, values, 0));
    case FieldOperator::SoundsLike:
        return Expr::sound_like(r, field, required_value(r, op, values, 0));
    case FieldOperator::IsNull:
        return Expr::is_null(r, field);
    case FieldOperator::IsNotNull:
        return Expr::is_not_null(r, field);
    }
    throw std::invalid_argument("Invalid field operator");
}

Expr field_operator_column_expr(Memory r,
                                std::string_view field,
                                FieldOperator op,
                                std::string_view other) {
    BinaryOp binary;
    switch (op) {
    case FieldOperator::Equal:
        binary = BinaryOp::Eq;
        break;
    case FieldOperator::NotEqual:
        binary = BinaryOp::Ne;
        break;
    case FieldOperator::GreaterThan:
        binary = BinaryOp::Gt;
        break;
    case FieldOperator::GreaterThanOrEqual:
        binary = BinaryOp::Gte;
        break;
    case FieldOperator::LessThan:
        binary = BinaryOp::Lt;
        break;
    case FieldOperator::LessThanOrEqual:
        binary = BinaryOp::Lte;
        break;
    case FieldOperator::Contain:
    case FieldOperator::BeginWith:
    case FieldOperator::EndWith:
        binary = BinaryOp::Like;
        break;
    case FieldOperator::NotContain:
    case FieldOperator::NotBeginWith:
    case FieldOperator::NotEndWith:
        binary = BinaryOp::NotLike;
        break;
    default:
        auto message = debug_string(r, op);
        message += " is not supported for property-to-property filters";
        throw std::invalid_argument(std::string(message));
    }
    return Expr::compare_columns(r, field, binary, other);
}

std::optional<Expr> remove_filter_expr(Memory r, const Expr& filter, const Expr& target) {
    if (filter == target) {
        return {};
    }
    if (filter.kind != Expr::Kind::And) {
        return filter.clone(r);
    }
    Expr retained(r, Expr::Kind::And);
    for (const auto& part : filter.args) {
        if (auto value = remove_filter_expr(r, part, target)) {
            retained.args.emplace_back(std::move(*value));
        }
    }
    if (retained.args.empty()) {
        return {};
    }
    if (retained.args.size() == 1) {
        return std::move(retained.args.front());
    }
    return retained;
}

std::optional<Expr> remove_default_live_filter(Memory r, const std::optional<Expr>& filter) {
    if (!filter) {
        return {};
    }
    return remove_filter_expr(r, *filter, Expr::gt(r, "version", std::int64_t(0)));
}

Value dynamic_json_value_to_teaql_value(Memory r, const Json& input) {
    return std::visit(
        [&](const auto& item) -> Value {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::same_as<T, std::monostate>) {
                return Value(r);
            } else if constexpr (std::same_as<T, std::uint64_t>) {
                return item <= std::uint64_t(INT64_MAX) ? Value(r, std::int64_t(item))
                                                        : Value(r, item);
            } else if constexpr (std::same_as<T, String>) {
                auto view = std::string_view(item);
                detail::trim(view);
                return Value(r, view);
            } else if constexpr (std::same_as<T, Json::Array>) {
                Value::List values(r);
                for (const auto& v : item) {
                    values.emplace_back(dynamic_json_value_to_teaql_value(r, v));
                }
                return Value(r, values);
            } else if constexpr (std::same_as<T, Json::Object>) {
                auto it = item.find("id");
                return it == item.end() ? Value(r)
                                        : dynamic_json_value_to_teaql_value(r, it->second);
            } else {
                return Value(r, item);
            }
        },
        input.data);
}

Vector<Value> dynamic_json_values(Memory r, const Json& value) {
    Vector<Value> values(r);
    if (auto items = std::get_if<Json::Array>(&value.data)) {
        for (const auto& v : *items) {
            values.emplace_back(dynamic_json_value_to_teaql_value(r, v));
        }
    } else {
        values.emplace_back(dynamic_json_value_to_teaql_value(r, value));
    }
    return values;
}

FieldOperator dynamic_json_operator(const Json& value) {
    if (auto text = std::get_if<String>(&value.data)) {
        auto equals = [&](std::string_view expected) {
            if (text->size() != expected.size()) {
                return false;
            }
            for (std::size_t i = 0; i < expected.size(); ++i) {
                auto c = (*text)[i];
                if (c >= 'A' && c <= 'Z') {
                    c = char(c - 'A' + 'a');
                }
                if (c != expected[i]) {
                    return false;
                }
            }
            return true;
        };
        if (equals("__is_null__")) {
            return FieldOperator::IsNull;
        }
        if (equals("__is_not_null__")) {
            return FieldOperator::IsNotNull;
        }
        return FieldOperator::Contain;
    }
    if (auto values = std::get_if<Json::Array>(&value.data)) {
        if (!values->empty() && (std::holds_alternative<String>(values->front().data) ||
                                 std::holds_alternative<Json::Object>(values->front().data))) {
            return FieldOperator::In;
        }
        if (values->size() == 2) {
            return FieldOperator::Between;
        }
    }
    return FieldOperator::Equal;
}

Expr dynamic_json_filter_expr(Memory r, std::string_view field, const Json& value) {
    auto values = dynamic_json_values(r, value);
    return field_operator_expr(r, field, dynamic_json_operator(value), values);
}

std::optional<std::uint64_t> dynamic_json_u64_field(const Json::Object& object,
                                                    std::string_view name) {
    auto it = object.find(name);
    if (it == object.end()) {
        return {};
    }
    if (auto n = std::get_if<std::uint64_t>(&it->second.data)) {
        return *n;
    }
    if (auto n = std::get_if<std::int64_t>(&it->second.data); n && *n >= 0) {
        return std::uint64_t(*n);
    }
    return {};
}
}
