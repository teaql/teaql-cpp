#include <teaql/runtime/context.hpp>
#include <cstdio>

namespace teaql {
namespace {
class MutationReviewScope {
public:
    explicit MutationReviewScope(Arena& arena) {
        registration_.attach(arena, this, [](void*) noexcept {
            std::terminate();
        });
    }

private:
    Arena::Quiescence registration_;
};
}

std::string_view UserContext::trace_id() const {
    check_public();
    return *trace_id_;
}

void UserContext::set_trace_id(std::string_view value) {
    check_public();
    ensure_mutable();
    String copy(value, memory_resource());
    *trace_id_ = std::move(copy);
}

Result<MutationGovernanceSnapshot, RuntimeError>
UserContext::review_mutation_plan(const MutationPlan& plan) const {
    check_public();
    auto r = memory_resource();
    MutationReviewScope active(detail::resource_arena(r));
    auto policy = mutation_policy_registry_
                      ? mutation_policy_registry_->get().resolve(r, plan.request_key)
                      : std::optional<MutationPolicyBorrow>{};
    MutationGovernanceSnapshot snapshot(std::allocator_arg,
                                        Allocator(r),
                                        plan.execution_id,
                                        plan.request_key,
                                        MutationPolicySource::GeneratedDefault,
                                        MutationPolicyApprovalStatus::NotApplicable);
    if (!policy) {
        snapshot.warning_codes.emplace_back(missing_mutation_policy_warning);
    } else {
        auto identity = policy->get().identity(r);
        auto decision = policy->get().review(r, *this, plan);
        if (!decision.is_allowed()) {
            String message("[MUTATION POLICY DENIED] ", r);
            message += decision.code ? std::string_view(*decision.code) : "MUTATION-POLICY-DENIED";
            message += ": ";
            message += decision.message ? std::string_view(*decision.message) : "mutation rejected";
            return Result<MutationGovernanceSnapshot, RuntimeError>::err(
                r, RuntimeError::policy(r, message));
        }
        auto approval = mutation_policy_approval_provider_
                            ? mutation_policy_approval_provider_->get().find_approval(r, identity)
                            : std::optional<MutationPolicyApproval>{};
        bool approved = false;
        if (approval && approval->policy == identity) {
            std::string_view approver = approval->approved_by;
            detail::trim(approver);
            approved = !approver.empty();
        }
        snapshot.source = MutationPolicySource::Customer;
        snapshot.policy.emplace(std::allocator_arg, Allocator(r), identity);
        snapshot.approval_status = approved ? MutationPolicyApprovalStatus::Approved
                                            : MutationPolicyApprovalStatus::Missing;
        if (!approved) {
            snapshot.warning_codes.emplace_back(missing_mutation_policy_approval_warning);
        }
    }
    for (const auto& operation : plan.operations) {
        snapshot.operations.emplace_back(operation.kind, operation.entity);
        for (const auto& [field, value] : operation.changed_values) {
            snapshot.operations.back().changed_fields.emplace_back(field);
        }
    }
    for (const auto& code : snapshot.warning_codes) {
        emit_mutation_governance_warning(snapshot, code);
    }
    return Result<MutationGovernanceSnapshot, RuntimeError>::ok(r, std::move(snapshot));
}

void UserContext::emit_mutation_governance_warning(const MutationGovernanceSnapshot& snapshot,
                                                   std::string_view code) const {
    auto r = memory_resource();
    String key(snapshot.request_key, r);
    key += '|';
    if (snapshot.policy) {
        key += snapshot.policy->id;
        key += ':';
        key += snapshot.policy->version;
        key += ':';
        key += snapshot.policy->fingerprint;
    } else {
        key += "none";
    }
    key += '|';
    key += code;
    if (!emitted_mutation_governance_warnings_) {
        emitted_mutation_governance_warnings_ = &detail::resource_arena(r).make_payload<FieldSet>();
    }
    auto first = emitted_mutation_governance_warnings_->insert(std::move(key)).second;
    MutationGovernanceEvent event(std::allocator_arg, Allocator(r), snapshot, code, first);
    DefaultMutationGovernanceSink fallback;
    auto result = mutation_governance_sink_
                      ? mutation_governance_sink_->get().on_warning(r, *this, event)
                      : fallback.on_warning(r, *this, event);
    if (!result) {
        auto error = result.error().to_string(r);
        std::fputs("[TeaQL][MUTATION-POLICY-SINK-FAILED] request=", stderr);
        std::fwrite(snapshot.request_key.data(), 1, snapshot.request_key.size(), stderr);
        std::fputs(" error=", stderr);
        std::fwrite(error.data(), 1, error.size(), stderr);
        std::fputc('\n', stderr);
    }
}
}
