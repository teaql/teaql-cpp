#include <teaql/core/detail/aes_gcm.hpp>
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace teaql::detail {
namespace {
std::uint8_t xtime(std::uint8_t value) noexcept {
    auto mask = static_cast<std::uint8_t>(0u - (value >> 7));
    return static_cast<std::uint8_t>((value << 1) ^ (0x1b & mask));
}

std::uint8_t multiply(std::uint8_t a, std::uint8_t b) noexcept {
    std::uint8_t result = 0;
    for (unsigned i = 0; i < 8; ++i) {
        auto mask = static_cast<std::uint8_t>(0u - (b & 1));
        result ^= a & mask;
        a = xtime(a);
        b >>= 1;
    }
    return result;
}

std::uint8_t substitute(std::uint8_t x) noexcept {
    // x^254 is inversion in GF(2^8), with 0 mapping to 0. Fixed arithmetic
    // avoids a secret-indexed S-box table and secret-dependent branches.
    auto x2 = multiply(x, x);
    auto x3 = multiply(x2, x);
    auto x6 = multiply(x3, x3);
    auto x12 = multiply(x6, x6);
    auto x15 = multiply(x12, x3);
    auto x30 = multiply(x15, x15);
    auto x60 = multiply(x30, x30);
    auto x120 = multiply(x60, x60);
    auto x240 = multiply(x120, x120);
    auto x252 = multiply(x240, x12);
    auto inverse = multiply(x252, x2);
    return inverse ^ std::rotl(inverse, 1) ^ std::rotl(inverse, 2) ^ std::rotl(inverse, 3) ^
           std::rotl(inverse, 4) ^ 0x63;
}

void xor_block(CryptoBlock& a, const CryptoBlock& b) noexcept {
    for (std::size_t i = 0; i < a.size(); ++i) {
        a[i] ^= b[i];
    }
}

CryptoBlock galois_product(const CryptoBlock& x, CryptoBlock v) noexcept {
    CryptoBlock result{};
    for (unsigned bit = 0; bit < 128; ++bit) {
        auto mask = static_cast<std::uint8_t>(0u - ((x[bit / 8] >> (7 - bit % 8)) & 1));
        for (unsigned i = 0; i < 16; ++i) {
            result[i] ^= v[i] & mask;
        }
        auto reduction = static_cast<std::uint8_t>(0u - (v[15] & 1));
        for (unsigned i = 15; i > 0; --i) {
            v[i] = static_cast<std::uint8_t>((v[i] >> 1) | (v[i - 1] << 7));
        }
        v[0] = (v[0] >> 1) ^ (0xe1 & reduction);
    }
    wipe_bytes(v);
    return result;
}

void write_u64(std::span<std::uint8_t, 8> target, std::uint64_t value) noexcept {
    for (unsigned i = 0; i < 8; ++i) {
        target[7 - i] = static_cast<std::uint8_t>(value >> (8 * i));
    }
}

CryptoBlock authentication(const Aes256& aes,
                           const CryptoNonce& nonce,
                           CryptoView ciphertext,
                           CryptoView aad) noexcept {
    auto h = aes.encrypt({});
    CryptoBlock state{};
    auto absorb = [&](CryptoView bytes) {
        while (!bytes.empty()) {
            auto count = std::min<std::size_t>(16, bytes.size());
            for (std::size_t i = 0; i < count; ++i) {
                state[i] ^= bytes[i];
            }
            state = galois_product(state, h);
            bytes = bytes.subspan(count);
        }
    };
    absorb(aad);
    absorb(ciphertext);
    CryptoBlock lengths{};
    write_u64(std::span<std::uint8_t, 8>(lengths.data(), 8), std::uint64_t(aad.size()) * 8);
    write_u64(std::span<std::uint8_t, 8>(lengths.data() + 8, 8),
              std::uint64_t(ciphertext.size()) * 8);
    xor_block(state, lengths);
    state = galois_product(state, h);
    CryptoBlock initial{};
    std::copy(nonce.begin(), nonce.end(), initial.begin());
    initial[15] = 1;
    xor_block(state, aes.encrypt(initial));
    wipe_bytes(h);
    return state;
}

void counter_xor(const Aes256& aes,
                 const CryptoNonce& nonce,
                 CryptoView input,
                 std::span<std::uint8_t> output) noexcept {
    CryptoBlock counter{};
    std::copy(nonce.begin(), nonce.end(), counter.begin());
    std::uint32_t index = 1;
    std::size_t offset = 0;
    while (offset < input.size()) {
        ++index;
        for (unsigned i = 0; i < 4; ++i) {
            counter[15 - i] = static_cast<std::uint8_t>(index >> (8 * i));
        }
        auto stream = aes.encrypt(counter);
        auto count = std::min<std::size_t>(16, input.size() - offset);
        for (std::size_t i = 0; i < count; ++i) {
            output[offset + i] = input[offset + i] ^ stream[i];
        }
        offset += count;
        wipe_bytes(stream);
    }
}

bool valid_lengths(CryptoView message, CryptoView aad) noexcept {
    // SP 800-38D: plaintext <= 2^39-256 bits; AAD <= 2^64-1 bits.
    return std::uint64_t(message.size()) <= (std::uint64_t{1} << 36) - 32 &&
           std::uint64_t(aad.size()) <= std::numeric_limits<std::uint64_t>::max() / 8;
}
}

void wipe_bytes(std::span<std::uint8_t> bytes) noexcept {
    auto* target = static_cast<volatile std::uint8_t*>(bytes.data());
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        target[i] = 0;
    }
}

