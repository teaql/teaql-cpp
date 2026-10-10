#pragma once
#include <teaql/core/value.hpp>
#include <chrono>
#include <limits>

namespace teaql {
// Native Unix UTC instants correspond to the source DateTime<Utc> conversion.
// Floating duration representations require an explicit integral conversion.
template <std::signed_integral Rep, class Period>
    requires(sizeof(Rep) <= sizeof(std::int64_t) &&
             (std::ratio_equal_v<Period, std::ratio<1>> || std::ratio_equal_v<Period, std::milli> ||
              std::ratio_equal_v<Period, std::micro> || std::ratio_equal_v<Period, std::nano>))
Value value_from_utc(Memory owner,
                     std::chrono::time_point<std::chrono::system_clock,
                                             std::chrono::duration<Rep, Period>> instant) {
    auto ticks = std::int64_t(instant.time_since_epoch().count());
    std::int64_t milliseconds;
    if constexpr (std::ratio_equal_v<Period, std::ratio<1>>) {
        if (ticks > std::numeric_limits<std::int64_t>::max() / 1000 ||
            ticks < std::numeric_limits<std::int64_t>::min() / 1000) {
            throw std::overflow_error("UTC instant exceeds signed millisecond range");
        }
        milliseconds = ticks * 1000;
    } else {
        constexpr auto divisor = std::int64_t(Period::den / 1000);
        milliseconds = ticks / divisor;
        if (ticks < 0 && ticks % divisor != 0) {
            --milliseconds;
        }
    }
    return Value(owner, Timestamp(milliseconds));
}
}
