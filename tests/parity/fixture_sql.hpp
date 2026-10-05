#pragma once
#include <teaql/sql/dialect.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
using namespace teaql;

static std::uint64_t integer(const Json& v) {
    return std::get<std::uint64_t>(v.data);
}

static std::optional<std::uint32_t> uint_option(const Json& j, std::string_view key) {
    auto p = optional(j, key);
    return p && !std::holds_alternative<std::monostate>(p->data)
               ? std::optional<std::uint32_t>(integer(*p))
               : std::nullopt;
}

static std::span<const Json> items(const Json& j, std::string_view key) {
    auto p = optional(j, key);
    return p ? std::span<const Json>(std::get<Json::Array>(p->data)) : std::span<const Json>{};
}

class FixtureDialect : public SqlDialect {
public:
    bool numbered = false;

    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory r, std::string_view name) const override {
        return quote_identifier_if_needed(r, name, '"');
    }

    String placeholder(Memory r, std::size_t i) const override {
        String out("?", r);
        if (numbered) {
            char b[32];
            auto [end, ec] = std::to_chars(b, b + sizeof(b), i);
            (void)ec;
            out.append(b, end);
        }
        return out;
    }
};

static EntityDescriptor entity(Memory r, const Json& input) {
    EntityDescriptor e(std::allocator_arg, Allocator(r), text(field(input, "name")));
    if (auto p = optional(input, "table"))
        e.table(text(*p));
    if (auto p = optional(input, "declared"))
        e.audit_mask_fields_declared = std::get<bool>(p->data);
    for (const auto& mask : items(input, "masked"))
        e.audit_mask_fields.emplace_back(text(mask));
    for (const auto& item : items(input, "properties")) {
        auto& p = e.property(text(field(item, "name")), DataType(integer(field(item, "type"))));
        if (auto v = optional(item, "column"))
            p.column(text(*v));
        if (auto v = optional(item, "nullable"))
            p.nullable = std::get<bool>(v->data);
        if (auto v = optional(item, "id"))
            p.is_id = std::get<bool>(v->data);
        if (auto v = optional(item, "version"))
            p.is_version = std::get<bool>(v->data);
        p.max_length = uint_option(item, "length");
        p.numeric_precision = uint_option(item, "precision");
        p.numeric_scale = uint_option(item, "scale");
    }
    return e;
}

static SelectQuery query(Memory, const Json&);

static Expr expression(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    if (kind == "column")
        return Expr::column(r, text(field(input, "name")));
    if (kind == "value")
        return Expr::value(r, value(r, field(input, "value")));
    if (kind == "binary")
        return Expr::binary(expression(r, field(input, "left")),
                            BinaryOp(integer(field(input, "op"))),
                            expression(r, field(input, "right")));
    Expr out(r,
             kind == "function"   ? Expr::Kind::Function
             : kind == "subquery" ? Expr::Kind::SubQuery
             : kind == "between"  ? Expr::Kind::Between
             : kind == "null"     ? Expr::Kind::IsNull
             : kind == "not_null" ? Expr::Kind::IsNotNull
             : kind == "not"      ? Expr::Kind::Not
             : kind == "and"      ? Expr::Kind::And
                                  : Expr::Kind::Or);
    if (kind == "function")
        out.function_kind = ExprFunction(integer(field(input, "function")));
    if (kind == "subquery") {
        out.op = BinaryOp(integer(field(input, "op")));
        out.args.emplace_back(expression(r, field(input, "left")));
        auto e = entity(r, field(input, "entity"));
        auto q = query(r, field(input, "query"));
        out.subquery_data = detail::make_box<SubQueryExpr>(r, e, q);
    } else
        for (const auto& arg : items(input, "args"))
            out.args.emplace_back(expression(r, arg));
    return out;
}

static SelectQuery query(Memory r, const Json& input) {
    SelectQuery q(std::allocator_arg, Allocator(r), "FixtureQuery");
    for (const auto& name : items(input, "projection"))
        q.project(text(name));
    for (const auto& p : items(input, "expr_projection"))
        q.project_expr(text(field(p, "alias")), expression(r, field(p, "expr")));
    for (const auto& name : items(input, "group"))
        q.group_by(text(name));
    for (const auto& p : items(input, "raw_projection"))
        q.project_raw(text(field(p, "alias")), text(field(p, "sql")));
    for (const auto& p : items(input, "dynamic"))
        q.dynamic_property_raw(text(field(p, "alias")), text(field(p, "sql")));
    for (const auto& p : items(input, "raw_criteria"))
        q.raw_sql_search_criteria.emplace_back(text(p));
    for (const auto& a : items(input, "aggregates"))
        q.aggregates.emplace_back(AggregateFunction(integer(field(a, "function"))),
                                  text(field(a, "field")),
                                  text(field(a, "alias")));
    for (const auto& order : items(input, "order")) {
        auto d = SortDirection(integer(field(order, "direction")));
        if (auto p = optional(order, "expr"))
            q.order_by.emplace_back(expression(r, *p), d);
        else
            q.order_by.emplace_back(text(field(order, "field")), d);
    }
    if (auto p = optional(input, "filter"))
        q.predicate.emplace(expression(r, *p));
    if (auto p = optional(input, "having"))
        q.having_predicate.emplace(expression(r, *p));
    if (auto p = optional(input, "search"))
        q.search_text.emplace(text(*p), r);
    if (auto p = optional(input, "partition"))
        q.partition_field.emplace(text(*p), r);
    if (auto p = optional(input, "raw_sql"))
        q.raw_sql_text.emplace(text(*p), r);
    if (auto p = optional(input, "comment"))
        q.comment_text.emplace(text(*p), r);
    if (auto p = optional(input, "slice")) {
        q.slice.emplace();
        q.slice->offset = integer(field(*p, "offset"));
        if (auto v = optional(*p, "limit"))
            q.slice->limit = integer(*v);
    }
    return q;
}
