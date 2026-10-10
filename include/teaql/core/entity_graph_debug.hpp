#pragma once
#include <teaql/core/entity_graph.hpp>
#include <teaql/core/mutation_debug.hpp>

namespace teaql::core {
// Raw annotated input-node diagnostics, independent of runtime graph persistence.
inline void write_debug(DebugWriter& writer, EntityGraphOperation operation) {
    switch (operation) {
    case EntityGraphOperation::Save:
        writer.raw("Save");
        return;
    case EntityGraphOperation::Delete:
        writer.raw("Delete");
        return;
    }
    throw std::invalid_argument("Invalid input graph diagnostic operation");
}

inline void write_debug(DebugWriter& writer, const EntityGraphNode& value) {
    writer.structure("EntityGraphNode", [&](auto& fields) {
        fields.field("entity_type", [&](auto& out) {
            out.text(value.entity_type);
        });
        fields.field("values", [&](auto& out) {
            teaql::write_debug(out, value.values);
        });
        fields.field("comment", [&](auto& out) {
            if (value.comment) {
                out.tuple("Some", [&](auto& inner) {
                    inner.text(*value.comment);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("operation", [&](auto& out) {
            write_debug(out, value.operation);
        });
        fields.field("children", [&](auto& out) {
            out.sequence(value.children, [](auto& inner, const auto& child) {
                // A relation-name/node pair is a two-element tuple, not a map.
                inner.sequence(
                    std::array{0, 1},
                    [&](auto& tuple, int index) {
                        if (index == 0) {
                            tuple.text(child.first);
                        } else {
                            write_debug(tuple, child.second);
                        }
                    },
                    "(",
                    ")");
            });
        });
    });
}
}
