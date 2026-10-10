#pragma once
#include <bit>
#include <chrono>
#include <compare>
#include <concepts>
#include <cstdint>
#include <ctime>
#include <exception>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace teaql {
struct SystemDuration {
    std::uint64_t seconds = 0;
    std::uint32_t nanos = 0;
    bool operator==(const SystemDuration&) const = default;
};

class SystemTimeError {
public:
    explicit SystemTimeError(SystemDuration duration) noexcept : duration_(duration) {}

    SystemDuration duration() const noexcept {
        return duration_;
    }

    static constexpr std::string_view message() noexcept {
        return "second time provided was later than self";
    }

    const std::exception* source() const noexcept {
        return nullptr;
    }

private:
    SystemDuration duration_;
};

// Scalar Result carrier: neither alternative owns Context data or allocates.
class SystemTimeDifference {
public:
    explicit SystemTimeDifference(SystemDuration duration) noexcept : data_(duration) {}

    explicit SystemTimeDifference(SystemTimeError error) noexcept : data_(error) {}

    bool is_ok() const noexcept {
        return data_.index() == 0;
    }

    bool is_err() const noexcept {
        return !is_ok();
    }

    explicit operator bool() const noexcept {
        return is_ok();
    }

    const SystemDuration& value() const {
        return std::get<SystemDuration>(data_);
    }

    const SystemTimeError& error() const {
        return std::get<SystemTimeError>(data_);
    }

    const SystemDuration& operator*() const {
        return value();
    }

    const SystemDuration* operator->() const {
        return &value();
    }

    bool operator==(SystemDuration duration) const noexcept {
        return is_ok() && std::get<SystemDuration>(data_) == duration;
    }

private:
    std::variant<SystemDuration, SystemTimeError> data_;
};

// Signed Unix seconds and normalized nanos; chrono conversion is checked.
class SystemTime {
public:
    using duration = std::chrono::nanoseconds;
    static const SystemTime UNIX_EPOCH;

    struct clock {
        static SystemTime now() {
            return SystemTime::now();
        }
    };

    SystemTime() = default;

    explicit SystemTime(std::chrono::nanoseconds elapsed) noexcept {
        auto ticks = elapsed.count();
        seconds_ = ticks / 1'000'000'000;
        auto remainder = ticks % 1'000'000'000;
        if (remainder < 0) {
            --seconds_;
            remainder += 1'000'000'000;
        }
        nanos_ = static_cast<std::uint32_t>(remainder);
    }

    template <std::signed_integral Rep, class Period>
        requires(sizeof(Rep) <= sizeof(std::int64_t))
    explicit SystemTime(std::chrono::duration<Rep, Period> elapsed) {
        auto ticks = static_cast<std::int64_t>(elapsed.count());
        if constexpr (Period::den == 1) {
            constexpr auto multiplier = Period::num;
            if (ticks > std::numeric_limits<std::int64_t>::max() / multiplier ||
                ticks < std::numeric_limits<std::int64_t>::min() / multiplier) {
                throw std::overflow_error("SystemTime duration exceeds its seconds range");
            }
            seconds_ = ticks * multiplier;
        } else {
            static_assert(Period::num == 1 && 1'000'000'000 % Period::den == 0,
                          "SystemTime requires whole seconds or exact nanosecond units");
            seconds_ = ticks / Period::den;
            auto remainder = ticks % Period::den;
            if (remainder < 0) {
                --seconds_;
                remainder += Period::den;
            }
            nanos_ = static_cast<std::uint32_t>(remainder * (1'000'000'000 / Period::den));
        }
    }

    SystemTime(std::chrono::system_clock::time_point native) noexcept {
        auto elapsed = native.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed);
        seconds_ = seconds.count();
        auto remainder =
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed % std::chrono::seconds(1))
                .count();
        if (remainder < 0) {
            --seconds_;
            remainder += 1'000'000'000;
        }
        nanos_ = static_cast<std::uint32_t>(remainder);
    }

