#include <teaql/core/json.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <teaql/memory/scoped.hpp>
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace teaql {
Json::Json(Memory r, double v) : Json(r) {
    if (std::isfinite(v))
        data = v;
}

Json::Json(Memory r, const Array& a) : Json(r) {
    Array copy(r);
    copy.reserve(a.size());
    for (const auto& v : a)
        copy.emplace_back(v);
    data = std::move(copy);
}

Json::Json(Memory r, const Object& o) : Json(r) {
    Object copy(r);
    for (const auto& [k, v] : o)
        copy.emplace(k, v);
    data = std::move(copy);
}

Json::Json(Memory r, const Json& v) : Json(r) {
    std::visit(
        [&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::same_as<T, String> || std::same_as<T, Array> ||
                          std::same_as<T, Object>) {
                Json copy(r, item);
                data = std::move(copy.data);
            } else
                data = item;
        },
        v.data);
}

Json::Json(Memory r, Json&& v) : Json(r) {
    if (r == v.resource_)
        data = std::move(v.data);
    else {
        Json copy(r, std::as_const(v));
        data = std::move(copy.data);
    }
}

Json& Json::operator=(Json&& v) {
    return detail::assign_scoped(*this, std::move(v));
}

namespace {
void unicode(String& s, unsigned n) {
    if (n < 0x80)
        s += char(n);
    else if (n < 0x800) {
        s += char(0xc0 | (n >> 6));
        s += char(0x80 | (n & 63));
    } else if (n < 0x10000) {
        s += char(0xe0 | (n >> 12));
        s += char(0x80 | ((n >> 6) & 63));
        s += char(0x80 | (n & 63));
    } else {
        s += char(0xf0 | (n >> 18));
        s += char(0x80 | ((n >> 12) & 63));
        s += char(0x80 | ((n >> 6) & 63));
        s += char(0x80 | (n & 63));
    }
}

struct Parser {
    Memory r;
    std::string_view s;
    std::size_t pos = 0;

    [[noreturn]] void fail(std::string_view message, bool peek = false) const {
        auto end = std::min(s.size(), pos + std::size_t(peek));
        std::size_t line = 1, column = 0;
        for (std::size_t i = 0; i < end; ++i) {
            if (s[i] == '\n') {
                ++line;
                column = 0;
            } else
                ++column;
        }
        throw JsonParseError{message, line, column};
    }

