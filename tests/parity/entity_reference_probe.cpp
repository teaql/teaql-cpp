#include "../entity_reference_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>
#include <bit>
#include <iostream>

using namespace teaql::reference_fixture;

template <class T>
static String decimal(Memory r, T value) {
    std::array<char, 32> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    return String(buffer.data(), result.ptr, r);
}

static detail::CryptoBytes hex(Memory r, std::string_view text) {
    detail::CryptoBytes out(r);
    for (std::size_t i = 0; i < text.size(); i += 2) {
        unsigned value{};
        auto parsed = std::from_chars(text.data() + i, text.data() + i + 2, value, 16);
        if (parsed.ec != std::errc{}) {
            throw std::runtime_error("Invalid fixture hex");
        }
        out.push_back(static_cast<std::uint8_t>(value));
    }
    return out;
}

static UtcDateTime now(const Json& input) {
    const auto& a = std::get<Json::Array>(input.data);
    return *UtcDateTime::from_timestamp(number<std::int64_t>(a[0]),
                                        detail::model_integer<std::uint32_t>(a[1], "nanoseconds"));
}

static Vector<EntityReferenceKey> key_material(Memory r, const Json& input) {
    Vector<EntityReferenceKey> out(r);
    for (const auto& key : std::get<Json::Array>(input.data)) {
        const auto& k = std::get<Json::Array>(key.data);
        auto bytes = hex(r, text(k[1]));
        out.emplace_back(detail::model_integer<std::uint32_t>(k[0], "key version"), bytes);
    }
    return out;
}

static Json describe(Memory r, const EntityReferenceClaims& c) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", c.entity_type);
    detail::model_put(out, "id", decimal(r, c.id));
    detail::model_put(out, "version", decimal(r, c.version));
    detail::model_put(out, "issued", decimal(r, c.issued_at.timestamp()));
    detail::model_put(out, "expires", decimal(r, c.expires_at.timestamp()));
    detail::model_put(out, "purpose", c.purpose);
    detail::model_put(out, "key_version", c.key_version);
    return out;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    auto out = detail::model_json(r);
    auto material = key_material(r, field(input, "keys"));
    auto created = AeadEntityReferenceCodec::create(
        r, detail::model_integer<std::uint32_t>(field(input, "active"), "active"), material);
    if (!created) {
        detail::model_put(out, "error", created.error().code());
        return out;
    }
    auto op = text(field(input, "op"));
    if (op == "new") {
        detail::model_put(out, "ok", true);
        return out;
    }
    auto& codec = user.make<AeadEntityReferenceCodec>(std::move(created).value());
    auto nonce_bytes = hex(r, text(field(input, "nonce")));
    detail::CryptoNonce nonce{};
    std::copy(nonce_bytes.begin(), nonce_bytes.end(), nonce.begin());
    codec.with_clock(Clock(std::allocator_arg, Allocator(r), now(field(input, "now"))));
    codec.with_nonce_source(Nonce(std::allocator_arg, Allocator(r), nonce));
    bool context = std::get<bool>(field(input, "context").data);
    bool installed = std::get<bool>(field(input, "installed").data);
    String token(r);
    if (op == "decode") {
        token = text(field(input, "token"));
    } else if (op == "forged") {
        auto key_bytes = hex(r, text(field(input, "encryption_key")));
        detail::CryptoKey key{};
        std::copy(key_bytes.begin(), key_bytes.end(), key.begin());
        auto plain = hex(r, text(field(input, "plaintext")));
        auto encrypted = detail::aes256_gcm_encrypt(
            r, key, nonce, plain, detail::reference_bytes(ENTITY_REFERENCE_AAD));
        detail::CryptoBytes envelope(r);
        auto version =
            detail::model_integer<std::uint32_t>(field(input, "header_version"), "header version");
        for (int i = 3; i >= 0; --i) {
            envelope.push_back(static_cast<std::uint8_t>(version >> (i * 8)));
        }
        envelope.insert(envelope.end(), nonce.begin(), nonce.end());
        envelope.insert(envelope.end(), encrypted.begin(), encrypted.end());
        token = "tqr1.";
        token += detail::reference_base64(r, envelope);
    } else {
        const auto& lifetime = std::get<Json::Array>(field(input, "lifetime").data);
        auto duration =
            ReferenceLifetime{number<std::uint64_t>(lifetime[0]),
                              detail::model_integer<std::uint32_t>(lifetime[1], "duration nanos")};
        auto entity = text(field(input, "entity"));
        auto id = number<std::uint64_t>(field(input, "id"));
        auto version = number<std::int64_t>(field(input, "version"));
        auto purpose = text(field(input, "purpose"));
        if (context && installed) {
            user.set_entity_reference_codec(user, codec);
        }
        auto result = context ? user.encode_entity_reference(entity, id, version, purpose, duration)
                              : codec.encode(r, entity, id, version, purpose, duration);
        if (!result) {
            detail::model_put(out, "error", result.error().code());
            return out;
        }
        token = std::move(result).value();
    }
    auto decode_material = key_material(r, field(input, "decode_keys"));
    auto& decode_codec = user.make<AeadEntityReferenceCodec>(
        detail::model_integer<std::uint32_t>(field(input, "decode_active"), "decode active"),
        std::span<const EntityReferenceKey>(decode_material));
    decode_codec.with_clock(
        Clock(std::allocator_arg, Allocator(r), now(field(input, "consume_now"))));
    if (context && installed) {
        user.set_entity_reference_codec(user, decode_codec);
    }
    auto expected = text(field(input, "expected_entity"));
    auto purpose = text(field(input, "expected_purpose"));
    auto decoded = context ? user.decode_entity_reference(token, expected, purpose)
                           : decode_codec.decode(r, token, expected, purpose);
    detail::model_put(out, "token", token);
    if (decoded) {
        detail::model_put(out, "claims", describe(r, decoded.value()));
    } else {
        detail::model_put(out, "decode_error", decoded.error().code());
    }
    return out;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            return 2;
        }
        Json result(r);
        try {
            result = evaluate(*user, *input);
        } catch (const std::out_of_range&) {
            result = detail::model_json(r);
            detail::model_put(result, "panic", true);
        }
        std::cout << result.dump(r) << '\n';
    }
}
