#include <teaql/runtime/mutation_policy.hpp>
#include <cstdio>

namespace teaql {
MutationDecision MutationDecision::deny(Memory r,
                                        std::string_view code,
                                        std::string_view message,
                                        std::span<const String> fields) {
    auto trimmed = code;
    detail::trim(trimmed);
    if (trimmed.empty()) {
        throw std::invalid_argument("denied policy code must not be blank");
    }
    MutationDecision result(std::allocator_arg, Allocator(r));
    result.verdict = MutationVerdict::Deny;
    result.code.emplace(code, r);
    result.message.emplace(message, r);
    result.field_paths.assign(fields.begin(), fields.end());
    return result;
}

MutationPolicyApproval::MutationPolicyApproval(std::allocator_arg_t,
                                               Allocator a,
                                               const MutationPolicyIdentity& identity,
                                               std::string_view approver,
                                               SystemTime time)
    : policy(std::allocator_arg, a, identity), approved_by(approver, a.resource()),
      approved_at(time) {
    detail::trim(approver);
    if (approver.empty()) {
        throw std::invalid_argument("mutation policy approver must not be blank");
    }
}

Result<std::monostate, RuntimeError> DefaultMutationGovernanceSink::on_warning(
    Memory r, const UserContext&, const MutationGovernanceEvent& event) const {
    if (event.first_occurrence) {
        const auto& snapshot = event.snapshot;
        auto source =
            snapshot.source == MutationPolicySource::Customer ? "Customer" : "GeneratedDefault";
        auto approval =
            snapshot.approval_status == MutationPolicyApprovalStatus::Approved  ? "Approved"
            : snapshot.approval_status == MutationPolicyApprovalStatus::Missing ? "Missing"
                                                                                : "NotApplicable";
        std::fputs("[TeaQL][", stderr);
        std::fwrite(event.warning_code.data(), 1, event.warning_code.size(), stderr);
        std::fputs("] request=", stderr);
        std::fwrite(snapshot.request_key.data(), 1, snapshot.request_key.size(), stderr);
        std::fprintf(stderr, " source=%s approval=%s\n", source, approval);
    }
    return Result<std::monostate, RuntimeError>::ok(r, {});
}
}
