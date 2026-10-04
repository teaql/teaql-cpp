#include <teaql/core/decimal.hpp>
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace teaql {
namespace {
using Bits = std::array<std::uint32_t, 3>;
template<std::size_t N> bool zero(const std::array<std::uint32_t,N>& b) { for(auto n:b) if(n) return false;return true; }
template<std::size_t N> bool mul(std::array<std::uint32_t,N>& b, unsigned n, unsigned add = 0) {
    auto copy = b; std::uint64_t carry = add;
    for (auto& limb : copy) { carry += std::uint64_t(limb) * n; limb = std::uint32_t(carry); carry >>= 32; }
    if (carry) return false;
    b = copy; return true;
}
template<std::size_t N> unsigned div(std::array<std::uint32_t,N>& b, unsigned n) {
    std::uint64_t rest = 0;
    for (int i = int(N)-1; i >= 0; --i) { rest = (rest << 32) | b[i]; b[i] = std::uint32_t(rest / n); rest %= n; }
    return unsigned(rest);
}
Bits truncated(Bits b, unsigned scale) { while (scale--) div(b, 10); return b; }
// Correctly round a limb array to IEEE binary64 once.
template<std::size_t N> double as_double(std::array<std::uint32_t,N> b) {
    int width=0;for(unsigned i=0;i<N;++i) if(b[i]) width=int(i*32+std::bit_width(b[i]));
    int shift = width > 53 ? width - 53 : 0;
    bool sticky = false, last = false;
    for (int i = 0; i < shift; ++i) { sticky |= last; last = div(b, 2) != 0; }
    std::uint64_t n = (std::uint64_t(b[1]) << 32) | b[0];
    if (last && (sticky || (n & 1))) ++n;
    return std::ldexp(double(n), shift);
}
std::optional<Decimal> plain(std::string_view s, std::string_view& error) {
    auto fail = [&](std::string_view message) -> std::optional<Decimal> { error = message; return {}; };
    if (s.empty()) return fail("Invalid decimal: empty");
    bool negative = false, point = false, digit = false, wide = false; unsigned scale = 0; Bits b{};
    if (!s.empty() && (s.front() == '+' || s.front() == '-')) { negative = s.front() == '-'; s.remove_prefix(1); }
    for (char c : s) {
        if (c == '_' && digit) {
            // rust_decimal's 64-bit parser consumes '_' as a zero rounding
            // digit at scale 28; its wide parser skips separators instead.
            if (scale == 28 && !wide) return Decimal::from_parts(b, negative, scale);
            continue;
        }
        if (c == '.' && !point) { point = true; continue; }
        if (c < '0' || c > '9') return fail(c == '.' ? "Invalid decimal: two decimal points" : c == '_' ? "Invalid decimal: must start lead with a number" : "Invalid decimal: unknown character");
        digit = true;
        auto next = b;
        if ((point && scale == 28) || !mul(next, 10, unsigned(c - '0'))) {
            if (!point) return fail("Invalid decimal: overflow from too many digits");
            if (c >= '5' && !mul(b, 1, 1)) {
                if (!scale) return fail("Invalid decimal: overflow from mantissa after rounding");
                // The rounded coefficient needs one less fractional digit.
                const auto remainder = div(b, 10);
                if (remainder >= 4) mul(b, 1, 1);
                --scale;
            }
            return Decimal::from_parts(b, negative, scale);
        }
        b = next; if (point) ++scale;
        if (scale != 28 && (b[2] || ((std::uint64_t(b[1]) << 32) | b[0]) >= UINT64_MAX / 10 - 255)) wide = true;
    }
    if (!digit) return fail("Invalid decimal: no digits found");
    return Decimal::from_parts(b, negative, scale);
}
}
std::optional<Decimal> Decimal::from_parts(Bits b, bool negative, unsigned scale) {
    if (scale > 28) return {};
    Decimal d; d.coefficient_ = b; d.scale_ = static_cast<unsigned char>(scale); d.negative_ = negative && !zero(b); return d;
}
Decimal Decimal::from_u64(std::uint64_t n) { return *from_parts({std::uint32_t(n), std::uint32_t(n >> 32), 0}, false, 0); }
Decimal Decimal::from_i64(std::int64_t n) {
    auto d = from_u64(n < 0 ? std::uint64_t(0) - std::uint64_t(n) : std::uint64_t(n)); d.negative_ = n < 0; return d;
}
std::optional<Decimal> Decimal::parse(std::string_view s) {
    DecimalError::Code code{}; std::string_view message; std::uint32_t scale = 0;
    return parse_detail(s, code, message, scale);
}
Result<Decimal,DecimalError> Decimal::parse_result(Memory r,std::string_view s) {
    DecimalError::Code code{}; std::string_view message; std::uint32_t scale = 0;
    auto result = parse_detail(s, code, message, scale);
    if (result) return Result<Decimal,DecimalError>::ok(r,*result);
    return Result<Decimal,DecimalError>::err(r,DecimalError(std::allocator_arg,Allocator(r),code,message,scale));
}
std::optional<Decimal> Decimal::parse_detail(std::string_view s,DecimalError::Code& code,std::string_view& message,std::uint32_t& error_scale) {
    code = DecimalError::Code::ErrorString;
    if (auto d = plain(s,message)) return d;
    const auto pos = s.find_first_of("eE"); if (pos == s.npos) return {};
    auto base = plain(s.substr(0, pos),message); if (!base) return {};
    s.remove_prefix(pos + 1); bool negative = false;
    if (!s.empty() && s.front() == '-') { negative = true; s.remove_prefix(1); }
    if (!s.empty() && s.front() == '+') s.remove_prefix(1);
    unsigned exp = 0; auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), exp);
    if (s.empty() || ec != std::errc{} || end != s.data() + s.size()) { message = "Failed to parse"; return {}; }
    if (exp > 28) { code = DecimalError::Code::ScaleExceedsMaximumPrecision; error_scale = exp; return {}; }
    auto& b = base->coefficient_;
    if (negative) {
        unsigned next_scale = base->scale_ + exp, remainder = 0;
        while (next_scale > 28) { remainder = div(b, 10); --next_scale; }
        if (base->scale_ + exp > 28 && remainder >= 5) mul(b, 1, 1);
        base->scale_ = static_cast<unsigned char>(next_scale);
    } else if (exp <= base->scale_) base->scale_ -= static_cast<unsigned char>(exp);
    else {
        for (unsigned i = base->scale_; i < exp; ++i) if (!mul(b, 10)) { code = DecimalError::Code::ExceedsMaximumPossibleValue; return {}; }
        base->scale_ = 0;
    }
    if (zero(b)) base->negative_ = false;
    return base;
}
// Binary conversion adapted from rust_decimal 1.42.1 (Paul Mason, MIT).
// The license is retained in LICENSES/rust_decimal-MIT.txt. No library linkage.
std::optional<Decimal> Decimal::from_f64(double n) {
    if (!std::isfinite(n)) return {};
    const auto raw = std::bit_cast<std::uint64_t>(n);
    const bool negative = (raw >> 63) != 0;
    const auto exponent = int((raw >> 52) & 0x7ff);
    auto mantissa = raw & 0x000fffffffffffffULL;
    if (exponent) mantissa |= 0x0010000000000000ULL;
    Bits b{std::uint32_t(mantissa), std::uint32_t(mantissa >> 32), 0};
    int e10 = exponent - 1023 - 52 + (exponent == 0), e5 = -e10;
    if (zero(b)) { Decimal d; d.negative_ = negative; return d; }
    while (e5 > 0) {
        --e5;
        if (!(b[0] & 1)) { div(b, 2); ++e10; }
        else if (!mul(b, 5)) { div(b, 2); ++e10; }
    }
    while (e5 < 0) {
        ++e5;
        if (!(b[2] & 0x80000000)) { mul(b, 2); --e10; }
        else if (e10 * 2 > -(e5 - 1)) return {};
        else div(b, 5);
    }
    while (e10 > 0) { if (!mul(b, 10)) return {}; --e10; }
    while (e10 < -28) {
        const auto rem = div(b, 10); ++e10;
        if (zero(b)) e10 = 0;
        else if (rem >= 5) mul(b, 1, 1);
    }
    while (e10 < 0 && (b[2] || (b[1] & 0xfff00000))) {
        const auto rem = div(b, 10); ++e10; if (rem >= 5) mul(b, 1, 1);
    }
    while (e10 < 0) { auto copy = b; if (div(copy, 10)) break; b = copy; ++e10; }
    auto d = *from_parts(b, negative, unsigned(-e10)); d.negative_ = negative; return d;
}
std::optional<std::uint64_t> Decimal::to_u64() const {
    if (negative_) return {};
    const auto b = truncated(coefficient_, scale_); if (b[2]) return {};
    return (std::uint64_t(b[1]) << 32) | b[0];
}
std::optional<std::int64_t> Decimal::to_i64() const {
    const auto b = truncated(coefficient_, scale_); if (b[2]) return {};
    auto n = (std::uint64_t(b[1]) << 32) | b[0];
    constexpr auto max = std::uint64_t(INT64_MAX);
    if (n > max + unsigned(negative_)) return {};
    if (negative_ && n == max + 1) return INT64_MIN;
    return negative_ ? -std::int64_t(n) : std::int64_t(n);
}
double Decimal::to_f64() const {
    if(scale_>38) throw std::overflow_error("Decimal floating conversion precision overflow");
    if(scale_>28) {
        std::array<std::uint32_t,4> power{1,0,0,0};for(unsigned i=0;i<scale_;++i) mul(power,10);
        auto fraction=as_double(coefficient_)/as_double(power);
        double round_to=1,factor=10;unsigned e=scale_;
        while(e) { if(e&1) round_to*=factor;e>>=1;if(e) factor*=factor; }
        auto result=std::round(fraction*round_to)/round_to;return negative_ ? -result:result;
    }
    // Rust converts scale-zero values through i128, which has no negative zero.
    if (!scale_) { const auto n = as_double(coefficient_); return negative_ && !is_zero() ? -n : n; }
    auto whole = truncated(coefficient_, scale_), scaled = whole, power = Bits{1, 0, 0};
    for (unsigned i = 0; i < scale_; ++i) { mul(scaled, 10); mul(power, 10); }
    Bits fraction = coefficient_; std::uint64_t borrow = 0;
    for (unsigned i = 0; i < 3; ++i) { auto sub = std::uint64_t(scaled[i]) + borrow; borrow = std::uint64_t(fraction[i]) < sub; fraction[i] = std::uint32_t(std::uint64_t(fraction[i]) - sub); }
    double n = as_double(whole);
    if (!zero(fraction)) {
        n += as_double(fraction) / as_double(power);
        double round_to = 1, factor = 10; unsigned e = scale_;
        while (e) { if (e & 1) round_to *= factor; e >>= 1; if (e) factor *= factor; }
        n = std::round(n * round_to) / round_to;
    }
    return negative_ ? -n : n;
}
String Decimal::to_string(Memory r) const {
    if(scale_>30) throw std::length_error("Decimal display exceeds the locked Rust formatter capacity");
    char buffer[32]; auto end = buffer + sizeof(buffer), start = end; auto b = coefficient_;
    do { *--start = char('0' + div(b, 10)); } while (!zero(b));
    String out(r); if (negative_) out += '-';
    auto size = unsigned(end - start);
    if (!scale_) out.append(start, end);
    else if (size > scale_) { out.append(start, end - scale_); out += '.'; out.append(end - scale_, end); }
    else { out += "0."; out.append(scale_ - size, '0'); out.append(start, end); }
    return out;
}
bool Decimal::operator==(const Decimal& other) const {
    if (zero(coefficient_) && zero(other.coefficient_)) return true;
    if (negative_ != other.negative_) return false;
    auto a = coefficient_, b = other.coefficient_; auto sa = scale_, sb = other.scale_;
    while (sa > sb) { if (div(a, 10)) return false; --sa; }
    while (sb > sa) { if (div(b, 10)) return false; --sb; }
    return a == b;
}
String DecimalError::to_string(Memory r) const {
    switch(code) {
        case Code::ErrorString:return String(text,r);
        case Code::ExceedsMaximumPossibleValue:return String("Number exceeds maximum value that can be represented.",r);
        case Code::LessThanMinimumPossibleValue:return String("Number less than minimum value that can be represented.",r);
        case Code::Underflow:return String("Number has a high precision that can not be represented.",r);
        case Code::ScaleExceedsMaximumPrecision: {
            String result("Scale exceeds the maximum precision allowed: ",r);char digits[12];auto [end,error]=std::to_chars(digits,digits+sizeof(digits),scale);(void)error;
            result.append(digits,end);result+=" > 28";return result;
        }
        case Code::ConversionTo: { String result("Error while converting to ",r);result+=text;return result; }
    }
    throw std::invalid_argument("Unknown Decimal error code");
}
Result<std::monostate,DecimalError> Decimal::set_scale(Memory r,unsigned scale) {
    using Outcome=Result<std::monostate,DecimalError>;
    if(scale>MAX_SCALE) return Outcome::err(r,DecimalError(std::allocator_arg,Allocator(r),DecimalError::Code::ScaleExceedsMaximumPrecision,{},scale));
    scale_=static_cast<unsigned char>(scale);return Outcome::ok(r,{});
}
void Decimal::adjust_scale(unsigned target,bool round) {
    if(scale_==target) return;
    if(is_zero()) { scale_=static_cast<unsigned char>(std::min(target,MAX_SCALE));return; }
    if(scale_>target) {
        unsigned last=0;
        for(unsigned i=target;i<scale_;++i) {
            if(is_zero()) { scale_=static_cast<unsigned char>(target);return; }
            last=div(coefficient_,10);
        }
        if(round && last>=5) mul(coefficient_,1,1);
        scale_=static_cast<unsigned char>(target);
    } else {
        while(scale_<target) { if(!mul(coefficient_,10)) break;++scale_; }
    }
}
bool Decimal::is_integer() const {
    auto value=coefficient_;for(unsigned i=0;i<scale_;++i) if(div(value,10)) return false;return true;
}
std::array<std::uint8_t,16> Decimal::serialize() const {
    std::array<std::uint8_t,16> result{};
    const std::array<std::uint32_t,4> words{(std::uint32_t(scale_)<<16)|(std::uint32_t(negative_)<<31),coefficient_[0],coefficient_[1],coefficient_[2]};
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<4;++j) result[4*i+j]=std::uint8_t(words[i]>>(8*j));
    return result;
}
Decimal Decimal::deserialize(const std::array<std::uint8_t,16>& bytes) {
    std::array<std::uint32_t,4> words{};
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<4;++j) words[i]|=std::uint32_t(bytes[4*i+j])<<(8*j);
    Decimal result;result.coefficient_={words[1],words[2],words[3]};result.negative_=(words[0]>>31)!=0;result.scale_=(words[0]>>16)&31;
    if(result.scale_>MAX_SCALE) {
        unsigned power=1;for(unsigned i=MAX_SCALE;i<result.scale_;++i) power*=10;
        // The upstream decoder compares the entire remainder to 5, including
        // scale 30/31. Do not replace this with generic decimal rounding.
        if(div(result.coefficient_,power)>=5) mul(result.coefficient_,1,1);
        result.scale_=MAX_SCALE;
    }
    return result;
}
std::optional<Decimal> Decimal::round_sf_with_strategy(unsigned digits,RoundingStrategy strategy) const {
    if(is_zero() || !digits) return Decimal{};
    auto working=coefficient_;unsigned count=0;do { div(working,10);++count; } while(!zero(working));
    if(digits>count) {
        if(std::uint64_t(scale_)+digits>UINT32_MAX) throw std::overflow_error("Decimal significant-figure scale overflow");
        auto result=*this;result.rescale(unsigned(scale_)+digits-count);return result;
    }
    if(digits==count) return *this;
    auto difference=count-digits;
    if(difference<=scale_) return round_dp_with_strategy(scale_-difference,strategy);
    unsigned exponent=difference-scale_;auto result=*this;
    for(unsigned remaining=exponent;remaining;) {
        auto chunk=std::min(remaining,19u);std::uint64_t power=1;for(unsigned i=0;i<chunk;++i) power*=10;
        auto value=result.checked_div(from_u64(power));if(!value) return {};result=*value;remaining-=chunk;
    }
    result=result.round_dp_with_strategy(0,strategy).trunc();
    for(unsigned remaining=exponent;remaining;) {
        auto chunk=std::min(remaining,19u);std::uint64_t power=1;for(unsigned i=0;i<chunk;++i) power*=10;
        auto value=result.checked_mul(from_u64(power));if(!value) return {};result=*value;remaining-=chunk;
    }
    return result;
}
}
