#pragma once
#include <teaql/runtime/round_trip_reference_types.hpp>
#include <teaql/runtime/entity_reference.hpp>
#include <teaql/runtime/resources.hpp>
#include <teaql/runtime/document_round_trip.hpp>

namespace teaql {
namespace detail {
struct RoundTripReferenceAccess;
}

class ReferenceKey {
public:
    using allocator_type = Allocator;
    String key_id;
    ReferenceKey(std::allocator_arg_t, Allocator, std::string_view, detail::CryptoView master);
    ReferenceKey(std::allocator_arg_t, Allocator, const ReferenceKey&);

    ReferenceKey(std::allocator_arg_t, Allocator a, ReferenceKey&& other)
        : ReferenceKey(detail::move_to(a.resource(), std::move(other))) {}

    ReferenceKey(const ReferenceKey&) = delete;
    ReferenceKey(ReferenceKey&&) noexcept = default;

    ReferenceKey& operator=(ReferenceKey&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static RoundTripResult<ReferenceKey> create(Memory, std::string_view, detail::CryptoView);
    String debug(Memory) const;

    Memory memory_resource() const {
        return key_id.get_allocator().resource();
    }

private:
    friend struct detail::RoundTripReferenceAccess;
    detail::ReferenceSecret master_;
};

class ReferenceKeyProvider {
public:
    virtual ~ReferenceKeyProvider() = default;
    virtual RoundTripResult<ReferenceKey> current_key(Memory destination) const = 0;
    virtual RoundTripResult<std::optional<ReferenceKey>> key_by_id(Memory destination,
                                                                   std::string_view) const = 0;
};

class StaticReferenceKeyProvider final : public ReferenceKeyProvider {
public:
    using allocator_type = Allocator;
    StaticReferenceKeyProvider(std::allocator_arg_t,
                               Allocator,
                               std::string_view current_key_id,
                               std::span<const ReferenceKey>);
    StaticReferenceKeyProvider(std::allocator_arg_t, Allocator, const StaticReferenceKeyProvider&);

    StaticReferenceKeyProvider(std::allocator_arg_t,
                               Allocator a,
                               StaticReferenceKeyProvider&& other)
        : StaticReferenceKeyProvider(detail::move_to(a.resource(), std::move(other))) {}

    StaticReferenceKeyProvider(const StaticReferenceKeyProvider&) = delete;
    StaticReferenceKeyProvider(StaticReferenceKeyProvider&&) noexcept = default;
    static RoundTripResult<StaticReferenceKeyProvider>
        create(Memory, std::string_view, std::span<const ReferenceKey>);
    RoundTripResult<ReferenceKey> current_key(Memory) const override;
    RoundTripResult<std::optional<ReferenceKey>> key_by_id(Memory, std::string_view) const override;

    Memory memory_resource() const {
        return current_key_id_.get_allocator().resource();
    }

private:
    String current_key_id_;
    std::pmr::map<String, ReferenceKey, std::less<>> keys_;
};

class ReferenceAuthorizationPolicy {
public:
    virtual ~ReferenceAuthorizationPolicy() = default;
    virtual ReferenceAuthorizationResult authorize(Memory destination,
                                                   const TrustedReferencePrincipal&,
                                                   const ReferenceDocumentScope&,
                                                   const ReferenceIdentity&) const = 0;
};

// C++ equivalent of the source blanket Fn implementation, with scoped captures.
template <class F>
class FunctionReferenceAuthorizationPolicy final : public ReferenceAuthorizationPolicy {
public:
    using allocator_type = Allocator;

    FunctionReferenceAuthorizationPolicy(std::allocator_arg_t, Allocator a, F function)
        : resource_(a.resource()),
          function_(std::make_obj_using_allocator<F>(a, std::move(function))) {
        static_assert(std::uses_allocator_v<F, Allocator> || std::is_trivially_destructible_v<F>);
    }

    FunctionReferenceAuthorizationPolicy(std::allocator_arg_t,
                                         Allocator a,
                                         const FunctionReferenceAuthorizationPolicy& other)
        : resource_(a.resource()),
          function_(detail::copy_callable(a.resource(), other.resource_, other.function_)) {}

    ReferenceAuthorizationResult authorize(Memory r,
                                           const TrustedReferencePrincipal& principal,
                                           const ReferenceDocumentScope& scope,
                                           const ReferenceIdentity& identity) const override {
        return function_(r, principal, scope, identity);
    }

