#include <teaql/provider_sqlite/detail/decoding.hpp>
#include <teaql/core/detail/text.hpp>
#include <sqlite3.h>
#include <array>
#include <charconv>

namespace teaql::detail {
namespace {
using Decoded = Result<Value, MutationExecutorError>;

template <class T>
void append_number(String& out, T value) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), value).ptr;
    out.append(buffer, end);
}

Decoded invalid(Memory r, std::string_view type, std::string_view message) {
    String text("invalid sqlite ", r);
    text += type;
    text += ": ";
    text += message;
    return Decoded::err(r, MutationExecutorError::bind(r, text));
}

std::optional<String> utf8_error(Memory r, std::string_view s) {
    for (std::size_t p = 0; p < s.size();) {
        const auto first = static_cast<unsigned char>(s[p]);
        if (first < 128) {
            ++p;
            continue;
        }
        const unsigned width = first >= 0xc2 && first <= 0xdf   ? 2
                               : first >= 0xe0 && first <= 0xef ? 3
                               : first >= 0xf0 && first <= 0xf4 ? 4
                                                                : 0;
        unsigned length = 1;
        bool incomplete = false;
        if (width) {
            for (; length < width; ++length) {
                if (p + length == s.size()) {
                    incomplete = true;
                    break;
                }
                const auto byte = static_cast<unsigned char>(s[p + length]);
                if ((byte & 0xc0) != 0x80 ||
                    (length == 1 &&
                     ((first == 0xe0 && byte < 0xa0) || (first == 0xed && byte >= 0xa0) ||
                      (first == 0xf0 && byte < 0x90) || (first == 0xf4 && byte >= 0x90))))
                    break;
            }
            if (length == width) {
                p += width;
                continue;
            }
        }
        String error(r);
        if (incomplete)
            error = "incomplete utf-8 byte sequence from index ";
        else {
            error = "invalid utf-8 sequence of ";
            append_number(error, length);
            error += " bytes from index ";
        }
        append_number(error, p);
        return error;
    }
    return {};
}
enum class TimeError { Range, Invalid, Short, Long };

std::string_view time_error(TimeError e) {
    switch (e) {
    case TimeError::Range:
        return "input is out of range";
    case TimeError::Invalid:
        return "input contains invalid characters";
    case TimeError::Short:
        return "premature end of input";
    case TimeError::Long:
        return "trailing input";
    }
    std::terminate();
}

// Implements the provider's chrono format items. Field range validation happens
// while scanning; calendar validation happens after checking trailing input.
struct TimeParser {
    std::string_view s;
    std::int32_t year = 0;
    unsigned month = 0, day = 0, hour = 0, minute = 0, second = 0, millis = 0;

    void space() {
        trim_start(s);
    }

    bool take(char c) {
        if (s.empty() || s.front() != c)
            return false;
        s.remove_prefix(1);
        return true;
    }

    void literal(char c) {
        if (s.empty())
            throw TimeError::Short;
        if (!take(c))
            throw TimeError::Invalid;
    }

    std::int64_t number(std::size_t width, unsigned minimum = 1, bool skip = true) {
        if (skip)
            space();
        if (s.size() < minimum)
            throw TimeError::Short;
        std::int64_t value = 0;
        std::size_t n = 0;
        while (n < s.size() && n < width && s[n] >= '0' && s[n] <= '9') {
            auto digit = s[n] - '0';
            if (value > (INT64_MAX - digit) / 10)
                throw TimeError::Range;
            value = value * 10 + digit;
            ++n;
        }
        if (n < minimum)
            throw TimeError::Invalid;
        s.remove_prefix(n);
        return value;
    }

    unsigned field(unsigned maximum, unsigned minimum = 0) {
        auto value = number(2);
        if (value < minimum || value > maximum)
            throw TimeError::Range;
        return unsigned(value);
    }

    void date() {
        space();
        bool negative = take('-');
        bool signed_year = negative || take('+');
        auto value = number(signed_year ? s.size() : 4, 1, false);
        if (negative)
            value = -value;
        if (value < INT32_MIN || value > INT32_MAX)
            throw TimeError::Range;
        year = std::int32_t(value);
        literal('-');
        month = field(12, 1);
        literal('-');
        day = field(31, 1);
    }

    void time() {
        space();
        hour = field(23);
        literal(':');
        minute = field(59);
        literal(':');
        second = field(60);
        if (take('.')) {
            auto start = s.size();
            auto nanos = number(9, 1, false);
            auto digits = start - s.size();
            for (; digits < 9; ++digits)
                nanos *= 10;
            millis = unsigned(nanos / 1000000);
            while (!s.empty() && s.front() >= '0' && s.front() <= '9')
                s.remove_prefix(1);
        }
    }

