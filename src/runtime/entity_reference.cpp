#include <teaql/runtime/entity_reference.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>
#include <teaql/core/detail/text.hpp>
#include <teaql/runtime/detail/context_identity.hpp>
#include <bit>
#include <limits>

namespace teaql {
namespace {
void write_integer(detail::CryptoBytes& bytes, std::uint64_t value, unsigned length = 8) {
    for (unsigned i = length; i > 0; --i) {
        bytes.push_back(static_cast<std::uint8_t>(value >> ((i - 1) * 8)));
    }
}

void write_text(detail::CryptoBytes& bytes, std::string_view value) {
    if (value.size() > std::numeric_limits<std::uint16_t>::max() || !detail::identity_utf8(value)) {
        throw EntityReferenceTokenError::invalid();
    }
    write_integer(bytes, value.size(), 2);
    bytes.insert(bytes.end(), value.begin(), value.end());
}

std::uint64_t read_integer(detail::CryptoView bytes, std::size_t& offset, unsigned length = 8) {
    if (offset > bytes.size() || bytes.size() - offset < length) {
        throw EntityReferenceTokenError::invalid();
    }
    std::uint64_t result = 0;
    for (unsigned i = 0; i < length; ++i) {
        result = (result << 8) | bytes[offset++];
    }
    return result;
}

String read_text(Memory r, detail::CryptoView bytes, std::size_t& offset) {
    auto size = read_integer(bytes, offset, 2);
    if (size > bytes.size() - offset) {
        throw EntityReferenceTokenError::invalid();
    }
    std::string_view text(reinterpret_cast<const char*>(bytes.data() + offset), size);
    if (!detail::identity_utf8(text)) {
        throw EntityReferenceTokenError::invalid();
    }
    offset += size;
    return String(text, r);
}
}

AeadEntityReferenceCodec::AeadEntityReferenceCodec(std::allocator_arg_t,
                                                   Allocator a,
                                                   std::uint32_t active,
                                                   std::span<const EntityReferenceKey> keys)
    : active_(active), keys_(a.resource()) {
    for (const auto& key : keys) {
        if (key.bytes.size() != 32) {
            throw EntityReferenceTokenError::invalid();
        }
        keys_.insert_or_assign(key.version, detail::ReferenceSecret(key.bytes));
    }
    if (!keys_.contains(active_)) {
        throw EntityReferenceTokenError::invalid();
    }
}

AeadEntityReferenceCodec::AeadEntityReferenceCodec(std::allocator_arg_t,
                                                   Allocator a,
                                                   const AeadEntityReferenceCodec& other)
    : active_(other.active_), keys_(other.keys_, a.resource()),
      clock_(detail::clone_in(a.resource(), other.clock_)),
      nonce_(detail::clone_in(a.resource(), other.nonce_)) {}

Result<AeadEntityReferenceCodec, EntityReferenceTokenError> AeadEntityReferenceCodec::create(
    Memory r, std::uint32_t active, std::span<const EntityReferenceKey> keys) {
    using Output = Result<AeadEntityReferenceCodec, EntityReferenceTokenError>;
    try {
        return Output::ok(r,
                          AeadEntityReferenceCodec(std::allocator_arg, Allocator(r), active, keys));
    } catch (const EntityReferenceTokenError& error) {
        return Output::err(r, error);
    }
}

EntityReferenceEncodeResult AeadEntityReferenceCodec::encode(Memory r,
                                                             std::string_view entity,
                                                             std::uint64_t id,
                                                             std::int64_t version,
                                                             std::string_view purpose,
                                                             ReferenceLifetime lifetime) const {
    detail::ReferenceCallGuard guard(memory_resource(), r);
    auto trimmed = entity;
    detail::trim(trimmed);
    if (trimmed.empty() || id == 0 || (lifetime.seconds == 0 && lifetime.nanoseconds == 0) ||
        !detail::reference_duration_valid(lifetime)) {
        return EntityReferenceEncodeResult::err(r, EntityReferenceTokenError::invalid());
    }
    auto now = clock_ ? (*clock_)() : UtcDateTime::now();
    auto expires = detail::reference_add(now, lifetime);
    detail::CryptoBytes plaintext(r);

    struct Wipe {
        detail::CryptoBytes& value;

        ~Wipe() {
            detail::wipe_bytes(value);
        }
    } wipe{plaintext};

    try {
        write_text(plaintext, entity);
        write_integer(plaintext, id);
        write_integer(plaintext, std::bit_cast<std::uint64_t>(version));
        write_integer(plaintext, std::bit_cast<std::uint64_t>(now.timestamp()));
        write_integer(plaintext, std::bit_cast<std::uint64_t>(expires.timestamp()));
        write_text(plaintext, purpose);
    } catch (const EntityReferenceTokenError& error) {
        detail::wipe_bytes(plaintext);
        return EntityReferenceEncodeResult::err(r, error);
    }
    auto nonce = nonce_ ? (*nonce_)() : detail::reference_nonce();
    auto encrypted = detail::aes256_gcm_encrypt(r,
                                                keys_.at(active_).bytes,
                                                nonce,
                                                plaintext,
                                                detail::reference_bytes(ENTITY_REFERENCE_AAD));
    detail::wipe_bytes(plaintext);
    detail::CryptoBytes envelope(r);
    write_integer(envelope, active_, 4);
    envelope.insert(envelope.end(), nonce.begin(), nonce.end());
    envelope.insert(envelope.end(), encrypted.begin(), encrypted.end());
    String token("tqr1.", r);
    token += detail::reference_base64(r, envelope);
    return EntityReferenceEncodeResult::ok(r, std::move(token));
}

EntityReferenceDecodeResult AeadEntityReferenceCodec::decode(Memory r,
                                                             std::string_view token,
                                                             std::string_view expected_entity,
                                                             std::string_view purpose) const {
    detail::ReferenceCallGuard guard(memory_resource(), r);
    auto invalid = [&] {
        return EntityReferenceDecodeResult::err(r, EntityReferenceTokenError::invalid());
    };
    if (!token.starts_with("tqr1.")) {
        return invalid();
    }
    auto envelope = detail::reference_unbase64(r, token.substr(5));
    if (!envelope || envelope->size() < 32) {
        return invalid();
    }
    std::size_t offset = 0;
    auto key_version = static_cast<std::uint32_t>(read_integer(*envelope, offset, 4));
    auto key = keys_.find(key_version);
    if (key == keys_.end()) {
        return invalid();
    }
    detail::CryptoNonce nonce{};
    std::copy_n(envelope->begin() + 4, nonce.size(), nonce.begin());
    auto plaintext = detail::aes256_gcm_decrypt(r,
                                                key->second.bytes,
                                                nonce,
                                                detail::CryptoView(*envelope).subspan(16),
                                                detail::reference_bytes(ENTITY_REFERENCE_AAD));
    if (!plaintext) {
        return invalid();
    }

    struct Wipe {
        detail::CryptoBytes& value;

        ~Wipe() {
            detail::wipe_bytes(value);
        }
    } wipe{*plaintext};

    try {
        offset = 0;
        auto entity = read_text(r, *plaintext, offset);
        auto id = read_integer(*plaintext, offset);
        auto version = std::bit_cast<std::int64_t>(read_integer(*plaintext, offset));
        auto issued = UtcDateTime::from_timestamp(
            std::bit_cast<std::int64_t>(read_integer(*plaintext, offset)), 0);
        auto expires = UtcDateTime::from_timestamp(
            std::bit_cast<std::int64_t>(read_integer(*plaintext, offset)), 0);
        auto parsed_purpose = read_text(r, *plaintext, offset);
        if (offset != plaintext->size() || entity.empty() || id == 0 || !issued || !expires) {
            return invalid();
        }
        auto now = clock_ ? (*clock_)() : UtcDateTime::now();
        if (detail::reference_time_less_equal(*expires, now) ||
            !detail::reference_time_less_equal(*issued, detail::reference_add(now, {60, 0})) ||
            entity != expected_entity || parsed_purpose != purpose) {
            return invalid();
        }
        return EntityReferenceDecodeResult::ok(r,
                                               EntityReferenceClaims(std::allocator_arg,
                                                                     Allocator(r),
                                                                     entity,
                                                                     id,
                                                                     version,
                                                                     *issued,
                                                                     *expires,
                                                                     parsed_purpose,
                                                                     key_version));
    } catch (const EntityReferenceTokenError& error) {
        return EntityReferenceDecodeResult::err(r, error);
    }
}
}
