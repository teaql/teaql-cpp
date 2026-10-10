#pragma once
#include <teaql/runtime/mutation_policy.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <teaql/core/system_time_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, MutationOperationKind value) {
    switch (value) {
    case MutationOperationKind::Create:
        writer.raw("Create");
        return;
    case MutationOperationKind::Update:
        writer.raw("Update");
        return;
    case MutationOperationKind::Delete:
        writer.raw("Delete");
        return;
    case MutationOperationKind::Recover:
        writer.raw("Recover");
        return;
    }
    throw std::invalid_argument("Unknown mutation operation kind");
}

inline void write_debug(DebugWriter& writer, MutationVerdict value) {
    writer.raw(value == MutationVerdict::Allow ? "Allow" : "Deny");
}

inline void write_debug(DebugWriter& writer, MutationPolicySource value) {
    writer.raw(value == MutationPolicySource::GeneratedDefault ? "GeneratedDefault" : "Customer");
}

inline void write_debug(DebugWriter& writer, MutationPolicyApprovalStatus value) {
    switch (value) {
    case MutationPolicyApprovalStatus::NotApplicable:
        writer.raw("NotApplicable");
        return;
    case MutationPolicyApprovalStatus::Missing:
        writer.raw("Missing");
        return;
    case MutationPolicyApprovalStatus::Approved:
        writer.raw("Approved");
        return;
    }
    throw std::invalid_argument("Unknown mutation approval status");
}

inline void write_debug(DebugWriter& writer, const MutationPolicyIdentity& value) {
    writer.structure("MutationPolicyIdentity", [&](auto& fields) {
        fields.field("id", [&](auto& out) {
            write_debug(out, value.id);
        });
        fields.field("version", [&](auto& out) {
            write_debug(out, value.version);
        });
        fields.field("fingerprint", [&](auto& out) {
            write_debug(out, value.fingerprint);
        });
    });
}

namespace detail {
template <class T>
void write_policy_optional(DebugWriter& writer, const std::optional<T>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_debug(out, *value);
        });
    } else {
        writer.raw("None");
    }
}

template <class T>
void write_policy_list(DebugWriter& writer, const Vector<T>& values) {
    writer.sequence(values, [](auto& out, const auto& value) {
        write_debug(out, value);
    });
}
}

inline void write_debug(DebugWriter& writer, const MutationOperation& value) {
    writer.structure("MutationOperation", [&](auto& fields) {
        fields.field("kind", [&](auto& out) {
            write_debug(out, value.kind);
        });
        fields.field("entity", [&](auto& out) {
            write_debug(out, value.entity);
        });
        fields.field("original_version", [&](auto& out) {
            detail::write_policy_optional(out, value.original_version);
        });
        fields.field("changed_values", [&](auto& out) {
            out.sequence(
                value.changed_values,
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

inline void write_debug(DebugWriter& writer, const MutationPlan& value) {
    writer.structure("MutationPlan", [&](auto& fields) {
        fields.field("execution_id", [&](auto& out) {
            write_debug(out, value.execution_id);
        });
        fields.field("request_key", [&](auto& out) {
            write_debug(out, value.request_key);
        });
        fields.field("root_entity_type", [&](auto& out) {
            write_debug(out, value.root_entity_type);
        });
        fields.field("audit_reason", [&](auto& out) {
            detail::write_policy_optional(out, value.audit_reason);
        });
        fields.field("operations", [&](auto& out) {
            detail::write_policy_list(out, value.operations);
        });
    });
}

inline void write_debug(DebugWriter& writer, const MutationDecision& value) {
    writer.structure("MutationDecision", [&](auto& fields) {
        fields.field("verdict", [&](auto& out) {
            write_debug(out, value.verdict);
        });
        fields.field("code", [&](auto& out) {
            detail::write_policy_optional(out, value.code);
        });
        fields.field("message", [&](auto& out) {
            detail::write_policy_optional(out, value.message);
        });
        fields.field("field_paths", [&](auto& out) {
            detail::write_policy_list(out, value.field_paths);
        });
    });
}

inline void write_debug(DebugWriter& writer, const MutationPolicyApproval& value) {
    writer.structure("MutationPolicyApproval", [&](auto& fields) {
        fields.field("policy", [&](auto& out) {
            write_debug(out, value.policy);
        });
        fields.field("approved_by", [&](auto& out) {
            write_debug(out, value.approved_by);
        });
        fields.field("approved_at", [&](auto& out) {
            write_debug(out, value.approved_at);
        });
    });
}

inline void write_debug(DebugWriter& writer, const MutationOperationSummary& value) {
    writer.structure("MutationOperationSummary", [&](auto& fields) {
        fields.field("kind", [&](auto& out) {
            write_debug(out, value.kind);
        });
        fields.field("entity", [&](auto& out) {
            write_debug(out, value.entity);
        });
        fields.field("changed_fields", [&](auto& out) {
            detail::write_policy_list(out, value.changed_fields);
        });
    });
}

inline void write_debug(DebugWriter& writer, const MutationGovernanceSnapshot& value) {
    writer.structure("MutationGovernanceSnapshot", [&](auto& fields) {
        fields.field("execution_id", [&](auto& out) {
            write_debug(out, value.execution_id);
        });
        fields.field("request_key", [&](auto& out) {
            write_debug(out, value.request_key);
        });
        fields.field("source", [&](auto& out) {
            write_debug(out, value.source);
        });
        fields.field("policy", [&](auto& out) {
            detail::write_policy_optional(out, value.policy);
        });
        fields.field("approval_status", [&](auto& out) {
            write_debug(out, value.approval_status);
        });
        fields.field("warning_codes", [&](auto& out) {
            detail::write_policy_list(out, value.warning_codes);
        });
        fields.field("operations", [&](auto& out) {
            detail::write_policy_list(out, value.operations);
        });
    });
}

inline void write_debug(DebugWriter& writer, const MutationGovernanceEvent& value) {
    writer.structure("MutationGovernanceEvent", [&](auto& fields) {
        fields.field("snapshot", [&](auto& out) {
            write_debug(out, value.snapshot);
        });
        fields.field("warning_code", [&](auto& out) {
            write_debug(out, value.warning_code);
        });
        fields.field("first_occurrence", [&](auto& out) {
            out.boolean(value.first_occurrence);
        });
    });
}
}
