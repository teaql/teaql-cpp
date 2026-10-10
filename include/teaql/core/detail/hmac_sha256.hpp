#pragma once
#include <teaql/core/detail/aes_gcm.hpp>
#include <teaql/core/detail/sha256.hpp>
#include <algorithm>
#include <type_traits>

namespace teaql::detail {
// Streaming HMAC with fixed state. Keys and pads never use default heap storage.
class HmacSha256 final {
public:
    explicit HmacSha256(CryptoView key) {
        std::array<std::uint8_t, 64> inner_pad{};
        if (key.size() > inner_pad.size()) {
            Sha256 hash;
            hash.update(key);
            auto digest = hash.finish();
            std::copy(digest.begin(), digest.end(), inner_pad.begin());
            wipe_bytes(digest);
        } else {
            std::copy(key.begin(), key.end(), inner_pad.begin());
        }
        for (std::size_t i = 0; i < inner_pad.size(); ++i) {
            outer_pad_[i] = inner_pad[i] ^ 0x5c;
            inner_pad[i] ^= 0x36;
        }
        inner_.update(inner_pad);
        wipe_bytes(inner_pad);
    }

    HmacSha256(const HmacSha256&) = delete;
    HmacSha256& operator=(const HmacSha256&) = delete;

    ~HmacSha256() {
        wipe_bytes(outer_pad_);
        // Sha256 owns fixed primitive state only; wipe the key-bearing final
        // block before its trivial destructor, including HKDF extraction input.
        static_assert(std::is_trivially_copyable_v<Sha256>);
        wipe_bytes({reinterpret_cast<std::uint8_t*>(&inner_), sizeof(inner_)});
    }

    void update(CryptoView input) {
        inner_.update(input);
    }

    CryptoKey finish() {
        auto digest = inner_.finish();
        Sha256 outer;
        outer.update(outer_pad_);
        outer.update(digest);
        wipe_bytes(digest);
        return outer.finish();
    }

private:
    Sha256 inner_;
    std::array<std::uint8_t, 64> outer_pad_{};
};

inline CryptoKey hmac_sha256(CryptoView key, CryptoView input) {
    HmacSha256 mac(key);
    mac.update(input);
    return mac.finish();
}

// HKDF-SHA256's single output block, the original reference runtime's 32-byte key.
inline CryptoKey hkdf_sha256_32(CryptoView salt, CryptoView master, CryptoView info) {
    auto prk = hmac_sha256(salt, master);
    HmacSha256 expansion(prk);
    expansion.update(info);
    const std::uint8_t counter = 1;
    expansion.update({&counter, 1});
    wipe_bytes(prk);
    return expansion.finish();
}
}
