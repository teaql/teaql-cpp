#pragma once
#include <bit>
#include <chrono>
#include <compare>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

namespace teaql {
// Rust SystemTime on the pinned Unix target has signed 64-bit seconds plus
// nanoseconds. Do not narrow cursor deadlines to system_clock's nanosecond range.
class SystemTime {
public:
    SystemTime() = default;

    static SystemTime from_unix_seconds(std::int64_t seconds, std::uint32_t nanos = 0) {
        if (nanos >= 1'000'000'000)
            throw std::invalid_argument("SystemTime nanoseconds must be below one second");
        return SystemTime(seconds, nanos);
    }

    static SystemTime now() {
        auto elapsed = std::chrono::system_clock::now().time_since_epoch();
        auto seconds = std::chrono::floor<std::chrono::seconds>(elapsed);
        return SystemTime(
            seconds.count(),
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed - seconds).count());
    }

    std::int64_t unix_seconds() const noexcept {
        return seconds_;
    }

    std::uint32_t subsec_nanos() const noexcept {
        return nanos_;
    }

    std::optional<SystemTime> checked_add_seconds(std::uint64_t n) const noexcept {
        auto room =
            std::uint64_t(std::numeric_limits<std::int64_t>::max()) - std::uint64_t(seconds_);
        if (n > room)
            return {};
        return SystemTime(std::bit_cast<std::int64_t>(std::uint64_t(seconds_) + n), nanos_);
    }

    auto operator<=>(const SystemTime&) const = default;

private:
    SystemTime(std::int64_t s, std::uint32_t n) : seconds_(s), nanos_(n) {}

    std::int64_t seconds_ = 0;
    std::uint32_t nanos_ = 0;
};
}
