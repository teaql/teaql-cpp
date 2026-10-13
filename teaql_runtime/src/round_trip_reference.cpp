#include <teaql/runtime/round_trip_reference.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>
#include <teaql/runtime/detail/context/identity.hpp>
#include <teaql/core/detail/hmac_sha256.hpp>
#include <bit>
#include <cstdio>

namespace teaql::detail {
void require_reference_text(std::string_view);
void require_reference_key_id(std::string_view);

struct RoundTripReferenceAccess {
    static const CryptoKey& secret(const ReferenceKey& key) {
        return key.master_.bytes;
    }
};
}

namespace teaql {
namespace {
using namespace detail;

void write_integer(CryptoBytes& out, std::uint64_t value, unsigned width = 8) {
    for (unsigned i = width; i > 0; --i) {
        out.push_back(static_cast<std::uint8_t>(value >> ((i - 1) * 8)));
    }
}

void write_bytes(CryptoBytes& out, CryptoView value) {
    if (value.size() > 65535) {
        throw RoundTripReferenceError::invalid();
    }
    write_integer(out, value.size(), 2);
    out.insert(out.end(), value.begin(), value.end());
}

void write_text(CryptoBytes& out, std::string_view value) {
    require_reference_text(value);
    write_bytes(out, reference_bytes(value));
}

CryptoView read_bytes(CryptoView input, std::size_t& offset, std::size_t count) {
    if (offset > input.size() || count > input.size() - offset) {
        throw RoundTripReferenceError::invalid();
    }
    auto result = input.subspan(offset, count);
    offset += count;
    return result;
}

std::uint64_t read_integer(CryptoView input, std::size_t& offset, unsigned width = 8) {
    auto bytes = read_bytes(input, offset, width);
    std::uint64_t value = 0;
    for (auto byte : bytes) {
        value = (value << 8) | byte;
    }
    return value;
}

String read_text(Memory r, CryptoView input, std::size_t& offset) {
    auto length = read_integer(input, offset, 2);
    auto bytes = read_bytes(input, offset, length);
    std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    require_reference_text(text);
    return String(text, r);
}

ReferenceSecret derive(Memory r,
                       const ReferenceKey& key,
                       std::string_view label,
                       std::string_view service,
                       std::string_view environment) {
    CryptoBytes info(r);
    write_bytes(info, reference_bytes(label));
    write_text(info, service);
    write_text(info, environment);
    write_text(info, key.key_id);
    auto output = hkdf_sha256_32(reference_bytes("teaql.round-trip-reference.v1"),
                                 RoundTripReferenceAccess::secret(key),
                                 info);
    ReferenceSecret result(output);
    wipe_bytes(output);
    return result;
}

CryptoKey fingerprint(Memory r,
                      const ReferenceKey& key,
                      std::string_view service,
                      std::string_view environment,
                      const TrustedReferencePrincipal& principal) {
    auto derived = derive(r, key, "actor-fingerprint", service, environment);
    CryptoBytes canonical(r);
    write_text(canonical, principal.authentication_realm);
    write_text(canonical, principal.subject);
    write_text(canonical, principal.domain_root_type);
    write_integer(canonical, principal.domain_root_id);
    return hmac_sha256(derived.bytes, canonical);
}

CryptoBytes aad(Memory r,
                const ReferenceKey& key,
                std::string_view service,
                std::string_view environment,
                std::string_view purpose) {
    CryptoBytes out(r);
    write_text(out, "tqr1");
    write_text(out, key.key_id);
    write_text(out, service);
    write_text(out, environment);
    write_text(out, purpose);
    return out;
}

struct Wipe {
    CryptoBytes& bytes;

