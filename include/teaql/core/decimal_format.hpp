#pragma once
#include <cstddef>
#include <optional>

namespace teaql {
enum class DecimalNotation { Display, Debug, LowerExp, UpperExp };
enum class DecimalAlignment { Right, Left, Center };

struct DecimalFormat {
    DecimalNotation notation = DecimalNotation::Display;
    DecimalAlignment alignment = DecimalAlignment::Right;
    std::optional<std::size_t> precision;
    std::size_t width = 0;
    char32_t fill = U' ';
    bool sign_plus = false;
    bool zero_pad = false;
};
}
