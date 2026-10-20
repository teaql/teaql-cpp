#pragma once
#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/purposed_query.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const PurposedSelectQuery& value) {
    writer.structure("PurposedSelectQuery", [&](auto& fields) {
        fields.field("query", [&](auto& out) {
            write_debug(out, value.as_query());
        });
        fields.field("has_diagnostic_source", [&](auto& out) {
            write_debug(out, bool(value.diagnostic_source_));
        });
    });
}
}
