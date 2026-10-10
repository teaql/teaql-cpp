#pragma once
#include "entity_reference_fixture.hpp"
#include <teaql/runtime/round_trip_reference.hpp>

namespace teaql::round_trip_fixture {
struct Policy final : ReferenceAuthorizationPolicy {
    using allocator_type = Allocator;
    Memory resource;
    mutable Vector<String> calls;
    bool allowed = true;

    Policy(std::allocator_arg_t, Allocator a) : resource(a.resource()), calls(a.resource()) {}

    Policy(std::allocator_arg_t, Allocator a, const Policy& other)
        : resource(a.resource()), calls(other.calls, a.resource()), allowed(other.allowed) {}

    ReferenceAuthorizationResult authorize(Memory r,
                                           const TrustedReferencePrincipal& principal,
                                           const ReferenceDocumentScope& scope,
                                           const ReferenceIdentity& identity) const override {
        String call(principal.subject, resource);
        call += ':';
        call += scope.document_id;
        call += ':';
        call += identity.entity_type;
        calls.emplace_back(std::move(call));
        if (allowed && principal.subject == "alice" && identity.entity_type == "OrderItem" &&
            identity.id == 42) {
            return ReferenceAuthorizationResult::ok(r, {});
        }
        return ReferenceAuthorizationResult::err(r,
                                                 RoundTripReferenceError::authorization_required());
    }
};

inline Vector<ReferenceKey> keys(Memory r) {
    Vector<ReferenceKey> out(r);
    detail::CryptoKey key;
    key.fill(0x11);
    out.emplace_back("k1", key);
    key.fill(0x22);
    out.emplace_back("k2", key);
    return out;
}

inline auto principal(Memory r) {
    return TrustedReferencePrincipal(
        std::allocator_arg, Allocator(r), "oidc", "alice", "Platform", 7);
}

inline auto scope(Memory r) {
    return ReferenceDocumentScope(
        std::allocator_arg, Allocator(r), "doc-100", "edit-order", "Order", 100, 9);
}

inline auto identity(Memory r) {
    return ReferenceIdentity(std::allocator_arg, Allocator(r), "OrderItem", 42, 3);
}

inline auto now() {
    return *UtcDateTime::from_ymd_hms_nano(2026, 9, 27, 0, 0, 0, 0);
}

inline void configure(ContextBoundReferenceRuntime& runtime, UtcDateTime time = now()) {
    detail::CryptoNonce nonce;
    nonce.fill(0x33);
    runtime.with_clock(
        reference_fixture::Clock(std::allocator_arg, Allocator(runtime.memory_resource()), time));
    runtime.with_nonce_source(
        reference_fixture::Nonce(std::allocator_arg, Allocator(runtime.memory_resource()), nonce));
}
}
