#pragma once
#include <teaql/core/detail/request_intent.hpp>
#include <teaql/core/debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, RequestKind value) {
    writer.raw(value == RequestKind::Query ? "Query" : "Mutation");
}

inline void write_debug(DebugWriter& writer, const RequestIntentError& value) {
    writer.structure("RequestIntentError", [&](auto& fields) {
        fields.field("request_kind", [&](auto& out) {
            write_debug(out, value.request_kind);
        });
        fields.field("field", [&](auto& out) {
            out.text(value.field);
        });
    });
}

inline void write_debug(DebugWriter& writer, const QueryIntent& value) {
    writer.structure("QueryIntent", [&](auto& fields) {
        fields.field("comment", [&](auto& out) {
            out.text(value.comment());
        });
        fields.field("purpose", [&](auto& out) {
            out.text(value.purpose());
        });
    });
}

inline void write_debug(DebugWriter& writer, const MutationIntent& value) {
    writer.structure("MutationIntent", [&](auto& fields) {
        fields.field("comment", [&](auto& out) {
            out.text(value.comment());
        });
    });
}
}
