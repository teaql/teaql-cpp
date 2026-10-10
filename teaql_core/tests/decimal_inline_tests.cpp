#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <cstdlib>
#include <iostream>
#include <new>

// Test-only allocation instrumentation; owns no Context data.
static std::size_t allocations = 0;

void* operator new(std::size_t bytes) {
    ++allocations;
    if (auto* result = std::malloc(bytes ? bytes : 1)) {
        return result;
    }
    throw std::bad_alloc();
}

void* operator new[](std::size_t bytes) {
    return ::operator new(bytes);
}

void operator delete(void* pointer) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer) noexcept {
    std::free(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept {
    std::free(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept {
    std::free(pointer);
}

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static_assert(std::is_trivially_copyable_v<teaql::DecimalInlineString>);
static_assert(std::is_trivially_destructible_v<teaql::DecimalInlineString>);
static_assert(sizeof(teaql::DecimalInlineString) == 33);

static void inline_character_contracts() {
    const auto negative = *teaql::Decimal::from_parts({1290, 0, 0}, true, 2);
    const auto maximum = teaql::Decimal::MAX;
    auto fraction = *teaql::Decimal::from_parts({1, 0, 0}, true, 28);
    auto zero = *teaql::Decimal::from_parts({0, 0, 0}, true, 0);
    auto before = allocations;
    auto text = negative.array_string();
    auto copy = text;
    auto moved = std::move(copy);
    auto max_text = maximum.array_string();
    auto fraction_text = fraction.array_string();
    auto zero_text = zero.array_string();
    check(allocations == before, "normal inline formatting/copy/move performs no heap allocation");
    check(text.view() == "12.90" && moved.view() == text.view() &&
              moved.view().data() != text.view().data(),
          "original unsigned characters retain scale and copies own independent inline bytes");
    check(max_text.view() == "79228162514264337593543950335" &&
              fraction_text.view() == "0.0000000000000000000000000001" && zero_text.view() == "0",
          "original 96-bit coefficient, fractional and negative-zero boundaries");
    fraction.rescale(30);
    before = allocations;
    auto full = fraction.array_string();
    check(allocations == before && full.size() == 32 &&
              full.view() == "0.000000000000000000000000000100",
          "the complete 32-byte inline capacity remains usable without heap storage");
    fraction.rescale(31);
    try {
        (void)fraction.array_string();
        throw std::runtime_error("Original fixed-buffer overflow was accepted");
    } catch (const std::length_error&) {
    }
}

static void owner_independence_contract() {
    teaql::GlobalContext global;
    teaql::DecimalInlineString retained;
    {
        auto user = global.make_user_context();
        auto value = user->make_ref<teaql::Decimal>(teaql::Decimal::from_i64_with_scale(-1290, 2));
        auto objects = user->allocation_stats().issued_objects;
        auto blocks = user->allocation_stats().live_blocks;
        retained = user->get(value).array_string();
        check(user->allocation_stats().issued_objects == objects &&
                  user->allocation_stats().live_blocks == blocks,
              "inline character results publish no Context object or payload allocation");
    }
    check(global.live_user_bytes() == 0 && retained.view() == "12.90",
          "the copied inline scalar remains valid after actual source User deletion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        inline_character_contracts();
        owner_independence_contract();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
