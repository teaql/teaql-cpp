#pragma once
#include <bit>
#include <cstdint>
#include <string_view>

namespace teaql::detail {
// DefaultHasher's pinned Rust-toolchain SipHash-1-3 with zero keys. This is a
// process-local compatibility key, not a portable wire hash or cryptographic API.
class RustDefaultHasher {
    std::uint64_t v0 = 0x736f6d6570736575ULL, v1 = 0x646f72616e646f6dULL,
                  v2 = 0x6c7967656e657261ULL, v3 = 0x7465646279746573ULL;
    std::uint64_t tail = 0, length = 0;
    unsigned used = 0;

    void round() {
        v0 += v1;
        v1 = std::rotl(v1, 13);
        v1 ^= v0;
        v0 = std::rotl(v0, 32);
        v2 += v3;
        v3 = std::rotl(v3, 16);
        v3 ^= v2;
        v0 += v3;
        v3 = std::rotl(v3, 21);
        v3 ^= v0;
        v2 += v1;
        v1 = std::rotl(v1, 17);
        v1 ^= v2;
        v2 = std::rotl(v2, 32);
    }

    void block(std::uint64_t n) {
        v3 ^= n;
        round();
        v0 ^= n;
    }

public:
    void byte(unsigned char b) {
        tail |= std::uint64_t(b) << (8 * used);
        ++length;
        if (++used == 8) {
            block(tail);
            tail = 0;
            used = 0;
        }
    }

    void string(std::string_view s) {
        for (unsigned char b : s) {
            byte(b);
        }
        byte(0xff);
    }

    void u64(std::uint64_t n) {
        for (unsigned i = 0; i < 8; ++i) {
            byte(static_cast<unsigned char>(n >> (8 * i)));
        }
    }

    std::uint64_t finish() {
        block(tail | ((length & 0xff) << 56));
        v2 ^= 0xff;
        round();
        round();
        round();
        return v0 ^ v1 ^ v2 ^ v3;
    }
};

}
