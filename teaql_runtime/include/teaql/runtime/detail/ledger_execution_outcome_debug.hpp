#pragma once
#include <teaql/runtime/detail/ledger_execution_outcome.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <teaql/core/dynamic_fields_debug.hpp>

namespace teaql::detail {
// Raw model diagnostics follow the existing explicit DebugWriter boundary.
// Published execution records must continue through the privacy-aware sinks.
inline void write_debug(DebugWriter& writer, const DynamicReadbacks& value) {
    writer.structure("DynamicReadbacks", [&](auto& fields) {
        fields.field("rows", [&](auto& out) {
            out.sequence(
                value.rows,
                [](auto& inner, const auto& pair) {
                    teaql::write_debug(inner, pair.first);
                    inner.raw(": ");
                    teaql::write_debug(inner, pair.second);
                },
                "{",
                "}");
        });
    });
}

inline void write_debug(DebugWriter& writer, const LedgerExecutionOutcome& value) {
    writer.structure("LedgerExecutionOutcome", [&](auto& fields) {
        fields.field("generated_ids", [&](auto& out) {
            out.sequence(
                value.generated_ids,
                [](auto& inner, const auto& pair) {
                    teaql::write_debug(inner, pair.first);
                    inner.raw(": ");
                    teaql::write_debug(inner, pair.second);
                },
                "{",
                "}");
        });
        fields.field("dynamic_readbacks", [&](auto& out) {
            if (value.dynamic_readbacks) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.dynamic_readbacks);
                });
            } else {
                out.raw("None");
            }
        });
    });
}
}
