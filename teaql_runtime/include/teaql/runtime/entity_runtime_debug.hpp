#pragma once
#include <teaql/core/mutation_debug.hpp>
#include <teaql/runtime/entity_runtime.hpp>

namespace teaql {
void write_debug(DebugWriter&, const EntityKey&);
void write_debug(DebugWriter&, const EntityGraphBuilder&);
void write_debug(DebugWriter&, const ChangeSetStack&);
void write_debug(DebugWriter&, const EntityRuntimeState&);

inline void write_debug(DebugWriter& writer, LoadedRelation value) {
    switch (value) {
    case LoadedRelation::Loaded:
        writer.raw("Loaded");
        return;
    case LoadedRelation::Empty:
        writer.raw("Empty");
        return;
    case LoadedRelation::NotLoaded:
        writer.raw("NotLoaded");
        return;
    }
    throw std::invalid_argument("Unknown relation diagnostic state");
}

template <class T>
void write_debug(DebugWriter& writer, const RelationHandle<T>& value) {
    writer.structure("RelationHandle", [&](auto& fields) {
        fields.field("state", [&](auto& out) {
            write_debug(out, value.state());
        });
        fields.field("value", [&](auto& out) {
            if (const auto* borrowed = value.value()) {
                out.tuple("Some", [&](auto& inner) {
                    if constexpr (requires { write_debug(inner, *borrowed); }) {
                        write_debug(inner, *borrowed);
                    } else {
                        write_debug(inner, *borrowed, DefaultDebugElement{});
                    }
                });
            } else {
                out.raw("None");
            }
        });
    });
}

inline void write_debug(DebugWriter& writer, const EntityChangeSet& value) {
    writer.structure("EntityChangeSet", [&](auto& fields) {
        fields.field("changes", [&](auto& out) {
            out.sequence(
                value.changes(),
                [](auto& inner, const auto& entry) {
                    write_debug(inner, entry.first);
                    inner.raw(": ");
                    write_debug(inner, entry.second);
                },
                "{",
                "}");
        });
    });
}

inline void write_debug(DebugWriter& writer, const LedgerCompositionError& value) {
    switch (value.kind) {
    case LedgerCompositionError::Kind::MissingTargetState:
        writer.raw("MissingTargetState");
        return;
    case LedgerCompositionError::Kind::MissingSourceState:
        writer.raw("MissingSourceState");
        return;
    case LedgerCompositionError::Kind::ConflictingOriginalVersion:
        writer.structure("ConflictingOriginalVersion", [&](auto& fields) {
            fields.field("entity", [&](auto& out) {
                out.text(value.entity);
            });
            fields.field("id", [&](auto& out) {
                out.text(value.id);
            });
            fields.field("target", [&](auto& out) {
                out.integer(value.target);
            });
            fields.field("source", [&](auto& out) {
                out.integer(value.source);
            });
        });
        return;
    }
    throw std::invalid_argument("Unknown composition diagnostic variant");
}
}
