#include <teaql/core/decimal.hpp>
#include <algorithm>
#include <stdexcept>

// rust_decimal 1.42.1 arithmetic/rounding semantics (Paul Mason, MIT).
// Division's batch/unscale rules are adapted from that implementation;
// LICENSES/rust_decimal-MIT.txt retains its license. Wide math uses C++20 only.

namespace teaql {
namespace detail {
// Arithmetic can retain scales created by rescale; public from_parts stays checked.
struct DecimalArithmeticAccess {
    static Decimal make(std::array<std::uint32_t, 3> coefficient, bool negative, unsigned scale) {
        if (scale > 255) {
            throw std::domain_error("Arithmetic scale exceeds representation");
        }
        return Decimal(coefficient, negative, static_cast<unsigned char>(scale));
    }
};
}

namespace {
// Six little-endian limbs hold both a 96x96 product and 96-bit inputs aligned
// across the full 28-digit scale range. Larger intermediates fail explicitly,
// including extended-scale alignment; never silently discard a high carry.
struct Wide {
    std::array<std::uint32_t, 6> limbs{};
    Wide() = default;

    explicit Wide(const Decimal& value) {
        std::copy(value.coefficient().begin(), value.coefficient().end(), limbs.begin());
    }

    bool zero() const {
        for (auto n : limbs) {
            if (n) {
                return false;
            }
        }
        return true;
    }

    bool fits() const {
        return !(limbs[3] | limbs[4] | limbs[5]);
    }

    int compare(const Wide& other) const {
        for (int i = 5; i >= 0; --i) {
            if (limbs[i] != other.limbs[i]) {
                return limbs[i] < other.limbs[i] ? -1 : 1;
            }
        }
        return 0;
    }

    bool multiply(unsigned factor) {
        auto next_limbs = limbs;
        std::uint64_t carry = 0;
        for (auto& n : next_limbs) {
            auto next = std::uint64_t(n) * factor + carry;
            n = std::uint32_t(next);
            carry = next >> 32;
        }
        if (carry) {
            return false;
        }
        limbs = next_limbs;
        return true;
    }

    bool scale(unsigned count) {
        while (count--) {
            if (!multiply(10)) {
                return false;
            }
        }
        return true;
    }

    unsigned divide(unsigned divisor) {
        std::uint64_t remainder = 0;
        for (int i = 5; i >= 0; --i) {
            auto next = (remainder << 32) | limbs[i];
            limbs[i] = std::uint32_t(next / divisor);
            remainder = next % divisor;
        }
        return unsigned(remainder);
    }

    bool add(const Wide& other) {
        std::uint64_t carry = 0;
        for (unsigned i = 0; i < 6; ++i) {
            auto next = std::uint64_t(limbs[i]) + other.limbs[i] + carry;
            limbs[i] = std::uint32_t(next);
            carry = next >> 32;
        }
        return carry == 0;
    }

    void increment() {
        for (auto& n : limbs) {
            if (++n) {
                break;
            }
        }
    }

    void subtract(const Wide& other) {
        std::uint64_t borrow = 0;
        for (unsigned i = 0; i < 6; ++i) {
            auto sub = std::uint64_t(other.limbs[i]) + borrow;
            borrow = std::uint64_t(limbs[i]) < sub;
            limbs[i] = std::uint32_t(std::uint64_t(limbs[i]) - sub);
        }
    }

    Wide quotient(const Wide& divisor, Wide& remainder) const {
        Wide result;
        remainder = Wide{};
        for (int bit = 191; bit >= 0; --bit) {
            remainder.multiply(2);
            remainder.limbs[0] |= (limbs[bit / 32] >> (bit % 32)) & 1;
            if (remainder.compare(divisor) >= 0) {
                remainder.subtract(divisor);
                result.limbs[bit / 32] |= std::uint32_t(1) << (bit % 32);
            }
        }
        return result;
    }

