#pragma once
#include <teaql/memory/types.hpp>
#include <cstdint>
#include <compare>
#include <functional>
#include <optional>
#include <string_view>

namespace teaql {
struct Json;

class Date {
public:
    Date() = default;
    static std::optional<Date> from_ymd(std::int32_t year, unsigned month, unsigned day);
    static std::optional<Date> parse(std::string_view);
    static std::optional<Date> from_epoch_days(std::int64_t);
    std::int64_t epoch_days() const;
    String to_string(Memory) const;

    std::int32_t year() const {
        return year_;
    }

    unsigned month() const {
        return month_;
    }

    unsigned day() const {
        return day_;
    }

    bool operator==(const Date&) const = default;

private:
    std::int32_t year_ = 1970;
    unsigned char month_ = 1, day_ = 1;
};

struct DateTime {
    Date date;
    unsigned hour = 0, minute = 0, second = 0, millisecond = 0;
    bool operator==(const DateTime&) const = default;
};

class Timestamp {
public:
    // Named counterpart of the source's public signed-millisecond tuple field.
    std::int64_t millis = 0;

    constexpr Timestamp() = default;

    explicit constexpr Timestamp(std::int64_t milliseconds) noexcept : millis(milliseconds) {}

    static Timestamp from_u64(std::uint64_t);
    static Timestamp now();
    static std::optional<Timestamp> parse(std::string_view);

    constexpr std::int64_t as_millis() const noexcept {
        return millis;
    }

    DateTime to_datetime() const; // Out-of-chrono-range milliseconds fall back to epoch.
    Json to_json_value(Memory destination) const;
    static std::optional<Timestamp> from_json_value(const Json&);
    constexpr bool operator==(const Timestamp&) const noexcept = default;
    constexpr auto operator<=>(const Timestamp&) const noexcept = default;
};
}

template <>
struct std::hash<teaql::Timestamp> {
    std::size_t operator()(const teaql::Timestamp& value) const noexcept {
        // Like the source Hash input, identity is the complete signed millis value.
        // The standard bucket hash is not a persisted Rust hasher result.
        return std::hash<std::int64_t>{}(value.as_millis());
    }
};
