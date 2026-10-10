#pragma once
#include <teaql/core/dynamic_search.hpp>
#include <teaql/core/query_debug.hpp>

namespace teaql::dynamic_search {
// These are raw models, not warning sinks or safe logging projections.
inline void write_debug(DebugWriter& writer, const SearchModel& value) {
    writer.structure("SearchModel", [&](auto& fields) {
        auto map = [](auto& out, const auto& entries) {
            out.sequence(
                entries,
                [](auto& inner, const auto& entry) {
                    inner.text(entry.first);
                    inner.raw(": ");
                    inner.text(entry.second);
                },
                "{",
                "}");
        };
        fields.field("fields", [&](auto& out) {
            map(out, value.fields);
        });
        fields.field("relations", [&](auto& out) {
            map(out, value.relations);
        });
    });
}

inline void write_debug(DebugWriter& writer, const DynamicSearchWarning& value) {
    writer.structure("DynamicSearchWarning", [&](auto& fields) {
        fields.field("code", [&](auto& out) {
            out.text(value.code);
        });
        fields.field("entity", [&](auto& out) {
            out.text(value.entity);
        });
        fields.field("clause", [&](auto& out) {
            out.text(value.clause);
        });
        fields.field("field_path", [&](auto& out) {
            out.text(value.field_path);
        });
    });
}

inline void write_debug(DebugWriter& writer, const DynamicSearchFilter& value) {
    writer.structure("DynamicSearchFilter", [&](auto& fields) {
        fields.field("field_path", [&](auto& out) {
            out.text(value.field_path);
        });
        fields.field("operator", [&](auto& out) {
            out.text(value.operator_name);
        });
        fields.field("value", [&](auto& out) {
            teaql::write_debug(out, value.value);
        });
    });
}

inline void write_debug(DebugWriter& writer, const DynamicSearchOrder& value) {
    writer.structure("DynamicSearchOrder", [&](auto& fields) {
        fields.field("field_path", [&](auto& out) {
            out.text(value.field_path);
        });
        fields.field("descending", [&](auto& out) {
            out.boolean(value.descending);
        });
    });
}

inline void write_debug(DebugWriter& writer, const NormalizedDynamicSearch& value) {
    writer.structure("NormalizedDynamicSearch", [&](auto& fields) {
        fields.field("filters", [&](auto& out) {
            out.sequence(value.filters, DefaultDebugElement{});
        });
        fields.field("orders", [&](auto& out) {
            out.sequence(value.orders, DefaultDebugElement{});
        });
        fields.field("warnings", [&](auto& out) {
            out.sequence(value.warnings, DefaultDebugElement{});
        });
    });
}

inline void write_debug(DebugWriter& writer, const DynamicSearchResult& value) {
    writer.structure("DynamicSearchResult", [&](auto& fields) {
        fields.field("query", [&](auto& out) {
            teaql::write_debug(out, value.query);
        });
        fields.field("warnings", [&](auto& out) {
            out.sequence(value.warnings, DefaultDebugElement{});
        });
    });
}

inline void write_debug(DebugWriter& writer, const DynamicSearchError& value) {
    writer.tuple("DynamicSearchError", [&](auto& out) {
        out.text(value.message);
    });
}
}
