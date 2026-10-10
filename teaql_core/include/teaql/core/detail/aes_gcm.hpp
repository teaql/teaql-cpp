#pragma once
#include <teaql/memory/types.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace teaql::detail {
using CryptoBlock = std::array<std::uint8_t, 16>;
using CryptoKey = std::array<std::uint8_t, 32>;
using CryptoNonce = std::array<std::uint8_t, 12>;
using CryptoBytes = Vector<std::uint8_t>;
using CryptoView = std::span<const std::uint8_t>;

void wipe_bytes(std::span<std::uint8_t>) noexcept;

// AES-256 encryption only: GCM uses the block cipher's forward operation.
class Aes256 final {
public:
    explicit Aes256(const CryptoKey&) noexcept;
    ~Aes256();
    Aes256(const Aes256&) = delete;
    Aes256& operator=(const Aes256&) = delete;
    CryptoBlock encrypt(CryptoBlock) const noexcept;

private:
    std::array<std::uint8_t, 240> expanded_{};
};

// Fixed 96-bit IV and full 128-bit tag, matching both original token codecs.
CryptoBytes aes256_gcm_encrypt(
    Memory, const CryptoKey&, const CryptoNonce&, CryptoView plaintext, CryptoView aad);
std::optional<CryptoBytes> aes256_gcm_decrypt(
    Memory, const CryptoKey&, const CryptoNonce&, CryptoView ciphertext_and_tag, CryptoView aad);
}
