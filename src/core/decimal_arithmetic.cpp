#include <teaql/core/decimal.hpp>
#include <algorithm>
#include <stdexcept>

// rust_decimal 1.42.1 arithmetic/rounding semantics (Paul Mason, MIT).
// Division's batch/unscale rules are adapted from that implementation;
// LICENSES/rust_decimal-MIT.txt retains its license. Wide math uses C++20 only.

namespace teaql {
namespace {
// Six little-endian limbs hold both a 96x96 product and 96-bit inputs aligned
// across the full 28-digit scale range. All intermediate storage is on stack.
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

    void multiply(unsigned factor) {
        std::uint64_t carry = 0;
        for (auto& n : limbs) {
            auto next = std::uint64_t(n) * factor + carry;
            n = std::uint32_t(next);
            carry = next >> 32;
        }
    }

    void scale(unsigned count) {
        while (count--) {
            multiply(10);
        }
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

    void add(const Wide& other) {
        std::uint64_t carry = 0;
        for (unsigned i = 0; i < 6; ++i) {
            auto next = std::uint64_t(limbs[i]) + other.limbs[i] + carry;
            limbs[i] = std::uint32_t(next);
            carry = next >> 32;
        }
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
        auto checked = Decimal::from_parts({limbs[0], limbs[1], limbs[2]}, negative, scale);
        if (!checked) {
            throw std::domain_error("Scale exceeds maximum supported scale");
        }
        auto result = *checked;
        if (raw_zero) {
            result.set_sign_negative(negative);
        }
        return result;
    }
};

std::optional<Decimal>
fit(Wide value, unsigned scale, bool negative, bool raw_zero = false, bool sticky = false) {
    unsigned last = 0;
    bool discarded = false;
    for (;;) {
        while (scale > 28 || !value.fits()) {
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
    Wide left(a), right(b);
    left.scale(scale - a.scale());
    right.scale(scale - b.scale());
    bool negative = a.is_sign_negative();
    if (negative == (b.is_sign_negative() != subtract)) {
        left.add(right);
    } else if (left.compare(right) < 0) {
        right.subtract(left);
        left = right;
        negative = !negative;
    } else {
        left.subtract(right);
    }
    return fit(left, scale, negative);
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
    Wide a(*this), b(other);
    auto scale = std::max(scale_, other.scale_);
    a.scale(scale - scale_);
    b.scale(scale - other.scale_);
    auto order = a.compare(b);
    if (order == 0) {
        return Decimal{};
    }
    if (order < 0) {
        return *this;
    }
    Wide rest;
    (void)a.quotient(b, rest);
    return rest.decimal(negative_, scale);
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
