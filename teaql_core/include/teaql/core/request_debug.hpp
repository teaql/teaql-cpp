#pragma once
#include <teaql/core/query_debug.hpp>
#include <teaql/core/request.hpp>

namespace teaql::request {
// Raw builder diagnostics; these do not authorize or sanitize raw SQL.
inline void write_debug(DebugWriter&, const QuerySelection&);
inline void write_debug(DebugWriter&, const RelationSelection&);
inline void write_debug(DebugWriter&, const RelationFilter&);
inline void write_debug(DebugWriter&, const QueryOptions&);
inline void write_debug(DebugWriter&, const RawDynamicProperty&);
inline void write_debug(DebugWriter&, const RawProjection&);
inline void write_debug(DebugWriter&, const RelationAggregate&);
inline void write_debug(DebugWriter&, const FacetRequest&);
inline void write_debug(DebugWriter&, const ObjectGroupBy&);
inline void write_debug(DebugWriter&, const UnsafeRawSqlSegment&);

inline void write_debug(DebugWriter& writer, FieldOperator value) {
    constexpr std::string_view names[]{"Equal",
                                       "NotEqual",
                                       "GreaterThan",
                                       "GreaterThanOrEqual",
                                       "LessThan",
                                       "LessThanOrEqual",
                                       "Between",
                                       "In",
                                       "NotIn",
                                       "Contain",
                                       "NotContain",
                                       "BeginWith",
                                       "NotBeginWith",
                                       "EndWith",
                                       "NotEndWith",
                                       "SoundsLike",
                                       "IsNull",
                                       "IsNotNull"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Invalid request diagnostic operator");
    }
    writer.raw(names[index]);
}

namespace debug_detail {
template <class T>
void range_debug_element(DebugWriter&, const Vector<T>&);

template <class T>
void range_debug_element(DebugWriter& writer, const T& value) {
    write_debug(writer, value);
}

template <class T>
void range_debug_element(DebugWriter& writer, const std::optional<T>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            range_debug_element(out, *value);
        });
    } else {
        writer.raw("None");
    }
}

template <class T>
void range_debug_element(DebugWriter& writer, const Vector<T>& value) {
    writer.sequence(value, [](auto& out, const auto& item) {
        range_debug_element(out, item);
    });
}
}

template <class T>
void write_debug(DebugWriter& writer, const DateRange<T>& value) {
    writer.structure("DateRange", [&](auto& fields) {
        fields.field("start", [&](auto& out) {
            debug_detail::range_debug_element(out, value.start);
        });
        fields.field("end", [&](auto& out) {
            debug_detail::range_debug_element(out, value.end);
        });
    });
}

inline void write_debug(DebugWriter& writer, const UnsafeRawSqlSegment& value) {
    writer.structure("UnsafeRawSqlSegment", [&](auto& fields) {
        fields.field("sql", [&](auto& out) {
            out.text(value.sql_);
        });
    });
}

inline void write_debug(DebugWriter& writer, const QuerySelection& value) {
    writer.structure("QuerySelection", [&](auto& fields) {
        fields.field("query", [&](auto& out) {
            write_debug(out, value.query);
        });
        fields.field("relation_selections", [&](auto& out) {
            out.sequence(value.relation_selections, DefaultDebugElement{});
        });
        fields.field("relation_filters", [&](auto& out) {
            out.sequence(value.relation_filters, DefaultDebugElement{});
        });
        fields.field("child_enhancements", [&](auto& out) {
            out.sequence(value.child_enhancements, DefaultDebugElement{});
        });
        fields.field("query_options", [&](auto& out) {
            write_debug(out, value.query_options);
        });
    });
}

