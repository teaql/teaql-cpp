#pragma once
#include <teaql/memory/scoped.hpp>
#include <teaql/memory/types.hpp>
#include <string_view>

namespace teaql {
struct DecimalJsonError {
    enum class Code { InvalidType, InvalidValue };
    using allocator_type = Allocator;
    Code code;
    String message;

    DecimalJsonError(std::allocator_arg_t, Allocator a, Code code, std::string_view message)
        : code(code), message(message, a.resource()) {}

    DecimalJsonError(std::allocator_arg_t, Allocator a, const DecimalJsonError& other)
        : DecimalJsonError(std::allocator_arg, a, other.code, other.message) {}

    DecimalJsonError(std::allocator_arg_t, Allocator a, DecimalJsonError&& other)
        : DecimalJsonError(detail::move_to(a.resource(), std::move(other))) {}

    DecimalJsonError(const DecimalJsonError&) = delete;
    DecimalJsonError(DecimalJsonError&&) noexcept = default;

    DecimalJsonError& operator=(DecimalJsonError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return message.get_allocator().resource();
    }
};
}
