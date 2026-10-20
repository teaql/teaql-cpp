#include <teaql/core/time.hpp>
#include <teaql/core/json.hpp>
#include <teaql/core/detail/text.hpp>
#include <teaql/core/detail/system_clock.hpp>
#include <bit>
#include <charconv>
#include <stdexcept>

namespace teaql {
Json Timestamp::to_json_value(Memory destination) const {
    return Json(destination, millis);
}

std::optional<Timestamp> Timestamp::from_json_value(const Json& value) {
    if (const auto* signed_value = std::get_if<std::int64_t>(&value.data)) {
        return Timestamp(*signed_value);
    }
    if (const auto* unsigned_value = std::get_if<std::uint64_t>(&value.data)) {
        if (*unsigned_value <= std::uint64_t(INT64_MAX)) {
            return Timestamp(std::int64_t(*unsigned_value));
        }
    }
    return {};
}

namespace {
constexpr std::int64_t day_ms = 86400000;

std::int64_t floor_div(std::int64_t a, std::int64_t b) {
    auto q = a / b;
    return q - (a % b < 0);
}

bool leap(std::int32_t y) {
    return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

struct Parser {
    std::string_view s;
    std::size_t pos = 0;

    void whitespace() {
        auto rest = s.substr(pos);
        detail::trim_start(rest);
        pos = s.size() - rest.size();
    }

    bool take(char c) {
        if (pos == s.size() || s[pos] != c) {
            return false;
        }
        ++pos;
        return true;
    }

    std::optional<unsigned> number(unsigned min, std::size_t max, bool skip = true) {
        if (skip) {
            whitespace();
        }
        auto start = pos;
        unsigned value = 0;
        while (pos < s.size() && pos - start < max && s[pos] >= '0' && s[pos] <= '9') {
            auto digit = unsigned(s[pos++] - '0');
            if (value > (UINT32_MAX - digit) / 10) {
                return {};
            }
            value = value * 10 + digit;
        }
        if (pos - start < min) {
            return {};
        }
        return value;
    }

    std::optional<Date> date(bool strict) {
        if (!strict) {
            whitespace();
        }
        bool negative = false, sign = false;
        if (!strict && pos < s.size() && (s[pos] == '-' || s[pos] == '+')) {
            sign = true;
            negative = s[pos++] == '-';
        }
        auto y = number(strict ? 4 : 1, sign ? s.size() : 4, false);
        if (!y || *y > 262143 || !take('-')) {
            return {};
        }
        auto m = number(strict ? 2 : 1, 2, !strict);
        if (!m || !take('-')) {
            return {};
        }
        auto d = number(strict ? 2 : 1, 2, !strict);
        if (!d) {
            return {};
        }
        return Date::from_ymd(negative ? -std::int32_t(*y) : std::int32_t(*y), *m, *d);
    }
};

std::optional<Timestamp> parse_timestamp(std::string_view s, bool rfc) {
    Parser p{s};
    auto date = p.date(rfc);
    if (!date) {
        return {};
    }
    if (rfc) {
        if (!(p.take('T') || p.take('t') || p.take(' '))) {
            return {};
        }
    } else {
        p.whitespace();
    }
    auto h = p.number(rfc ? 2 : 1, 2, !rfc);
    if (!h || *h > 23 || !p.take(':')) {
        return {};
    }
    auto m = p.number(rfc ? 2 : 1, 2, !rfc);
    if (!m || *m > 59 || !p.take(':')) {
        return {};
    }
    auto sec = p.number(rfc ? 2 : 1, 2, !rfc);
    if (!sec || *sec > 60) {
        return {};
    }
    unsigned ms = 0;
    if (rfc && p.take('.')) {
        auto start = p.pos;
        while (p.pos < s.size() && s[p.pos] >= '0' && s[p.pos] <= '9') {
            if (p.pos - start < 3) {
                ms = ms * 10 + unsigned(s[p.pos] - '0');
            }
            ++p.pos;
        }
        auto digits = p.pos - start;
        if (!digits) {
            return {};
        }
        while (digits++ < 3) {
            ms *= 10;
        }
    }
    int offset = 0;
    if (rfc && !(p.take('Z') || p.take('z'))) {
        bool minus = p.take('-');
        if (!minus && s.substr(p.pos).starts_with("−")) {
            minus = true;
            p.pos += 3;
        }
        if (!minus && !p.take('+')) {
            return {};
        }
        auto oh = p.number(2, 2, false);
        if (!oh || *oh > 23 || !p.take(':')) {
            return {};
        }
        auto om = p.number(2, 2, false);
        if (!om || *om > 59) {
            return {};
        }
        offset = int(*oh * 60 + *om) * (minus ? -1 : 1);
    }
    if (p.pos != s.size()) {
        return {};
    }
    auto millis = date->epoch_days() * day_ms + std::int64_t(*h * 3600 + *m * 60 + *sec) * 1000 +
                  ms - std::int64_t(offset) * 60000;
    // Chrono represents a leap second as second 59 plus >= 1e9 nanoseconds;
    // its last representable date therefore still admits the final leap second.
    if (!Date::from_epoch_days(floor_div(millis - (*sec == 60 ? 1000 : 0), day_ms))) {
        return {};
    }
    return Timestamp(millis);
}
}

std::optional<Date> Date::from_ymd(std::int32_t y, unsigned m, unsigned d) {
    constexpr unsigned days[]{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (y < -262143 || y > 262142 || m < 1 || m > 12 || d < 1 ||
        d > days[m - 1] + unsigned(m == 2 && leap(y))) {
        return {};
    }
    Date date;
    date.year_ = y;
    date.month_ = static_cast<unsigned char>(m);
    date.day_ = static_cast<unsigned char>(d);
    return date;
}

std::int64_t Date::epoch_days() const {
    auto y = std::int64_t(year_) - (month_ <= 2), era = floor_div(y, 400), yo = y - era * 400;
    auto m = int(month_) + (month_ > 2 ? -3 : 9);
    return era * 146097 + yo * 365 + yo / 4 - yo / 100 + (153 * m + 2) / 5 + day_ - 1 - 719468;
}

std::optional<Date> Date::from_epoch_days(std::int64_t days) {
    // Check before arithmetic so arbitrary Timestamp values cannot overflow.
    if (days < -96465292 || days > 95026236) {
        return {};
    }
    auto z = days + 719468, era = floor_div(z, 146097), doe = z - era * 146097;
    auto yo = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    auto y = yo + era * 400, doy = doe - (yo * 365 + yo / 4 - yo / 100), mp = (5 * doy + 2) / 153;
    auto d = doy - (153 * mp + 2) / 5 + 1, m = mp + (mp < 10 ? 3 : -9);
    y += m <= 2;
    return from_ymd(std::int32_t(y), unsigned(m), unsigned(d));
}

std::optional<Date> Date::parse(std::string_view s) {
    Parser p{s};
    auto d = p.date(false);
    return p.pos == s.size() ? d : std::nullopt;
}

String Date::to_string(Memory r) const {
    String out(r);
    char buffer[16];
    if (year_ < 0) {
        out += '-';
    } else if (year_ > 9999) {
        out += '+';
    }
    auto [end, ec] = std::to_chars(buffer, buffer + sizeof(buffer), year_ < 0 ? -year_ : year_);
    (void)ec;
    auto size = std::size_t(end - buffer);
    if (size < 4) {
        out.append(4 - size, '0');
    }
    out.append(buffer, end);
    out += '-';
    out += char('0' + month_ / 10);
    out += char('0' + month_ % 10);
    out += '-';
    out += char('0' + day_ / 10);
    out += char('0' + day_ % 10);
    return out;
}

Timestamp Timestamp::from_u64(std::uint64_t n) {
    return Timestamp(std::bit_cast<std::int64_t>(n));
}

Timestamp Timestamp::now() {
    auto time = detail::system_utc_time();
    // The validated chrono date range fits in signed milliseconds.
    return Timestamp(time.seconds * 1000 + time.nanoseconds / 1000000);
}

std::optional<Timestamp> Timestamp::parse(std::string_view s) {
    if (auto t = parse_timestamp(s, true)) {
        return t;
    }
    if (auto t = parse_timestamp(s, false)) {
        return t;
    }
    if (auto d = Date::parse(s)) {
        return Timestamp(d->epoch_days() * day_ms);
    }
    return {};
}

DateTime Timestamp::to_datetime() const {
    auto days = floor_div(millis, day_ms);
    auto d = Date::from_epoch_days(days);
    if (!d) {
        return {};
    }
    auto rest = millis - days * day_ms;
    return {*d,
            unsigned(rest / 3600000),
            unsigned(rest / 60000 % 60),
            unsigned(rest / 1000 % 60),
            unsigned(rest % 1000)};
}
}
