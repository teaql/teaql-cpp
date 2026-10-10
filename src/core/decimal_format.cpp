#include <teaql/core/decimal.hpp>
#include <algorithm>
#include <charconv>
#include <limits>

namespace teaql {
namespace {
unsigned divide10(std::array<std::uint32_t, 3>& words) {
    std::uint64_t remainder = 0;
    for (int i = 2; i >= 0; --i) {
        auto value = (remainder << 32) | words[i];
        words[i] = std::uint32_t(value / 10);
        remainder = value % 10;
    }
    return unsigned(remainder);
}

void repeat(String& out, std::string_view fill, std::size_t count) {
    if (count > (out.max_size() - out.size()) / fill.size()) {
        throw std::length_error("Decimal formatting size overflow");
    }
    if (fill.size() == 1) {
        out.append(count, fill.front());
    } else {
        for (std::size_t i = 0; i < count; ++i) {
            out.append(fill);
        }
    }
}

std::size_t encode_fill(char32_t code, std::array<char, 4>& bytes) {
    if (code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) {
        throw std::invalid_argument("Decimal fill must be a Unicode scalar");
    }
    unsigned length = code < 0x80 ? 1 : code < 0x800 ? 2 : code < 0x10000 ? 3 : 4;
    auto remaining = std::uint32_t(code);
    for (unsigned i = length - 1; i != 0; --i) {
        bytes[i] = char(0x80u | (remaining & 63u));
        remaining >>= 6;
    }
    constexpr std::array<unsigned, 4> prefixes{0, 0xc0, 0xe0, 0xf0};
    bytes[0] = char(prefixes[length - 1] | remaining);
    return length;
}
}

// Preserve rust_decimal 1.42.1's Display/Debug/Exp rules (MIT; see LICENSES).
String DecimalError::format(Memory destination, DecimalFormat options) const {
    if (code != Code::ErrorString) {
        return to_string(destination);
    }
    // Only ErrorString calls Formatter::pad; the other variants use write!.
    std::size_t end = 0;
    std::size_t characters = 0;
    while (end < text.size() && (!options.precision || characters < *options.precision)) {
        ++end;
        while (end < text.size() && (static_cast<unsigned char>(text[end]) & 0xc0u) == 0x80u) {
            ++end;
        }
        ++characters;
    }
    auto padding = options.width > characters ? options.width - characters : 0;
    auto left = options.alignment == DecimalAlignment::Left     ? 0
                : options.alignment == DecimalAlignment::Center ? padding / 2
                                                                : padding;
    std::array<char, 4> fill_bytes{};
    auto fill_size = encode_fill(options.fill, fill_bytes);
    std::string_view fill(fill_bytes.data(), fill_size);
    String output(destination);
    repeat(output, fill, left);
    output.append(text.data(), end);
    repeat(output, fill, padding - left);
    return output;
}

String Decimal::format(Memory r, DecimalFormat options) const {
    std::array<char, 4> fill_bytes{};
    auto fill_size = encode_fill(options.fill, fill_bytes);
    std::string_view fill(fill_bytes.data(), fill_size);
    bool exponential = options.notation == DecimalNotation::LowerExp ||
                       options.notation == DecimalNotation::UpperExp;
    if (exponential && is_zero()) {
        // The source bypasses pad_integral, including UpperExp and all flags.
        return String("0e0", r);
    }
    std::array<char, 32> reversed{};
    auto words = coefficient();
    std::size_t length = 0;
    while (words[0] || words[1] || words[2]) {
        reversed[length++] = char('0' + divide10(words));
    }
    String body(r);
    if (!exponential) {
        if (scale() > reversed.size()) {
            throw std::length_error("Decimal display exceeds the locked Rust formatter capacity");
        }
        while (length < scale()) {
            reversed[length++] = '0';
        }
        auto precision = options.precision ? std::min(*options.precision, std::size_t(MAX_SCALE))
                                           : std::size_t(scale());
        auto whole = length - scale();
        for (std::size_t i = 0; i < whole + precision; ++i) {
            if (i == whole) {
                if (i == 0) {
                    body += '0';
                }
                body += '.';
            }
            body += i < length ? reversed[length - i - 1] : '0';
        }
        if (body.empty()) {
            body = "0";
        }
        if (body.size() > reversed.size()) {
            throw std::length_error("Decimal display exceeds the locked Rust formatter capacity");
        }
        if (options.precision && *options.precision > MAX_SCALE) {
            repeat(body, "0", *options.precision - MAX_SCALE);
        }
    } else {
        String digits(r);
        for (std::size_t i = length; i != 0; --i) {
            digits += reversed[i - 1];
        }
        body += digits.front();
        if (options.precision) {
            if (*options.precision > 0) {
                body += '.';
                auto count = std::min(*options.precision, digits.size() - 1);
                body.append(digits.data() + 1, count);
                repeat(body, "0", *options.precision - count);
            }
        } else if (length > 1 && !std::all_of(digits.begin() + 1, digits.end(), [](char c) {
                       return c == '0';
                   })) {
            body += '.';
            body.append(digits.data() + 1, digits.size() - 1);
        }
        body += options.notation == DecimalNotation::UpperExp ? 'E' : 'e';
        char exponent[16];
        auto end =
            std::to_chars(exponent, exponent + sizeof(exponent), int(length) - 1 - int(scale()))
                .ptr;
        body.append(exponent, end);
    }
    char sign = is_sign_negative() ? '-' : options.sign_plus ? '+' : '\0';
    auto size = body.size() + std::size_t(sign != '\0');
    auto padding = options.width > size ? options.width - size : 0;
    String out(r);
    if (options.zero_pad) {
        if (sign) {
            out += sign;
        }
        repeat(out, "0", padding);
        out += body;
    } else {
        auto left = options.alignment == DecimalAlignment::Left     ? 0
                    : options.alignment == DecimalAlignment::Center ? padding / 2
                                                                    : padding;
        repeat(out, fill, left);
        if (sign) {
            out += sign;
        }
        out += body;
        repeat(out, fill, padding - left);
    }
    return out;
}
}