    template <class Duration>
    SystemTime(std::chrono::time_point<std::chrono::system_clock, Duration> native)
        : SystemTime(native.time_since_epoch()) {}

    static SystemTime from_unix_seconds(std::int64_t seconds, std::uint32_t nanos = 0) {
        if (nanos >= 1'000'000'000) {
            throw std::invalid_argument("SystemTime nanoseconds must be below one second");
        }
        return SystemTime(seconds, nanos);
    }

    static SystemTime now() {
        std::timespec value{};
        if (std::timespec_get(&value, TIME_UTC) != TIME_UTC) {
            throw std::runtime_error("Unable to read the system clock");
        }
        return from_unix_seconds(value.tv_sec, static_cast<std::uint32_t>(value.tv_nsec));
    }

    static SystemTime min() noexcept {
        return SystemTime(std::numeric_limits<std::int64_t>::min(), 0);
    }

    static SystemTime max() noexcept {
        return SystemTime(std::numeric_limits<std::int64_t>::max(), 999'999'999);
    }

    std::int64_t unix_seconds() const noexcept {
        return seconds_;
    }

    std::uint32_t subsec_nanos() const noexcept {
        return nanos_;
    }

    std::optional<SystemTime> checked_add_seconds(std::uint64_t count) const noexcept {
        auto room =
            std::uint64_t(std::numeric_limits<std::int64_t>::max()) - std::uint64_t(seconds_);
        if (count > room) {
            return {};
        }
        return SystemTime(std::bit_cast<std::int64_t>(std::uint64_t(seconds_) + count), nanos_);
    }

    std::optional<SystemTime> checked_sub_seconds(std::uint64_t count) const noexcept {
        auto room =
            std::uint64_t(seconds_) - std::uint64_t(std::numeric_limits<std::int64_t>::min());
        if (count > room) {
            return {};
        }
        return SystemTime(std::bit_cast<std::int64_t>(std::uint64_t(seconds_) - count), nanos_);
    }

