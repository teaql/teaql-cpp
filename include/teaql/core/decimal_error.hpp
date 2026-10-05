#pragma once
#include <teaql/core/result.hpp>
#include <cstdint>
#include <string_view>

namespace teaql {
struct DecimalError {
    enum class Code {
        ErrorString,
        ExceedsMaximumPossibleValue,
        LessThanMinimumPossibleValue,
        Underflow,
        ScaleExceedsMaximumPrecision,
        ConversionTo
    };
    using allocator_type = Allocator;
    Code code;
    String text;
    std::uint32_t scale = 0;

    DecimalError(std::allocator_arg_t,
                 Allocator a,
                 Code code,
                 std::string_view text = {},
                 std::uint32_t scale = 0)
        : code(code), text(text, a.resource()), scale(scale) {}

    DecimalError(std::allocator_arg_t, Allocator a, const DecimalError& other)
        : DecimalError(std::allocator_arg, a, other.code, other.text, other.scale) {}

    DecimalError(std::allocator_arg_t, Allocator a, DecimalError&& other)
        : DecimalError(detail::move_to(a.resource(), std::move(other))) {}

    DecimalError(const DecimalError&) = delete;
    DecimalError(DecimalError&&) noexcept = default;

    DecimalError& operator=(DecimalError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return text.get_allocator().resource();
    }

    String to_string(Memory destination) const;

    bool operator==(const DecimalError& other) const {
        return code == other.code && text == other.text && scale == other.scale;
    }
};
}