    int offset() {
        space();
        if (take('Z') || take('z'))
            return 0;
        bool negative = take('-');
        if (!negative && s.starts_with("−")) {
            negative = true;
            s.remove_prefix(3);
        }
        if (!negative)
            literal('+');
        auto h = number(2, 2, false);
        for (;;) {
            space();
            if (!take(':'))
                break;
        }
        auto m = s.empty() ? 0 : number(2, 2, false);
        if (h > 23 || m > 59)
            throw TimeError::Range;
        return int(h * 3600 + m * 60) * (negative ? -1 : 1);
    }

    Date finish_date() {
        if (!s.empty())
            throw TimeError::Long;
        auto result = Date::from_ymd(year, month, day);
        if (!result)
            throw TimeError::Range;
        return *result;
    }

    Timestamp finish_time(int offset = 0) {
        auto result = finish_date().epoch_days() * 86400000 +
                      (std::int64_t(hour) * 3600 + minute * 60 + second - offset) * 1000 + millis;
        auto calendar_millis = result - (second == 60 ? 1000 : 0);
        auto days = calendar_millis / 86400000 - (calendar_millis % 86400000 < 0);
        if (!Date::from_epoch_days(days))
            throw TimeError::Range;
        return Timestamp(result);
    }
};

std::optional<Timestamp> fixed_timestamp(std::string_view s) {
    if (s.size() < 19 || s[4] != '-' || s[7] != '-' || (s[10] != ' ' && s[10] != 'T') ||
        s[13] != ':' || s[16] != ':')
        return {};
    for (unsigned i : {0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18})
        if (s[i] < '0' || s[i] > '9')
            return {};
    if (s[17] == '6')
        return {}; // Fast path excludes leap seconds.
    std::array<char, 19> base{};
    std::copy_n(s.begin(), 19, base.begin());
    base[10] = ' ';
    auto time = Timestamp::parse(std::string_view(base.data(), base.size()));
    if (!time)
        return {};
    TimeParser p{s.substr(19)};
    unsigned millis = 0;
    try {
        if (p.take('.')) {
            auto start = p.s.size();
            auto nanos = p.number(9, 1, false);
            auto digits = start - p.s.size();
            for (; digits < 9; ++digits)
                nanos *= 10;
            millis = unsigned(nanos / 1000000);
            while (!p.s.empty() && p.s.front() >= '0' && p.s.front() <= '9')
                p.s.remove_prefix(1);
        }
        int offset = 0;
        if (!p.s.empty()) {
            if (p.take('Z') || p.take('z')) {
                if (!p.s.empty())
                    return {};
            } else {
                const bool negative = p.take('-');
                if (!negative && !p.take('+'))
                    return {};
                auto hours = p.number(2, 2, false);
                std::int64_t minutes = 0;
                if (!p.s.empty()) {
                    p.take(':');
                    minutes = p.number(2, 2, false);
                }
                if (!p.s.empty() || hours > 23 || minutes > 59)
                    return {};
                offset = int(hours * 3600 + minutes * 60) * (negative ? -1 : 1);
            }
        }
        return Timestamp(time->as_millis() + millis - std::int64_t(offset) * 1000);
    } catch (TimeError) {
        return {};
    }
}

Decoded timestamp(Memory r, std::string_view s) {
    if (auto value = fixed_timestamp(s))
        return Decoded::ok(r, Value(r, *value));
    if (auto value = Timestamp::parse(s))
        return Decoded::ok(r, Value(r, *value));
    try {
        TimeParser p{s};
        p.date();
        p.time();
        auto offset = p.offset();
        return Decoded::ok(r, Value(r, p.finish_time(offset)));
    } catch (TimeError) {
    }
    try {
        TimeParser p{s};
        p.date();
        p.time();
        return Decoded::ok(r, Value(r, p.finish_time()));
    } catch (TimeError e) {
        return invalid(r, "timestamp", time_error(e));
    }
}

Vector<String> names(Memory r, sqlite3_stmt& stmt) {
    Vector<String> result(r);
    for (int i = 0; i < sqlite3_column_count(&stmt); ++i) {
        auto* name = sqlite3_column_name(&stmt, i);
        if (!name)
            throw std::bad_alloc();
        result.emplace_back(name);
    }
    return result;
}
}

SqliteDecodeKind sqlite_decode_kind(std::optional<std::string_view> declaration) {
    if (!declaration)
        return SqliteDecodeKind::Infer;
    auto base = declaration->substr(0, declaration->find('('));
    trim(base);
    auto is = [&](std::string_view type) {
        if (base.size() != type.size())
            return false;
        for (std::size_t i = 0; i < base.size(); ++i) {
            auto c = base[i];
            if (c >= 'a' && c <= 'z')
                c = char(c - 'a' + 'A');
            if (c != type[i])
                return false;
        }
        return true;
    };
    if (is("BOOL") || is("BOOLEAN"))
        return SqliteDecodeKind::Bool;
    if (is("NUMERIC") || is("DECIMAL"))
        return SqliteDecodeKind::Decimal;
    if (is("JSON"))
        return SqliteDecodeKind::Json;
    if (is("DATE"))
        return SqliteDecodeKind::Date;
    if (is("TIMESTAMP") || is("DATETIME"))
        return SqliteDecodeKind::Timestamp;
    if (is("TEXT") || is("VARCHAR") || is("CHAR") || is("CLOB"))
        return SqliteDecodeKind::Text;
    return SqliteDecodeKind::Infer;
}

