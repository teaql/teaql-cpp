#pragma once
#include <teaql/core/time.hpp>
#include <ctime>
#include <stdexcept>
#include <utility>

namespace teaql::detail {
struct SystemUtcTime {
    std::int64_t seconds;
    std::uint32_t nanoseconds;
};

inline SystemUtcTime system_utc_time() {
    // Read seconds and nanoseconds separately. A system_clock duration whose
    // representation is signed nanoseconds cannot carry dates beyond 2262.
    std::timespec time{};
    if (std::timespec_get(&time, TIME_UTC) != TIME_UTC) {
        throw std::runtime_error("Unable to read the system UTC clock");
    }
    if (time.tv_sec < 0) {
        throw std::out_of_range("system time before Unix epoch");
    }
    if (!std::in_range<std::int64_t>(time.tv_sec) || time.tv_nsec < 0 ||
        time.tv_nsec >= 1000000000) {
        throw std::out_of_range("Invalid system UTC clock value");
    }
    auto seconds = static_cast<std::int64_t>(time.tv_sec);
    if (!Date::from_epoch_days(seconds / 86400)) {
        throw std::out_of_range("System business time exceeds chrono's UTC date range");
    }
    return {seconds, static_cast<std::uint32_t>(time.tv_nsec)};
}
}