    ~Wipe() {
        wipe_bytes(bytes);
    }
};
}

detail::ReferenceSecret
ContextBoundReferenceRuntime::document_derived_key(Memory r, const ReferenceKey& key) const {
    return derive(r, key, "document-snapshot-encryption", service_, environment_);
}

detail::CryptoKey ContextBoundReferenceRuntime::document_actor_fingerprint(
    Memory r, const ReferenceKey& key, const TrustedReferencePrincipal& principal) const {
    return fingerprint(r, key, service_, environment_, principal);
}

ContextBoundReferenceRuntime::ContextBoundReferenceRuntime(
    std::allocator_arg_t,
    Allocator a,
    DeploymentProfile profile,
    std::string_view service,
    std::string_view environment,
    const ResourceBorrow<const ReferenceKeyProvider>& keys,
    const ResourceBorrow<const ReferenceAuthorizationPolicy>& authorization,
    std::optional<std::string_view> acknowledgement)
    : profile_(profile), mode_(ReferenceMode::Governed), service_(service, a.resource()),
      environment_(environment, a.resource()), keys_(std::allocator_arg, a, keys),
      authorization_(std::allocator_arg, a, authorization) {
    try {
        detail::require_reference_text(service);
        detail::require_reference_text(environment);
    } catch (const RoundTripReferenceError&) {
        throw RoundTripReferenceError::configuration();
    }
    bool raw = acknowledgement == UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT;
    if (raw && profile == DeploymentProfile::Production) {
        throw RoundTripReferenceError::configuration();
    }
    mode_ = raw ? ReferenceMode::Raw : ReferenceMode::Governed;
    detail::ReferenceCallGuard guard(memory_resource(), memory_resource());
    auto current = keys_.get().current_key(memory_resource());
    if (!current) {
        throw current.error();
    }
}

ContextBoundReferenceRuntime::ContextBoundReferenceRuntime(
    std::allocator_arg_t, Allocator a, const ContextBoundReferenceRuntime& other)
    : profile_(other.profile_), mode_(other.mode_), service_(other.service_, a.resource()),
      environment_(other.environment_, a.resource()), keys_(std::allocator_arg, a, other.keys_),
      authorization_(std::allocator_arg, a, other.authorization_),
      clock_(detail::clone_in(a.resource(), other.clock_)),
      nonce_(detail::clone_in(a.resource(), other.nonce_)) {}

RoundTripResult<ContextBoundReferenceRuntime> ContextBoundReferenceRuntime::governed(
    Memory r,
    DeploymentProfile profile,
    std::string_view service,
    std::string_view environment,
    const ResourceBorrow<const ReferenceKeyProvider>& keys,
    const ResourceBorrow<const ReferenceAuthorizationPolicy>& authorization) {
    try {
        return RoundTripResult<ContextBoundReferenceRuntime>::ok(
            r,
            ContextBoundReferenceRuntime(std::allocator_arg,
                                         Allocator(r),
                                         profile,
                                         service,
                                         environment,
                                         keys,
                                         authorization));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<ContextBoundReferenceRuntime>::err(r, error);
    }
}

RoundTripResult<ContextBoundReferenceRuntime>
ContextBoundReferenceRuntime::from_process_environment(
    Memory r,
    DeploymentProfile profile,
    std::string_view service,
    std::string_view environment,
    const ResourceBorrow<const ReferenceKeyProvider>& keys,
    const ResourceBorrow<const ReferenceAuthorizationPolicy>& authorization) {
    auto* text = std::getenv(UNSAFE_EXPOSE_RAW_ENTITY_IDS_ENVIRONMENT.data());
    std::optional<std::string_view> acknowledgement;
    if (text && detail::identity_utf8(text)) {
        acknowledgement = text;
    }
    try {
        ContextBoundReferenceRuntime result(std::allocator_arg,
                                            Allocator(r),
                                            profile,
                                            service,
                                            environment,
                                            keys,
                                            authorization,
                                            acknowledgement);
        if (auto notice = result.startup_notice()) {
            std::fprintf(stderr,
                         "%.*s: %.*s; deployment_profile=%.*s; %.*s=%.*s\n",
                         int(notice->level.size()),
                         notice->level.data(),
                         int(notice->message.size()),
                         notice->message.data(),
                         int(notice->deployment_profile.size()),
                         notice->deployment_profile.data(),
                         int(notice->telemetry_key.size()),
                         notice->telemetry_key.data(),
                         int(notice->telemetry_value.size()),
                         notice->telemetry_value.data());
        }
        return RoundTripResult<ContextBoundReferenceRuntime>::ok(r, std::move(result));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<ContextBoundReferenceRuntime>::err(r, error);
    }
}

std::optional<ReferenceStartupNotice>
ContextBoundReferenceRuntime::startup_notice() const noexcept {
    if (mode_ != ReferenceMode::Raw) {
        return {};
    }
    return ReferenceStartupNotice{
        "ERROR",
        "TeaQL raw internal entity reference mode is active for local diagnostics",
        deployment_profile_label(profile_),
        "teaql.reference.mode",
        "raw",
        {{{"TeaQL-Reference-Mode", "raw-internal-id"}, {"Cache-Control", "no-store"}}}};
}

RoundTripResult<ExternalEntityReference>
ContextBoundReferenceRuntime::issue(Memory r,
                                    const TrustedReferencePrincipal& principal,
                                    const ReferenceDocumentScope& scope,
                                    const ReferenceIdentity& identity,
                                    ReferenceLifetime lifetime) const {
    detail::ReferenceCallGuard guard(memory_resource(), r);
    try {
        principal.validate();
        scope.validate();
        identity.validate();
        if (lifetime.seconds == 0) {
            throw RoundTripReferenceError::invalid();
        }
        auto allowed = authorization_.get().authorize(r, principal, scope, identity);
        if (!allowed) {
            throw allowed.error();
        }
        if (mode_ == ReferenceMode::Raw) {
            return RoundTripResult<ExternalEntityReference>::ok(
                r,
                ExternalEntityReference(std::allocator_arg,
                                        Allocator(r),
                                        RawEntityReference{identity.id, identity.version}));
        }
        return RoundTripResult<ExternalEntityReference>::ok(
            r, issue_governed(r, principal, scope, identity, lifetime));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<ExternalEntityReference>::err(r, error);
    }
}

ExternalEntityReference
ContextBoundReferenceRuntime::issue_governed(Memory r,
                                             const TrustedReferencePrincipal& principal,
                                             const ReferenceDocumentScope& scope,
                                             const ReferenceIdentity& identity,
                                             ReferenceLifetime lifetime) const {
    auto selected = keys_.get().current_key(r);
    if (!selected) {
        throw selected.error();
    }
    const auto& key = selected.value();
    auto now = clock_ ? (*clock_)() : UtcDateTime::now();
    if (!detail::reference_duration_valid(lifetime)) {
        throw RoundTripReferenceError::invalid();
    }
    auto expires = [&] {
        try {
            return detail::reference_add(now, lifetime);
        } catch (const std::out_of_range&) {
            throw RoundTripReferenceError::invalid();
        }
    }();
    auto actor = fingerprint(r, key, service_, environment_, principal);
    detail::CryptoBytes plaintext(actor.begin(), actor.end(), r);
    Wipe wipe{plaintext};
    detail::wipe_bytes(actor);
    write_text(plaintext, identity.entity_type);
    write_integer(plaintext, identity.id);
    write_integer(plaintext, std::bit_cast<std::uint64_t>(identity.version));
    write_text(plaintext, scope.document_id);
    write_text(plaintext, scope.aggregate_type);
    write_integer(plaintext, scope.aggregate_id);
    write_integer(plaintext, std::bit_cast<std::uint64_t>(scope.aggregate_revision));
    write_integer(plaintext, std::bit_cast<std::uint64_t>(now.timestamp()));
    write_integer(plaintext, std::bit_cast<std::uint64_t>(expires.timestamp()));
    auto encryption = derive(r, key, "entity-reference-encryption", service_, environment_);
    auto bound_aad = aad(r, key, service_, environment_, scope.purpose);
    auto nonce = nonce_ ? (*nonce_)() : detail::reference_nonce();
    auto encrypted = detail::aes256_gcm_encrypt(r, encryption.bytes, nonce, plaintext, bound_aad);
    detail::CryptoBytes envelope(nonce.begin(), nonce.end(), r);
    envelope.insert(envelope.end(), encrypted.begin(), encrypted.end());
    String token("tqr1.", r);
    token += key.key_id;
    token += '.';
    token += detail::reference_base64(r, envelope);
    return ExternalEntityReference(std::allocator_arg, Allocator(r), token);
}

RoundTripResult<ResolvedEntityReference>
ContextBoundReferenceRuntime::consume(Memory r,
                                      const TrustedReferencePrincipal& principal,
                                      const ReferenceDocumentScope& scope,
                                      const ExternalEntityReference& reference,
                                      std::string_view entity) const {
    detail::ReferenceCallGuard guard(memory_resource(), r);
    try {
        principal.validate();
        scope.validate();
        detail::require_reference_text(entity);
        auto resolved = [&] {
            if (reference.mode() != mode_) {
                throw RoundTripReferenceError::invalid();
            }
            if (auto* raw = std::get_if<RawEntityReference>(&reference.data)) {
                ReferenceIdentity identity(
                    std::allocator_arg, Allocator(r), entity, raw->id, raw->version);
                identity.validate();
                return ResolvedEntityReference(
                    std::allocator_arg, Allocator(r), identity, ReferenceMode::Raw);
            }
            return consume_governed(r, principal, scope, std::get<String>(reference.data), entity);
        }();
        auto allowed = authorization_.get().authorize(r, principal, scope, resolved.identity);
        if (!allowed) {
            throw allowed.error();
        }
        return RoundTripResult<ResolvedEntityReference>::ok(r, std::move(resolved));
    } catch (const RoundTripReferenceError& error) {
        return RoundTripResult<ResolvedEntityReference>::err(r, error);
    }
}

ResolvedEntityReference
ContextBoundReferenceRuntime::consume_governed(Memory r,
                                               const TrustedReferencePrincipal& principal,
                                               const ReferenceDocumentScope& scope,
                                               std::string_view token,
                                               std::string_view entity) const {
    if (token.size() > 24000 || !token.starts_with("tqr1.")) {
        throw RoundTripReferenceError::invalid();
    }
    auto separator = token.find('.', 5);
    if (separator == std::string_view::npos ||
        token.find('.', separator + 1) != std::string_view::npos) {
        throw RoundTripReferenceError::invalid();
    }
    auto id = token.substr(5, separator - 5);
    try {
        detail::require_reference_key_id(id);
    } catch (const RoundTripReferenceError&) {
        throw RoundTripReferenceError::invalid();
    }
    auto selected = keys_.get().key_by_id(r, id);
    if (!selected) {
        throw selected.error();
    }
    if (!selected.value()) {
        throw RoundTripReferenceError::invalid();
    }
    const auto& key = *selected.value();
    auto envelope = detail::reference_unbase64(r, token.substr(separator + 1));
    if (!envelope || envelope->size() < 28) {
        throw RoundTripReferenceError::invalid();
    }
    auto encryption = derive(r, key, "entity-reference-encryption", service_, environment_);
    auto bound_aad = aad(r, key, service_, environment_, scope.purpose);
    detail::CryptoNonce nonce{};
    std::copy_n(envelope->begin(), nonce.size(), nonce.begin());
    auto plaintext = detail::aes256_gcm_decrypt(
        r, encryption.bytes, nonce, detail::CryptoView(*envelope).subspan(12), bound_aad);
    if (!plaintext) {
        throw RoundTripReferenceError::invalid();
    }
    Wipe wipe{*plaintext};
    std::size_t offset = 0;
    auto actor = read_bytes(*plaintext, offset, 32);
    auto entity_type = read_text(r, *plaintext, offset);
    auto identity_id = read_integer(*plaintext, offset);
    auto version = std::bit_cast<std::int64_t>(read_integer(*plaintext, offset));
    auto document_id = read_text(r, *plaintext, offset);
    auto aggregate_type = read_text(r, *plaintext, offset);
    auto aggregate_id = read_integer(*plaintext, offset);
    auto revision = std::bit_cast<std::int64_t>(read_integer(*plaintext, offset));
    auto issued = std::bit_cast<std::int64_t>(read_integer(*plaintext, offset));
    auto expires = std::bit_cast<std::int64_t>(read_integer(*plaintext, offset));
    if (offset != plaintext->size()) {
        throw RoundTripReferenceError::invalid();
    }
    ReferenceIdentity identity(std::allocator_arg, Allocator(r), entity_type, identity_id, version);
    identity.validate();
    auto now = clock_ ? (*clock_)() : UtcDateTime::now();
    if (expires <= now.timestamp()) {
        throw RoundTripReferenceError::expired();
    }
    if (issued > now.timestamp() + 60) {
        throw RoundTripReferenceError::invalid();
    }
    auto expected = fingerprint(r, key, service_, environment_, principal);
    std::uint8_t difference = 0;
    for (unsigned i = 0; i < 32; ++i) {
        difference |= actor[i] ^ expected[i];
    }
    detail::wipe_bytes(expected);
    if (difference || document_id != scope.document_id || aggregate_type != scope.aggregate_type ||
        aggregate_id != scope.aggregate_id || revision != scope.aggregate_revision ||
        entity_type != entity) {
        throw RoundTripReferenceError::scope_mismatch();
    }
    return ResolvedEntityReference(std::allocator_arg,
                                   Allocator(r),
                                   identity,
                                   ReferenceMode::Governed,
                                   key.key_id,
                                   UtcDateTime::from_timestamp(issued, 0),
                                   UtcDateTime::from_timestamp(expires, 0));
}
}