std::optional<String> sqlite_utf8_error(Memory r, std::string_view text) {
    return utf8_error(r, text);
}

SqliteRowLayout::SqliteRowLayout(std::allocator_arg_t, Allocator a, sqlite3_stmt& statement)
    : columns_(std::allocator_arg, a, names(a.resource(), statement)), kinds_(a.resource()) {
    for (int i = 0; i < sqlite3_column_count(&statement); ++i) {
        auto* type = sqlite3_column_decltype(&statement, i);
        kinds_.push_back(
            sqlite_decode_kind(type ? std::optional<std::string_view>(type) : std::nullopt));
    }
}

SqliteRowLayout::SqliteRowLayout(std::allocator_arg_t, Allocator a, const SqliteRowLayout& other)
    : columns_(std::allocator_arg, a, other.columns_), kinds_(other.kinds_, a.resource()) {}

Decoded decode_sqlite_text(Memory r, std::string_view s, SqliteDecodeKind kind) {
    if (auto error = utf8_error(r, s))
        return invalid(r, "text", *error);
    switch (kind) {
    case SqliteDecodeKind::Decimal: {
        auto value = Decimal::parse_result(r, s);
        if (!value)
            return invalid(r, "decimal", value.error().to_string(r));
        return Decoded::ok(r, Value(r, value.value()));
    }
    case SqliteDecodeKind::Json: {
        auto value = Json::parse_result(r, s);
        if (!value)
            return invalid(r, "json value", value.error().to_string(r));
        return Decoded::ok(r, Value(r, std::move(value).value()));
    }
    case SqliteDecodeKind::Date:
        try {
            TimeParser p{s};
            p.date();
            return Decoded::ok(r, Value(r, p.finish_date()));
        } catch (TimeError e) {
            return invalid(r, "date", time_error(e));
        }
    case SqliteDecodeKind::Timestamp:
        return timestamp(r, s);
    case SqliteDecodeKind::Infer:
        if (auto date = Date::parse(s))
            return Decoded::ok(r, Value(r, *date));
        if (auto value = Timestamp::parse(s))
            return Decoded::ok(r, Value(r, *value));
        [[fallthrough]];
    case SqliteDecodeKind::Bool:
    case SqliteDecodeKind::Text:
        return Decoded::ok(r, Value(r, s));
    }
    std::terminate();
}

Result<CompactRow, MutationExecutorError>
decode_sqlite_row(Memory r, sqlite3_stmt& statement, const SqliteRowLayout& layout) {
    using Output = Result<CompactRow, MutationExecutorError>;
    Vector<Value> values(r);
    values.reserve(layout.kinds().size());
    for (std::size_t i = 0; i < layout.kinds().size(); ++i) {
        if (i >= std::size_t(sqlite3_column_count(&statement))) {
            String message("Invalid column index: ", r);
            append_number(message, i);
            return Output::err(r, MutationExecutorError::sqlite(r, 0, message));
        }
        auto column = int(i);
        auto kind = layout.kinds()[i];
        switch (sqlite3_column_type(&statement, column)) {
        case SQLITE_NULL:
            values.emplace_back();
            break;
        case SQLITE_INTEGER:
            if (kind == SqliteDecodeKind::Bool)
                values.emplace_back(sqlite3_column_int64(&statement, column) != 0);
            else
                values.emplace_back(std::int64_t(sqlite3_column_int64(&statement, column)));
            break;
        case SQLITE_FLOAT:
            values.emplace_back(sqlite3_column_double(&statement, column));
            break;
        case SQLITE_TEXT: {
            auto* text = sqlite3_column_text(&statement, column);
            if (!text)
                return Output::err(r,
                                   MutationExecutorError::sqlite(r, SQLITE_NOMEM, "out of memory"));
            auto decoded =
                decode_sqlite_text(r,
                                   std::string_view(reinterpret_cast<const char*>(text),
                                                    sqlite3_column_bytes(&statement, column)),
                                   kind);
            if (!decoded)
                return Output::err(r, std::move(decoded).error());
            values.push_back(std::move(decoded).value());
            break;
        }
        case SQLITE_BLOB:
            return Output::err(r, MutationExecutorError::unsupported_column_type(r, "BLOB"));
        default:
            throw std::logic_error("unknown SQLite storage class");
        }
    }
    return Output::ok(
        r, CompactRow(std::allocator_arg, Allocator(r), layout.columns(), std::move(values)));
}
}
