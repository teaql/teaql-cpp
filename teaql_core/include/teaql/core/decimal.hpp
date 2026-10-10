#pragma once
#include <teaql/memory/types.hpp>
#include <teaql/core/decimal_error.hpp>
#include <teaql/core/decimal_format.hpp>
#include <teaql/core/decimal_inline_string.hpp>
#include <teaql/core/decimal_json_error.hpp>
#include <teaql/core/integer128.hpp>
#include <array>
#include <cstdint>
#include <compare>
#include <optional>
#include <string_view>
#include <concepts>
#include <type_traits>
#include <limits>
#include <ranges>
#include <functional>

namespace teaql {
namespace detail {
struct DecimalArithmeticAccess;
}
struct Json;
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

struct UnpackedDecimal {
    bool negative = false;
    std::uint32_t scale = 0;
    std::uint32_t hi = 0;
    std::uint32_t mid = 0;
    std::uint32_t lo = 0;

    friend bool operator==(UnpackedDecimal, UnpackedDecimal) = default;
};

// 96-bit coefficient; normal construction admits 0..28 scale. Like the locked
// Rust rescale API, explicit scale adjustment can produce extended scales.
class Decimal {
public:
    static constexpr unsigned MAX_SCALE = 28;
    static const Decimal MIN;
    static const Decimal MAX;
    static const Decimal ZERO;
    static const Decimal ONE;
    static const Decimal NEGATIVE_ONE;
    static const Decimal TWO;
    static const Decimal TEN;
    static const Decimal ONE_HUNDRED;
    static const Decimal ONE_THOUSAND;
    Decimal() = default;
    static Decimal from_i64(std::int64_t);
    static Decimal from_u64(std::uint64_t);

    template <std::integral T>
        requires(!std::same_as<T, bool> && sizeof(T) <= sizeof(std::uint64_t))
    static Decimal from_integer(T value) {
        if constexpr (std::is_signed_v<T>) {
            return from_i64(value);
        } else {
            return from_u64(value);
        }
    }

    static std::optional<Decimal> from_i128(Int128);
    static std::optional<Decimal> from_u128(UInt128);

    static Decimal from_integer(Int128 value) {
        return from_i128_with_scale(value, 0);
    }

    static Decimal from_integer(UInt128 value) {
        auto result = from_u128(value);
        if (!result) {
            throw std::overflow_error("Decimal integer coefficient exceeds 96 bits");
        }
        return *result;
    }

    static Decimal from_i64_with_scale(std::int64_t, unsigned scale);
    static Result<Decimal, DecimalError> try_new(Memory error_domain, std::int64_t, unsigned scale);
    static Decimal from_i128_with_scale(Int128, unsigned scale);
    static Result<Decimal, DecimalError>
    try_from_i128_with_scale(Memory error_domain, Int128, unsigned scale);
    static std::optional<Decimal> from_f32(float);
    static std::optional<Decimal> from_f64(double);
    static std::optional<Decimal> from_f32_retain(float);
    static std::optional<Decimal> from_f64_retain(double);
    static Result<Decimal, DecimalError> try_from_f32(Memory error_domain, float);
    static Result<Decimal, DecimalError> try_from_f64(Memory error_domain, double);
    static std::optional<Decimal> parse(std::string_view);
    static Result<Decimal, DecimalError> parse_result(Memory error_domain, std::string_view);
    static Result<Decimal, DecimalError> from_str_exact(Memory error_domain, std::string_view);
    static Result<Decimal, DecimalError> from_scientific(Memory error_domain, std::string_view);
    static Result<Decimal, DecimalError> from_scientific_lossy(Memory error_domain,
                                                               std::string_view);
    static Result<Decimal, DecimalError>
    from_str_radix(Memory error_domain, std::string_view, unsigned radix);

    static constexpr std::optional<Decimal>
    from_parts(std::array<std::uint32_t, 3> coefficient, bool negative, unsigned scale) {
        if (scale > MAX_SCALE) {
            return {};
        }
        return Decimal(coefficient,
                       negative && (coefficient[0] | coefficient[1] | coefficient[2]),
                       static_cast<unsigned char>(scale));
    }

    std::optional<std::int64_t> to_i64() const;
    std::optional<std::uint64_t> to_u64() const;

    template <std::integral T>
        requires(!std::same_as<T, bool> && sizeof(T) <= sizeof(std::uint64_t))
    std::optional<T> to_integer() const {
        if constexpr (std::is_signed_v<T>) {
            auto value = to_i64();
            if (value && *value >= std::numeric_limits<T>::min() &&
                *value <= std::numeric_limits<T>::max()) {
                return T(*value);
            }
        } else {
            auto value = to_u64();
            if (value && *value <= std::numeric_limits<T>::max()) {
                return T(*value);
            }
        }
        return {};
    }

