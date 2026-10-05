#include <teaql/core/detail/query_debug.hpp>
#include <teaql/core/detail/debug.hpp>
#include <charconv>

namespace teaql::detail {
namespace {
struct Debug {
    String out;

    explicit Debug(Memory r) : out(r) {}

    void text(std::string_view s) {
        out += debug_text(out.get_allocator().resource(), s);
    }

    void value(const Value& v) {
        out += debug_value(out.get_allocator().resource(), v);
    }

    void boolean(bool b) {
        out += b ? "true" : "false";
    }

    template <class T>
    void number(T v) {
        char b[32];
        auto [end, ec] = std::to_chars(b, b + sizeof(b), v);
        if (ec != std::errc{}) {
            throw std::overflow_error("query Debug number");
        }
        out.append(b, end);
    }

    template <class T, class F>
    void option(const T& v, F f) {
        if (v) {
            out += "Some(";
            f(*v);
            out += ')';
        } else {
            out += "None";
        }
    }

    template <class T>
    void optional_number(const T& v) {
        option(v, [&](auto n) {
            number(n);
        });
    }

    template <class T>
    void optional_text(const T& v) {
        option(v, [&](auto& s) {
            text(s);
        });
    }

    template <class T, class F>
    void list(const T& v, F f) {
        out += '[';
        bool first = true;
        for (const auto& item : v) {
            if (!first) {
                out += ", ";
            }
            first = false;
            f(item);
        }
        out += ']';
    }

    void strings(const Vector<String>& v) {
        list(v, [&](auto& s) {
            text(s);
        });
    }

    static std::string_view op(BinaryOp v) {
        constexpr std::string_view names[]{"Eq",
                                           "Ne",
                                           "Gt",
                                           "Gte",
                                           "Lt",
                                           "Lte",
                                           "Like",
                                           "NotLike",
                                           "In",
                                           "NotIn",
                                           "InLarge",
                                           "NotInLarge"};
        return names[unsigned(v)];
    }

    static std::string_view function(ExprFunction v) {
        constexpr std::string_view names[]{"Soundex",
                                           "Gbk",
                                           "Count",
                                           "Sum",
                                           "Avg",
                                           "Min",
                                           "Max",
                                           "Stddev",
                                           "StddevPop",
                                           "VarSamp",
                                           "VarPop",
                                           "BitAnd",
                                           "BitOr",
                                           "BitXor"};
        return names[unsigned(v)];
    }

    void expr(const Expr& e) {
        switch (e.kind) {
        case Expr::Kind::Column:
            out += "Column(";
            text(e.name);
            out += ')';
            break;
        case Expr::Kind::Value:
            out += "Value(";
            value(e.literal);
            out += ')';
            break;
        case Expr::Kind::Function:
            out += "Function { function: ";
            out += function(e.function_kind);
            out += ", args: ";
            list(e.args, [&](auto& a) {
                expr(a);
            });
            out += " }";
            break;
        case Expr::Kind::Binary:
        case Expr::Kind::SubQuery:
            out += e.kind == Expr::Kind::Binary ? "Binary { left: " : "SubQuery { left: ";
            expr(e.args.at(0));
            out += ", op: ";
            out += op(e.op);
            if (e.kind == Expr::Kind::Binary) {
                out += ", right: ";
                expr(e.args.at(1));
            } else {
                out += ", entity: ";
                entity(e.subquery_data->entity);
                out += ", query: ";
                query(e.subquery_data->query);
            }
            out += " }";
            break;
        case Expr::Kind::Between:
            out += "Between { expr: ";
            expr(e.args.at(0));
            out += ", lower: ";
            expr(e.args.at(1));
            out += ", upper: ";
            expr(e.args.at(2));
            out += " }";
            break;
        case Expr::Kind::IsNull:
        case Expr::Kind::IsNotNull:
        case Expr::Kind::Not:
            out += e.kind == Expr::Kind::IsNull      ? "IsNull("
                   : e.kind == Expr::Kind::IsNotNull ? "IsNotNull("
                                                     : "Not(";
            expr(e.args.at(0));
            out += ')';
            break;
        case Expr::Kind::And:
        case Expr::Kind::Or:
            out += e.kind == Expr::Kind::And ? "And(" : "Or(";
            list(e.args, [&](auto& a) {
                expr(a);
            });
            out += ')';
            break;
        }
    }