    Decimal decimal(bool negative, unsigned scale, bool raw_zero = false) const {
        return detail::DecimalArithmeticAccess::make(
            {limbs[0], limbs[1], limbs[2]}, negative && (raw_zero || !zero()), scale);
    }
};

std::optional<Decimal> fit(Wide value,
                           unsigned scale,
                           bool negative,
                           bool raw_zero = false,
                           bool sticky = false,
                           bool preserve_scale = false) {
    unsigned last = 0;
    bool discarded = false;
    for (;;) {
        while ((!preserve_scale && scale > 28) || !value.fits()) {
            if (!scale) {
                return {};
            }
            sticky |= last != 0;
            last = value.divide(10);
            --scale;
            discarded = true;
        }
        auto rounded = value;
        if (discarded && (last > 5 || (last == 5 && (sticky || (value.limbs[0] & 1))))) {
            rounded.increment();
        }
        if (rounded.fits()) {
            return rounded.decimal(negative, scale, raw_zero);
        }
        if (!scale) {
            return {};
        }
        sticky |= last != 0;
        last = value.divide(10);
        --scale;
        discarded = true;
    }
}

std::optional<Decimal> add_sub(const Decimal& a, const Decimal& b, bool subtract) {
    if (a.is_zero()) {
        return subtract && !b.is_zero() ? -b : b;
    }
    if (b.is_zero()) {
        return a;
    }
    auto scale = std::max(a.scale(), b.scale());
    bool preserve_scale = true;
    if (a.scale() != b.scale()) {
        const auto& lower = a.scale() < b.scale() ? a : b;
        Wide scaled(lower);
        auto remaining = scale - lower.scale();
        while (remaining) {
            // Original unaligned addition enters its wide-buffer path once
            // a high limb is already present before another scaling step.
            if (scaled.limbs[2] != 0 && scale > 28) {
                preserve_scale = false;
            }
            auto step = std::min(remaining, 9u);
            if (!scaled.scale(step) || !scaled.fits()) {
                preserve_scale = false;
            }
            remaining -= step;
        }
    }
    Wide left(a), right(b);
    if (!left.scale(scale - a.scale()) || !right.scale(scale - b.scale())) {
        throw std::domain_error("Addition exceeds original intermediate buffer");
    }
    bool negative = a.is_sign_negative();
    if (negative == (b.is_sign_negative() != subtract)) {
        if (!left.add(right)) {
            return {};
        }
    } else if (left.compare(right) < 0) {
        right.subtract(left);
        left = right;
        negative = !negative;
    } else {
        left.subtract(right);
    }
    return fit(left, scale, negative, false, false, preserve_scale);
}

Decimal bound(bool negative) {
    return *Decimal::from_parts({UINT32_MAX, UINT32_MAX, UINT32_MAX}, negative, 0);
}

Decimal require(std::optional<Decimal> value, const char* message) {
    if (!value) {
        throw std::overflow_error(message);
    }
    return *value;
}

// Match rust_decimal 1.42.1's division-specific unscale order. This deliberately
// differs from normalize(): its 4/2/1-digit steps are each attempted only once.
void partial_remainder96(std::array<std::uint32_t, 4>& input, const Wide& divisor) {
    auto high = std::uint64_t(input[2]) | (std::uint64_t(input[3]) << 32);
    auto divisor_hi = divisor.limbs[2];
    if (high < divisor_hi) {
        return;
    }
    auto quotient = std::uint32_t(high / divisor_hi);
    auto remainder = std::uint32_t(high) - quotient * divisor_hi;
    auto product_low = std::uint64_t(quotient) * divisor.limbs[0];
    auto product_high = std::uint64_t(quotient) * divisor.limbs[1];
    product_high += product_low >> 32;
    product_low = std::uint32_t(product_low) | (product_high << 32);
    product_high >>= 32;
    auto low = (std::uint64_t(input[0]) | (std::uint64_t(input[1]) << 32)) - product_low;
    remainder -= std::uint32_t(product_high);
    bool adjusted = false;
    if (low > ~product_low) {
        --remainder;
        adjusted = remainder < ~std::uint32_t(product_high);
    } else {
        adjusted = remainder <= ~std::uint32_t(product_high);
    }
    if (!adjusted) {
        auto divisor_low =
            std::uint64_t(divisor.limbs[0]) | (std::uint64_t(divisor.limbs[1]) << 32);
        for (;;) {
            --quotient;
            low += divisor_low;
            remainder += divisor_hi;
            if (low < divisor_low) {
                auto previous = remainder;
                ++remainder;
                if (previous < divisor_hi) {
                    break;
                }
            }
            if (remainder < divisor_hi) {
                break;
            }
        }
    }
    input[0] = std::uint32_t(low);
    input[1] = std::uint32_t(low >> 32);
    input[2] = remainder;
    // The original partial division retains input[3] for the next window.
}

void partial_remainder64(std::array<std::uint32_t, 4>& input, std::uint64_t divisor) {
    auto low = std::uint64_t(input[0]) | (std::uint64_t(input[1]) << 32);
    auto mid = std::uint64_t(input[1]) | (std::uint64_t(input[2]) << 32);
    if (divisor <= mid) {
        throw std::domain_error("Original remainder divisor normalization assertion");
    }
    if (input[2] == 0) {
        low %= divisor;
    } else if (input[2] >= std::uint32_t(divisor >> 32)) {
        low = low - (divisor << 32) + divisor;
        while (low >= divisor) {
            low += divisor;
        }
    } else {
        auto divisor_hi = divisor >> 32;
        if (mid < divisor_hi) {
            return;
        }
        auto quotient = mid / divisor_hi;
        auto remainder = std::uint64_t(input[0]) | ((mid - quotient * divisor_hi) << 32);
        auto product = quotient * std::uint32_t(divisor);
        remainder -= product;
        if (remainder > ~product) {
            for (;;) {
                --quotient;
                remainder += divisor;
                if (remainder < divisor) {
                    break;
                }
            }
        }
        low = remainder;
    }
    input[0] = std::uint32_t(low);
    input[1] = std::uint32_t(low >> 32);
}

Decimal buffered_remainder(const Wide& dividend,
                           const Wide& divisor,
                           int remaining_scale,
                           unsigned result_scale,
                           bool negative) {
    unsigned shift = std::countl_zero(divisor.limbs[2] ? divisor.limbs[2] : divisor.limbs[1]);
    std::array<std::uint32_t, 7> buffer{};
    auto low64 = std::uint64_t(dividend.limbs[0]) | (std::uint64_t(dividend.limbs[1]) << 32);
    auto high64 = std::uint64_t(dividend.limbs[1]) | (std::uint64_t(dividend.limbs[2]) << 32);
    auto low = low64 << shift;
    auto mid = high64 >> (32 - shift);
    buffer[0] = std::uint32_t(low);
    buffer[1] = std::uint32_t(low >> 32);
    buffer[2] = std::uint32_t(mid);
    buffer[3] = std::uint32_t(mid >> 32);
    unsigned upper = 3;
    while (remaining_scale < 0) {
        auto digits = unsigned(std::min(9, -remaining_scale));
        std::uint64_t power = 1;
        for (unsigned i = 0; i < digits; ++i) {
            power *= 10;
        }
        std::uint64_t carry = 0;
        for (unsigned i = 0; i <= upper; ++i) {
            carry += std::uint64_t(buffer[i]) * power;
            buffer[i] = std::uint32_t(carry);
            if (i != upper) {
                carry >>= 32;
            }
        }
        // Original grows on the last full product, not just its carry.
        if (carry > 0x7fffffff) {
            ++upper;
            if (upper >= buffer.size()) {
                throw std::domain_error("Original remainder intermediate overflow");
            }
            buffer[upper] = std::uint32_t(carry >> 32);
        }
        remaining_scale += 9;
    }
    Wide normalized = divisor;
    for (unsigned i = 0; i < shift; ++i) {
        normalized.multiply(2);
    }
    auto wide_divisor = divisor.limbs[2] != 0;
    auto window = [&](unsigned start, bool top64 = false) {
        std::array<std::uint32_t, 4> current{};
        for (unsigned i = 0; i < 4; ++i) {
            current[i] = top64 && i == 3 ? 0 : buffer[start + i];
        }
        if (wide_divisor) {
            partial_remainder96(current, normalized);
        } else {
            partial_remainder64(current,
                                std::uint64_t(normalized.limbs[0]) |
                                    (std::uint64_t(normalized.limbs[1]) << 32));
        }
        for (unsigned i = 0; i < 4 && start + i < buffer.size(); ++i) {
            buffer[start + i] = current[i];
        }
    };
    if (wide_divisor) {
        while (upper > 3) {
            window(upper - 3);
            --upper;
        }
        window(0);
    } else {
        if (upper == 6) {
            window(4, true);
            --upper;
        }
        while (upper > 3) {
            window(upper - 2, true);
            --upper;
        }
        window(1, true);
        window(0, true);
    }
    Wide remainder;
    auto joined = std::uint64_t(buffer[0]) | (std::uint64_t(buffer[1]) << 32);
    joined >>= shift;
    if (wide_divisor) {
        // Avoid a native shift by 32 while retaining the original u64 wrap.
        if (shift != 0) {
            joined += (std::uint64_t(buffer[2]) << (32 - shift)) << 32;
        }
        remainder.limbs[2] = buffer[2] >> shift;
    }
    remainder.limbs[0] = std::uint32_t(joined);
    remainder.limbs[1] = std::uint32_t(joined >> 32);
    if (result_scale > 28) {
        throw std::domain_error("Buffered remainder scale exceeds checked construction");
    }
    return remainder.decimal(negative, result_scale);
}

Decimal division_unscale(const Decimal& input) {
    Wide value(input);
    auto scale = input.scale();
    auto attempt = [&](unsigned digits, unsigned power) {
        if (scale < digits) {
            return false;
        }
        auto copy = value;
        if (copy.divide(power)) {
            return false;
        }
        value = copy;
        scale -= digits;
        return true;
    };
    while (value.limbs[0] == 0 && attempt(8, 100000000)) {
    }
    if (!(value.limbs[0] & 15)) {
        attempt(4, 10000);
    }
    if (!(value.limbs[0] & 3)) {
        attempt(2, 100);
    }
    if (!(value.limbs[0] & 1)) {
        attempt(1, 10);
    }
    return value.decimal(input.is_sign_negative(), scale);
}
}

std::strong_ordering Decimal::operator<=>(const Decimal& other) const {
    if (is_zero() && other.is_zero()) {
        return std::strong_ordering::equal;
    }
    const bool negative = negative_ && !is_zero(),
               other_negative = other.negative_ && !other.is_zero();
    if (negative != other_negative) {
        return negative ? std::strong_ordering::less : std::strong_ordering::greater;
    }
    Wide a(*this), b(other);
    int order = 0;
    for (unsigned i = scale_; i < other.scale_; ++i) {
        a.multiply(10);
        if (!a.fits()) {
            order = 1;
            break;
        }
    }
    for (unsigned i = other.scale_; !order && i < scale_; ++i) {
        b.multiply(10);
        if (!b.fits()) {
            order = -1;
            break;
        }
    }
    if (!order) {
        order = a.compare(b);
    }
    if (negative) {
        order = -order;
    }
    return order < 0   ? std::strong_ordering::less
           : order > 0 ? std::strong_ordering::greater
                       : std::strong_ordering::equal;
}

std::optional<Decimal> Decimal::checked_add(const Decimal& other) const {
    return add_sub(*this, other, false);
}

std::optional<Decimal> Decimal::checked_sub(const Decimal& other) const {
    return add_sub(*this, other, true);
}

std::optional<Decimal> Decimal::checked_mul(const Decimal& other) const {
    if (is_zero() || other.is_zero()) {
        return Decimal{};
    }
    unsigned scale = scale_ + other.scale_;
    bool negative = negative_ != other.negative_;
    bool small =
        !(coefficient_[1] | coefficient_[2] | other.coefficient_[1] | other.coefficient_[2]);
    if (small && scale > 47) {
        return Decimal{};
    }
    Wide product;
    for (unsigned i = 0; i < 3; ++i) {
        std::uint64_t carry = 0;
        for (unsigned j = 0; j < 3; ++j) {
            auto n = std::uint64_t(coefficient_[i]) * other.coefficient_[j] + product.limbs[i + j] +
                     carry;
            product.limbs[i + j] = std::uint32_t(n);
            carry = n >> 32;
        }
        product.limbs[i + 3] = std::uint32_t(carry);
    }
    return fit(product, scale, negative, !small);
}

std::optional<Decimal> Decimal::checked_div(const Decimal& other) const {
    if (other.is_zero()) {
        return {};
    }
    if (is_zero()) {
        return Decimal{};
    }
    Wide divisor(other), remainder;
    auto quotient = Wide(*this).quotient(divisor, remainder);
    int scale = int(scale_) - int(other.scale_);
    if (scale > 28 && !remainder.zero() && quotient.limbs[2] <= 429496729u) {
        // Original find_scale indexes its finite power table with 28-scale.
        throw std::domain_error("Division scale exceeds original power table");
    }
    bool unscale = false;
    while (!remainder.zero() || scale < 0) {
        unscale |= !remainder.zero();
        auto next = quotient;
        unsigned count = 0;
        auto limit = remainder.zero() ? std::min(9, -scale) : std::min(9, 28 - scale);
        for (int i = 0; i < limit; ++i) {
            auto candidate = next;
            candidate.multiply(10);
            if (!candidate.fits()) {
                break;
            }
            next = candidate;
            ++count;
        }
        if (!count) {
            break;
        }
        auto next_remainder = remainder;
        next_remainder.scale(count);
        Wide rest;
        auto digit = next_remainder.quotient(divisor, rest);
        next.add(digit);
        scale += int(count);
        if (!next.fits()) {
            if (scale < 0) {
                return {};
            }
            auto result =
                fit(next, unsigned(scale), negative_ != other.negative_, false, !rest.zero());
            if (result && unscale) {
                return division_unscale(*result);
            }
            return result;
        }
        quotient = next;
        remainder = rest;
    }
    if (scale < 0) {
        return {};
    }
    auto twice = remainder;
    twice.multiply(2);
    auto order = twice.compare(divisor);
    if (order > 0 || (order == 0 && (quotient.limbs[0] & 1))) {
        quotient.increment();
    }
    if (scale > 28) {
        auto result = quotient.decimal(negative_ != other.negative_, unsigned(scale));
        if (unscale) {
            result = division_unscale(result);
        }
        if (result.scale() > 28) {
            throw std::domain_error("Division scale exceeds maximum supported scale");
        }
        return result;
    }
    auto result = fit(quotient, unsigned(scale), negative_ != other.negative_);
    if (result && unscale) {
        return division_unscale(*result);
    }
    return result;
}

std::optional<Decimal> Decimal::checked_rem(const Decimal& other) const {
    if (other.is_zero()) {
        return {};
    }
    if (is_zero()) {
        return Decimal{};
    }
    auto magnitude = abs() <=> other.abs();
    if (magnitude == 0) {
        return Decimal{};
    }
    if (magnitude < 0) {
        return *this;
    }
    Wide dividend(*this), divisor(other);
    int remaining_scale = int(scale_) - int(other.scale_);
    unsigned result_scale = scale_;
    if (remaining_scale > 0) {
        if (!divisor.scale(unsigned(remaining_scale))) {
            throw std::domain_error("Remainder divisor alignment overflow");
        }
        remaining_scale = 0;
    }
    for (;;) {
        if (remaining_scale < 0) {
            for (;;) {
                unsigned count = 0;
                Wide candidate = dividend;
                auto limit = unsigned(std::min(9, -remaining_scale));
                for (unsigned i = 0; i < limit; ++i) {
                    auto next = candidate;
                    next.multiply(10);
                    if (!next.fits()) {
                        break;
                    }
                    candidate = next;
                    ++count;
                }
                // Original find_scale has an early nine-digit fast path;
                // shorter growth must still satisfy its signed scale bound.
                if (count < 9 && int(count) + 28 + remaining_scale < 0) {
                    return {};
                }
                if (!count) {
                    break;
                }
                dividend = candidate;
                remaining_scale += int(count);
                if (count != 9 || remaining_scale >= 0) {
                    break;
                }
            }
            result_scale = other.scale_;
        }
        if (dividend.limbs[2] == 0) {
            auto lhs = std::uint64_t(dividend.limbs[0]) | (std::uint64_t(dividend.limbs[1]) << 32);
            auto rhs = std::uint64_t(divisor.limbs[0]) | (std::uint64_t(divisor.limbs[1]) << 32);
            auto value = lhs % rhs;
            return detail::DecimalArithmeticAccess::make(
                {std::uint32_t(value), std::uint32_t(value >> 32), 0},
                negative_ && value != 0,
                result_scale);
        }
        if (divisor.limbs[1] || divisor.limbs[2]) {
            return buffered_remainder(dividend, divisor, remaining_scale, result_scale, negative_);
        }
        auto remainder = dividend.divide(divisor.limbs[0]);
        dividend = Wide{};
        dividend.limbs[0] = remainder;
        if (remaining_scale >= 0) {
            return dividend.decimal(negative_, result_scale);
        }
    }
}

Decimal Decimal::saturating_add(const Decimal& other) const {
    auto result = checked_add(other);
    return result ? *result : bound(negative_);
}

Decimal Decimal::saturating_sub(const Decimal& other) const {
    auto result = checked_sub(other);
    return result ? *result : bound(negative_);
}

Decimal Decimal::saturating_mul(const Decimal& other) const {
    auto result = checked_mul(other);
    return result ? *result : bound(negative_ != other.negative_);
}

Decimal Decimal::operator+(const Decimal& other) const {
    return require(checked_add(other), "Addition overflowed");
}

Decimal Decimal::operator-(const Decimal& other) const {
    return require(checked_sub(other), "Subtraction overflowed");
}

Decimal Decimal::operator*(const Decimal& other) const {
    return require(checked_mul(other), "Multiplication overflowed");
}

Decimal Decimal::operator/(const Decimal& other) const {
    if (other.is_zero()) {
        throw std::domain_error("Division by zero");
    }
    return require(checked_div(other), "Division overflowed");
}

Decimal Decimal::operator%(const Decimal& other) const {
    if (other.is_zero()) {
        throw std::domain_error("Division by zero");
    }
    return require(checked_rem(other), "Remainder overflowed");
}

Decimal Decimal::trunc() const {
    Wide value(*this);
    for (unsigned i = 0; i < scale_; ++i) {
        value.divide(10);
    }
    return value.decimal(negative_, 0, true);
}

Decimal Decimal::floor() const {
    auto value = trunc();
    return negative_ && *this != value ? value - from_i64(1) : value;
}

Decimal Decimal::ceil() const {
    auto value = trunc();
    return !negative_ && *this != value ? value + from_i64(1) : value;
}

Decimal Decimal::normalize() const {
    if (is_zero()) {
        return Decimal{};
    }
    Wide value(*this);
    auto scale = unsigned(scale_);
    while (scale) {
        auto next = value;
        if (next.divide(10)) {
            break;
        }
        value = next;
        --scale;
    }
    auto result = *this;
    result.coefficient_ = {value.limbs[0], value.limbs[1], value.limbs[2]};
    result.scale_ = static_cast<unsigned char>(scale);
    return result;
}

Decimal Decimal::round_dp_with_strategy(unsigned dp, RoundingStrategy strategy) const {
    if (dp >= scale_) {
        return *this;
    }
    if (is_zero()) {
        auto result = *this;
        result.scale_ = static_cast<unsigned char>(dp);
        return result;
    }
    Wide value(*this);
    unsigned last = 0;
    bool sticky = false;
    for (unsigned i = dp; i < scale_; ++i) {
        sticky |= last != 0;
        last = value.divide(10);
    }
    bool nonzero = last || sticky, up = false;
    switch (strategy) {
    case RoundingStrategy::BankersRounding:
    case RoundingStrategy::MidpointNearestEven:
        up = last > 5 || (last == 5 && (sticky || (value.limbs[0] & 1)));
        break;
    case RoundingStrategy::RoundHalfUp:
    case RoundingStrategy::MidpointAwayFromZero:
        up = last >= 5;
        break;
    case RoundingStrategy::RoundHalfDown:
    case RoundingStrategy::MidpointTowardZero:
        up = last > 5 || (last == 5 && sticky);
        break;
    case RoundingStrategy::RoundDown:
    case RoundingStrategy::ToZero:
        break;
    case RoundingStrategy::RoundUp:
    case RoundingStrategy::AwayFromZero:
        up = nonzero;
        break;
    case RoundingStrategy::ToNegativeInfinity:
        up = negative_ && nonzero;
        break;
    case RoundingStrategy::ToPositiveInfinity:
        up = !negative_ && nonzero;
        break;
    }
    if (up) {
        value.increment();
    }
    return value.decimal(negative_, dp);
}
}
