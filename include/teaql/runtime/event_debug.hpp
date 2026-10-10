#pragma once
#include <teaql/runtime/event.hpp>
#include <teaql/runtime/mutation_policy_debug.hpp>
#include <teaql/core/mutation_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, RawAuditEventKind value) {
    switch (value) {
    case RawAuditEventKind::Created:
        writer.raw("Created");
        return;
    case RawAuditEventKind::Updated:
        writer.raw("Updated");
        return;
    case RawAuditEventKind::Deleted:
        writer.raw("Deleted");
        return;
    case RawAuditEventKind::Recovered:
        writer.raw("Recovered");
        return;
    case RawAuditEventKind::SchemaCreated:
        writer.raw("SchemaCreated");
        return;
    case RawAuditEventKind::SchemaVerified:
        writer.raw("SchemaVerified");
        return;
    case RawAuditEventKind::FieldAdded:
        writer.raw("FieldAdded");
        return;
    case RawAuditEventKind::DataSeeded:
        writer.raw("DataSeeded");
        return;
    }
    throw std::invalid_argument("Unknown raw audit event kind");
}

namespace detail {
inline void write_event_record(DebugWriter& writer, const Record& value) {
    writer.sequence(
        value,
        [](auto& out, const auto& entry) {
            write_debug(out, entry.first);
            out.raw(": ");
            write_debug(out, entry.second);
        },
        "{",
        "}");
}

inline void write_event_record_optional(DebugWriter& writer, const std::optional<Record>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_event_record(out, *value);
        });
    } else {
        writer.raw("None");
    }
}
}

inline void write_debug(DebugWriter& writer, const EntityPropertyChange& value) {
    writer.structure("EntityPropertyChange", [&](auto& fields) {
        fields.field("field", [&](auto& out) {
            write_debug(out, value.field);
        });
        fields.field("old_value", [&](auto& out) {
            detail::write_policy_optional(out, value.old_value);
        });
        fields.field("new_value", [&](auto& out) {
            detail::write_policy_optional(out, value.new_value);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BootstrapAuditIdentity& value) {
    writer.structure("BootstrapAuditIdentity", [&](auto& fields) {
        fields.field("actor", [&](auto& out) {
            write_debug(out, value.actor);
        });
        fields.field("category", [&](auto& out) {
            write_debug(out, value.category);
        });
        fields.field("reason", [&](auto& out) {
            write_debug(out, value.reason);
        });
        fields.field("resulting_version", [&](auto& out) {
            detail::write_policy_optional(out, value.resulting_version);
        });
        fields.field("occurred_at_millis", [&](auto& out) {
            out.integer(value.occurred_at_millis);
        });
    });
}

inline void write_debug(DebugWriter& writer, const SafeAuditField& value) {
    writer.structure("SafeAuditField", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            write_debug(out, value.name);
        });
        fields.field("value", [&](auto& out) {
            detail::write_policy_optional(out, value.value);
        });
        fields.field("masked", [&](auto& out) {
            out.boolean(value.masked);
        });
        fields.field("truncated", [&](auto& out) {
            out.boolean(value.truncated);
        });
        fields.field("raw_length", [&](auto& out) {
            detail::write_policy_optional(out, value.raw_length);
        });
        fields.field("output_length", [&](auto& out) {
            detail::write_policy_optional(out, value.output_length);
        });
        fields.field("mask_reason", [&](auto& out) {
            detail::write_policy_optional(out, value.mask_reason);
        });
        fields.field("truncate_reason", [&](auto& out) {
            detail::write_policy_optional(out, value.truncate_reason);
        });
    });
}

inline void write_debug(DebugWriter& writer, const SafeAuditEvent& value) {
    writer.structure("SafeAuditEvent", [&](auto& fields) {
        fields.field("kind", [&](auto& out) {
            write_debug(out, value.kind);
        });
        fields.field("entity", [&](auto& out) {
            write_debug(out, value.entity);
        });
        fields.field("fields", [&](auto& out) {
            detail::write_policy_list(out, value.fields);
        });
        fields.field("trace_chain", [&](auto& out) {
            detail::write_policy_list(out, value.trace_chain);
        });
        fields.field("mutation_governance", [&](auto& out) {
            detail::write_policy_optional(out, value.mutation_governance);
        });
    });
}

inline void write_debug(DebugWriter& writer, const RawAuditEvent& value) {
    writer.structure("RawAuditEvent", [&](auto& fields) {
        fields.field("kind", [&](auto& out) {
            write_debug(out, value.kind);
        });
        fields.field("entity", [&](auto& out) {
            write_debug(out, value.entity);
        });
        fields.field("values", [&](auto& out) {
            detail::write_event_record(out, value.values);
        });
        fields.field("updated_fields", [&](auto& out) {
            detail::write_policy_list(out, value.updated_fields);
        });
        fields.field("old_values", [&](auto& out) {
            detail::write_event_record_optional(out, value.old_values);
        });
        fields.field("new_values", [&](auto& out) {
            detail::write_event_record_optional(out, value.new_values);
        });
        fields.field("changes", [&](auto& out) {
            detail::write_policy_list(out, value.changes);
        });
        fields.field("trace_chain", [&](auto& out) {
            detail::write_policy_list(out, value.trace_chain);
        });
        fields.field("bootstrap_audit", [&](auto& out) {
            detail::write_policy_optional(out, value.bootstrap_audit);
        });
        fields.field("mutation_governance", [&](auto& out) {
            detail::write_policy_optional(out, value.mutation_governance);
        });
    });
}
}
