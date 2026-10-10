#pragma once
#include <span>
#include <teaql/runtime/mutation_governance.hpp>
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/resources.hpp>
#include <teaql/runtime/system_time.hpp>
#include <teaql/memory/function.hpp>

namespace teaql {
class UserContext;
class EntityRuntimeState;
struct GraphMutationPlan;
struct GraphNode;
inline constexpr std::string_view missing_mutation_policy_warning = "MUTATION-POLICY-001";
inline constexpr std::string_view missing_mutation_policy_approval_warning = "MUTATION-POLICY-002";

struct MutationOperation {
    using allocator_type = Allocator;
    MutationOperationKind kind;
    EntityKey entity;
    std::optional<std::int64_t> original_version;
    Record changed_values;

    MutationOperation(std::allocator_arg_t,
                      Allocator a,
                      MutationOperationKind kind,
                      const EntityKey& entity)
        : kind(kind), entity(std::allocator_arg, a, entity), changed_values(a.resource()) {}

    MutationOperation(std::allocator_arg_t, Allocator a, const MutationOperation& other)
        : kind(other.kind), entity(std::allocator_arg, a, other.entity),
          original_version(other.original_version),
          changed_values(other.changed_values, a.resource()) {}

    MutationOperation(std::allocator_arg_t, Allocator a, MutationOperation&& other)
        : MutationOperation(detail::move_to(a.resource(), std::move(other))) {}

    MutationOperation(const MutationOperation&) = delete;
    MutationOperation(MutationOperation&&) noexcept = default;

    MutationOperation& operator=(MutationOperation&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return changed_values.get_allocator().resource();
    }

    bool operator==(const MutationOperation&) const = default;
};

struct MutationPlan {
    using allocator_type = Allocator;
    String execution_id, request_key, root_entity_type;
    std::optional<String> audit_reason;
    Vector<MutationOperation> operations;

    MutationPlan(std::allocator_arg_t, Allocator a)
        : execution_id(a.resource()), request_key(a.resource()), root_entity_type(a.resource()),
          operations(a.resource()) {}

    MutationPlan(std::allocator_arg_t, Allocator a, const MutationPlan& other)
        : execution_id(other.execution_id, a.resource()),
          request_key(other.request_key, a.resource()),
          root_entity_type(other.root_entity_type, a.resource()),
          audit_reason(detail::clone_in(a.resource(), other.audit_reason)),
          operations(other.operations, a.resource()) {}

    MutationPlan(std::allocator_arg_t, Allocator a, MutationPlan&& other)
        : MutationPlan(detail::move_to(a.resource(), std::move(other))) {}

    MutationPlan(const MutationPlan&) = delete;
    MutationPlan(MutationPlan&&) noexcept = default;

    MutationPlan& operator=(MutationPlan&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return execution_id.get_allocator().resource();
    }

    bool operator==(const MutationPlan&) const = default;
};
enum class MutationVerdict { Allow, Deny };

struct MutationDecision {
    using allocator_type = Allocator;
    MutationVerdict verdict = MutationVerdict::Allow;
    std::optional<String> code, message;
    Vector<String> field_paths;

    MutationDecision(std::allocator_arg_t, Allocator a) : field_paths(a.resource()) {}

    MutationDecision(std::allocator_arg_t, Allocator a, const MutationDecision& other)
        : verdict(other.verdict), code(detail::clone_in(a.resource(), other.code)),
          message(detail::clone_in(a.resource(), other.message)),
          field_paths(other.field_paths, a.resource()) {}

    MutationDecision(std::allocator_arg_t, Allocator a, MutationDecision&& other)
        : MutationDecision(detail::move_to(a.resource(), std::move(other))) {}

    MutationDecision(const MutationDecision&) = delete;
    MutationDecision(MutationDecision&&) noexcept = default;

    MutationDecision& operator=(MutationDecision&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return field_paths.get_allocator().resource();
    }

    bool is_allowed() const noexcept {
        return verdict == MutationVerdict::Allow;
    }

    static MutationDecision allow(Memory r) {
        return MutationDecision(std::allocator_arg, Allocator(r));
    }

