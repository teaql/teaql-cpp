#pragma once
#include <teaql/core/meta_debug.hpp>
#include <teaql/runtime/registry.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const InMemoryMetadataStore& metadata) {
    writer.structure("InMemoryMetadataStore", [&](auto& fields) {
        fields.field("entities", [&](auto& out) {
            out.sequence(
                metadata.entities_,
                [](auto& inner, const auto& entry) {
                    inner.text(entry.first);
                    inner.raw(": ");
                    write_debug(inner, entry.second);
                },
                "{",
                "}");
        });
    });
}

inline void write_debug(DebugWriter& writer, const InMemoryEntityRegistry& registry) {
    writer.structure("InMemoryEntityRegistry", [&](auto& fields) {
        fields.field("entities", [&](auto& out) {
            // Source storage maps each name to itself; native storage is a set.
            out.sequence(
                registry.entities_,
                [](auto& inner, const auto& name) {
                    inner.text(name);
                    inner.raw(": ");
                    inner.text(name);
                },
                "{",
                "}");
        });
    });
}
}
