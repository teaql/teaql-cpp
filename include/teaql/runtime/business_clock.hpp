#pragma once
#include <teaql/core/time.hpp>

namespace teaql {
// UTC carrier for chrono::DateTime<Utc> at the BusinessClock boundary. Keep
// seconds and nanoseconds separate: system_clock::time_point cannot represent
// chrono's complete date range, and Timestamp intentionally stores milliseconds.
class UtcDateTime {
public:
    static std::optional<UtcDateTime> from_timestamp(std::int64_t seconds,
                                                     std::uint32_t nanoseconds);
    static std::optional<UtcDateTime> from_ymd_hms_nano(std::int32_t year,
                                                        unsigned month,
                                                        unsigned day,
                                                        unsigned hour,
                                                        unsigned minute,
                                                        unsigned second,
                                                        std::uint32_t nanoseconds);
    static UtcDateTime now();

    std::int64_t timestamp() const noexcept {
        return seconds_;
    }

    std::uint32_t timestamp_subsec_nanos() const noexcept {
        return nanoseconds_;
    }

    Date date_naive() const;
    bool operator==(const UtcDateTime&) const = default;

private:
    UtcDateTime(std::int64_t seconds, std::uint32_t nanos)
        : seconds_(seconds), nanoseconds_(nanos) {}

    std::int64_t seconds_;
    std::uint32_t nanoseconds_;
};

class BusinessClock {
public:
    virtual ~BusinessClock() = default;
    virtual UtcDateTime business_time() const = 0;

    virtual Date business_date() const {
        return business_time().date_naive();
    }
};

class SystemBusinessClock final : public BusinessClock {
public:
    using allocator_type = Allocator;
    SystemBusinessClock() = default;

    SystemBusinessClock(std::allocator_arg_t, Allocator) {}

    SystemBusinessClock(std::allocator_arg_t, Allocator, const SystemBusinessClock&) {}

    UtcDateTime business_time() const override {
        return UtcDateTime::now();
    }
};

class FixedBusinessClock final : public BusinessClock {
public:
    using allocator_type = Allocator;

    explicit FixedBusinessClock(UtcDateTime value) : value_(value) {}

    FixedBusinessClock(std::allocator_arg_t, Allocator, UtcDateTime value) : value_(value) {}

    FixedBusinessClock(std::allocator_arg_t, Allocator, const FixedBusinessClock& other)
        : value_(other.value_) {}

    UtcDateTime business_time() const override {
        return value_;
    }

private:
    UtcDateTime value_;
};

struct BusinessDate {
    Date value;

    static BusinessDate today() {
        return {UtcDateTime::now().date_naive()};
    }

    bool operator==(const BusinessDate&) const = default;
};
}