    static MutationDecision
    deny(Memory, std::string_view code, std::string_view message, std::span<const String> fields);
    bool operator==(const MutationDecision&) const = default;
};

class MutationPolicy {
public:
    virtual ~MutationPolicy() = default;
    virtual MutationPolicyIdentity identity(Memory) const = 0;
    virtual MutationDecision review(Memory, const UserContext&, const MutationPlan&) const = 0;
};

using MutationPolicyBorrow = ResourceBorrow<const MutationPolicy>;

class MutationPolicyRegistry {
public:
    virtual ~MutationPolicyRegistry() = default;
    virtual std::optional<MutationPolicyBorrow> resolve(Memory,
                                                        std::string_view request_key) const = 0;
};

class EmptyMutationPolicyRegistry final : public MutationPolicyRegistry {
public:
    using allocator_type = Allocator;
    EmptyMutationPolicyRegistry() = default;

    EmptyMutationPolicyRegistry(std::allocator_arg_t, Allocator) {}

    std::optional<MutationPolicyBorrow> resolve(Memory, std::string_view) const override {
        return {};
    }
};

struct MutationPolicyApproval {
    using allocator_type = Allocator;
    MutationPolicyIdentity policy;
    String approved_by;
    SystemTime approved_at;
    MutationPolicyApproval(std::allocator_arg_t,
                           Allocator,
                           const MutationPolicyIdentity&,
                           std::string_view,
                           SystemTime);

    MutationPolicyApproval(std::allocator_arg_t, Allocator a, const MutationPolicyApproval& other)
        : policy(std::allocator_arg, a, other.policy), approved_by(other.approved_by, a.resource()),
          approved_at(other.approved_at) {}

    MutationPolicyApproval(std::allocator_arg_t, Allocator a, MutationPolicyApproval&& other)
        : MutationPolicyApproval(detail::move_to(a.resource(), std::move(other))) {}

    MutationPolicyApproval(const MutationPolicyApproval&) = delete;
    MutationPolicyApproval(MutationPolicyApproval&&) noexcept = default;

    MutationPolicyApproval& operator=(MutationPolicyApproval&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return approved_by.get_allocator().resource();
    }

    bool operator==(const MutationPolicyApproval&) const = default;
};

class MutationPolicyApprovalProvider {
public:
    virtual ~MutationPolicyApprovalProvider() = default;
    virtual std::optional<MutationPolicyApproval>
    find_approval(Memory, const MutationPolicyIdentity&) const = 0;
};

class NoMutationPolicyApprovalProvider final : public MutationPolicyApprovalProvider {
public:
    using allocator_type = Allocator;
    NoMutationPolicyApprovalProvider() = default;

    NoMutationPolicyApprovalProvider(std::allocator_arg_t, Allocator) {}

    std::optional<MutationPolicyApproval>
    find_approval(Memory, const MutationPolicyIdentity&) const override {
        return {};
    }
};

struct MutationGovernanceEvent {
    using allocator_type = Allocator;
    MutationGovernanceSnapshot snapshot;
    String warning_code;
    bool first_occurrence;

    MutationGovernanceEvent(std::allocator_arg_t,
                            Allocator a,
                            const MutationGovernanceSnapshot& snapshot,
                            std::string_view warning,
                            bool first)
        : snapshot(std::allocator_arg, a, snapshot), warning_code(warning, a.resource()),
          first_occurrence(first) {}

    MutationGovernanceEvent(std::allocator_arg_t, Allocator a, const MutationGovernanceEvent& other)
        : MutationGovernanceEvent(
              std::allocator_arg, a, other.snapshot, other.warning_code, other.first_occurrence) {}

    MutationGovernanceEvent(std::allocator_arg_t, Allocator a, MutationGovernanceEvent&& other)
        : MutationGovernanceEvent(detail::move_to(a.resource(), std::move(other))) {}

    MutationGovernanceEvent(const MutationGovernanceEvent&) = delete;
    MutationGovernanceEvent(MutationGovernanceEvent&&) noexcept = default;

    MutationGovernanceEvent& operator=(MutationGovernanceEvent&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return warning_code.get_allocator().resource();
    }

