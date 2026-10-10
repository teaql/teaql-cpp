#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace teaql {
class Decimal;

// Inline value storage. A view borrows this carrier, not a Context or heap owner.
class DecimalInlineString {
public:
    std::string_view view() const noexcept {
        return {data_.data(), size_};
    }

    std::size_t size() const noexcept {
        return size_;
    }

private:
    friend class Decimal;
    std::array<char, 32> data_{};
    std::uint8_t size_ = 0;
};
}
