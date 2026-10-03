#include <teaql/core/meta.hpp>
#include <teaql/core/query.hpp>
#include <algorithm>

namespace teaql {
std::optional<std::int64_t> Value::try_i64() const {
    if (auto v = std::get_if<std::int64_t>(&data)) return *v;
    if (auto v = std::get_if<std::uint64_t>(&data); v && *v <= std::uint64_t(INT64_MAX)) return std::int64_t(*v);
    return {};
}
std::optional<std::uint64_t> Value::try_u64() const {
    if (auto v = std::get_if<std::uint64_t>(&data)) return *v;
    if (auto v = std::get_if<std::int64_t>(&data); v && *v >= 0) return std::uint64_t(*v);
    return {};
}
String default_table_name(Memory r, std::string_view name) {
    String out(r);
    for (std::size_t i = 0; i < name.size(); ++i) {
        auto c = name[i];
        if (c >= 'A' && c <= 'Z') { if (i) out += '_'; c = char(c - 'A' + 'a'); }
        out += c;
    }
    out += "_data"; return out;
}
const PropertyDescriptor& EntityDescriptor::property_by_name(std::string_view field) const {
    for (const auto& p : properties) if (p.name == field) return p;
    throw std::invalid_argument("UnknownField");
}
const PropertyDescriptor& EntityDescriptor::id_property() const {
    for (const auto& p : properties) if (p.is_id) return p;
    throw std::invalid_argument("MissingIdProperty");
}
const PropertyDescriptor& EntityDescriptor::version_property() const {
    for (const auto& p : properties) if (p.is_version) return p;
    throw std::invalid_argument("MissingVersionProperty");
}
Expr Expr::clone(Memory r) const {
    Expr copy(r, kind); copy.name = name; copy.literal = Value(r, literal); copy.op = op;
    for (const auto& arg : args) copy.args.push_back(arg.clone(r));
    for (const auto& value : values) copy.values.emplace_back(value);
    return copy;
}
Expr Expr::column(Memory r, std::string_view name) { Expr e(r, Kind::Column); e.name = name; return e; }
Expr Expr::binary(Expr left, BinaryOp op, Expr right) {
    Expr e(left.memory_resource(), Kind::Binary); e.op = op;
    e.args.push_back(std::move(left)); e.args.push_back(right.clone(e.memory_resource())); return e;
}
Expr Expr::in_list(Memory r, std::string_view field, Vector<teaql::Value> values, bool negate) {
    Expr e(r, Kind::Binary); e.args.push_back(column(r, field));
    const bool large = values.size() > LARGE_IN_THRESHOLD;
    e.op = negate ? (large ? BinaryOp::NotInLarge : BinaryOp::NotIn) : (large ? BinaryOp::InLarge : BinaryOp::In);
    for (const auto& v : values) e.values.emplace_back(v);
    return e;
}
Expr Expr::between(Memory r, std::string_view field, const teaql::Value& lower, const teaql::Value& upper) {
    Expr e(r, Kind::Between); e.args.push_back(column(r, field)); e.args.push_back(value(r, lower)); e.args.push_back(value(r, upper)); return e;
}
Expr Expr::is_null(Memory r, std::string_view field) { Expr e(r, Kind::IsNull); e.args.push_back(column(r, field)); return e; }
Expr Expr::is_not_null(Memory r, std::string_view field) { auto e = is_null(r, field); e.kind = Kind::IsNotNull; return e; }
Expr Expr::and_expr(const Expr& other) const {
    if (*this == other) return clone(memory_resource());
    Expr e(memory_resource(), Kind::And);
    if (kind == Kind::And) for (const auto& part : args) e.args.push_back(part.clone(memory_resource()));
    else e.args.push_back(clone(memory_resource()));
    if (std::find(e.args.begin(), e.args.end(), other) == e.args.end()) e.args.push_back(other.clone(memory_resource()));
    return e;
}
bool Expr::operator==(const Expr& other) const { return kind == other.kind && name == other.name && literal == other.literal && op == other.op && args == other.args && values == other.values; }
Slice SelectQuery::prepare_for_list() const {
    auto result = slice.value_or(Slice{});
    if (result.limit && *result.limit > hard_limit_value) throw std::invalid_argument("QUERY_HARD_LIMIT_EXCEEDED");
    if (!result.limit) result.limit = hard_limit_value;
    return result;
}
}
