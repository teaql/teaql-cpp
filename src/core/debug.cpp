#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/debug_escape_ranges.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <algorithm>
#include <charconv>
#include <cmath>

namespace teaql::detail {
String debug_text(Memory r, std::string_view text) {
    String out("\"", r);
    for (std::size_t i = 0; i < text.size();) {
        auto start = i;
        auto byte = static_cast<unsigned char>(text[i++]);
        char32_t code = byte;
        if (byte >= 0x80) {
            unsigned count = byte >= 0xF0 ? 3 : byte >= 0xE0 ? 2 : 1;
            code = byte & ((1u << (6 - count)) - 1);
            if (byte < 0xC2 || byte > 0xF4 || i + count > text.size()) {
                throw std::invalid_argument("Invalid UTF-8 in debug text");
            }
            for (unsigned j = 0; j < count; ++j) {
                auto next = static_cast<unsigned char>(text[i++]);
                if ((next & 0xC0) != 0x80) {
                    throw std::invalid_argument("Invalid UTF-8 in debug text");
                }
                code = (code << 6) | (next & 63);
            }
            if ((count == 1 && code < 0x80) || (count == 2 && code < 0x800) ||
                (count == 3 && code < 0x10000) || code > 0x10FFFF ||
                (code >= 0xD800 && code <= 0xDFFF)) {
                throw std::invalid_argument("Invalid UTF-8 in debug text");
            }
        }
        if (code == '\n') {
            out += "\\n";
        } else if (code == '\r') {
            out += "\\r";
        } else if (code == '\t') {
            out += "\\t";
        } else if (!code) {
            out += "\\0";
        } else if (code == '\\') {
            out += "\\\\";
        } else if (code == '"') {
            out += "\\\"";
        } else {
            auto it = std::lower_bound(detail::debug_escape_ranges.begin(),
                                       detail::debug_escape_ranges.end(),
                                       code,
                                       [](const auto& range, char32_t c) {
                                           return range.second < c;
                                       });
            if (it != detail::debug_escape_ranges.end() && it->first <= code) {
                char buf[8];
                auto [end, error] = std::to_chars(buf, buf + sizeof(buf), std::uint32_t(code), 16);
                (void)error;
                out += "\\u{";
                out.append(buf, end);
                out += '}';
            } else {
                out.append(text.substr(start, i - start));
            }
        }
    }
    out += '"';
    return out;
}
}

namespace teaql::detail {
namespace {
template <class T>
String number(Memory r, T n) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), n).ptr;
    return String(buffer, end, r);
}

String debug_float(Memory r, double n) {
    String display(r);
    append_float(display, n, false);
    if (!std::isfinite(n)) {
        return display;
    }
    auto unsigned_text = std::string_view(display);
    bool negative = unsigned_text.starts_with('-');
    if (negative) {
        unsigned_text.remove_prefix(1);
    }
    auto point = unsigned_text.find('.');
    if (point == unsigned_text.npos) {
        point = unsigned_text.size();
    }
    String digits(r);
    for (char c : unsigned_text) {
        if (c != '.') {
            digits += c;
        }
    }
    auto first = digits.find_first_not_of('0');
    if (first == digits.npos) {
        display += ".0";
        return display;
    }
    auto exponent = std::int64_t(point) - std::int64_t(first) - 1;
    if (exponent >= -4 && exponent <= 15) {
        if (display.find('.') == display.npos) {
            display += ".0";
        }
        return display;
    }
    digits.erase(0, first);
    while (digits.size() > 1 && digits.back() == '0') {
        digits.pop_back();
    }
    String result(r);
    if (negative) {
        result += '-';
    }
    result += digits.front();
    if (digits.size() > 1) {
        result += '.';
        result.append(digits.data() + 1, digits.size() - 1);
    }
    result += 'e';
    result += number(r, exponent);
    return result;
}