Aes256::Aes256(const CryptoKey& key) noexcept {
    std::copy(key.begin(), key.end(), expanded_.begin());
    std::uint8_t rcon = 1;
    for (unsigned position = 32; position < expanded_.size(); position += 4) {
        std::array<std::uint8_t, 4> temp{};
        std::copy_n(expanded_.begin() + position - 4, 4, temp.begin());
        if (position % 32 == 0) {
            std::rotate(temp.begin(), temp.begin() + 1, temp.end());
            for (auto& byte : temp) {
                byte = substitute(byte);
            }
            temp[0] ^= rcon;
            rcon = xtime(rcon);
        } else if (position % 32 == 16) {
            for (auto& byte : temp) {
                byte = substitute(byte);
            }
        }
        for (unsigned i = 0; i < 4; ++i) {
            expanded_[position + i] = expanded_[position - 32 + i] ^ temp[i];
        }
        wipe_bytes(temp);
    }
}

Aes256::~Aes256() {
    wipe_bytes(expanded_);
}

CryptoBlock Aes256::encrypt(CryptoBlock state) const noexcept {
    auto add_round = [&](unsigned round) {
        for (unsigned i = 0; i < 16; ++i) {
            state[i] ^= expanded_[round * 16 + i];
        }
    };
    add_round(0);
    for (unsigned round = 1; round <= 14; ++round) {
        for (auto& byte : state) {
            byte = substitute(byte);
        }
        auto shifted = state;
        for (unsigned row = 0; row < 4; ++row) {
            for (unsigned col = 0; col < 4; ++col) {
                state[4 * col + row] = shifted[4 * ((col + row) % 4) + row];
            }
        }
        wipe_bytes(shifted);
        if (round != 14) {
            for (unsigned col = 0; col < 4; ++col) {
                auto* b = state.data() + col * 4;
                auto a = b[0], c = b[1], d = b[2], e = b[3];
                auto all = a ^ c ^ d ^ e;
                b[0] = a ^ all ^ xtime(a ^ c);
                b[1] = c ^ all ^ xtime(c ^ d);
                b[2] = d ^ all ^ xtime(d ^ e);
                b[3] = e ^ all ^ xtime(e ^ a);
            }
        }
        add_round(round);
    }
    return state;
}

CryptoBytes aes256_gcm_encrypt(Memory r,
                               const CryptoKey& key,
                               const CryptoNonce& nonce,
                               CryptoView plaintext,
                               CryptoView aad) {
    if (!valid_lengths(plaintext, aad) ||
        plaintext.size() > std::numeric_limits<std::size_t>::max() - 16) {
        throw std::length_error("AES-GCM input exceeds the supported length");
    }
    Aes256 aes(key);
    CryptoBytes output(plaintext.size() + 16, r);
    counter_xor(aes, nonce, plaintext, std::span<std::uint8_t>(output).first(plaintext.size()));
    auto tag = authentication(aes, nonce, CryptoView(output).first(plaintext.size()), aad);
    std::copy(tag.begin(), tag.end(), output.begin() + plaintext.size());
    return output;
}

std::optional<CryptoBytes> aes256_gcm_decrypt(
    Memory r, const CryptoKey& key, const CryptoNonce& nonce, CryptoView input, CryptoView aad) {
    if (input.size() < 16 || !valid_lengths(input.first(input.size() - 16), aad)) {
        return {};
    }
    auto ciphertext = input.first(input.size() - 16);
    Aes256 aes(key);
    auto expected = authentication(aes, nonce, ciphertext, aad);
    std::uint8_t difference = 0;
    for (unsigned i = 0; i < 16; ++i) {
        difference |= expected[i] ^ input[ciphertext.size() + i];
    }
    if (difference != 0) {
        return {};
    }
    // No plaintext allocation or exposure before the full tag is validated.
    CryptoBytes output(ciphertext.size(), r);
    counter_xor(aes, nonce, ciphertext, output);
    return output;
}
}