    void entity(const EntityDescriptor& e) {
        out += "EntityDescriptor { name: ";
        text(e.name);
        out += ", table_name: ";
        text(e.table_name);
        out += ", data_service: ";
        optional_text(e.data_service);
        out += ", properties: ";
        list(e.properties, [&](const auto& p) {
            out += "PropertyDescriptor { name: ";
            text(p.name);
            out += ", data_type: ";
            constexpr std::string_view types[]{"Bool",
                                               "I64",
                                               "U64",
                                               "F64",
                                               "Decimal",
                                               "Text",
                                               "LargeText",
                                               "Json",
                                               "Date",
                                               "Timestamp"};
            out += types[unsigned(p.data_type)];
            out += ", nullable: ";
            boolean(p.nullable);
            out += ", column_name: ";
            text(p.column_name);
            out += ", is_id: ";
            boolean(p.is_id);
            out += ", is_version: ";
            boolean(p.is_version);
            out += ", max_length: ";
            optional_number(p.max_length);
            out += ", numeric_precision: ";
            optional_number(p.numeric_precision);
            out += ", numeric_scale: ";
            optional_number(p.numeric_scale);
            out += " }";
        });
        out += ", relations: ";
        list(e.relations, [&](const auto& p) {
            out += "RelationDescriptor { name: ";
            text(p.name);
            out += ", target_entity: ";
            text(p.target_entity);
            out += ", local_key: ";
            text(p.local_key);
            out += ", foreign_key: ";
            text(p.foreign_key);
            out += ", many: ";
            boolean(p.many);
            out += ", attach: ";
            boolean(p.attach);
            out += ", delete_missing: ";
            boolean(p.delete_missing);
            out += " }";
        });
        out += ", audit_mask_fields: ";
        strings(e.audit_mask_fields);
        out += ", audit_mask_fields_declared: ";
        boolean(e.audit_mask_fields_declared);
        out += ", audit_value_max_len: ";
        optional_number(e.audit_value_max_len);
        out += " }";
    }

    void raw(const Vector<RawSqlProjection>& v) {
        list(v, [&](auto& p) {
            out += "RawSqlProjection { property_name: ";
            text(p.property_name);
            out += ", raw_sql_segment: ";
            text(p.raw_sql_segment);
            out += " }";
        });
    }

