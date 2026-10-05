#pragma once
#include <teaql/memory/types.hpp>
#include <teaql/core/decimal_error.hpp>
#include <array>
#include <cstdint>
#include <compare>
#include <optional>
#include <string_view>

namespace teaql {
enum class RoundingStrategy {
    MidpointNearestEven,
    MidpointAwayFromZero,
    MidpointTowardZero,
    ToZero,
    AwayFromZero,
    ToNegativeInfinity,
    ToPositiveInfinity,
    BankersRounding,
    RoundHalfUp,
    RoundHalfDown,
    RoundDown,
    RoundUp
};

// 96-bit coefficient; normal construction admits 0..28 scale. Like the locked
// Rust rescale API, explicit scale adjustment can produce extended scales.
class Decimal {
public:
    static constexpr unsigned MAX_SCALE = 28;
    Decimal() = default;
    static Decimal from_i64(std::int64_t);
    static Decimal from_u64(std::uint64_t);
    static std::optional<Decimal> from_f64(double);
    static std::optional<Decimal> parse(std::string_view);
    static Result<Decimal, DecimalError> parse_result(Memory error_domain, std::string_view);
    static std::optional<Decimal>
    from_parts(std::array<std::uint32_t, 3>, bool negative, unsigned scale);
    std::optional<std::int64_t> to_i64() const;
    std::optional<std::uint64_t> to_u64() const;
    double to_f64() const;
    String to_string(Memory) const;

    unsigned scale() const {
        return scale_;
    }

    bool is_sign_negative() const {
        return negative_;
    }

    const std::array<std::uint32_t, 3>& coefficient() const {
        return coefficient_;
    }

    bool operator==(const Decimal&) const;
    std::strong_ordering operator<=>(const Decimal&) const;

    bool is_zero() const {
        return !(coefficient_[0] | coefficient_[1] | coefficient_[2]);
    }

    bool is_sign_positive() const {
        return !negative_;
    }

    void set_sign_negative(bool value) {
        negative_ = value;
    }

    void set_sign_positive(bool value) {
        negative_ = !value;
    }

    std::optional<Decimal> checked_add(const Decimal&) const;
    std::optional<Decimal> checked_sub(const Decimal&) const;
    std::optional<Decimal> checked_mul(const Decimal&) const;
    std::optional<Decimal> checked_div(const Decimal&) const;
    std::optional<Decimal> checked_rem(const Decimal&) const;
    Decimal saturating_add(const Decimal&) const;
    Decimal saturating_sub(const Decimal&) const;
    Decimal saturating_mul(const Decimal&) const;
    Decimal operator+(const Decimal&) const;
    Decimal operator-(const Decimal&) const;
    Decimal operator*(const Decimal&) const;
    Decimal operator/(const Decimal&) const;
    Decimal operator%(const Decimal&) const;

    Decimal operator-() const {
        auto result = *this;
        result.negative_ = !negative_;
        return result;
    }

    Decimal& operator+=(const Decimal& value) {
        return *this = *this + value;
    }

    Decimal& operator-=(const Decimal& value) {
        return *this = *this - value;
    }

    Decimal& operator*=(const Decimal& value) {
        return *this = *this * value;
    }

    Decimal& operator/=(const Decimal& value) {
        return *this = *this / value;
    }

    Decimal& operator%=(const Decimal& value) {
        return *this = *this % value;
    }

    Decimal abs() const {
        auto result = *this;
        result.negative_ = false;
        return result;
    }

    Decimal trunc() const;

    Decimal fract() const {
        return *this - trunc();
    }

    Decimal floor() const;
    Decimal ceil() const;
    Decimal normalize() const;

    void normalize_assign() {
        *this = normalize();
    }

    Decimal round_dp_with_strategy(unsigned dp, RoundingStrategy) const;

    Decimal round_dp(unsigned dp) const {
        return round_dp_with_strategy(dp, RoundingStrategy::MidpointNearestEven);
    }

    Decimal round() const {
        return round_dp(0);
    }

    Result<std::monostate, DecimalError> set_scale(Memory error_domain, unsigned scale);

    void rescale(unsigned scale) {
        adjust_scale(scale, true);
    }

    Decimal trunc_with_scale(unsigned scale) const {
        auto result = *this;
        result.adjust_scale(scale, false);
        return result;
    }

    std::array<std::uint8_t, 16> serialize() const;
    static Decimal deserialize(const std::array<std::uint8_t, 16>&);
    bool is_integer() const;

    Decimal min(const Decimal& other) const {
        return *this > other ? other : *this;
    }

    Decimal max(const Decimal& other) const {
        return *this < other ? other : *this;
    }

    static Decimal max_value() {
        return *from_parts({UINT32_MAX, UINT32_MAX, UINT32_MAX}, false, 0);
    }

    static Decimal min_value() {
        return -max_value();
    }

    std::optional<Decimal> round_sf_with_strategy(unsigned digits, RoundingStrategy) const;

    std::optional<Decimal> round_sf(unsigned digits) const {
        return round_sf_with_strategy(digits, RoundingStrategy::MidpointNearestEven);
    }

private:
    static std::optional<Decimal>
    parse_detail(std::string_view, DecimalError::Code&, std::string_view&, std::uint32_t&);
    void adjust_scale(unsigned scale, bool round);
    std::array<std::uint32_t, 3> coefficient_{};
    unsigned char scale_ = 0;
    bool negative_ = false;
};
}