inline void write_debug(DebugWriter& writer, const RelationSelection& value) {
    writer.structure("RelationSelection", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            write_debug(out, value.name);
        });
        fields.field("query", [&](auto& out) {
            write_debug(out, value.query);
        });
        fields.field("relation_selections", [&](auto& out) {
            out.sequence(value.relation_selections, DefaultDebugElement{});
        });
        fields.field("relation_filters", [&](auto& out) {
            out.sequence(value.relation_filters, DefaultDebugElement{});
        });
        fields.field("child_enhancements", [&](auto& out) {
            out.sequence(value.child_enhancements, DefaultDebugElement{});
        });
        fields.field("query_options", [&](auto& out) {
            write_debug(out, value.query_options);
        });
    });
}

inline void write_debug(DebugWriter& writer, const RelationFilter& value) {
    writer.structure("RelationFilter", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            write_debug(out, value.name);
        });
        fields.field("query", [&](auto& out) {
            write_debug(out, value.query);
        });
        fields.field("relation_selections", [&](auto& out) {
            out.sequence(value.relation_selections, DefaultDebugElement{});
        });
        fields.field("relation_filters", [&](auto& out) {
            out.sequence(value.relation_filters, DefaultDebugElement{});
        });
        fields.field("child_enhancements", [&](auto& out) {
            out.sequence(value.child_enhancements, DefaultDebugElement{});
        });
        fields.field("query_options", [&](auto& out) {
            write_debug(out, value.query_options);
        });
    });
}

inline void write_debug(DebugWriter& writer, const QueryOptions& value) {
    writer.structure("QueryOptions", [&](auto& fields) {
        fields.field("comment", [&](auto& out) {
            if (value.comment) {
                out.tuple("Some", [&](auto& inner) {
                    inner.text(*value.comment);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("raw_sql", [&](auto& out) {
            if (value.raw_sql) {
                out.tuple("Some", [&](auto& inner) {
                    inner.text(*value.raw_sql);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("raw_sql_search_criteria", [&](auto& out) {
            out.sequence(value.raw_sql_search_criteria, DefaultDebugElement{});
        });
        fields.field("dynamic_properties", [&](auto& out) {
            out.sequence(value.dynamic_properties, DefaultDebugElement{});
        });
        fields.field("raw_projections", [&](auto& out) {
            out.sequence(value.raw_projections, DefaultDebugElement{});
        });
        fields.field("relation_aggregates", [&](auto& out) {
            out.sequence(value.relation_aggregates, DefaultDebugElement{});
        });
        fields.field("object_group_bys", [&](auto& out) {
            out.sequence(value.object_group_bys, DefaultDebugElement{});
        });
        fields.field("facets", [&](auto& out) {
            out.sequence(value.facets, DefaultDebugElement{});
        });
    });
}

inline void write_debug(DebugWriter& writer, const RawDynamicProperty& value) {
    writer.structure("RawDynamicProperty", [&](auto& fields) {
        fields.field("property_name", [&](auto& out) {
            write_debug(out, value.property_name);
        });
        fields.field("raw_sql_segment", [&](auto& out) {
            write_debug(out, value.raw_sql_segment);
        });
    });
}

inline void write_debug(DebugWriter& writer, const RawProjection& value) {
    writer.structure("RawProjection", [&](auto& fields) {
        fields.field("property_name", [&](auto& out) {
            write_debug(out, value.property_name);
        });
        fields.field("raw_sql_segment", [&](auto& out) {
            write_debug(out, value.raw_sql_segment);
        });
    });
}

inline void write_debug(DebugWriter& writer, const RelationAggregate& value) {
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

inline void write_debug(DebugWriter& writer, const FacetRequest& value) {
    writer.structure("FacetRequest", [&](auto& fields) {
        fields.field("facet_name", [&](auto& out) {
            write_debug(out, value.facet_name);
        });
        fields.field("relation_name", [&](auto& out) {
            write_debug(out, value.relation_name);
        });
        fields.field("query", [&](auto& out) {
            write_debug(out, value.query);
        });
        fields.field("include_all_facets", [&](auto& out) {
            write_debug(out, value.include_all_facets);
        });
    });
}

inline void write_debug(DebugWriter& writer, const ObjectGroupBy& value) {
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

}