    void query(const SelectQuery& q) {
        out += "SelectQuery { hard_limit: ";
        number(q.hard_limit_value);
        out += ", entity: ";
        text(q.entity);
        out += ", projection: ";
        strings(q.projection);
        out += ", expr_projection: ";
        list(q.expr_projection, [&](auto& p) {
            out += "NamedExpr { alias: ";
            text(p.alias);
            out += ", expr: ";
            expr(p.expr);
            out += " }";
        });
        out += ", search_with_text: ";
        optional_text(q.search_text);
        out += ", filter: ";
        option(q.predicate, [&](auto& e) {
            expr(e);
        });
        out += ", having: ";
        option(q.having_predicate, [&](auto& e) {
            expr(e);
        });
        out += ", order_by: ";
        list(q.order_by, [&](auto& o) {
            out += "OrderBy { field: ";
            text(o.field);
            out += ", expr: ";
            option(o.expr, [&](auto& e) {
                expr(e);
            });
            out += ", direction: ";
            out += o.direction == SortDirection::Asc ? "Asc" : "Desc";
            out += " }";
        });
        out += ", slice: ";
        option(q.slice, [&](auto& s) {
            out += "Slice { limit: ";
            optional_number(s.limit);
            out += ", offset: ";
            number(s.offset);
            out += " }";
        });
        out += ", partition_by: ";
        optional_text(q.partition_field);
        out += ", top_n_probe_parent_threshold: ";
        optional_number(q.top_n_probe_parent_threshold_value);
        out += ", aggregates: ";
        list(q.aggregates, [&](auto& a) {
            out += "Aggregate { function: ";
            out += function(ExprFunction(unsigned(a.function) + 2));
            out += ", field: ";
            text(a.field);
            out += ", alias: ";
            text(a.alias);
            out += " }";
        });
        out += ", group_by: ";
        strings(q.group_by_fields);
        out += ", relations: ";
        list(q.relations, [&](auto& l) {
            out += "RelationLoad { name: ";
            text(l.name);
            out += ", query: ";
            option(l.query, [&](auto& child) {
                query(child);
            });
            out += " }";
        });
        out += ", aggregation_cache: ";
        option(q.aggregation_cache, [&](auto& o) {
            out += "AggregationCacheOptions { enabled: ";
            boolean(o.enabled);
            out += ", cache_expired_millis: ";
            number(o.cache_expired_millis);
            out += ", propagate: ";
            boolean(o.propagate);
            out += ", propagate_cache_expired_millis: ";
            number(o.propagate_cache_expired_millis);
            out += " }";
        });
        out += ", comment: ";
        optional_text(q.comment_text);
        out += ", trace_chain: ";
        list(q.trace_chain, [&](auto& n) {
            out += "TraceNode { kind: ";
            constexpr std::string_view kinds[]{"Operation",
                                               "Request",
                                               "Relation",
                                               "Entity",
                                               "Provider",
                                               "Sql",
                                               "Comment",
                                               "Purpose",
                                               "AuditReason"};
            out += kinds[unsigned(n.kind)];
            out += ", entity_type: ";
            text(n.entity_type);
            out += ", entity_id: ";
            optional_number(n.entity_id);
            out += ", comment: ";
            text(n.comment);
            out += " }";
        });
        out += ", raw_sql: ";
        optional_text(q.raw_sql_text);
        out += ", raw_sql_search_criteria: ";
        strings(q.raw_sql_search_criteria);
        out += ", dynamic_properties: ";
        raw(q.dynamic_properties);
        out += ", raw_projections: ";
        raw(q.raw_projections);
        out += ", object_group_bys: ";
        list(q.object_group_bys, [&](auto& g) {
            out += "ObjectGroupBy { property_name: ";
            text(g.property_name);
            out += ", storage_field: ";
            text(g.storage_field);
            out += ", query: ";
            query(g.query);
            out += " }";
        });
        out += ", child_enhancements: ";
        list(q.child_enhancements, [&](auto& c) {
            query(c);
        });
        out += ", stream_config: ";
        option(q.stream_config, [&](auto& s) {
            out += "StreamConfig { chunk_size: ";
            number(s.chunk_size);
            out += " }";
        });
        out += ", continuous_page_fetch: ";
        option(q.continuous_page_fetch, [&](auto& o) {
            out += "ContinuousPageFetchOptions { namespace: ";
            text(o.namespace_name);
            out += ", ttl_seconds: ";
            number(o.ttl_seconds);
            out += " }";
        });
        out += ", id_set_pagination: ";
        option(q.id_set_pagination, [&](auto& o) {
            out += "IdSetPaginationOptions { namespace: ";
            text(o.namespace_name);
            out += ", ttl_seconds: ";
            number(o.ttl_seconds);
            out += ", max_ids: ";
            number(o.max_ids);
            out += " }";
        });
        out += " }";
    }
};
}

String debug_query(Memory r, const SelectQuery& q) {
    Debug d(r);
    d.query(q);
    return std::move(d.out);
}

String debug_expr(Memory r, const Expr& e) {
    Debug d(r);
    d.expr(e);
    return std::move(d.out);
}
}
