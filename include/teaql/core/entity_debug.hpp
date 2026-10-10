#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/core/entity.hpp>

namespace teaql {
// Raw source-compatible diagnostics; safe audit/log projections are separate.
inline void write_debug(DebugWriter& writer, const EntityError& error) {
    writer.structure("EntityError", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            out.text(error.entity);
        });
        fields.field("message", [&](auto& out) {
            out.text(error.message);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BaseEntityData& entity) {
    writer.structure("BaseEntityData", [&](auto& fields) {
        fields.field("id", [&](auto& out) {
            out.integer(entity.id);
        });
        fields.field("version", [&](auto& out) {
            out.integer(entity.version);
        });
        fields.field("dynamic", [&](auto& out) {
            out.sequence(
                entity.dynamic_values,
                [](auto& inner, const auto& pair) {
                    inner.text(pair.first);
                    inner.raw(": ");
                    write_debug(inner, pair.second);
                },
                "{",
                "}");
        });
    });
}
}