    void whitespace() {
        while (pos < s.size() &&
               (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
            ++pos;
    }

    bool take(char c) {
        if (pos == s.size() || s[pos] != c)
            return false;
        ++pos;
        return true;
    }

    void expect(char c) {
        if (!take(c))
            fail("expected value", true);
    }

    unsigned hex() {
        unsigned n = 0;
        if (s.size() - pos < 4) {
            pos = s.size();
            fail("EOF while parsing a string");
        }
        bool valid = true;
        for (unsigned i = 0; i < 4; ++i) {
            if (pos == s.size())
                fail("EOF while parsing a string");
            auto c = s[pos++];
            if (c >= '0' && c <= '9')
                n = n * 16 + unsigned(c - '0');
            else if (c >= 'a' && c <= 'f')
                n = n * 16 + unsigned(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                n = n * 16 + unsigned(c - 'A' + 10);
            else
                valid = false;
        }
        if (!valid)
            fail("invalid escape");
        return n;
    }

    String string() {
        expect('"');
        String out(r);
        while (pos < s.size()) {
            auto c = static_cast<unsigned char>(s[pos++]);
            if (c == '"')
                return out;
            if (c < 32)
                fail("control character (\\u0000-\\u001F) found while parsing a string");
            if (c == '\\') {
                if (pos == s.size())
                    fail("EOF while parsing a string");
                auto escape = s[pos++];
                switch (escape) {
                case '"':
                case '\\':
                case '/':
                    out += escape;
                    break;
                case 'b':
                    out += '\b';
                    break;
                case 'f':
                    out += '\f';
                    break;
                case 'n':
                    out += '\n';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case 't':
                    out += '\t';
                    break;
                case 'u': {
                    auto n = hex();
                    if (n >= 0xdc00 && n <= 0xdfff)
                        fail("lone leading surrogate in hex escape");
                    if (n >= 0xd800 && n <= 0xdbff) {
                        if (pos == s.size())
                            fail("EOF while parsing a string");
                        if (s[pos++] != '\\')
                            fail("unexpected end of hex escape");
                        if (pos == s.size())
                            fail("EOF while parsing a string");
                        if (s[pos++] != 'u')
                            fail("unexpected end of hex escape");
                        auto low = hex();
                        if (low < 0xdc00 || low > 0xdfff)
                            fail("lone leading surrogate in hex escape");
                        n = 0x10000 + ((n - 0xd800) << 10) + low - 0xdc00;
                    }
                    unicode(out, n);
                    break;
                }
                default:
                    fail("invalid escape");
                }
            } else if (c < 128)
                out += char(c);
            else {
                unsigned count = c >= 0xc2 && c <= 0xdf   ? 1
                                 : c >= 0xe0 && c <= 0xef ? 2
                                 : c >= 0xf0 && c <= 0xf4 ? 3
                                                          : 0;
                if (!count)
                    fail("invalid unicode code point");
                unsigned n = c & ((1u << (6 - count)) - 1);
                for (unsigned i = 0; i < count; ++i) {
                    if (pos == s.size())
                        fail("EOF while parsing a string");
                    auto next = static_cast<unsigned char>(s[pos++]);
                    if ((next & 0xc0) != 0x80)
                        fail("invalid unicode code point");
                    n = (n << 6) | (next & 63);
                }
                if (n < (count == 1   ? 0x80u
                         : count == 2 ? 0x800u
                                      : 0x10000u) ||
                    n > 0x10ffff || (n >= 0xd800 && n <= 0xdfff))
                    fail("invalid unicode code point");
                unicode(out, n);
            }
        }
        fail("EOF while parsing a string");
    }

    bool digit() const {
        return pos < s.size() && s[pos] >= '0' && s[pos] <= '9';
    }

    Json number() {
        auto start = pos;
        const bool negative = take('-');
        bool floating = false;
        auto required_digit = [&] {
            if (!digit())
                fail(pos == s.size() ? "EOF while parsing a value" : "invalid number", true);
        };
        if (!take('0')) {
            required_digit();
            while (digit())
                ++pos;
        } else if (digit())
            fail("invalid number", true);
        if (take('.')) {
            floating = true;
            required_digit();
            while (digit())
                ++pos;
        }
        if (take('e') || take('E')) {
            floating = true;
            if (!take('+'))
                take('-');
            required_digit();
            while (digit())
                ++pos;
        }
        // Match serde_json 1.0.150's default (non-float_roundtrip) parser.
        // Adapted numeric conversion: see LICENSES/serde_json-MIT.txt.
        auto literal = s.substr(start + unsigned(negative), pos - start - unsigned(negative));
        std::size_t i = 0;
        std::uint64_t significand = 0;
        int exponent = 0;
        bool overflow = false;
        while (i < literal.size() && literal[i] >= '0' && literal[i] <= '9') {
            const auto digit = unsigned(literal[i++] - '0');
            if (overflow || significand > (UINT64_MAX - digit) / 10) {
                overflow = true;
                ++exponent;
            } else
                significand = significand * 10 + digit;
        }
        if (!floating && !overflow) {
            if (!negative)
                return Json(r, significand);
            if (significand != 0 && significand <= std::uint64_t(INT64_MAX) + 1)
                return Json(r,
                            significand == std::uint64_t(INT64_MAX) + 1
                                ? INT64_MIN
                                : -std::int64_t(significand));
            return Json(r, -double(significand));
        }
        if (i < literal.size() && literal[i] == '.') {
            ++i;
            bool full = false;
            while (i < literal.size() && literal[i] >= '0' && literal[i] <= '9') {
                const auto digit = unsigned(literal[i++] - '0');
                if (full || significand > (UINT64_MAX - digit) / 10)
                    full = true;
                else {
                    significand = significand * 10 + digit;
                    --exponent;
                }
            }
        }
        if (i < literal.size()) {
            ++i;
            bool minus = false;
            if (literal[i] == '+' || literal[i] == '-')
                minus = literal[i++] == '-';
            int explicit_exp = 0;
            for (; i < literal.size(); ++i) {
                explicit_exp = explicit_exp * 10 + literal[i] - '0';
                if (explicit_exp > 1000000)
                    break;
            }
            exponent += minus ? -explicit_exp : explicit_exp;
        }
        double n = double(significand);
        while (exponent < -308 && n != 0) {
            n /= 1e308;
            exponent += 308;
        }
        if (n != 0) {
            if (exponent > 308)
                fail("number out of range");
            char power_text[16]{'1', 'e'};
            auto [last, ignored] = std::to_chars(power_text + 2,
                                                 power_text + sizeof(power_text),
                                                 exponent < 0 ? -exponent : exponent);
            (void)ignored;
            double power = 1;
            std::from_chars(power_text, last, power);
            if (exponent < 0)
                n /= power;
            else
                n *= power;
            if (!std::isfinite(n))
                fail("number out of range");
        }
        return Json(r, negative ? -n : n);
    }

    Json value(unsigned depth = 0) {
        whitespace();
        if (pos == s.size())
            fail("EOF while parsing a value");
        Json result(r);
        if (s[pos] == '"') {
            result.data = string();
            return result;
        }
        if (s[pos] == '[' || s[pos] == '{') {
            if (depth >= 127)
                fail("recursion limit exceeded", true);
            if (take('[')) {
                Json::Array list(r);
                whitespace();
                if (pos == s.size())
                    fail("EOF while parsing a list");
                if (!take(']')) {
                    while (true) {
                        list.emplace_back(value(depth + 1));
                        whitespace();
                        if (take(']'))
                            break;
                        if (pos == s.size())
                            fail("EOF while parsing a list");
                        if (!take(','))
                            fail("expected `,` or `]`", true);
                        whitespace();
                        if (pos < s.size() && s[pos] == ']')
                            fail("trailing comma", true);
                    }
                }
                result.data = std::move(list);
            } else {
                expect('{');
                Json::Object object(r);
                whitespace();
                if (!take('}')) {
                    while (true) {
                        whitespace();
                        if (pos == s.size())
                            fail("EOF while parsing an object");
                        if (s[pos] != '"')
                            fail("key must be a string", true);
                        auto key = string();
                        whitespace();
                        if (pos == s.size())
                            fail("EOF while parsing an object");
                        if (!take(':'))
                            fail("expected `:`", true);
                        auto v = value(depth + 1);
                        object.insert_or_assign(std::move(key), std::move(v));
                        whitespace();
                        if (take('}'))
                            break;
                        if (pos == s.size())
                            fail("EOF while parsing an object");
                        if (!take(','))
                            fail("expected `,` or `}`", true);
                        whitespace();
                        if (pos == s.size())
                            fail("EOF while parsing a value");
                        if (s[pos] == '}')
                            fail("trailing comma", true);
                    }
                }
                result.data = std::move(object);
            }
            return result;
        }
        for (auto word :
             {std::string_view("null"), std::string_view("true"), std::string_view("false")}) {
            if (s[pos] == word.front()) {
                for (char c : word) {
                    if (pos == s.size())
                        fail("EOF while parsing a value");
                    if (s[pos++] != c)
                        fail("expected ident");
                }
                if (word != "null")
                    result.data = word == "true";
                return result;
            }
        }
        if (s[pos] != '-' && !digit())
            fail("expected value", true);
        return number();
    }
};

void quoted(String& out, std::string_view s) {
    constexpr char hex[] = "0123456789abcdef";
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 32) {
                out += "\\u00";
                out += hex[c >> 4];
                out += hex[c & 15];
            } else
                out += char(c);
        }
    }
    out += '"';
}

void dump_into(String& out, const Json& v) {
    std::visit(
        [&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::same_as<T, std::monostate>)
                out += "null";
            else if constexpr (std::same_as<T, bool>)
                out += item ? "true" : "false";
            else if constexpr (std::same_as<T, double>)
                detail::append_float(out, item, true);
            else if constexpr (std::is_arithmetic_v<T>) {
                char buffer[64];
                auto [end, ec] = std::to_chars(buffer, buffer + sizeof(buffer), item);
                if (ec != std::errc{})
                    throw std::runtime_error("JSON number formatting failed");
                out.append(buffer, end);
            } else if constexpr (std::same_as<T, String>)
                quoted(out, item);
            else if constexpr (std::same_as<T, Json::Array>) {
                out += '[';
                bool comma = false;
                for (const auto& e : item) {
                    if (comma)
                        out += ',';
                    dump_into(out, e);
                    comma = true;
                }
                out += ']';
            } else {
                out += '{';
                bool comma = false;
                for (const auto& [k, e] : item) {
                    if (comma)
                        out += ',';
                    quoted(out, k);
                    out += ':';
                    dump_into(out, e);
                    comma = true;
                }
                out += '}';
            }
        },
        v.data);
}
}

std::optional<Json> Json::parse(Memory r, std::string_view s) {
    auto result = parse_result(r, s);
    if (!result)
        return {};
    return std::move(result).value();
}

Result<Json, JsonParseError> Json::parse_result(Memory r, std::string_view s) {
    try {
        Parser p{r, s};
        auto result = p.value();
        p.whitespace();
        if (p.pos != s.size())
            p.fail("trailing characters", true);
        return Result<Json, JsonParseError>::ok(r, std::move(result));
    } catch (const JsonParseError& e) {
        return Result<Json, JsonParseError>::err(r, e);
    }
}

String JsonParseError::to_string(Memory r) const {
    String out(message, r);
    out += " at line ";
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), line).ptr;
    out.append(buffer, end);
    out += " column ";
    end = std::to_chars(buffer, buffer + sizeof(buffer), column).ptr;
    out.append(buffer, end);
    return out;
}

String Json::dump(Memory r) const {
    String out(r);
    dump_into(out, *this);
    return out;
}
}
