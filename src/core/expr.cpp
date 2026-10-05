#include <teaql/core/query.hpp>
#include <algorithm>

namespace teaql {
Expr::Expr(Memory r, Kind kind) : kind(kind), name(r), literal(r), args(r) {}

Expr::Expr(std::allocator_arg_t, Allocator a, Kind kind) : Expr(a.resource(), kind) {}

Expr::Expr(std::allocator_arg_t, Allocator a, const Expr& other) : Expr(a.resource(), other.kind) {
    name = other.name;
    literal = Value(a.resource(), other.literal);
    op = other.op;
    function_kind = other.function_kind;
    for (const auto& arg : other.args) {
        args.emplace_back(arg);
    }
    if (other.subquery_data) {
        subquery_data = detail::make_box<SubQueryExpr>(a.resource(), *other.subquery_data);
    }
}

Expr::Expr(std::allocator_arg_t, Allocator a, Expr&& other)
    : Expr(detail::move_to(a.resource(), std::move(other))) {}

Expr::Expr(Expr&&) noexcept = default;

Expr& Expr::operator=(Expr&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

Expr::~Expr() = default;

Expr Expr::clone(Memory r) const {
    return Expr(std::allocator_arg, Allocator(r), *this);
}

Expr Expr::column(Memory r, std::string_view name) {
    Expr result(r, Kind::Column);
    result.name = name;
    return result;
}

Expr Expr::function(Memory r, ExprFunction fn, std::span<const Expr> parts) {
    Expr result(r, Kind::Function);
    result.function_kind = fn;
    for (const auto& part : parts) {
        result.args.emplace_back(part);
    }
    return result;
}

Expr Expr::count_all(Memory r) {
    return function(r, ExprFunction::Count, {});
}

Expr Expr::binary(Expr left, BinaryOp op, Expr right) {
    Expr result(left.memory_resource(), Kind::Binary);
    result.op = op;
    result.args.emplace_back(std::move(left));
    result.args.emplace_back(std::move(right));
    return result;
}

Expr Expr::compare_columns(Memory r, std::string_view left, BinaryOp op, std::string_view right) {
    return binary(column(r, left), op, column(r, right));
}

namespace {
Expr list_comparison(Memory r, std::string_view field, std::span<const Value> values, BinaryOp op) {
    Value::List copy(r);
    for (const auto& v : values) {
        copy.emplace_back(v);
    }
    return Expr::binary(Expr::column(r, field), op, Expr::value(r, copy));
}

Expr string_match(Memory r,
                  std::string_view field,
                  std::string_view text,
                  bool prefix,
                  bool suffix,
                  bool negative) {
    String pattern(r);
    if (prefix) {
        pattern += '%';
    }
    pattern += text;
    if (suffix) {
        pattern += '%';
    }
    return Expr::binary(Expr::column(r, field),
                        negative ? BinaryOp::NotLike : BinaryOp::Like,
                        Expr::value(r, pattern));
}

Expr logical(Memory r, Expr::Kind kind, std::span<const Expr> parts) {
    Expr result(r, kind);
    for (const auto& part : parts) {
        if (std::find(result.args.begin(), result.args.end(), part) == result.args.end()) {
            result.args.emplace_back(part);
        }
    }
    return result;
}

Expr join(const Expr& left, const Expr& right, Expr::Kind kind) {
    if (left == right) {
        return left.clone(left.memory_resource());
    }
    Expr result(left.memory_resource(), kind);
    if (left.kind == kind) {
        for (const auto& arg : left.args) {
            result.args.emplace_back(arg);
        }
    } else {
        result.args.emplace_back(left);
    }
    if (std::find(result.args.begin(), result.args.end(), right) == result.args.end()) {
        result.args.emplace_back(right);
    }
    return result;
}
}

Expr Expr::in_list(Memory r,
                   std::string_view field,
                   std::span<const teaql::Value> values,
                   bool negative) {
    const bool large = values.size() > LARGE_IN_THRESHOLD;
    return list_comparison(r,
                           field,
                           values,
                           negative ? (large ? BinaryOp::NotInLarge : BinaryOp::NotIn)
                                    : (large ? BinaryOp::InLarge : BinaryOp::In));
}

Expr Expr::in_large(Memory r, std::string_view field, std::span<const teaql::Value> values) {
    return list_comparison(r, field, values, BinaryOp::InLarge);
}

Expr Expr::not_in_large(Memory r, std::string_view field, std::span<const teaql::Value> values) {
    return list_comparison(r, field, values, BinaryOp::NotInLarge);
}

Expr Expr::subquery(Expr left,
                    BinaryOp op,
                    const EntityDescriptor& entity,
                    const SelectQuery& query,
                    std::string_view field) {
    auto r = left.memory_resource();
    Expr result(r, Kind::SubQuery);
    result.op = op;
    result.args.emplace_back(std::move(left));
    result.subquery_data = detail::make_box<SubQueryExpr>(r, entity, query);
    auto& inner = result.subquery_data->query;
    if (op == BinaryOp::NotIn || op == BinaryOp::NotInLarge) {
        inner.and_filter(is_not_null(r, field));
    }
    inner.projection.clear();
    inner.project(field);
    return result;
}

Expr Expr::in_subquery(Memory r,
                       std::string_view column_name,
                       const EntityDescriptor& entity,
                       const SelectQuery& query,
                       std::string_view field) {
    return subquery(column(r, column_name), BinaryOp::In, entity, query, field);
}

Expr Expr::not_in_subquery(Memory r,
                           std::string_view column_name,
                           const EntityDescriptor& entity,
                           const SelectQuery& query,
                           std::string_view field) {
    return subquery(column(r, column_name), BinaryOp::NotIn, entity, query, field);
}

Expr Expr::between(Memory r,
                   std::string_view field,
                   const teaql::Value& lower,
                   const teaql::Value& upper) {
    Expr result(r, Kind::Between);
    result.args.emplace_back(column(r, field));
    result.args.emplace_back(value(r, lower));
    result.args.emplace_back(value(r, upper));
    return result;
}

Expr Expr::is_null(Memory r, std::string_view field) {
    Expr result(r, Kind::IsNull);
    result.args.emplace_back(column(r, field));
    return result;
}

Expr Expr::is_not_null(Memory r, std::string_view field) {
    auto result = is_null(r, field);
    result.kind = Kind::IsNotNull;
    return result;
}

Expr Expr::all_of(Memory r, std::span<const Expr> parts) {
    return logical(r, Kind::And, parts);
}

Expr Expr::any_of(Memory r, std::span<const Expr> parts) {
    return logical(r, Kind::Or, parts);
}

Expr Expr::negate(const Expr& expr) {
    Expr result(expr.memory_resource(), Kind::Not);
    result.args.emplace_back(expr);
    return result;
}

Expr Expr::and_expr(const Expr& other) const {
    return join(*this, other, Kind::And);
}

Expr Expr::or_expr(const Expr& other) const {
    return join(*this, other, Kind::Or);
}

bool Expr::operator==(const Expr& other) const {
    if (kind != other.kind) {
        return false;
    }
    switch (kind) {
    case Kind::Column:
        return name == other.name;
    case Kind::Value:
        return literal == other.literal;
    case Kind::Function:
        return function_kind == other.function_kind && args == other.args;
    case Kind::Binary:
        return op == other.op && args == other.args;
    case Kind::SubQuery:
        return op == other.op && args == other.args &&
               bool(subquery_data) == bool(other.subquery_data) &&
               (!subquery_data || *subquery_data == *other.subquery_data);
    default:
        return args == other.args;
    }
}

Expr Expr::soundex(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::Soundex, {&expr, 1});
}

Expr Expr::gbk(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::Gbk, {&expr, 1});
}

