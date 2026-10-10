// Semantic port of all four locked runtime business-ID permutation tests.
#include <teaql/runtime/business_id.hpp>
#include <teaql/memory/arena.hpp>
#include <algorithm>
#include <charconv>
#include <fstream>
#include <unordered_set>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static std::array<std::uint8_t, 32> hex_key(std::string_view value) {
    check(value.size() == 64, "source key fixture has 32 hex bytes");
    std::array<std::uint8_t, 32> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        unsigned byte;
        auto parsed = std::from_chars(value.data() + 2 * i, value.data() + 2 * i + 2, byte, 16);
        check(parsed.ec == std::errc{} && parsed.ptr == value.data() + 2 * i + 2 &&
                  byte <= UINT8_MAX,
              "complete valid source hex byte");
        bytes[i] = std::uint8_t(byte);
    }
    return bytes;
}

static std::uint64_t integer(std::string_view value) {
    std::uint64_t result;
    auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    check(parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size(),
          "valid source fixture integer");
    return result;
}

static BusinessIdScope scope(Memory r) {
    return BusinessIdScope(
        std::allocator_arg, Allocator(r), "tenant-a", "commerce_order", "order_number", "20260925");
}

static void matches_every_cross_language_golden_vector(Memory r, const char* path) {
    std::ifstream file(path);
    check(file.good(), "original cross-language vectors are available");
    std::string line;
    std::getline(file, line);
    check(
        line ==
            "case_id,key_hex,key_version,domain_root_key,aggregate_type,namespace,period_key,sequence,expected_code,expected_business_id",
        "original fixture header");
    unsigned rows = 0;
    while (std::getline(file, line)) {
        std::array<std::string_view, 10> fields;
        auto rest = std::string_view(line);
        for (std::size_t index = 0; index < fields.size(); ++index) {
            auto comma = rest.find(',');
            check((index + 1 < fields.size()) == (comma != rest.npos),
                  "exactly ten source vector columns");
            fields[index] = rest.substr(0, comma);
            rest = comma == rest.npos ? std::string_view{} : rest.substr(comma + 1);
        }
        check(rest.empty(), "exactly ten source vector columns");
        BusinessIdScope fixture(
            std::allocator_arg, Allocator(r), fields[3], fields[4], fields[5], fields[6]);
        auto key =
            BusinessIdEncodingKey::create(r, std::uint32_t(integer(fields[2])), hex_key(fields[1]));
        check(key.is_ok(), "golden key is valid");
        auto actual = BusinessIdPermutationV1::encode(r, integer(fields[7]), fixture, key.value());
        check(actual && actual.value() == fields[8], "original cross-language code matches");
        String business("ORD-", r);
        business += fields[6];
        business += '-';
        business += actual.value();
        check(business == fields[9], "original full business ID matches");
        ++rows;
    }
    check(rows == 10, "all ten original vectors are consumed");
}

static void deterministic_unique_and_canonical_for_retained_range(Memory r) {
    auto initial_scope = scope(r);
    auto bytes = hex_key("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    auto key = BusinessIdEncodingKey::create(r, 1, bytes);
    std::pmr::unordered_set<String> values(r);
    for (std::uint64_t sequence = 0; sequence < 20'000; ++sequence) {
        auto first = BusinessIdPermutationV1::encode(r, sequence, initial_scope, key.value());
        auto after_restart = BusinessIdPermutationV1::encode(
            r, sequence, scope(r), BusinessIdEncodingKey::create(r, 1, bytes).value());
        check(first && after_restart && first.value() == after_restart.value() &&
                  first.value().size() == 6,
              "all 20000 original retained-range values are deterministic after reconstruction");
        check(std::ranges::all_of(first.value(),
                                  [](unsigned char ch) {
                                      return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z');
                                  }) &&
                  values.emplace(first.value()).second,
              "all original retained-range codes are canonical and unique");
    }
}

static void domain_and_allocation_failures(Memory r) {
    auto key = BusinessIdEncodingKey::create(r, 1, std::array<std::uint8_t, 32>{});
    auto exhausted =
        BusinessIdPermutationV1::encode(r, BUSINESS_ID_V1_DOMAIN_SIZE, scope(r), key.value());
    check(!exhausted && exhausted.error().code == BusinessIdErrorCode::Exhausted,
          "domain boundary is exhaustion");
    check(!BusinessIdEncodingKey::create(r, 0, std::array<std::uint8_t, 32>{}),
          "version zero is invalid");
    auto invalid_scope = scope(r);
    invalid_scope.namespace_name.clear();
    auto invalid = BusinessIdPermutationV1::encode(r, 0, invalid_scope, key.value());
    check(!invalid && invalid.error().code == BusinessIdErrorCode::InvalidDefinition,
          "blank scope is invalid definition");
    BusinessIdPlan plan(
        std::allocator_arg, Allocator(r), scope(r), "ORD", "20260925", 6, "-", 2, 1);
    InMemoryBusinessIdAllocator allocator(std::allocator_arg, Allocator(r));
    auto allocation = allocator.allocate(r, plan);
    check(!allocation && allocation.error().code == BusinessIdErrorCode::InvalidDefinition,
          "invalid allocation range fails before consuming a sequence");
}

int main(int argc, char** argv) {
    check(argc == 2, "supply the locked cross-language vector file");
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    Arena memory(4096, std::pmr::new_delete_resource());
    matches_every_cross_language_golden_vector(&memory, argv[1]);
    deterministic_unique_and_canonical_for_retained_range(&memory);
    domain_and_allocation_failures(&memory);
}
