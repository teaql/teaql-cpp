#include <teaql/core/detail/number_text.hpp>
#include <charconv>
#include <bit>
#include <cstdint>
#include <cmath>
#include <stdexcept>

namespace teaql::detail {
void append_float(String& out, double value, bool json) {
    if (!std::isfinite(value)) {
        out += json ? "null" : std::isnan(value) ? "NaN" : std::signbit(value) ? "-inf" : "inf";
        return;
    }
    if (std::signbit(value))
        out += '-';
    if (value == 0) {
        out += json ? "0.0" : "0";
        return;
    }
    char text[32];
    auto [end, error] =
        std::to_chars(text, text + sizeof(text), std::abs(value), std::chars_format::scientific);
    if (error != std::errc{})
        throw std::runtime_error("float formatting failed");
    char digits[20];
    int count = 0;
    const char* p = text;
    for (; p != end && *p != 'e'; ++p)
        if (*p != '.')
            digits[count++] = *p;
    ++p;
    bool negative = *p == '-';
    ++p;
    int exponent = 0;
    for (; p != end; ++p)
        exponent = exponent * 10 + (*p - '0');
    if (negative)
        exponent = -exponent;
    if (!json) {
        // Rust Display breaks shortest-decimal ties away from zero; to_chars
        // and serde_json use ties-to-even. Detect the exact binary midpoint,
        // without floating arithmetic or allocating a big integer.
        const auto bits = std::bit_cast<std::uint64_t>(std::abs(value));
        const int binary_exponent = int(bits >> 52);
        auto significand = bits & ((std::uint64_t(1) << 52) - 1);
        if (binary_exponent)
            significand |= std::uint64_t(1) << 52;
        const int trailing = std::countr_zero(significand);
        significand >>= trailing;
        const int power = exponent - count + 1;
        const int binary_power = (binary_exponent ? binary_exponent - 1075 : -1074) + trailing;
        bool divisible = true;
        for (int i = 0; i < power; ++i) {
            if (significand % 5) {
                divisible = false;
                break;
            }
            significand /= 5;
        }
        if (divisible && binary_power - power == -1 && (significand & 3) == 1)
            ++digits[count - 1];
    }
    if (json && (exponent < -5 || exponent > 15)) {
        out += digits[0];
        if (count > 1) {
            out += '.';
            out.append(digits + 1, digits + count);
        }
        out += exponent < 0 ? "e-" : "e+";
        char exp[4];
        auto [last, ignored] = std::to_chars(exp, exp + sizeof(exp), std::abs(exponent));
        (void)ignored;
        out.append(exp, last);
    } else if (exponent < 0) {
        out += "0.";
        out.append(-exponent - 1, '0');
        out.append(digits, digits + count);
    } else if (exponent + 1 >= count) {
        out.append(digits, digits + count);
        out.append(exponent + 1 - count, '0');
        if (json)
            out += ".0";
    } else {
        out.append(digits, digits + exponent + 1);
        out += '.';
        out.append(digits + exponent + 1, digits + count);
    }
}
}
