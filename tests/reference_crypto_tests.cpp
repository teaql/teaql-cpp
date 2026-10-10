#include <teaql/core/detail/aes_gcm.hpp>
#include <teaql/core/json.hpp>
#include <teaql/runtime/context.hpp>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace teaql;
using namespace teaql::detail;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

static CryptoBytes hex(Memory r, std::string_view text) {
    CryptoBytes bytes(r);
    auto digit = [](char c) -> unsigned {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        throw std::runtime_error("Invalid hexadecimal vector");
    };
    check(text.size() % 2 == 0, "Hexadecimal vector must contain whole bytes");
    for (std::size_t i = 0; i < text.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>((digit(text[i]) << 4) | digit(text[i + 1])));
    }
    return bytes;
}

static std::string_view field(const Json& input, std::string_view name) {
    return std::get<String>(std::get<Json::Object>(input.data).find(name)->second.data);
}

static void block_vector_and_wiping(Memory r) {
    auto key_bytes = hex(r, "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    CryptoKey key{};
    std::copy(key_bytes.begin(), key_bytes.end(), key.begin());
    auto plaintext = hex(r, "00112233445566778899aabbccddeeff");
    CryptoBlock block{};
    std::copy(plaintext.begin(), plaintext.end(), block.begin());
    alignas(Aes256) std::array<std::byte, sizeof(Aes256)> storage;
    auto* aes = std::construct_at(reinterpret_cast<Aes256*>(storage.data()), key);
    auto ciphertext = aes->encrypt(block);
    auto expected = hex(r, "8ea2b7ca516745bfeafc49904b496089");
    check(std::ranges::equal(ciphertext, expected), "FIPS 197 Appendix C.3 AES-256 vector");
    std::destroy_at(aes);
    check(std::all_of(storage.begin(),
                      storage.end(),
                      [](std::byte byte) {
                          return byte == std::byte{};
                      }),
          "AES key schedule is wiped before storage reclamation");
}

static void vectors(const Json& input) {
    GlobalContext global;
    const auto& cases =
        std::get<Json::Array>(std::get<Json::Object>(input.data).find("cases")->second.data);
    std::size_t authenticated = 0, rejected = 0;
    for (const auto& item : cases) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto key_bytes = hex(r, field(item, "Key"));
        auto nonce_bytes = hex(r, field(item, "IV"));
        CryptoKey key{};
        CryptoNonce nonce{};
        check(key_bytes.size() == key.size() && nonce_bytes.size() == nonce.size(),
              "NIST vector dimensions");
        std::copy(key_bytes.begin(), key_bytes.end(), key.begin());
        std::copy(nonce_bytes.begin(), nonce_bytes.end(), nonce.begin());
        auto aad = hex(r, field(item, "AAD"));
        auto ciphertext = hex(r, field(item, "CT"));
        auto tag = hex(r, field(item, "Tag"));
        ciphertext.insert(ciphertext.end(), tag.begin(), tag.end());
        const auto& object = std::get<Json::Object>(item.data);
        if (object.contains("FAIL")) {
            auto result =
                aes256_gcm_decrypt(std::pmr::null_memory_resource(), key, nonce, ciphertext, aad);
            check(!result, "Unauthenticated NIST vector exposes no plaintext or allocation");
            ++rejected;
            continue;
        }
        auto plaintext = hex(r, field(item, "PT"));
        auto encoded = aes256_gcm_encrypt(r, key, nonce, plaintext, aad);
        check(encoded == ciphertext, "NIST GCM ciphertext and full tag match");
        auto decoded = aes256_gcm_decrypt(r, key, nonce, encoded, aad);
        check(decoded && *decoded == plaintext && decoded->get_allocator().resource() == r,
              "Authenticated NIST plaintext belongs to the requested domain");
        auto first_tag_byte = authenticated == 0 ? encoded.size() - 16 : encoded.size() - 1;
        for (std::size_t i = first_tag_byte; i < encoded.size(); ++i) {
            encoded[i] ^= 1;
            check(!aes256_gcm_decrypt(std::pmr::null_memory_resource(), key, nonce, encoded, aad),
                  "Every tag byte is checked before plaintext allocation");
            encoded[i] ^= 1;
        }
        ++authenticated;
    }
    check(authenticated != 0 && rejected != 0,
          "NIST acceptance includes success and authentication failures");
    std::cout << "NIST AES-256-GCM vectors passed: " << cases.size() << '\n';
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        check(argc == 2, "NIST vector path required");
        GlobalContext global;
        block_vector_and_wiping(global.memory_resource());
        std::ifstream file(argv[1]);
        check(bool(file), "Cannot read NIST vector corpus");
        std::string contents{std::istreambuf_iterator<char>(file), {}};
        auto input = Json::parse(global.memory_resource(), contents);
        check(bool(input), "Invalid NIST vector JSON");
        vectors(*input);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
