#pragma once
#include "fixture_sql.hpp"

static EntityDescriptor key_entity(Memory r, const Json& j) {
    auto e = entity(r, j);
    if (auto* v = optional(j, "service"))
        e.service(text(*v));
    if (auto* v = optional(j, "audit_max"))
        e.audit_value_max_len = integer(*v);
    for (const auto& rel : items(j, "relations")) {
        auto& p = e.relation(text(field(rel, "name")), text(field(rel, "target")));
        if (auto* v = optional(rel, "local"))
            p.local_key = text(*v);
        if (auto* v = optional(rel, "foreign"))
            p.foreign_key = text(*v);
        if (auto* v = optional(rel, "many"))
            p.many = std::get<bool>(v->data);
        if (auto* v = optional(rel, "attach"))
            p.attach = std::get<bool>(v->data);
        if (auto* v = optional(rel, "delete_missing"))
            p.delete_missing = std::get<bool>(v->data);
    }
    return e;
}

static SelectQuery key_query(Memory, const Json&);

static Expr key_expression(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    if (kind == "column")
        return Expr::column(r, text(field(input, "name")));
    if (kind == "value")
        return Expr::value(r, value(r, field(input, "value")));
    if (kind == "binary")
        return Expr::binary(key_expression(r, field(input, "left")),
                            BinaryOp(integer(field(input, "op"))),
                            key_expression(r, field(input, "right")));
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
        out.args.emplace_back(key_expression(r, field(input, "left")));
        auto e = key_entity(r, field(input, "entity"));
        auto q = key_query(r, field(input, "query"));
        out.subquery_data = detail::make_box<SubQueryExpr>(r, e, q);
    } else
        for (const auto& arg : items(input, "args"))
            out.args.emplace_back(key_expression(r, arg));
    return out;
}

static SelectQuery key_query(Memory r, const Json& j) {
    auto q = query(r, j);
    if (auto* v = optional(j, "entity"))
        q.entity = text(*v);
    if (auto* v = optional(j, "hard_limit"))
        q.hard_limit_value = integer(*v);
    if (auto* v = optional(j, "threshold"))
        q.top_n_probe_parent_threshold_value = integer(*v);
    if (auto* v = optional(j, "filter"))
        q.predicate.emplace(key_expression(r, *v));
    if (auto* v = optional(j, "having"))
        q.having_predicate.emplace(key_expression(r, *v));
    q.expr_projection.clear();
    for (const auto& p : items(j, "expr_projection"))
        q.project_expr(text(field(p, "alias")), key_expression(r, field(p, "expr")));
    q.order_by.clear();
    for (const auto& o : items(j, "order")) {
        auto d = SortDirection(integer(field(o, "direction")));
        if (auto* v = optional(o, "expr"))
            q.order_by.emplace_back(key_expression(r, *v), d);
        else
            q.order_by.emplace_back(text(field(o, "field")), d);
    }
    for (const auto& v : items(j, "relations")) {
        auto name = text(field(v, "name"));
        if (auto* child = optional(v, "query"))
            q.relation_query(name, key_query(r, *child));
        else
            q.relation(name);
    }
    for (const auto& g : items(j, "object_groups"))
        q.object_group_by(
            text(field(g, "property")), text(field(g, "storage")), key_query(r, field(g, "query")));
    for (const auto& child : items(j, "children"))
        q.child_enhancement(key_query(r, child));
    if (auto* v = optional(j, "cache"))
        q.aggregation_cache = AggregationCacheOptions{std::get<bool>(field(*v, "enabled").data),
                                                      integer(field(*v, "age")),
                                                      std::get<bool>(field(*v, "propagate").data),
                                                      integer(field(*v, "propagate_age"))};
    if (auto* v = optional(j, "stream"))
        q.stream_config = StreamConfig{integer(*v)};
    if (auto* v = optional(j, "continuous"))
        q.continuous_page_fetch.emplace(std::allocator_arg,
                                        Allocator(r),
                                        text(field(*v, "namespace")),
                                        integer(field(*v, "ttl")));
    if (auto* v = optional(j, "ids"))
        q.id_set_pagination.emplace(std::allocator_arg,
                                    Allocator(r),
                                    text(field(*v, "namespace")),
                                    integer(field(*v, "ttl")),
                                    integer(field(*v, "max")));
    for (const auto& n : items(j, "trace")) {
        std::optional<std::uint64_t> id;
        if (auto* v = optional(n, "id"))
            id = integer(*v);
        q.trace_chain.emplace_back(TraceKind(integer(field(n, "kind"))),
                                   text(field(n, "entity")),
                                   text(field(n, "comment")),
                                   id);
    }
    return q;
}
