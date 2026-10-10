#include <teaql/core/query_debug.hpp>
#include <teaql/core/detail/query_debug.hpp>

namespace teaql {
namespace {
template <class T, std::size_t N>
void enum_debug(DebugWriter& writer, T value, const std::string_view (&names)[N]) {
    auto index = static_cast<std::size_t>(value);
    if (index >= N) {
        throw std::invalid_argument("Invalid query diagnostic enum");
    }
    writer.raw(names[index]);
}

template <class T>
void optional_debug(DebugWriter& writer, const T& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_debug(out, *value);
        });
    } else {
        writer.raw("None");
    }
}

template <class T>
void sequence_debug(DebugWriter& writer, const T& value) {
    writer.sequence(value, DefaultDebugElement{});
}
}

void write_debug(DebugWriter& writer, BinaryOp value) {
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
    enum_debug(writer, value, names);
}

void write_debug(DebugWriter& writer, ExprFunction value) {
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
    enum_debug(writer, value, names);
}

void write_debug(DebugWriter& writer, SortDirection value) {
    constexpr std::string_view names[]{"Asc", "Desc"};
    enum_debug(writer, value, names);
}

void write_debug(DebugWriter& writer, AggregateFunction value) {
    constexpr std::string_view names[]{"Count",
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
    enum_debug(writer, value, names);
}

void write_debug(DebugWriter& writer, TraceKind value) {
    constexpr std::string_view names[]{"Operation",
                                       "Request",
                                       "Relation",
                                       "Entity",
                                       "Provider",
                                       "Sql",
                                       "Comment",
                                       "Purpose",
                                       "AuditReason"};
    enum_debug(writer, value, names);
}

void write_debug(DebugWriter& writer, const Expr& value) {
    switch (value.kind) {
    case Expr::Kind::Column:
        writer.tuple("Column", [&](auto& out) {
            out.text(value.name);
        });
        return;
    case Expr::Kind::Value:
        writer.tuple("Value", [&](auto& out) {
            write_debug(out, value.literal);
        });
        return;
    case Expr::Kind::Function:
        writer.structure("Function", [&](auto& fields) {
            fields.field("function", [&](auto& out) {
                write_debug(out, value.function_kind);
            });
            fields.field("args", [&](auto& out) {
                sequence_debug(out, value.args);
            });
        });
        return;
    case Expr::Kind::Binary:
    case Expr::Kind::SubQuery:
        writer.structure(value.kind == Expr::Kind::Binary ? "Binary" : "SubQuery",
                         [&](auto& fields) {
                             fields.field("left", [&](auto& out) {
                                 write_debug(out, value.args.at(0));
                             });
                             fields.field("op", [&](auto& out) {
                                 write_debug(out, value.op);
                             });
                             if (value.kind == Expr::Kind::Binary) {
                                 fields.field("right", [&](auto& out) {
                                     write_debug(out, value.args.at(1));
                                 });
                             } else {
                                 if (!value.subquery_data) {
                                     throw std::invalid_argument(
                                         "SubQuery diagnostics require entity and query data");
                                 }
                                 fields.field("entity", [&](auto& out) {
                                     write_debug(out, value.subquery_data->entity);
                                 });
                                 fields.field("query", [&](auto& out) {
                                     write_debug(out, value.subquery_data->query);
                                 });
                             }
                         });
        return;
    case Expr::Kind::Between:
        writer.structure("Between", [&](auto& fields) {
            fields.field("expr", [&](auto& out) {
                write_debug(out, value.args.at(0));
            });
            fields.field("lower", [&](auto& out) {
                write_debug(out, value.args.at(1));
            });
            fields.field("upper", [&](auto& out) {
                write_debug(out, value.args.at(2));
            });
        });
        return;
    case Expr::Kind::IsNull:
    case Expr::Kind::IsNotNull:
    case Expr::Kind::Not:
        writer.tuple(value.kind == Expr::Kind::IsNull      ? "IsNull"
                     : value.kind == Expr::Kind::IsNotNull ? "IsNotNull"
                                                           : "Not",
                     [&](auto& out) {
                         write_debug(out, value.args.at(0));
                     });
        return;
    case Expr::Kind::And:
    case Expr::Kind::Or:
        writer.tuple(value.kind == Expr::Kind::And ? "And" : "Or", [&](auto& out) {
            sequence_debug(out, value.args);
        });
        return;
    }
    throw std::invalid_argument("Invalid expression diagnostic kind");
}

void write_debug(DebugWriter& writer, const NamedExpr& value) {
    writer.structure("NamedExpr", [&](auto& fields) {
        fields.field("alias", [&](auto& out) {
            write_debug(out, value.alias);
        });
        fields.field("expr", [&](auto& out) {
            write_debug(out, value.expr);
        });
    });
}

void write_debug(DebugWriter& writer, const OrderBy& value) {
    writer.structure("OrderBy", [&](auto& fields) {
        fields.field("field", [&](auto& out) {
            write_debug(out, value.field);
        });
        fields.field("expr", [&](auto& out) {
            optional_debug(out, value.expr);
        });
        fields.field("direction", [&](auto& out) {
            write_debug(out, value.direction);
        });
    });
}

void write_debug(DebugWriter& writer, const Aggregate& value) {
    writer.structure("Aggregate", [&](auto& fields) {
        fields.field("function", [&](auto& out) {
            write_debug(out, value.function);
        });
        fields.field("field", [&](auto& out) {
            write_debug(out, value.field);
        });
        fields.field("alias", [&](auto& out) {
            write_debug(out, value.alias);
        });
    });
}

void write_debug(DebugWriter& writer, const Slice& value) {
    writer.structure("Slice", [&](auto& fields) {
        fields.field("limit", [&](auto& out) {
            optional_debug(out, value.limit);
        });
        fields.field("offset", [&](auto& out) {
            write_debug(out, value.offset);
        });
    });
}

void write_debug(DebugWriter& writer, const RelationLoad& value) {
    writer.structure("RelationLoad", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            write_debug(out, value.name);
        });
        fields.field("query", [&](auto& out) {
            optional_debug(out, value.query);
        });
    });
}

