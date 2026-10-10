#include <teaql/runtime/detail/graph_fix_scope.hpp>

namespace teaql {
Timestamp UserContext::fix_time() const {
    auto* state = detail::GraphFixActivation::current(*this);
    return state ? state->time : Timestamp::now();
}

void UserContext::record_fix_evidence(const FixEvidence& evidence) const {
    if (auto* state = detail::GraphFixActivation::current(*this)) {
        state->evidence.emplace_back(evidence);
    }
}

void UserContext::record_fix_evidence(FixEvidence&& evidence) const {
    if (auto* state = detail::GraphFixActivation::current(*this)) {
        state->evidence.emplace_back(std::move(evidence));
    }
}

Vector<FixEvidence> UserContext::last_fix_evidence() const {
    memory_.check_thread();
    ensure_open();
    return last_fix_evidence_ ? Vector<FixEvidence>(*last_fix_evidence_, memory_resource())
                              : Vector<FixEvidence>(memory_resource());
}

void UserContext::replace_last_fix_evidence(std::span<const FixEvidence> evidence) const {
    memory_.check_thread();
    ensure_open();
    Vector<FixEvidence> copy(memory_resource());
    copy.reserve(evidence.size());
    for (const auto& value : evidence) {
        copy.emplace_back(value);
    }
    if (!last_fix_evidence_) {
        last_fix_evidence_ =
            &const_cast<detail::ContextArena&>(memory_).make_payload<Vector<FixEvidence>>(
                std::move(copy));
    } else {
        *last_fix_evidence_ = std::move(copy);
    }
}

RuntimeHookResult UserContext::validate_required_create_payload(
    std::string_view entity, const EntityValues& values, const ObjectLocation& location) const {
    check_public();
    auto r = memory_resource();
    CheckResults results(r);
    collect_required_property_results(entity, values, CheckObjectStatus::Create, location, results);
    if (results.empty()) {
        return RuntimeHookResult::ok(r, {});
    }
    translate_check_results(results);
    return RuntimeHookResult::err(r, RuntimeError::check(r, std::move(results)));
}
}