    Int128 to_i128() const;
    std::optional<UInt128> to_u128() const;

    template <std::integral T>
        requires(!std::same_as<T, bool> && sizeof(T) <= sizeof(std::uint64_t))
    Result<T, DecimalError> try_to_integer(Memory r) const {
        constexpr std::string_view target = std::is_signed_v<T> ? (sizeof(T) == 1   ? "i8"
                                                                   : sizeof(T) == 2 ? "i16"
                                                                   : sizeof(T) == 4 ? "i32"
                                                                                    : "i64")
                                                                : (sizeof(T) == 1   ? "u8"
                                                                   : sizeof(T) == 2 ? "u16"
                                                                   : sizeof(T) == 4 ? "u32"
                                                                                    : "u64");
        return integer_result<T>(r, target);
    }

    Result<std::intptr_t, DecimalError> try_to_isize(Memory r) const {
        return integer_result<std::intptr_t>(r, "isize");
    }

    Result<std::uintptr_t, DecimalError> try_to_usize(Memory r) const {
        return integer_result<std::uintptr_t>(r, "usize");
    }

    Result<Int128, DecimalError> try_to_i128(Memory r) const {
        return Result<Int128, DecimalError>::ok(r, to_i128());
    }

    Result<UInt128, DecimalError> try_to_u128(Memory error_domain) const;

    constexpr Int128 mantissa() const {
        return Int128::from_magnitude({{coefficient_[0], coefficient_[1], coefficient_[2], 0}},
                                      negative_);
    }

    float to_f32() const;
    double to_f64() const;

    Result<float, DecimalError> try_to_f32(Memory r) const {
        return Result<float, DecimalError>::ok(r, to_f32());
    }

    Result<double, DecimalError> try_to_f64(Memory r) const {
        return Result<double, DecimalError>::ok(r, to_f64());
    }

    String to_string(Memory) const;
    DecimalInlineString array_string() const;
    String format(Memory destination, DecimalFormat options = {}) const;
    Json to_json_value(Memory destination) const;
    static std::optional<Decimal> from_json_value(const Json&);
    static Result<Decimal, DecimalJsonError> from_json_value_result(Memory error_domain,
                                                                    const Json&);

    constexpr UnpackedDecimal unpack() const {
        return {negative_, scale_, coefficient_[2], coefficient_[1], coefficient_[0]};
    }

    static Decimal from_unpacked(UnpackedDecimal value) {
        auto result = from_parts({value.lo, value.mid, value.hi}, value.negative, value.scale);
        if (!result) {
            throw std::out_of_range("Decimal scale exceeds maximum supported scale");
        }
        return *result;
    }

    std::array<std::uint32_t, 4> hash_words() const;

    static Decimal zero() {
        return ZERO;
    }

    static Decimal one() {
        return ONE;
    }

    void set_zero() {
        *this = ZERO;
    }

    void set_one() {
        *this = ONE;
    }

    bool is_one() const {
        return *this == ONE;
    }

    template <std::ranges::input_range Range>
        requires std::same_as<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, Decimal>
    static Decimal sum(Range&& values) {
        auto result = ZERO;
        for (const auto& value : values) {
            result += value;
        }
        return result;
    }

    template <std::ranges::input_range Range>
        requires std::same_as<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, Decimal>
    static Decimal product(Range&& values) {
        auto result = ONE;
        for (const auto& value : values) {
            result *= value;
        }
        return result;
    }

    constexpr unsigned scale() const {
        return scale_;
    }

    constexpr bool is_sign_negative() const {
        return negative_;
    }

    constexpr const std::array<std::uint32_t, 3>& coefficient() const {
        return coefficient_;
    }

    bool operator==(const Decimal&) const;
    std::strong_ordering operator<=>(const Decimal&) const;

    constexpr bool is_zero() const {
        return !(coefficient_[0] | coefficient_[1] | coefficient_[2]);
    }

    constexpr bool is_sign_positive() const {
        return !negative_;
    }

    bool is_negative() const {
        return is_sign_negative();
    }

    bool is_positive() const {
        return is_sign_positive();
    }

