#pragma once
#include <teaql/core/detail/aes_gcm.hpp>
#include <teaql/core/result.hpp>
#include <teaql/memory/function.hpp>
#include <teaql/runtime/business_clock.hpp>
#include <map>
#include <limits>

namespace teaql {
inline constexpr std::string_view ENTITY_REFERENCE_AAD = "teaql.entity-reference.v1";
[[deprecated("Use UNSAFE_EXPOSE_RAW_ENTITY_IDS_ENVIRONMENT with ContextBoundReferenceRuntime")]]
inline constexpr std::string_view UNSAFE_RAW_ENTITY_REFERENCES_ENVIRONMENT =
    "TEAQL_UNSAFE_EXPOSE_RAW_ENTITY_IDS";
[[deprecated("Use UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT with ContextBoundReferenceRuntime")]]
inline constexpr std::string_view UNSAFE_RAW_ENTITY_REFERENCES_ACKNOWLEDGEMENT =
    "I_UNDERSTAND_THIS_EXPOSES_INTERNAL_ENTITY_IDS_FOR_LOCAL_DEBUGGING_ONLY";

// Rust Duration's full nonnegative seconds/nanoseconds domain, including MIPS.
struct ReferenceLifetime {
    std::uint64_t seconds = 0;
    std::uint32_t nanoseconds = 0;

    ReferenceLifetime(std::uint64_t seconds = 0, std::uint32_t nanoseconds = 0) {
        auto carry = nanoseconds / 1'000'000'000;
        if (seconds > std::numeric_limits<std::uint64_t>::max() - carry) {
            throw std::out_of_range("Reference duration overflows Rust Duration");
        }
        this->seconds = seconds + carry;
        this->nanoseconds = nanoseconds % 1'000'000'000;
    }
};

class EntityReferenceTokenError final : public std::exception {
public:
    using allocator_type = Allocator;
    enum class Kind { Invalid, CodecRequired };

    EntityReferenceTokenError(std::allocator_arg_t,
                              Allocator,
                              const EntityReferenceTokenError& other) noexcept
        : kind_(other.kind_) {}

    EntityReferenceTokenError(std::allocator_arg_t,
                              Allocator,
                              EntityReferenceTokenError&& other) noexcept
        : kind_(other.kind_) {}

    static EntityReferenceTokenError invalid() noexcept {
        return EntityReferenceTokenError(Kind::Invalid);
    }

    static EntityReferenceTokenError codec_required() noexcept {
        return EntityReferenceTokenError(Kind::CodecRequired);
    }

    std::string_view code() const noexcept {
        return kind_ == Kind::Invalid ? "ENTITY_REFERENCE_INVALID"
                                      : "ENTITY_REFERENCE_CODEC_REQUIRED";
    }

    const std::exception* source() const noexcept {
        return nullptr;
    }

    const char* what() const noexcept override {
        return code().data();
    }

    bool operator==(const EntityReferenceTokenError& other) const noexcept {
        return kind_ == other.kind_;
    }

private:
    explicit EntityReferenceTokenError(Kind kind) noexcept : kind_(kind) {}

    Kind kind_;
};

struct EntityReferenceClaims {
    using allocator_type = Allocator;
    String entity_type;
    std::uint64_t id;
    std::int64_t version;
    UtcDateTime issued_at, expires_at;
    String purpose;
    std::uint32_t key_version;

    EntityReferenceClaims(std::allocator_arg_t,
                          Allocator a,
                          std::string_view entity,
                          std::uint64_t id,
                          std::int64_t version,
                          UtcDateTime issued,
                          UtcDateTime expires,
                          std::string_view purpose,
                          std::uint32_t key_version)
        : entity_type(entity, a.resource()), id(id), version(version), issued_at(issued),
          expires_at(expires), purpose(purpose, a.resource()), key_version(key_version) {}

    EntityReferenceClaims(std::allocator_arg_t, Allocator a, const EntityReferenceClaims& other)
        : EntityReferenceClaims(std::allocator_arg,
                                a,
                                other.entity_type,
                                other.id,
                                other.version,
                                other.issued_at,
                                other.expires_at,
                                other.purpose,
                                other.key_version) {}

    EntityReferenceClaims(std::allocator_arg_t, Allocator a, EntityReferenceClaims&& other)
        : EntityReferenceClaims(detail::move_to(a.resource(), std::move(other))) {}

    EntityReferenceClaims(const EntityReferenceClaims&) = delete;
    EntityReferenceClaims(EntityReferenceClaims&&) noexcept = default;

