#pragma once
#include <teaql/runtime/graph.hpp>
#include <teaql/core/mutation_debug.hpp>
#include <array>

namespace teaql {
// Raw source-model diagnostics; publication must use the safe audit/log path.
inline void write_debug(DebugWriter&, const EntityValues&);
inline void write_debug(DebugWriter&, const GraphNode&);
inline void write_debug(DebugWriter&, const TraceScopeToken&);
inline void write_debug(DebugWriter&, const ScopedCommentNode&);
inline void write_debug(DebugWriter&, const GraphMutationPlanItem&);
inline void write_debug(DebugWriter&, const GraphMutationBatch&);
inline void write_debug(DebugWriter&, const GraphMutationPlan&);

namespace detail {
template <class T>
void graph_debug_element(DebugWriter&, const T&);
template <class T>
void graph_debug_element(DebugWriter&, const std::optional<T>&);
template <class T>
void graph_debug_element(DebugWriter&, const T*);
template <class T>
void graph_debug_element(DebugWriter&, const Vector<T>&);
inline void graph_debug_element(DebugWriter&, const std::pmr::set<String, std::less<>>&);

template <class T>
void graph_debug_element(DebugWriter& writer, const T& value) {
    write_debug(writer, value);
}

template <class T>
void graph_debug_element(DebugWriter& writer, const std::optional<T>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            graph_debug_element(out, *value);
        });
    } else {
        writer.raw("None");
    }
}

template <class T>
void graph_debug_element(DebugWriter& writer, const T* value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            graph_debug_element(out, *value);
        });
    } else {
        writer.raw("None");
    }
}

template <class T>
void graph_debug_element(DebugWriter& writer, const Vector<T>& values) {
    writer.sequence(values, [](auto& out, const auto& value) {
        graph_debug_element(out, value);
    });
}

inline void graph_debug_element(DebugWriter& writer,
                                const std::pmr::set<String, std::less<>>& values) {
    writer.sequence(
        values,
        [](auto& out, const auto& value) {
            out.text(value);
        },
        "{",
        "}");
}
}

inline void write_debug(DebugWriter& writer, GraphOperation value) {
    constexpr std::string_view names[]{"Upsert", "Create", "Reference", "Remove"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Invalid graph diagnostic kind");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, GraphMutationKind value) {
    constexpr std::string_view names[]{"Create", "Update", "Delete", "Reference"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Invalid graph diagnostic kind");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, GraphTransactionBoundary value) {
    constexpr std::string_view names[]{"Started", "AlreadyActive", "Unsupported"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Invalid graph diagnostic kind");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, const EntityValues& value) {
    writer.tuple("EntityValues", [&](auto& out) {
        out.sequence(
            value.fields,
            [](auto& inner, const auto& pair) {
                inner.text(pair.first);
                inner.raw(": ");
                write_debug(inner, pair.second);
            },
            "{",
            "}");
    });
}

inline void write_debug(DebugWriter& writer, const GraphNode& value) {
    writer.structure("GraphNode", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::graph_debug_element(out, value.entity);
        });
        fields.field("values", [&](auto& out) {
            detail::graph_debug_element(out, value.values);
        });
        fields.field("relations", [&](auto& out) {
            out.sequence(
                value.relations,
                [](auto& inner, const auto& pair) {
                    inner.text(pair.first);
                    inner.raw(": ");
                    detail::graph_debug_element(inner, pair.second);
                },
                "{",
                "}");
        });
        fields.field("operation", [&](auto& out) {
            detail::graph_debug_element(out, value.operation);
        });
        fields.field("comment", [&](auto& out) {
            detail::graph_debug_element(out, value.comment);
        });
        fields.field("dirty_fields", [&](auto& out) {
            detail::graph_debug_element(out, value.dirty_fields);
        });
        fields.field("original_values", [&](auto& out) {
            detail::graph_debug_element(out, value.original_values);
        });
    });
}

inline void write_debug(DebugWriter& writer, const TraceScopeToken& value) {
    writer.structure("TraceScopeToken", [&](auto& fields) {
        fields.field("parent", [&](auto& out) {
            detail::graph_debug_element(out, value.parent());
        });
        fields.field("track", [&](auto& out) {
            detail::graph_debug_element(out, value.track);
        });
        fields.field("node_index", [&](auto& out) {
            detail::graph_debug_element(out, value.node_index);
        });
    });
}

inline void write_debug(DebugWriter& writer, const ScopedCommentNode& value) {
    writer.structure("ScopedCommentNode", [&](auto& fields) {
        fields.field("parent", [&](auto& out) {
            detail::graph_debug_element(out, value.parent);
        });
        fields.field("track", [&](auto& out) {
            detail::graph_debug_element(out, value.track);
        });
    });
}

inline void write_debug(DebugWriter& writer, const GraphMutationPlanItem& value) {
    writer.structure("GraphMutationPlanItem", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::graph_debug_element(out, value.entity);
        });
        fields.field("kind", [&](auto& out) {
            detail::graph_debug_element(out, value.kind);
        });
        fields.field("values", [&](auto& out) {
            detail::graph_debug_element(out, value.values);
        });
        fields.field("update_fields", [&](auto& out) {
            detail::graph_debug_element(out, value.update_fields);
        });
        fields.field("item_index", [&](auto& out) {
            detail::graph_debug_element(out, value.item_index);
        });
        fields.field("scope_token", [&](auto& out) {
            detail::graph_debug_element(out, value.scope_token);
        });
        fields.field("old_values", [&](auto& out) {
            detail::graph_debug_element(out, value.old_values);
        });
    });
}

inline void write_debug(DebugWriter& writer, const GraphMutationBatch& value) {
    writer.structure("GraphMutationBatch", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::graph_debug_element(out, value.entity);
        });
        fields.field("kind", [&](auto& out) {
            detail::graph_debug_element(out, value.kind);
        });
        fields.field("update_fields", [&](auto& out) {
            detail::graph_debug_element(out, value.update_fields);
        });
        fields.field("items", [&](auto& out) {
            detail::graph_debug_element(out, value.items);
        });
    });
}

inline void write_debug(DebugWriter& writer, const GraphMutationPlan& value) {
    writer.structure("GraphMutationPlan", [&](auto& fields) {
        fields.field("planned_root", [&](auto& out) {
            detail::graph_debug_element(out, value.planned_root);
        });
        fields.field("items", [&](auto& out) {
            detail::graph_debug_element(out, value.items);
        });
        fields.field("batches", [&](auto& out) {
            detail::graph_debug_element(out, value.batches);
        });
        fields.field("next_item_index", [&](auto& out) {
            detail::graph_debug_element(out, value.next_item_index);
        });
        fields.field("visited_nodes", [&](auto& out) {
            out.sequence(
                value.visited_nodes,
                [](auto& inner, const auto& pair) {
                    const std::array strings{&pair.first, &pair.second};
                    inner.sequence(
                        strings,
                        [](auto& item, const auto* text) {
                            item.text(*text);
                        },
                        "(",
                        ")");
                },
                "{",
                "}");
        });
    });
}
}
