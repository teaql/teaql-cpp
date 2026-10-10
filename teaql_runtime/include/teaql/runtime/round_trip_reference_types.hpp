#pragma once
#include <teaql/core/json.hpp>
#include <teaql/runtime/business_clock.hpp>
#include <teaql/memory/scoped.hpp>
#include <array>
#include <exception>

namespace teaql {
inline constexpr std::string_view UNSAFE_EXPOSE_RAW_ENTITY_IDS_ENVIRONMENT =
    "TEAQL_UNSAFE_EXPOSE_RAW_ENTITY_IDS";
inline constexpr std::string_view UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT =
    "I_UNDERSTAND_THIS_EXPOSES_INTERNAL_ENTITY_IDS_FOR_LOCAL_DEBUGGING_ONLY";
enum class DeploymentProfile { Development, Test, Production };
enum class ReferenceMode { Governed, Raw };
std::string_view deployment_profile_label(DeploymentProfile) noexcept;

class RoundTripReferenceError final : public std::exception {
public:
    using allocator_type = Allocator;
    enum class Kind { Invalid, Expired, ScopeMismatch, AuthorizationRequired, Configuration };

    RoundTripReferenceError(std::allocator_arg_t,
                            Allocator,
                            const RoundTripReferenceError& other) noexcept
        : kind_(other.kind_) {}

    RoundTripReferenceError(std::allocator_arg_t,
                            Allocator,
                            RoundTripReferenceError&& other) noexcept
        : kind_(other.kind_) {}

    static RoundTripReferenceError invalid() noexcept {
        return RoundTripReferenceError(Kind::Invalid);
    }

    static RoundTripReferenceError expired() noexcept {
        return RoundTripReferenceError(Kind::Expired);
    }

    static RoundTripReferenceError scope_mismatch() noexcept {
        return RoundTripReferenceError(Kind::ScopeMismatch);
    }

    static RoundTripReferenceError authorization_required() noexcept {
        return RoundTripReferenceError(Kind::AuthorizationRequired);
    }

    static RoundTripReferenceError configuration() noexcept {
        return RoundTripReferenceError(Kind::Configuration);
    }

    std::string_view code() const noexcept;

    const std::exception* source() const noexcept {
        return nullptr;
    }

    const char* what() const noexcept override {
        return code().data();
    }

    bool operator==(const RoundTripReferenceError& other) const noexcept {
        return kind_ == other.kind_;
    }

private:
    explicit RoundTripReferenceError(Kind kind) noexcept : kind_(kind) {}

    Kind kind_;
};

template <class T>
using RoundTripResult = Result<T, RoundTripReferenceError>;
using ReferenceAuthorizationResult = RoundTripResult<std::monostate>;

struct TrustedReferencePrincipal {
    using allocator_type = Allocator;
    String authentication_realm, subject, domain_root_type;
    std::uint64_t domain_root_id;
    TrustedReferencePrincipal(std::allocator_arg_t,
                              Allocator,
                              std::string_view realm,
                              std::string_view subject,
                              std::string_view root_type,
                              std::uint64_t root_id);
    TrustedReferencePrincipal(std::allocator_arg_t, Allocator, const TrustedReferencePrincipal&);

    TrustedReferencePrincipal(std::allocator_arg_t, Allocator a, TrustedReferencePrincipal&& other)
        : TrustedReferencePrincipal(detail::move_to(a.resource(), std::move(other))) {}

    TrustedReferencePrincipal(const TrustedReferencePrincipal&) = delete;
    TrustedReferencePrincipal(TrustedReferencePrincipal&&) noexcept = default;

    TrustedReferencePrincipal& operator=(TrustedReferencePrincipal&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static RoundTripResult<TrustedReferencePrincipal>
        create(Memory, std::string_view, std::string_view, std::string_view, std::uint64_t);
    void validate() const;

    Memory memory_resource() const {
        return authentication_realm.get_allocator().resource();
    }

    bool operator==(const TrustedReferencePrincipal&) const = default;
};

struct ReferenceDocumentScope {
    using allocator_type = Allocator;
    String document_id, purpose, aggregate_type;
    std::uint64_t aggregate_id;
    std::int64_t aggregate_revision;
    ReferenceDocumentScope(std::allocator_arg_t,
                           Allocator,
                           std::string_view document,
                           std::string_view purpose,
                           std::string_view aggregate_type,
                           std::uint64_t aggregate_id,
                           std::int64_t revision);
    ReferenceDocumentScope(std::allocator_arg_t, Allocator, const ReferenceDocumentScope&);

    ReferenceDocumentScope(std::allocator_arg_t, Allocator a, ReferenceDocumentScope&& other)
        : ReferenceDocumentScope(detail::move_to(a.resource(), std::move(other))) {}

