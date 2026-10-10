#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_reference.hpp>

namespace teaql::reference_fixture {
struct Clock {
    using allocator_type = Allocator;
    UtcDateTime time;

    Clock(std::allocator_arg_t, Allocator, UtcDateTime time) : time(time) {}

    Clock(std::allocator_arg_t, Allocator, const Clock& other) : time(other.time) {}

    UtcDateTime operator()() const {
        return time;
    }
};

struct Nonce {
    using allocator_type = Allocator;
    detail::CryptoNonce bytes;

    Nonce(std::allocator_arg_t, Allocator, detail::CryptoNonce bytes) : bytes(bytes) {}

    Nonce(std::allocator_arg_t, Allocator, const Nonce& other) : bytes(other.bytes) {}

    detail::CryptoNonce operator()() const {
        return bytes;
    }
};

inline auto keys(Memory r, std::uint32_t version = 2, std::uint8_t byte = 0x22) {
    Vector<EntityReferenceKey> result(r);
    detail::CryptoKey key;
    key.fill(byte);
    result.emplace_back(version, key);
    return result;
}

inline void
configure(AeadEntityReferenceCodec& codec, UtcDateTime now, std::uint8_t nonce_byte = 0x33) {
    detail::CryptoNonce nonce;
    nonce.fill(nonce_byte);
    codec.with_clock(Clock(std::allocator_arg, Allocator(codec.memory_resource()), now));
    codec.with_nonce_source(Nonce(std::allocator_arg, Allocator(codec.memory_resource()), nonce));
}
}
