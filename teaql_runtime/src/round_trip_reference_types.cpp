#include <teaql/runtime/round_trip_reference.hpp>
#include <teaql/runtime/reference_model_debug.hpp>
#include <teaql/core/detail/text.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/runtime/detail/context/identity.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>

namespace teaql {
namespace detail {
void require_reference_text(std::string_view text) {
    auto trimmed = text;
    trim(trimmed);
    if (text.empty() || text.size() > 4096 || trimmed != text || !identity_utf8(text)) {
        throw RoundTripReferenceError::invalid();
    }
}

void require_reference_key_id(std::string_view id) {
    if (id.empty() || id.size() > 64 || !std::all_of(id.begin(), id.end(), [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                   c == '-' || c == '_';
        })) {
        throw RoundTripReferenceError::configuration();
    }
}
}

std::string_view deployment_profile_label(DeploymentProfile profile) noexcept {
    switch (profile) {
    case DeploymentProfile::Development:
        return "development";
    case DeploymentProfile::Test:
        return "test";
    case DeploymentProfile::Production:
        return "production";
    }
    std::terminate();
}

std::string_view RoundTripReferenceError::code() const noexcept {
    switch (kind_) {
    case Kind::Invalid:
        return "ROUND_TRIP_REFERENCE_INVALID";
    case Kind::Expired:
        return "ROUND_TRIP_REFERENCE_EXPIRED";
    case Kind::ScopeMismatch:
        return "ROUND_TRIP_REFERENCE_SCOPE_MISMATCH";
    case Kind::AuthorizationRequired:
        return "ROUND_TRIP_REFERENCE_AUTHORIZATION_REQUIRED";
    case Kind::Configuration:
        return "ROUND_TRIP_REFERENCE_CONFIGURATION_INVALID";
    }
    std::terminate();
}

TrustedReferencePrincipal::TrustedReferencePrincipal(std::allocator_arg_t,
                                                     Allocator a,
                                                     std::string_view realm,
                                                     std::string_view subject,
                                                     std::string_view root_type,
                                                     std::uint64_t root_id)
    : authentication_realm(realm, a.resource()), subject(subject, a.resource()),
      domain_root_type(root_type, a.resource()), domain_root_id(root_id) {}

TrustedReferencePrincipal::TrustedReferencePrincipal(std::allocator_arg_t,
                                                     Allocator a,
                                                     const TrustedReferencePrincipal& other)
    : TrustedReferencePrincipal(std::allocator_arg,
                                a,
                                other.authentication_realm,
                                other.subject,
                                other.domain_root_type,
                                other.domain_root_id) {}

void TrustedReferencePrincipal::validate() const {
    detail::require_reference_text(authentication_realm);
    detail::require_reference_text(subject);
    detail::require_reference_text(domain_root_type);
    if (!domain_root_id) {
        throw RoundTripReferenceError::invalid();
    }
}

RoundTripResult<TrustedReferencePrincipal>
TrustedReferencePrincipal::create(Memory r,
                                  std::string_view realm,
                                  std::string_view subject,
                                  std::string_view root_type,
                                  std::uint64_t root_id) {
    try {
        TrustedReferencePrincipal result(
            std::allocator_arg, Allocator(r), realm, subject, root_type, root_id);
        result.validate();
        return RoundTripResult<TrustedReferencePrincipal>::ok(r, std::move(result));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<TrustedReferencePrincipal>::err(r, error);
    }
}

ReferenceDocumentScope::ReferenceDocumentScope(std::allocator_arg_t,
                                               Allocator a,
                                               std::string_view document,
                                               std::string_view purpose,
                                               std::string_view aggregate,
                                               std::uint64_t id,
                                               std::int64_t revision)
    : document_id(document, a.resource()), purpose(purpose, a.resource()),
      aggregate_type(aggregate, a.resource()), aggregate_id(id), aggregate_revision(revision) {}

ReferenceDocumentScope::ReferenceDocumentScope(std::allocator_arg_t,
                                               Allocator a,
                                               const ReferenceDocumentScope& other)
    : ReferenceDocumentScope(std::allocator_arg,
                             a,
                             other.document_id,
                             other.purpose,
                             other.aggregate_type,
                             other.aggregate_id,
                             other.aggregate_revision) {}

void ReferenceDocumentScope::validate() const {
    detail::require_reference_text(document_id);
    detail::require_reference_text(purpose);
    detail::require_reference_text(aggregate_type);
    if (!aggregate_id || aggregate_revision < 0) {
        throw RoundTripReferenceError::invalid();
    }
}

RoundTripResult<ReferenceDocumentScope> ReferenceDocumentScope::create(Memory r,
                                                                       std::string_view document,
                                                                       std::string_view purpose,
                                                                       std::string_view aggregate,
                                                                       std::uint64_t id,
                                                                       std::int64_t revision) {
    try {
        ReferenceDocumentScope result(
            std::allocator_arg, Allocator(r), document, purpose, aggregate, id, revision);
        result.validate();
        return RoundTripResult<ReferenceDocumentScope>::ok(r, std::move(result));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<ReferenceDocumentScope>::err(r, error);
    }
}

ReferenceIdentity::ReferenceIdentity(std::allocator_arg_t,
                                     Allocator a,
                                     std::string_view entity,
                                     std::uint64_t id,
                                     std::int64_t version)
    : entity_type(entity, a.resource()), id(id), version(version) {}

ReferenceIdentity::ReferenceIdentity(std::allocator_arg_t,
                                     Allocator a,
                                     const ReferenceIdentity& other)
    : ReferenceIdentity(std::allocator_arg, a, other.entity_type, other.id, other.version) {}

void ReferenceIdentity::validate() const {
    detail::require_reference_text(entity_type);
    if (!id || version < 0) {
        throw RoundTripReferenceError::invalid();
    }
}

RoundTripResult<ReferenceIdentity> ReferenceIdentity::create(Memory r,
                                                             std::string_view entity,
                                                             std::uint64_t id,
                                                             std::int64_t version) {
    try {
        ReferenceIdentity result(std::allocator_arg, Allocator(r), entity, id, version);
        result.validate();
        return RoundTripResult<ReferenceIdentity>::ok(r, std::move(result));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<ReferenceIdentity>::err(r, error);
    }
}

ExternalEntityReference::ExternalEntityReference(std::allocator_arg_t,
                                                 Allocator a,
                                                 const ExternalEntityReference& other)
    : data(std::in_place_index<1>, RawEntityReference{}), resource_(a.resource()) {
    if (auto* token = std::get_if<String>(&other.data)) {
        data.emplace<String>(*token, a.resource());
    } else {
        data = std::get<RawEntityReference>(other.data);
    }
}

ResolvedEntityReference::ResolvedEntityReference(std::allocator_arg_t,
                                                 Allocator a,
                                                 const ReferenceIdentity& identity,
                                                 ReferenceMode mode,
                                                 std::optional<std::string_view> key,
                                                 std::optional<UtcDateTime> issued,
                                                 std::optional<UtcDateTime> expires)
    : identity(std::allocator_arg, a, identity), mode(mode), issued_at(issued),
      expires_at(expires) {
    if (key) {
        key_id.emplace(*key, a.resource());
    }
}

ResolvedEntityReference::ResolvedEntityReference(std::allocator_arg_t,
                                                 Allocator a,
                                                 const ResolvedEntityReference& other)
    : identity(std::allocator_arg, a, other.identity), mode(other.mode),
      key_id(detail::clone_in(a.resource(), other.key_id)), issued_at(other.issued_at),
      expires_at(other.expires_at) {}

Json ReferenceWireCodec::serialize(Memory r, const ExternalEntityReference& reference) {
    if (auto* token = std::get_if<String>(&reference.data)) {
        return Json(r, *token);
    }
    auto raw = std::get<RawEntityReference>(reference.data);
    Json::Object object(r);
    object.emplace(String("id", r), Json(r, raw.id));
    object.emplace(String("version", r), Json(r, raw.version));
    return Json(r, object);
}

RoundTripResult<ExternalEntityReference>
ReferenceWireCodec::deserialize(Memory r, const Json& value, ReferenceMode mode) {
    using Output = RoundTripResult<ExternalEntityReference>;
    if (mode == ReferenceMode::Governed) {
        if (auto* token = std::get_if<String>(&value.data); token && token->starts_with("tqr1.")) {
            return Output::ok(r, ExternalEntityReference(std::allocator_arg, Allocator(r), *token));
        }
    } else if (auto* object = std::get_if<Json::Object>(&value.data);
               object && object->size() == 2) {
        auto id = object->find("id"), version = object->find("version");
        if (id != object->end() && version != object->end()) {
            std::optional<std::uint64_t> identity;
            std::optional<std::int64_t> revision;
            if (auto* v = std::get_if<std::uint64_t>(&id->second.data)) {
                identity = *v;
            }
            if (auto* v = std::get_if<std::int64_t>(&id->second.data); v && *v >= 0) {
                identity = *v;
            }
            if (auto* v = std::get_if<std::int64_t>(&version->second.data)) {
                revision = *v;
            }
            if (auto* v = std::get_if<std::uint64_t>(&version->second.data);
                v && *v <= std::uint64_t(std::numeric_limits<std::int64_t>::max())) {
                revision = *v;
            }
            if (identity && *identity && revision && *revision >= 0) {
                return Output::ok(
                    r,
                    ExternalEntityReference(std::allocator_arg,
                                            Allocator(r),
                                            RawEntityReference{*identity, *revision}));
            }
        }
    }
    return Output::err(r, RoundTripReferenceError::invalid());
}

namespace {
detail::ReferenceSecret key_secret(std::string_view id, detail::CryptoView bytes) {
    detail::require_reference_key_id(id);
    if (bytes.size() != 32) {
        throw RoundTripReferenceError::configuration();
    }
    return detail::ReferenceSecret(bytes);
}
}

ReferenceKey::ReferenceKey(std::allocator_arg_t,
                           Allocator a,
                           std::string_view id,
                           detail::CryptoView master)
    : key_id(id, a.resource()), master_(key_secret(id, master)) {}

ReferenceKey::ReferenceKey(std::allocator_arg_t, Allocator a, const ReferenceKey& other)
    : key_id(other.key_id, a.resource()), master_(other.master_) {}

RoundTripResult<ReferenceKey>
ReferenceKey::create(Memory r, std::string_view id, detail::CryptoView bytes) {
    try {
        return RoundTripResult<ReferenceKey>::ok(
            r, ReferenceKey(std::allocator_arg, Allocator(r), id, bytes));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<ReferenceKey>::err(r, error);
    }
}

String ReferenceKey::debug(Memory r) const {
    return debug_string(r, *this);
}

StaticReferenceKeyProvider::StaticReferenceKeyProvider(std::allocator_arg_t,
                                                       Allocator a,
                                                       std::string_view current,
                                                       std::span<const ReferenceKey> keys)
    : current_key_id_(current, a.resource()), keys_(a.resource()) {
    detail::require_reference_key_id(current);
    for (const auto& key : keys) {
        if (!keys_.try_emplace(String(key.key_id, a.resource()), key).second) {
            throw RoundTripReferenceError::configuration();
        }
    }
    if (!keys_.contains(current)) {
        throw RoundTripReferenceError::configuration();
    }
}

StaticReferenceKeyProvider::StaticReferenceKeyProvider(std::allocator_arg_t,
                                                       Allocator a,
                                                       const StaticReferenceKeyProvider& other)
    : current_key_id_(other.current_key_id_, a.resource()), keys_(other.keys_, a.resource()) {}

RoundTripResult<StaticReferenceKeyProvider> StaticReferenceKeyProvider::create(
    Memory r, std::string_view current, std::span<const ReferenceKey> keys) {
    try {
        return RoundTripResult<StaticReferenceKeyProvider>::ok(
            r, StaticReferenceKeyProvider(std::allocator_arg, Allocator(r), current, keys));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<StaticReferenceKeyProvider>::err(r, error);
    }
}

RoundTripResult<ReferenceKey> StaticReferenceKeyProvider::current_key(Memory r) const {
    detail::ReferenceCallGuard guard(memory_resource(), r);
    auto found = keys_.find(current_key_id_);
    if (found == keys_.end()) {
        return RoundTripResult<ReferenceKey>::err(r, RoundTripReferenceError::configuration());
    }
    return RoundTripResult<ReferenceKey>::ok(
        r, ReferenceKey(std::allocator_arg, Allocator(r), found->second));
}

RoundTripResult<std::optional<ReferenceKey>>
StaticReferenceKeyProvider::key_by_id(Memory r, std::string_view id) const {
    detail::ReferenceCallGuard guard(memory_resource(), r);
    auto found = keys_.find(id);
    std::optional<ReferenceKey> result;
    if (found != keys_.end()) {
        result.emplace(std::allocator_arg, Allocator(r), found->second);
    }
    return RoundTripResult<std::optional<ReferenceKey>>::ok(r, std::move(result));
}
}
