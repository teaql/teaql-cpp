#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

namespace teaql {
// Portable value carriers for Rust's primitive integers on targets without
// compiler 128-bit integers. Words are least significant first, independent of
// host byte order. Int128 uses two's complement; neither type owns memory.
struct UInt128 {
    std::array<std::uint32_t, 4> words{};

    static constexpr UInt128 from_u64(std::uint64_t value) {
        return {{std::uint32_t(value), std::uint32_t(value >> 32), 0, 0}};
    }

    friend bool operator==(UInt128, UInt128) = default;
};

struct Int128 {
    std::array<std::uint32_t, 4> words{};

    static constexpr Int128 from_i64(std::int64_t value) {
        auto extension = value < 0 ? UINT32_MAX : 0;
        return {{std::uint32_t(value),
                 std::uint32_t(std::uint64_t(value) >> 32),
                 extension,
                 extension}};
    }

    constexpr bool negative() const {
        return (words[3] & 0x80000000u) != 0;
    }

    constexpr UInt128 unsigned_abs() const {
        UInt128 result{words};
        if (negative()) {
            std::uint64_t carry = 1;
            for (auto& word : result.words) {
                carry += std::uint32_t(~word);
                word = std::uint32_t(carry);
                carry >>= 32;
            }
        }
        return result;
    }

    static constexpr Int128 from_magnitude(UInt128 magnitude, bool negative) {
        if ((magnitude.words[3] & 0x80000000u) &&
            (!negative || magnitude.words[3] != 0x80000000u || magnitude.words[2] ||
             magnitude.words[1] || magnitude.words[0])) {
            throw std::out_of_range("Signed 128-bit magnitude exceeds its range");
        }
        Int128 result{magnitude.words};
        if (negative) {
            std::uint64_t carry = 1;
            for (auto& word : result.words) {
                carry += std::uint32_t(~word);
                word = std::uint32_t(carry);
                carry >>= 32;
            }
        }
        return result;
    }

    friend bool operator==(Int128, Int128) = default;
};

static_assert(sizeof(UInt128) == 16 && sizeof(Int128) == 16);
}
