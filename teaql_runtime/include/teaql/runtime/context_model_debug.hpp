#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/runtime/context_root.hpp>
#include <teaql/runtime/fix_evidence.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const ContextEntityRef& value) {
    writer.structure("ContextEntityRef", [&](auto& fields) {
        fields.field("entity_type", [&](auto& out) {
            out.text(value.entity_type);
        });
        fields.field("id", [&](auto& out) {
            out.integer(value.id);
        });
    });
}

inline void write_debug(DebugWriter& writer, const ContextRootError& value) {
    writer.structure("ContextRootError", [&](auto& fields) {
        fields.field("expected_entity_type", [&](auto& out) {
            out.text(value.expected_entity_type);
        });
        fields.field("actual_root", [&](auto& out) {
            if (value.actual_root) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.actual_root);
                });
            } else {
                out.raw("None");
            }
        });
    });
}

inline void write_debug(DebugWriter& writer, FixEvidenceSource value) {
    writer.raw(fix_evidence_source_name(value));
}

inline void write_debug(DebugWriter& writer, const FixEvidence& value) {
    writer.structure("FixEvidence", [&](auto& fields) {
        fields.field("entity_type", [&](auto& out) {
            out.text(value.entity_type);
        });
        fields.field("model_path", [&](auto& out) {
            out.text(value.model_path);
        });
        fields.field("source", [&](auto& out) {
            write_debug(out, value.source);
        });
        fields.field("source_label", [&](auto& out) {
            out.text(value.source_label);
        });
    });
}
}