    Memory memory_resource() const {
        return resource_;
    }

private:
    Memory resource_;
    F function_;
};

class ContextBoundReferenceRuntime {
public:
    using allocator_type = Allocator;
    ContextBoundReferenceRuntime(std::allocator_arg_t,
                                 Allocator,
                                 DeploymentProfile,
                                 std::string_view service,
                                 std::string_view environment,
                                 const ResourceBorrow<const ReferenceKeyProvider>&,
                                 const ResourceBorrow<const ReferenceAuthorizationPolicy>&,
                                 std::optional<std::string_view> acknowledgement = {});
    ContextBoundReferenceRuntime(std::allocator_arg_t,
                                 Allocator,
                                 const ContextBoundReferenceRuntime&);

    ContextBoundReferenceRuntime(std::allocator_arg_t,
                                 Allocator a,
                                 ContextBoundReferenceRuntime&& other)
        : ContextBoundReferenceRuntime(detail::move_to(a.resource(), std::move(other))) {}

    ContextBoundReferenceRuntime(const ContextBoundReferenceRuntime&) = delete;
    ContextBoundReferenceRuntime(ContextBoundReferenceRuntime&&) noexcept = default;
    static RoundTripResult<ContextBoundReferenceRuntime>
    governed(Memory,
             DeploymentProfile,
             std::string_view service,
             std::string_view environment,
             const ResourceBorrow<const ReferenceKeyProvider>&,
             const ResourceBorrow<const ReferenceAuthorizationPolicy>&);
    static RoundTripResult<ContextBoundReferenceRuntime>
    from_process_environment(Memory,
                             DeploymentProfile,
                             std::string_view service,
                             std::string_view environment,
                             const ResourceBorrow<const ReferenceKeyProvider>&,
                             const ResourceBorrow<const ReferenceAuthorizationPolicy>&);

    DeploymentProfile deployment_profile() const noexcept {
        return profile_;
    }

    ReferenceMode mode() const noexcept {
        return mode_;
    }

    std::optional<ReferenceStartupNotice> startup_notice() const noexcept;

    template <class F>
    ContextBoundReferenceRuntime& with_clock(F&& clock) {
        clock_.emplace(std::allocator_arg, Allocator(memory_resource()), std::forward<F>(clock));
        return *this;
    }

    template <class F>
    ContextBoundReferenceRuntime& with_nonce_source(F&& source) {
        nonce_.emplace(std::allocator_arg, Allocator(memory_resource()), std::forward<F>(source));
        return *this;
    }

    RoundTripResult<ExternalEntityReference> issue(Memory destination,
                                                   const TrustedReferencePrincipal&,
                                                   const ReferenceDocumentScope&,
                                                   const ReferenceIdentity&,
                                                   ReferenceLifetime) const;
    RoundTripResult<ResolvedEntityReference> consume(Memory destination,
                                                     const TrustedReferencePrincipal&,
                                                     const ReferenceDocumentScope&,
                                                     const ExternalEntityReference&,
                                                     std::string_view expected_entity_type) const;
    DocumentResult<String> issue_document_snapshot(Memory destination,
                                                   const TrustedReferencePrincipal&,
                                                   const DocumentSnapshot&,
                                                   ReferenceLifetime) const;
    DocumentResult<VerifiedDocumentSnapshot>
    consume_document_snapshot(Memory destination,
                              const TrustedReferencePrincipal&,
                              std::string_view token,
                              std::string_view expected_aggregate_type,
                              std::string_view expected_purpose) const;

    Memory memory_resource() const {
        return service_.get_allocator().resource();
    }

private:
    detail::ReferenceSecret document_derived_key(Memory, const ReferenceKey&) const;
    detail::CryptoKey
    document_actor_fingerprint(Memory, const ReferenceKey&, const TrustedReferencePrincipal&) const;
    DeploymentProfile profile_;
    ReferenceMode mode_;
    String service_, environment_;
    ResourceBorrow<const ReferenceKeyProvider> keys_;
    ResourceBorrow<const ReferenceAuthorizationPolicy> authorization_;
    std::optional<ContextFunction<UtcDateTime()>> clock_;
    std::optional<ContextFunction<detail::CryptoNonce()>> nonce_;
    ExternalEntityReference issue_governed(Memory,
                                           const TrustedReferencePrincipal&,
                                           const ReferenceDocumentScope&,
                                           const ReferenceIdentity&,
                                           ReferenceLifetime) const;
    ResolvedEntityReference consume_governed(Memory,
                                             const TrustedReferencePrincipal&,
                                             const ReferenceDocumentScope&,
                                             std::string_view token,
                                             std::string_view expected_entity_type) const;
};
}