    bool operator==(const MutationGovernanceEvent&) const = default;
};

class MutationGovernanceSink {
public:
    virtual ~MutationGovernanceSink() = default;
    virtual Result<std::monostate, RuntimeError>
    on_warning(Memory, const UserContext&, const MutationGovernanceEvent&) const = 0;
};

class DefaultMutationGovernanceSink final : public MutationGovernanceSink {
public:
    using allocator_type = Allocator;
    DefaultMutationGovernanceSink() = default;

    DefaultMutationGovernanceSink(std::allocator_arg_t, Allocator) {}

    Result<std::monostate, RuntimeError>
    on_warning(Memory, const UserContext&, const MutationGovernanceEvent&) const override;
};

// Source closure implementations become scoped callable adapters.
class FunctionMutationPolicyRegistry final : public MutationPolicyRegistry {
public:
    using allocator_type = Allocator;
    using Callback = ContextFunction<std::optional<MutationPolicyBorrow>(Memory, std::string_view)>;

    template <class F>
    FunctionMutationPolicyRegistry(std::allocator_arg_t, Allocator a, F&& fn)
        requires(!std::same_as<std::remove_cvref_t<F>, FunctionMutationPolicyRegistry>)
        : callback_(std::allocator_arg, a, std::forward<F>(fn)) {}

    FunctionMutationPolicyRegistry(std::allocator_arg_t,
                                   Allocator a,
                                   const FunctionMutationPolicyRegistry& other)
        : callback_(std::allocator_arg, a, other.callback_) {}

    Memory memory_resource() const {
        return callback_.memory_resource();
    }

    std::optional<MutationPolicyBorrow> resolve(Memory r, std::string_view key) const override {
        return callback_(r, key);
    }

private:
    Callback callback_;
};

class FunctionMutationPolicyApprovalProvider final : public MutationPolicyApprovalProvider {
public:
    using allocator_type = Allocator;
    using Callback = ContextFunction<std::optional<MutationPolicyApproval>(
        Memory, const MutationPolicyIdentity&)>;

    template <class F>
    FunctionMutationPolicyApprovalProvider(std::allocator_arg_t, Allocator a, F&& fn)
        requires(!std::same_as<std::remove_cvref_t<F>, FunctionMutationPolicyApprovalProvider>)
        : callback_(std::allocator_arg, a, std::forward<F>(fn)) {}

    FunctionMutationPolicyApprovalProvider(std::allocator_arg_t,
                                           Allocator a,
                                           const FunctionMutationPolicyApprovalProvider& other)
        : callback_(std::allocator_arg, a, other.callback_) {}

    Memory memory_resource() const {
        return callback_.memory_resource();
    }

    std::optional<MutationPolicyApproval>
    find_approval(Memory r, const MutationPolicyIdentity& policy) const override {
        return callback_(r, policy);
    }

private:
    Callback callback_;
};

class FunctionMutationGovernanceSink final : public MutationGovernanceSink {
public:
    using allocator_type = Allocator;
    using Callback = ContextFunction<Result<std::monostate, RuntimeError>(
        Memory, const UserContext&, const MutationGovernanceEvent&)>;

    template <class F>
    FunctionMutationGovernanceSink(std::allocator_arg_t, Allocator a, F&& fn)
        requires(!std::same_as<std::remove_cvref_t<F>, FunctionMutationGovernanceSink>)
        : callback_(std::allocator_arg, a, std::forward<F>(fn)) {}

    FunctionMutationGovernanceSink(std::allocator_arg_t,
                                   Allocator a,
                                   const FunctionMutationGovernanceSink& other)
        : callback_(std::allocator_arg, a, other.callback_) {}

    Memory memory_resource() const {
        return callback_.memory_resource();
    }

    Result<std::monostate, RuntimeError> on_warning(
        Memory r, const UserContext& context, const MutationGovernanceEvent& event) const override {
        return callback_(r, context, event);
    }

private:
    Callback callback_;
};

namespace detail {
struct ContextMutationPolicyAccess;
Result<MutationPlan, RuntimeError> graph_policy_plan(const UserContext&, const GraphMutationPlan&);
MutationPlan ledger_policy_plan(const UserContext&, const GraphNode&, const EntityRuntimeState&);
}
}