    ReferenceDocumentScope(const ReferenceDocumentScope&) = delete;
    ReferenceDocumentScope(ReferenceDocumentScope&&) noexcept = default;

    ReferenceDocumentScope& operator=(ReferenceDocumentScope&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static RoundTripResult<ReferenceDocumentScope> create(
        Memory, std::string_view, std::string_view, std::string_view, std::uint64_t, std::int64_t);
    void validate() const;

    Memory memory_resource() const {
        return document_id.get_allocator().resource();
    }

    bool operator==(const ReferenceDocumentScope&) const = default;
};

struct ReferenceIdentity {
    using allocator_type = Allocator;
    String entity_type;
    std::uint64_t id;
    std::int64_t version;
    ReferenceIdentity(std::allocator_arg_t,
                      Allocator,
                      std::string_view entity,
                      std::uint64_t id,
                      std::int64_t version);
    ReferenceIdentity(std::allocator_arg_t, Allocator, const ReferenceIdentity&);

    ReferenceIdentity(std::allocator_arg_t, Allocator a, ReferenceIdentity&& other)
        : ReferenceIdentity(detail::move_to(a.resource(), std::move(other))) {}

    ReferenceIdentity(const ReferenceIdentity&) = delete;
    ReferenceIdentity(ReferenceIdentity&&) noexcept = default;

    ReferenceIdentity& operator=(ReferenceIdentity&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static RoundTripResult<ReferenceIdentity>
        create(Memory, std::string_view, std::uint64_t, std::int64_t);
    void validate() const;

    Memory memory_resource() const {
        return entity_type.get_allocator().resource();
    }

    bool operator==(const ReferenceIdentity&) const = default;
};

struct RawEntityReference {
    std::uint64_t id;
    std::int64_t version;
    bool operator==(const RawEntityReference&) const = default;
};

class ExternalEntityReference {
public:
    using allocator_type = Allocator;
    std::variant<String, RawEntityReference> data;

    ExternalEntityReference(std::allocator_arg_t, Allocator a, std::string_view token)
        : data(String(token, a.resource())), resource_(a.resource()) {}

    ExternalEntityReference(std::allocator_arg_t, Allocator a, RawEntityReference raw)
        : data(raw), resource_(a.resource()) {}

    ExternalEntityReference(std::allocator_arg_t, Allocator, const ExternalEntityReference&);

    ExternalEntityReference(std::allocator_arg_t, Allocator a, ExternalEntityReference&& other)
        : ExternalEntityReference(detail::move_to(a.resource(), std::move(other))) {}

    ExternalEntityReference(const ExternalEntityReference&) = delete;
    ExternalEntityReference(ExternalEntityReference&&) noexcept = default;

    ExternalEntityReference& operator=(ExternalEntityReference&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return resource_;
    }

    ReferenceMode mode() const {
        return std::holds_alternative<String>(data) ? ReferenceMode::Governed : ReferenceMode::Raw;
    }

    bool operator==(const ExternalEntityReference& other) const {
        return data == other.data;
    }

private:
    Memory resource_;
};

struct ResolvedEntityReference {
    using allocator_type = Allocator;
    ReferenceIdentity identity;
    ReferenceMode mode;
    std::optional<String> key_id;
    std::optional<UtcDateTime> issued_at, expires_at;
    ResolvedEntityReference(std::allocator_arg_t,
                            Allocator,
                            const ReferenceIdentity&,
                            ReferenceMode,
                            std::optional<std::string_view> key = {},
                            std::optional<UtcDateTime> issued = {},
                            std::optional<UtcDateTime> expires = {});
    ResolvedEntityReference(std::allocator_arg_t, Allocator, const ResolvedEntityReference&);

    ResolvedEntityReference(std::allocator_arg_t, Allocator a, ResolvedEntityReference&& other)
        : ResolvedEntityReference(detail::move_to(a.resource(), std::move(other))) {}

    ResolvedEntityReference(const ResolvedEntityReference&) = delete;
    ResolvedEntityReference(ResolvedEntityReference&&) noexcept = default;

    ResolvedEntityReference& operator=(ResolvedEntityReference&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return identity.memory_resource();
    }

    bool operator==(const ResolvedEntityReference&) const = default;
};

struct ReferenceStartupNotice {
    std::string_view level, message, deployment_profile, telemetry_key, telemetry_value;
    std::array<std::pair<std::string_view, std::string_view>, 2> response_headers;
    bool operator==(const ReferenceStartupNotice&) const = default;
};

class ReferenceWireCodec {
public:
    static Json serialize(Memory, const ExternalEntityReference&);
    static RoundTripResult<ExternalEntityReference> deserialize(Memory, const Json&, ReferenceMode);
};
class ContextBoundReferenceRuntime;
}