    std::optional<SystemTime> checked_add(SystemDuration elapsed) const noexcept {
        if (elapsed.nanos >= 1'000'000'000) {
            return {};
        }
        auto result = checked_add_seconds(elapsed.seconds);
        if (!result) {
            return {};
        }
        auto nanos = std::uint64_t(nanos_) + elapsed.nanos;
        if (nanos >= 1'000'000'000) {
            result = result->checked_add_seconds(1);
            if (!result) {
                return {};
            }
        }
        result->nanos_ = static_cast<std::uint32_t>(nanos % 1'000'000'000);
        return result;
    }

    std::optional<SystemTime> checked_sub(SystemDuration elapsed) const noexcept {
        if (elapsed.nanos >= 1'000'000'000) {
            return {};
        }
        auto result = checked_sub_seconds(elapsed.seconds);
        if (!result) {
            return {};
        }
        if (nanos_ < elapsed.nanos) {
            result = result->checked_sub_seconds(1);
            if (!result) {
                return {};
            }
            result->nanos_ = nanos_ + 1'000'000'000 - elapsed.nanos;
        } else {
            result->nanos_ = nanos_ - elapsed.nanos;
        }
        return result;
    }

    SystemTime operator+(SystemDuration elapsed) const {
        auto result = checked_add(elapsed);
        if (!result) {
            throw std::overflow_error("SystemTime addition exceeds its supported range");
        }
        return *result;
    }

    SystemTime operator-(SystemDuration elapsed) const {
        auto result = checked_sub(elapsed);
        if (!result) {
            throw std::overflow_error("SystemTime subtraction exceeds its supported range");
        }
        return *result;
    }

    SystemTime& operator+=(SystemDuration elapsed) {
        return *this = *this + elapsed;
    }

    SystemTime& operator-=(SystemDuration elapsed) {
        return *this = *this - elapsed;
    }

    SystemTime& operator+=(std::chrono::nanoseconds elapsed) {
        auto ticks = elapsed.count();
        auto magnitude = ticks < 0 ? std::uint64_t(0) - std::uint64_t(ticks) : std::uint64_t(ticks);
        SystemDuration duration{magnitude / 1'000'000'000,
                                static_cast<std::uint32_t>(magnitude % 1'000'000'000)};
        return ticks < 0 ? *this -= duration : *this += duration;
    }

    SystemTime& operator-=(std::chrono::nanoseconds elapsed) {
        auto ticks = elapsed.count();
        auto magnitude = ticks < 0 ? std::uint64_t(0) - std::uint64_t(ticks) : std::uint64_t(ticks);
        SystemDuration duration{magnitude / 1'000'000'000,
                                static_cast<std::uint32_t>(magnitude % 1'000'000'000)};
        return ticks < 0 ? *this += duration : *this -= duration;
    }

    SystemTimeDifference duration_since(SystemTime earlier) const noexcept {
        if (*this < earlier) {
            return SystemTimeDifference(SystemTimeError(*earlier.duration_since(*this)));
        }
        auto seconds = std::uint64_t(seconds_) - std::uint64_t(earlier.seconds_);
        auto nanos = std::int64_t(nanos_) - earlier.nanos_;
        if (nanos < 0) {
            --seconds;
            nanos += 1'000'000'000;
        }
        return SystemTimeDifference(SystemDuration{seconds, static_cast<std::uint32_t>(nanos)});
    }

    SystemTimeDifference elapsed() const {
        return now().duration_since(*this);
    }

    std::chrono::nanoseconds time_since_epoch() const {
        constexpr auto limit = std::uint64_t(std::numeric_limits<std::int64_t>::max());
        std::uint64_t ticks;
        if (seconds_ >= 0) {
            auto seconds = std::uint64_t(seconds_);
            if (seconds > limit / 1'000'000'000) {
                throw std::overflow_error("SystemTime exceeds the chrono nanosecond range");
            }
            ticks = seconds * 1'000'000'000;
            if (nanos_ > limit - ticks) {
                throw std::overflow_error("SystemTime exceeds the chrono nanosecond range");
            }
            return std::chrono::nanoseconds(static_cast<std::int64_t>(ticks + nanos_));
        }
        auto magnitude = std::uint64_t(0) - std::uint64_t(seconds_);
        if (magnitude > (limit + 1) / 1'000'000'000 + 1) {
            throw std::overflow_error("SystemTime exceeds the chrono nanosecond range");
        }
        ticks = magnitude * 1'000'000'000 - nanos_;
        if (ticks > limit + 1) {
            throw std::overflow_error("SystemTime exceeds the chrono nanosecond range");
        }
        return std::chrono::nanoseconds(std::bit_cast<std::int64_t>(std::uint64_t(0) - ticks));
    }

    auto operator<=>(const SystemTime&) const = default;

private:
    SystemTime(std::int64_t seconds, std::uint32_t nanos) : seconds_(seconds), nanos_(nanos) {}

    std::int64_t seconds_ = 0;
    std::uint32_t nanos_ = 0;
};

inline constexpr SystemTime SystemTime::UNIX_EPOCH{};
inline constexpr SystemTime UNIX_EPOCH = SystemTime::UNIX_EPOCH;
}

template <>
struct std::hash<teaql::SystemTime> {
    std::size_t operator()(const teaql::SystemTime& value) const noexcept {
        auto seconds = std::hash<std::int64_t>{}(value.unix_seconds());
        auto nanos = std::hash<std::uint32_t>{}(value.subsec_nanos());
        // Bucket hashing is native; the complete normalized parts are the key.
        return seconds ^ (nanos + std::size_t(0x9e3779b9) + (seconds << 6) + (seconds >> 2));
    }
};
