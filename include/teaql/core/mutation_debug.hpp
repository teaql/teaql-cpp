#pragma once
#include <teaql/core/mutation.hpp>
#include <teaql/core/query_debug.hpp>

namespace teaql {
// Raw mutation model diagnostics, separate from safe audit/log projections.
inline void write_debug(DebugWriter&, const MutationValues&);
inline void write_debug(DebugWriter&, const GeneratedValues&);
inline void write_debug(DebugWriter&, const EntitySnapshot&);
inline void write_debug(DebugWriter&, const InsertCommand&);
inline void write_debug(DebugWriter&, const UpdateCommand&);
inline void write_debug(DebugWriter&, const BatchInsertCommand&);
inline void write_debug(DebugWriter&, const BatchUpdateCommand&);
inline void write_debug(DebugWriter&, const DeleteCommand&);
inline void write_debug(DebugWriter&, const RecoverCommand&);

namespace detail {
template <class T>
void mutation_debug_element(DebugWriter& writer, const T& value) {
    write_debug(writer, value);
}

template <class T>
void mutation_debug_element(DebugWriter& writer, const std::optional<T>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_debug(out, *value);
        });
    } else {
        writer.raw("None");
    }
}

template <class T>
void mutation_debug_element(DebugWriter& writer, const Vector<T>& value) {
    writer.sequence(value, [](auto& out, const auto& item) {
        mutation_debug_element(out, item);
    });
}
}

inline void write_debug(DebugWriter& writer, const MutationValues& value) {
    writer.tuple("MutationValues", [&](auto& out) {
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

inline void write_debug(DebugWriter& writer, const GeneratedValues& value) {
    writer.tuple("GeneratedValues", [&](auto& out) {
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

inline void write_debug(DebugWriter& writer, const EntitySnapshot& value) {
    writer.tuple("EntitySnapshot", [&](auto& out) {
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

inline void write_debug(DebugWriter& writer, MutationKind value) {
    constexpr std::string_view names[]{"Insert", "Update", "Delete", "Recover"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Invalid mutation diagnostic kind");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, const InsertCommand& value) {
    writer.structure("InsertCommand", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::mutation_debug_element(out, value.entity);
        });
        fields.field("values", [&](auto& out) {
            detail::mutation_debug_element(out, value.values);
        });
        fields.field("trace_chain", [&](auto& out) {
            detail::mutation_debug_element(out, value.trace_chain);
        });
    });
}

inline void write_debug(DebugWriter& writer, const UpdateCommand& value) {
    writer.structure("UpdateCommand", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::mutation_debug_element(out, value.entity);
        });
        fields.field("id", [&](auto& out) {
            detail::mutation_debug_element(out, value.id);
        });
        fields.field("expected_version", [&](auto& out) {
            detail::mutation_debug_element(out, value.expected_version_value);
        });
        fields.field("values", [&](auto& out) {
            detail::mutation_debug_element(out, value.values);
        });
        fields.field("trace_chain", [&](auto& out) {
            detail::mutation_debug_element(out, value.trace_chain);
        });
        fields.field("old_values", [&](auto& out) {
            detail::mutation_debug_element(out, value.old_values);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BatchInsertCommand& value) {
    writer.structure("BatchInsertCommand", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::mutation_debug_element(out, value.entity);
        });
        fields.field("batch_values", [&](auto& out) {
            detail::mutation_debug_element(out, value.batch_values);
        });
        fields.field("trace_chains", [&](auto& out) {
            detail::mutation_debug_element(out, value.trace_chains);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BatchUpdateCommand& value) {
    writer.structure("BatchUpdateCommand", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::mutation_debug_element(out, value.entity);
        });
        fields.field("batch_ids", [&](auto& out) {
            detail::mutation_debug_element(out, value.batch_ids);
        });
        fields.field("batch_expected_versions", [&](auto& out) {
            detail::mutation_debug_element(out, value.batch_expected_versions);
        });
        fields.field("batch_values", [&](auto& out) {
            detail::mutation_debug_element(out, value.batch_values);
        });
        fields.field("update_fields", [&](auto& out) {
            detail::mutation_debug_element(out, value.update_fields);
        });
        fields.field("trace_chains", [&](auto& out) {
            detail::mutation_debug_element(out, value.trace_chains);
        });
        fields.field("batch_old_values", [&](auto& out) {
            detail::mutation_debug_element(out, value.batch_old_values);
        });
    });
}

inline void write_debug(DebugWriter& writer, const DeleteCommand& value) {
    writer.structure("DeleteCommand", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::mutation_debug_element(out, value.entity);
        });
        fields.field("id", [&](auto& out) {
            detail::mutation_debug_element(out, value.id);
        });
        fields.field("expected_version", [&](auto& out) {
            detail::mutation_debug_element(out, value.expected_version_value);
        });
        fields.field("soft_delete", [&](auto& out) {
            detail::mutation_debug_element(out, value.soft_delete);
        });
        fields.field("trace_chain", [&](auto& out) {
            detail::mutation_debug_element(out, value.trace_chain);
        });
    });
}

inline void write_debug(DebugWriter& writer, const RecoverCommand& value) {
    writer.structure("RecoverCommand", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            detail::mutation_debug_element(out, value.entity);
        });
        fields.field("id", [&](auto& out) {
            detail::mutation_debug_element(out, value.id);
        });
        fields.field("expected_version", [&](auto& out) {
            detail::mutation_debug_element(out, value.expected_version);
        });
        fields.field("trace_chain", [&](auto& out) {
            detail::mutation_debug_element(out, value.trace_chain);
        });
    });
}

}