void write_debug(DebugWriter& writer, const RelationAggregate& value) {
    writer.structure("RelationAggregate", [&](auto& fields) {
        fields.field("relation_name", [&](auto& out) {
            write_debug(out, value.relation_name);
        });
        fields.field("alias", [&](auto& out) {
            write_debug(out, value.alias);
        });
        fields.field("query", [&](auto& out) {
            write_debug(out, value.query);
        });
        fields.field("single_result", [&](auto& out) {
            write_debug(out, value.single_result);
        });
    });
}

void write_debug(DebugWriter& writer, const RawSqlProjection& value) {
    writer.structure("RawSqlProjection", [&](auto& fields) {
        fields.field("property_name", [&](auto& out) {
            write_debug(out, value.property_name);
        });
        fields.field("raw_sql_segment", [&](auto& out) {
            write_debug(out, value.raw_sql_segment);
        });
    });
}

void write_debug(DebugWriter& writer, const ObjectGroupBy& value) {
    writer.structure("ObjectGroupBy", [&](auto& fields) {
        fields.field("property_name", [&](auto& out) {
            write_debug(out, value.property_name);
        });
        fields.field("storage_field", [&](auto& out) {
            write_debug(out, value.storage_field);
        });
        fields.field("query", [&](auto& out) {
            write_debug(out, value.query);
        });
    });
}

void write_debug(DebugWriter& writer, const AggregationCacheOptions& value) {
    writer.structure("AggregationCacheOptions", [&](auto& fields) {
        fields.field("enabled", [&](auto& out) {
            write_debug(out, value.enabled);
        });
        fields.field("cache_expired_millis", [&](auto& out) {
            write_debug(out, value.cache_expired_millis);
        });
        fields.field("propagate", [&](auto& out) {
            write_debug(out, value.propagate);
        });
        fields.field("propagate_cache_expired_millis", [&](auto& out) {
            write_debug(out, value.propagate_cache_expired_millis);
        });
    });
}

void write_debug(DebugWriter& writer, const StreamConfig& value) {
    writer.structure("StreamConfig", [&](auto& fields) {
        fields.field("chunk_size", [&](auto& out) {
            write_debug(out, value.chunk_size);
        });
    });
}

void write_debug(DebugWriter& writer, const ContinuousPageFetchOptions& value) {
    writer.structure("ContinuousPageFetchOptions", [&](auto& fields) {
        fields.field("namespace", [&](auto& out) {
            write_debug(out, value.namespace_name);
        });
        fields.field("ttl_seconds", [&](auto& out) {
            write_debug(out, value.ttl_seconds);
        });
    });
}

