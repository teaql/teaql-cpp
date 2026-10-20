#include <teaql/runtime/business_clock.hpp>
#include <teaql/core/detail/system_clock.hpp>
#include <stdexcept>

namespace teaql {
namespace {
std::int64_t days(std::int64_t seconds) {
    auto q = seconds / 86400;
    return q - (seconds % 86400 < 0);
}
}

std::optional<UtcDateTime> UtcDateTime::from_timestamp(std::int64_t seconds, std::uint32_t nanos) {
    // Chrono permits a leap second only as second 59 plus >= 1e9 nanos.
    auto second = seconds % 60;
    if (second < 0) {
        second += 60;
    }
    if (nanos >= 2000000000 || (nanos >= 1000000000 && second != 59) ||
        !Date::from_epoch_days(days(seconds))) {
        return {};
    }
    return UtcDateTime(seconds, nanos);
}

std::optional<UtcDateTime> UtcDateTime::from_ymd_hms_nano(std::int32_t y,
                                                          unsigned m,
                                                          unsigned d,
                                                          unsigned hour,
                                                          unsigned minute,
                                                          unsigned second,
                                                          std::uint32_t nanos) {
    auto date = Date::from_ymd(y, m, d);
    if (!date || hour > 23 || minute > 59 || second > 59) {
        return {};
    }
    return from_timestamp(date->epoch_days() * 86400 + hour * 3600 + minute * 60 + second, nanos);
}

UtcDateTime UtcDateTime::now() {
    auto time = detail::system_utc_time();
    return UtcDateTime(time.seconds, time.nanoseconds);
}

Date UtcDateTime::date_naive() const {
    return *Date::from_epoch_days(days(seconds_));
}
}