Expr Expr::count_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::Count, {&expr, 1});
}

Expr Expr::sum_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::Sum, {&expr, 1});
}

Expr Expr::avg_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::Avg, {&expr, 1});
}

Expr Expr::min_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::Min, {&expr, 1});
}

Expr Expr::max_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::Max, {&expr, 1});
}

Expr Expr::stddev_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::Stddev, {&expr, 1});
}

Expr Expr::stddev_pop_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::StddevPop, {&expr, 1});
}

Expr Expr::var_samp_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::VarSamp, {&expr, 1});
}

Expr Expr::var_pop_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::VarPop, {&expr, 1});
}

Expr Expr::bit_and_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::BitAnd, {&expr, 1});
}

Expr Expr::bit_or_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::BitOr, {&expr, 1});
}

Expr Expr::bit_xor_expr(const Expr& expr) {
    return function(expr.memory_resource(), ExprFunction::BitXor, {&expr, 1});
}

Expr Expr::like(Memory r, std::string_view field, std::string_view text) {
    return string_match(r, field, text, false, false, false);
}

Expr Expr::not_like(Memory r, std::string_view field, std::string_view text) {
    return string_match(r, field, text, false, false, true);
}

Expr Expr::contain(Memory r, std::string_view field, std::string_view text) {
    return string_match(r, field, text, true, true, false);
}

Expr Expr::not_contain(Memory r, std::string_view field, std::string_view text) {
    return string_match(r, field, text, true, true, true);
}

Expr Expr::begin_with(Memory r, std::string_view field, std::string_view text) {
    return string_match(r, field, text, false, true, false);
}

Expr Expr::not_begin_with(Memory r, std::string_view field, std::string_view text) {
    return string_match(r, field, text, false, true, true);
}

Expr Expr::end_with(Memory r, std::string_view field, std::string_view text) {
    return string_match(r, field, text, true, false, false);
}

Expr Expr::not_end_with(Memory r, std::string_view field, std::string_view text) {
    return string_match(r, field, text, true, false, true);
}
}