    void set_sign(bool positive) {
        set_sign_positive(positive);
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

    Decimal inv() const {
        return ONE / *this;
    }

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

    Decimal abs_sub(const Decimal& other) const {
        return *this <= other ? ZERO : *this - other;
    }

    Decimal signum() const {
        return is_zero() ? ZERO : negative_ ? NEGATIVE_ONE : ONE;
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

    constexpr std::array<std::uint8_t, 16> serialize() const {
        std::array<std::uint8_t, 16> result{};
        const std::array<std::uint32_t, 4> words{(std::uint32_t(scale_) << 16) |
                                                     (std::uint32_t(negative_) << 31),
                                                 coefficient_[0],
                                                 coefficient_[1],
                                                 coefficient_[2]};
        for (unsigned i = 0; i < 4; ++i) {
            for (unsigned j = 0; j < 4; ++j) {
                result[4 * i + j] = std::uint8_t(words[i] >> (8 * j));
            }
        }
        return result;
    }

    static Decimal deserialize(const std::array<std::uint8_t, 16>&);
    bool is_integer() const;

    Decimal min(const Decimal& other) const {
        return *this > other ? other : *this;
    }

    Decimal max(const Decimal& other) const {
        return *this < other ? other : *this;
    }

    Decimal ord_min(const Decimal& other) const {
        return other < *this ? other : *this;
    }

    Decimal ord_max(const Decimal& other) const {
        return other < *this ? *this : other;
    }

    Decimal clamp(const Decimal& minimum, const Decimal& maximum) const {
        if (minimum > maximum) {
            throw std::invalid_argument("Decimal clamp minimum exceeds maximum");
        }
        if (*this < minimum) {
            return minimum;
        }
        if (*this > maximum) {
            return maximum;
        }
        return *this;
    }

    static constexpr Decimal max_value() {
        return Decimal({UINT32_MAX, UINT32_MAX, UINT32_MAX}, false, 0);
    }

    static constexpr Decimal min_value() {
        return Decimal({UINT32_MAX, UINT32_MAX, UINT32_MAX}, true, 0);
    }

    std::optional<Decimal> round_sf_with_strategy(unsigned digits, RoundingStrategy) const;

    std::optional<Decimal> round_sf(unsigned digits) const {
        return round_sf_with_strategy(digits, RoundingStrategy::MidpointNearestEven);
    }

private:
    friend struct detail::DecimalArithmeticAccess;

    constexpr Decimal(std::array<std::uint32_t, 3> coefficient, bool negative, unsigned char scale)
        : coefficient_(coefficient), scale_(scale), negative_(negative) {}

    template <class T>
    Result<T, DecimalError> integer_result(Memory r, std::string_view target) const {
        if (auto value = to_integer<T>()) {
            return Result<T, DecimalError>::ok(r, *value);
        }
        return Result<T, DecimalError>::err(
            r,
            DecimalError(
                std::allocator_arg, Allocator(r), DecimalError::Code::ConversionTo, target));
    }

    static std::optional<Decimal> from_binary(
        std::array<std::uint32_t, 3>, int exponent, bool negative, bool binary64, bool retain);
    static std::optional<Decimal> from_f64_detail(double, bool retain);
    static std::optional<Decimal> from_f32_detail(float, bool retain);
    static std::optional<Decimal>
    parse_detail(std::string_view, DecimalError::Code&, std::string_view&, std::uint32_t&);
    static std::optional<Decimal> scientific_detail(
        std::string_view, bool lossy, DecimalError::Code&, std::string_view&, std::uint32_t&);
    void adjust_scale(unsigned scale, bool round);
    std::array<std::uint32_t, 3> coefficient_{};
    unsigned char scale_ = 0;
    bool negative_ = false;
};

inline constexpr Decimal Decimal::ZERO{};
inline constexpr Decimal Decimal::ONE({1, 0, 0}, false, 0);
inline constexpr Decimal Decimal::NEGATIVE_ONE({1, 0, 0}, true, 0);
inline constexpr Decimal Decimal::TWO({2, 0, 0}, false, 0);
inline constexpr Decimal Decimal::TEN({10, 0, 0}, false, 0);
inline constexpr Decimal Decimal::ONE_HUNDRED({100, 0, 0}, false, 0);
inline constexpr Decimal Decimal::ONE_THOUSAND({1000, 0, 0}, false, 0);
inline constexpr Decimal Decimal::MAX({UINT32_MAX, UINT32_MAX, UINT32_MAX}, false, 0);
inline constexpr Decimal Decimal::MIN({UINT32_MAX, UINT32_MAX, UINT32_MAX}, true, 0);
}

template <>
struct std::hash<teaql::Decimal> {
    std::size_t operator()(const teaql::Decimal& value) const {
        // C++ bucket hashing is not a persistent ID or Rust hasher algorithm.
        std::size_t result = 2166136261u;
        for (auto word : value.hash_words()) {
            result = (result ^ word) * std::size_t(16777619u);
        }
        return result;
    }
};
