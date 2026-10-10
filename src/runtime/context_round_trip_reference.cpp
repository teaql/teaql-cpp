#include <teaql/runtime/context.hpp>
#include <teaql/runtime/round_trip_reference.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>

namespace teaql {
void UserContext::set_trusted_reference_principal(const TrustedReferencePrincipal& principal) {
    check_public();
    auto& retained = memory_.make<TrustedReferencePrincipal>(principal);
    insert_resource(*this, retained);
}

RoundTripResult<ReferenceMode> UserContext::reference_mode() const {
    check_public();
    if (auto* runtime = get_resource<ContextBoundReferenceRuntime>()) {
        return RoundTripResult<ReferenceMode>::ok(memory_resource(), runtime->mode());
    }
    return RoundTripResult<ReferenceMode>::err(memory_resource(),
                                               RoundTripReferenceError::configuration());
}

RoundTripResult<std::optional<ReferenceStartupNotice>>
UserContext::reference_startup_notice() const {
    check_public();
    if (auto* runtime = get_resource<ContextBoundReferenceRuntime>()) {
        return RoundTripResult<std::optional<ReferenceStartupNotice>>::ok(
            memory_resource(), runtime->startup_notice());
    }
    return RoundTripResult<std::optional<ReferenceStartupNotice>>::err(
        memory_resource(), RoundTripReferenceError::configuration());
}

RoundTripResult<ExternalEntityReference>
UserContext::reference_for(const ReferenceIdentity& identity,
                           const ReferenceDocumentScope& scope,
                           ReferenceLifetime lifetime) const {
    check_public();
    detail::ReferenceCallGuard guard(memory_resource(), memory_resource());
    auto* runtime = get_resource<ContextBoundReferenceRuntime>();
    if (!runtime) {
        return RoundTripResult<ExternalEntityReference>::err(
            memory_resource(), RoundTripReferenceError::configuration());
    }
    auto* principal = get_resource<TrustedReferencePrincipal>();
    if (!principal) {
        return RoundTripResult<ExternalEntityReference>::err(
            memory_resource(), RoundTripReferenceError::authorization_required());
    }
    return runtime->issue(memory_resource(), *principal, scope, identity, lifetime);
}

RoundTripResult<ResolvedEntityReference>
UserContext::resolve_reference(const ExternalEntityReference& reference,
                               std::string_view entity,
                               const ReferenceDocumentScope& scope) const {
    check_public();
    detail::ReferenceCallGuard guard(memory_resource(), memory_resource());
    auto* runtime = get_resource<ContextBoundReferenceRuntime>();
    if (!runtime) {
        return RoundTripResult<ResolvedEntityReference>::err(
            memory_resource(), RoundTripReferenceError::configuration());
    }
    auto* principal = get_resource<TrustedReferencePrincipal>();
    if (!principal) {
        return RoundTripResult<ResolvedEntityReference>::err(
            memory_resource(), RoundTripReferenceError::authorization_required());
    }
    return runtime->consume(memory_resource(), *principal, scope, reference, entity);
}

RoundTripResult<Json>
UserContext::serialize_reference(const ExternalEntityReference& reference) const {
    auto mode = reference_mode();
    if (!mode) {
        return RoundTripResult<Json>::err(memory_resource(), mode.error());
    }
    if (mode.value() != reference.mode()) {
        return RoundTripResult<Json>::err(memory_resource(), RoundTripReferenceError::invalid());
    }
    return RoundTripResult<Json>::ok(memory_resource(),
                                     ReferenceWireCodec::serialize(memory_resource(), reference));
}

RoundTripResult<ExternalEntityReference>
UserContext::deserialize_reference(const Json& value) const {
    auto mode = reference_mode();
    if (!mode) {
        return RoundTripResult<ExternalEntityReference>::err(memory_resource(), mode.error());
    }
    return ReferenceWireCodec::deserialize(memory_resource(), value, mode.value());
}
}