    EntityReferenceClaims& operator=(EntityReferenceClaims&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return entity_type.get_allocator().resource();
    }

    bool operator==(const EntityReferenceClaims&) const = default;
};

using EntityReferenceEncodeResult = Result<String, EntityReferenceTokenError>;
using EntityReferenceDecodeResult = Result<EntityReferenceClaims, EntityReferenceTokenError>;

class EntityReferenceCodec {
public:
    virtual ~EntityReferenceCodec() = default;
    virtual EntityReferenceEncodeResult encode(Memory destination,
                                               std::string_view entity_type,
                                               std::uint64_t id,
                                               std::int64_t version,
                                               std::string_view purpose,
                                               ReferenceLifetime) const = 0;
    virtual EntityReferenceDecodeResult decode(Memory destination,
                                               std::string_view token,
                                               std::string_view expected_entity_type,
                                               std::string_view purpose) const = 0;
};

struct EntityReferenceKey {
    using allocator_type = Allocator;
    std::uint32_t version;
    detail::CryptoBytes bytes;

    EntityReferenceKey(std::allocator_arg_t,
                       Allocator a,
                       std::uint32_t version,
                       detail::CryptoView bytes)
        : version(version), bytes(bytes.begin(), bytes.end(), a.resource()) {}

    EntityReferenceKey(std::allocator_arg_t, Allocator a, const EntityReferenceKey& other)
        : EntityReferenceKey(std::allocator_arg, a, other.version, other.bytes) {}

    EntityReferenceKey(std::allocator_arg_t, Allocator a, EntityReferenceKey&& other)
        : EntityReferenceKey(detail::move_to(a.resource(), std::move(other))) {}

    EntityReferenceKey(EntityReferenceKey&&) noexcept = default;

    EntityReferenceKey& operator=(EntityReferenceKey&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    ~EntityReferenceKey() {
        detail::wipe_bytes(bytes);
    }

    Memory memory_resource() const {
        return bytes.get_allocator().resource();
    }
};

namespace detail {
struct ReferenceSecret {
    CryptoKey bytes{};

    explicit ReferenceSecret(CryptoView value) {
        if (value.size() != bytes.size()) {
            throw EntityReferenceTokenError::invalid();
        }
        std::copy(value.begin(), value.end(), bytes.begin());
    }

    ReferenceSecret(const ReferenceSecret&) = default;
    ReferenceSecret& operator=(const ReferenceSecret&) = default;

    ~ReferenceSecret() {
        wipe_bytes(bytes);
    }
};
}

class AeadEntityReferenceCodec final : public EntityReferenceCodec {
public:
    using allocator_type = Allocator;
    AeadEntityReferenceCodec(std::allocator_arg_t,
                             Allocator,
                             std::uint32_t active_key_version,
                             std::span<const EntityReferenceKey>);
    AeadEntityReferenceCodec(std::allocator_arg_t, Allocator, const AeadEntityReferenceCodec&);

    AeadEntityReferenceCodec(std::allocator_arg_t, Allocator a, AeadEntityReferenceCodec&& other)
        : AeadEntityReferenceCodec(detail::move_to(a.resource(), std::move(other))) {}

    AeadEntityReferenceCodec(const AeadEntityReferenceCodec&) = delete;
    AeadEntityReferenceCodec(AeadEntityReferenceCodec&&) noexcept = default;
    static Result<AeadEntityReferenceCodec, EntityReferenceTokenError>
    create(Memory, std::uint32_t active_key_version, std::span<const EntityReferenceKey>);

    template <class F>
    AeadEntityReferenceCodec& with_clock(F&& clock) {
        clock_.emplace(std::allocator_arg, Allocator(memory_resource()), std::forward<F>(clock));
        return *this;
    }

    template <class F>
    AeadEntityReferenceCodec& with_nonce_source(F&& source) {
        nonce_.emplace(std::allocator_arg, Allocator(memory_resource()), std::forward<F>(source));
        return *this;
    }

    EntityReferenceEncodeResult encode(Memory,
                                       std::string_view,
                                       std::uint64_t,
                                       std::int64_t,
                                       std::string_view,
                                       ReferenceLifetime) const override;
    EntityReferenceDecodeResult
        decode(Memory, std::string_view, std::string_view, std::string_view) const override;

    Memory memory_resource() const {
        return keys_.get_allocator().resource();
    }

private:
    std::uint32_t active_;
    std::pmr::map<std::uint32_t, detail::ReferenceSecret> keys_;
    std::optional<ContextFunction<UtcDateTime()>> clock_;
    std::optional<ContextFunction<detail::CryptoNonce()>> nonce_;
};
}
