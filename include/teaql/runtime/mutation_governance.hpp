#pragma once
#include <teaql/runtime/entity_key.hpp>
#include <teaql/core/detail/text.hpp>
#include <teaql/memory/clone.hpp>
#include <functional>

namespace teaql {
struct MutationPolicyIdentity {
    using allocator_type = Allocator;
    String id, version, fingerprint;

    MutationPolicyIdentity(std::allocator_arg_t,
                           Allocator a,
                           std::string_view id,
                           std::string_view version,
                           std::string_view fingerprint)
        : id(id, a.resource()), version(version, a.resource()),
          fingerprint(fingerprint, a.resource()) {
        detail::trim(id);
        detail::trim(version);
        detail::trim(fingerprint);
        if (id.empty()) {
            throw std::invalid_argument("policy id must not be blank");
        }
        if (version.empty()) {
            throw std::invalid_argument("policy version must not be blank");
        }
        if (fingerprint.empty()) {
            throw std::invalid_argument("policy fingerprint must not be blank");
        }
    }

    MutationPolicyIdentity(std::allocator_arg_t, Allocator a, const MutationPolicyIdentity& other)
        : id(other.id, a.resource()), version(other.version, a.resource()),
          fingerprint(other.fingerprint, a.resource()) {}

    MutationPolicyIdentity(std::allocator_arg_t, Allocator a, MutationPolicyIdentity&& other)
        : MutationPolicyIdentity(detail::move_to(a.resource(), std::move(other))) {}

    MutationPolicyIdentity(const MutationPolicyIdentity&) = delete;
    MutationPolicyIdentity(MutationPolicyIdentity&&) noexcept = default;

    MutationPolicyIdentity& operator=(MutationPolicyIdentity&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return id.get_allocator().resource();
    }

    auto operator<=>(const MutationPolicyIdentity&) const = default;
};
enum class MutationOperationKind { Create, Update, Delete, Recover };
enum class MutationPolicySource { GeneratedDefault, Customer };
enum class MutationPolicyApprovalStatus { NotApplicable, Missing, Approved };

struct MutationOperationSummary {
    using allocator_type = Allocator;
    MutationOperationKind kind;
    EntityKey entity;
    Vector<String> changed_fields;

    MutationOperationSummary(std::allocator_arg_t,
                             Allocator a,
                             MutationOperationKind kind,
                             const EntityKey& entity)
        : kind(kind), entity(std::allocator_arg, a, entity), changed_fields(a.resource()) {}

    MutationOperationSummary(std::allocator_arg_t,
                             Allocator a,
                             const MutationOperationSummary& other)
        : kind(other.kind), entity(std::allocator_arg, a, other.entity),
          changed_fields(other.changed_fields, a.resource()) {}

    MutationOperationSummary(std::allocator_arg_t, Allocator a, MutationOperationSummary&& other)
        : MutationOperationSummary(detail::move_to(a.resource(), std::move(other))) {}

    MutationOperationSummary(const MutationOperationSummary&) = delete;
    MutationOperationSummary(MutationOperationSummary&&) noexcept = default;

    MutationOperationSummary& operator=(MutationOperationSummary&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return changed_fields.get_allocator().resource();
    }

    bool operator==(const MutationOperationSummary&) const = default;
};

struct MutationGovernanceSnapshot {
    using allocator_type = Allocator;
    String execution_id, request_key;
    MutationPolicySource source;
    std::optional<MutationPolicyIdentity> policy;
    MutationPolicyApprovalStatus approval_status;
    Vector<String> warning_codes;
    Vector<MutationOperationSummary> operations;

    MutationGovernanceSnapshot(std::allocator_arg_t,
                               Allocator a,
                               std::string_view execution,
                               std::string_view request,
                               MutationPolicySource source,
                               MutationPolicyApprovalStatus status)
        : execution_id(execution, a.resource()), request_key(request, a.resource()), source(source),
          approval_status(status), warning_codes(a.resource()), operations(a.resource()) {}

    MutationGovernanceSnapshot(std::allocator_arg_t,
                               Allocator a,
                               const MutationGovernanceSnapshot& other)
        : execution_id(other.execution_id, a.resource()),
          request_key(other.request_key, a.resource()), source(other.source),
          policy(detail::clone_in(a.resource(), other.policy)),
          approval_status(other.approval_status), warning_codes(other.warning_codes, a.resource()),
          operations(other.operations, a.resource()) {}

    MutationGovernanceSnapshot(std::allocator_arg_t,
                               Allocator a,
                               MutationGovernanceSnapshot&& other)
        : MutationGovernanceSnapshot(detail::move_to(a.resource(), std::move(other))) {}

    MutationGovernanceSnapshot(const MutationGovernanceSnapshot&) = delete;
    MutationGovernanceSnapshot(MutationGovernanceSnapshot&&) noexcept = default;

    MutationGovernanceSnapshot& operator=(MutationGovernanceSnapshot&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return execution_id.get_allocator().resource();
    }

    bool operator==(const MutationGovernanceSnapshot&) const = default;
};
}

namespace std {
template <>
struct hash<teaql::MutationPolicyIdentity> {
    std::size_t operator()(const teaql::MutationPolicyIdentity& value) const noexcept {
        // Hash-table capability follows all equality fields. This is a native
        // process hash, not Rust DefaultHasher bytes or a persisted identity.
        std::size_t result = 0;
        for (std::string_view field : {std::string_view(value.id),
                                       std::string_view(value.version),
                                       std::string_view(value.fingerprint)}) {
            auto part = std::hash<std::string_view>{}(field);
            result ^= part + std::size_t(0x9e3779b9) + (result << 6) + (result >> 2);
        }
        return result;
    }
};
}
