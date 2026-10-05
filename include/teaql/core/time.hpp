#pragma once
#include <teaql/memory/types.hpp>
#include <cstdint>
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
    Timestamp() = default;

    explicit Timestamp(std::int64_t milliseconds) : millis_(milliseconds) {}

    static Timestamp from_u64(std::uint64_t);
    static Timestamp now();
    static std::optional<Timestamp> parse(std::string_view);

    std::int64_t as_millis() const {
        return millis_;
    }

    DateTime to_datetime() const; // Out-of-chrono-range milliseconds fall back to epoch.
    Json to_json_value(Memory destination) const;
    static std::optional<Timestamp> from_json_value(const Json&);
    bool operator==(const Timestamp&) const = default;

private:
    std::int64_t millis_ = 0;
};
}