void write_debug(DebugWriter& writer, const IdSetPaginationOptions& value) {
    writer.structure("IdSetPaginationOptions", [&](auto& fields) {
        fields.field("namespace", [&](auto& out) {
            write_debug(out, value.namespace_name);
        });
        fields.field("ttl_seconds", [&](auto& out) {
            write_debug(out, value.ttl_seconds);
        });
        fields.field("max_ids", [&](auto& out) {
            write_debug(out, value.max_ids);
        });
    });
}

void write_debug(DebugWriter& writer, const TraceNode& value) {
    writer.structure("TraceNode", [&](auto& fields) {
        fields.field("kind", [&](auto& out) {
            write_debug(out, value.kind);
        });
        fields.field("entity_type", [&](auto& out) {
            write_debug(out, value.entity_type);
        });
        fields.field("entity_id", [&](auto& out) {
            optional_debug(out, value.entity_id);
        });
        fields.field("comment", [&](auto& out) {
            write_debug(out, value.comment);
        });
    });
}

void write_debug(DebugWriter& writer, const SelectQuery& value) {
    writer.structure("SelectQuery", [&](auto& fields) {
        fields.field("hard_limit", [&](auto& out) {
            write_debug(out, value.hard_limit_value);
        });
        fields.field("entity", [&](auto& out) {
            write_debug(out, value.entity);
        });
        fields.field("projection", [&](auto& out) {
            sequence_debug(out, value.projection);
        });
        fields.field("expr_projection", [&](auto& out) {
            sequence_debug(out, value.expr_projection);
        });
        fields.field("search_with_text", [&](auto& out) {
            optional_debug(out, value.search_text);
        });
        fields.field("filter", [&](auto& out) {
            optional_debug(out, value.predicate);
        });
        fields.field("having", [&](auto& out) {
            optional_debug(out, value.having_predicate);
        });
        fields.field("order_by", [&](auto& out) {
            sequence_debug(out, value.order_by);
        });
        fields.field("slice", [&](auto& out) {
            optional_debug(out, value.slice);
        });
        fields.field("partition_by", [&](auto& out) {
            optional_debug(out, value.partition_field);
        });
        fields.field("top_n_probe_parent_threshold", [&](auto& out) {
            optional_debug(out, value.top_n_probe_parent_threshold_value);
        });
        fields.field("aggregates", [&](auto& out) {
            sequence_debug(out, value.aggregates);
        });
        fields.field("group_by", [&](auto& out) {
            sequence_debug(out, value.group_by_fields);
        });
        fields.field("relations", [&](auto& out) {
            sequence_debug(out, value.relations);
        });
        fields.field("aggregation_cache", [&](auto& out) {
            optional_debug(out, value.aggregation_cache);
        });
        fields.field("comment", [&](auto& out) {
            optional_debug(out, value.comment_text);
        });
        fields.field("trace_chain", [&](auto& out) {
            sequence_debug(out, value.trace_chain);
        });
        fields.field("raw_sql", [&](auto& out) {
            optional_debug(out, value.raw_sql_text);
        });
        fields.field("raw_sql_search_criteria", [&](auto& out) {
            sequence_debug(out, value.raw_sql_search_criteria);
        });
        fields.field("dynamic_properties", [&](auto& out) {
            sequence_debug(out, value.dynamic_properties);
        });
        fields.field("raw_projections", [&](auto& out) {
            sequence_debug(out, value.raw_projections);
        });
        fields.field("object_group_bys", [&](auto& out) {
            sequence_debug(out, value.object_group_bys);
        });
        fields.field("child_enhancements", [&](auto& out) {
            sequence_debug(out, value.child_enhancements);
        });
        fields.field("stream_config", [&](auto& out) {
            optional_debug(out, value.stream_config);
        });
        fields.field("continuous_page_fetch", [&](auto& out) {
            optional_debug(out, value.continuous_page_fetch);
        });
        fields.field("id_set_pagination", [&](auto& out) {
            optional_debug(out, value.id_set_pagination);
        });
    });
}
}

namespace teaql::detail {
String debug_query(Memory owner, const SelectQuery& query) {
    return debug_string(owner, query);
}

String debug_expr(Memory owner, const Expr& expr) {
    return debug_string(owner, expr);
}
}