String json_debug(Memory r, const Json& value) {
    return std::visit(
        [&](const auto& v) -> String {
            using T = std::decay_t<decltype(v)>;
            String out(r);
            if constexpr (std::same_as<T, std::monostate>) {
                out = "Null";
            } else if constexpr (std::same_as<T, bool>) {
                out = v ? "Bool(true)" : "Bool(false)";
            } else if constexpr (std::same_as<T, String>) {
                out = "String(";
                out += debug_text(r, v);
                out += ')';
            } else if constexpr (std::same_as<T, Json::Array>) {
                out = "Array [";
                bool first = true;
                for (const auto& item : v) {
                    if (!first) {
                        out += ", ";
                    }
                    first = false;
                    out += json_debug(r, item);
                }
                out += ']';
            } else if constexpr (std::same_as<T, Json::Object>) {
                out = "Object {";
                bool first = true;
                for (const auto& [key, item] : v) {
                    if (!first) {
                        out += ", ";
                    }
                    first = false;
                    out += debug_text(r, key);
                    out += ": ";
                    out += json_debug(r, item);
                }
                out += '}';
            } else {
                out = "Number(";
                out += value.dump(r);
                out += ')';
            }
            return out;
        },
        value.data);
}
}

String debug_value(Memory r, const Value& value) {
    return std::visit(
        [&](const auto& v) -> String {
            using T = std::decay_t<decltype(v)>;
            String out(r);
            if constexpr (std::same_as<T, std::monostate>) {
                return String("Null", r);
            } else if constexpr (std::same_as<T, bool>) {
                return String(v ? "Bool(true)" : "Bool(false)", r);
            } else if constexpr (std::same_as<T, std::int64_t>) {
                out = "I64(";
            } else if constexpr (std::same_as<T, std::uint64_t>) {
                out = "U64(";
            } else if constexpr (std::same_as<T, double>) {
                out = "F64(";
            } else if constexpr (std::same_as<T, Decimal>) {
                out = "Decimal(";
            } else if constexpr (std::same_as<T, String>) {
                out = "Text(";
            } else if constexpr (std::same_as<T, Json>) {
                out = "Json(";
            } else if constexpr (std::same_as<T, Date>) {
                out = "Date(";
            } else if constexpr (std::same_as<T, Timestamp>) {
                out = "Timestamp(Timestamp(";
            } else if constexpr (std::same_as<T, TypedNull>) {
                out = "TypedNull(";
            } else if constexpr (std::same_as<T, Value::List>) {
                out = "List([";
            } else if constexpr (std::same_as<T, Value::Object>) {
                out = "Object({";
            }
            if constexpr (std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t>) {
                out += number(r, v);
            } else if constexpr (std::same_as<T, double>) {
                out += debug_float(r, v);
            } else if constexpr (std::same_as<T, Decimal> || std::same_as<T, Date>) {
                out += v.to_string(r);
            } else if constexpr (std::same_as<T, String>) {
                out += debug_text(r, v);
            } else if constexpr (std::same_as<T, Json>) {
                out += json_debug(r, v);
            } else if constexpr (std::same_as<T, Timestamp>) {
                out += number(r, v.as_millis());
                out += ')';
            } else if constexpr (std::same_as<T, TypedNull>) {
                constexpr std::string_view names[]{"Bool",
                                                   "I64",
                                                   "U64",
                                                   "F64",
                                                   "Decimal",
                                                   "Text",
                                                   "LargeText",
                                                   "Json",
                                                   "Date",
                                                   "Timestamp"};
                out += names[unsigned(v.type)];
            } else if constexpr (std::same_as<T, Value::List>) {
                bool first = true;
                for (const auto& item : v) {
                    if (!first) {
                        out += ", ";
                    }
                    first = false;
                    out += debug_value(r, item);
                }
                out += ']';
            } else if constexpr (std::same_as<T, Value::Object>) {
                bool first = true;
                for (const auto& [key, item] : v) {
                    if (!first) {
                        out += ", ";
                    }
                    first = false;
                    out += debug_text(r, key);
                    out += ": ";
                    out += debug_value(r, item);
                }
                out += '}';
            }
            out += ')';
            return out;
        },
        value.data);
}
}
