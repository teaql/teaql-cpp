#include <teaql/core/decimal.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
#include <set>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static Decimal d(std::string_view value) {
    auto result = Decimal::parse(value);
    if (!result) {
        throw std::runtime_error("Invalid test decimal");
    }
    return *result;
}

struct NoDefaultAllocator {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~NoDefaultAllocator() {
        std::pmr::set_default_resource(previous);
    }
};

static void arithmetic() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto max = d("79228162514264337593543950335"), min = -max;
    check(!max.checked_add(d("1")) && !min.checked_sub(d("1")),
          "checked addition and subtraction reject 96-bit overflow");
    check(max.saturating_add(d("1")) == max && min.saturating_sub(d("1")) == min,
          "saturating addition/subtraction choose signed limits");
    check(min.saturating_mul(d("-2")) == max && min.saturating_mul(d("2")) == min,
          "multiplication saturation uses result sign");
    auto amount = d("12.34");
    amount += d("0.66");
    amount *= d("2");
    amount -= d("1");
    amount /= d("5");
    amount %= d("3");
    check(amount == d("2"), "compound arithmetic operators");
    check((d("1") / d("2")).to_string(r) == "0.50",
          "division preserves locked Rust batch-unscale text");
    check((d("1.0") / d("2")).to_string(r) == "0.5",
          "exact quotient preserves the input scale difference");
    check((d("-5.5") % d("2")).to_string(r) == "-1.5" && (d("5.5") % d("-2")).to_string(r) == "1.5",
          "remainder uses dividend sign");
    check((d("0.000") + d("1.20")).scale() == 2 && (d("1.20") * d("0.000")).scale() == 0,
          "zero add and multiply have different scale rules");
    auto previous = max;
    bool overflow = false;
    try {
        max += d("1");
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    check(overflow && max == previous, "throwing compound overflow preserves destination");
    check(!d("1").checked_div(d("0")) && !d("1").checked_rem(d("0")),
          "checked zero divisor returns None");
    bool divide = false, remainder = false;
    try {
        (void)(d("1") / d("0"));
    } catch (const std::domain_error&) {
        divide = true;
    }
    try {
        (void)(d("1") % d("0"));
    } catch (const std::domain_error&) {
        remainder = true;
    }
    check(divide && remainder, "unchecked zero divisor reports domain errors");
}

static void rounding_and_order() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    check(d("2.5").round() == d("2") && d("3.5").round() == d("4"),
          "default rounding is ties-to-even");
    check(d("-2.5").round_dp_with_strategy(0, RoundingStrategy::MidpointAwayFromZero) == d("-3"),
          "midpoint away from zero");
    check(d("-2.5001").round_dp_with_strategy(0, RoundingStrategy::MidpointTowardZero) == d("-3"),
          "sticky fractional digits distinguish exact midpoint");
    check(d("2.1").round_dp_with_strategy(0, RoundingStrategy::ToPositiveInfinity) == d("3"),
          "positive infinity rounding");
    check(d("-2.1").round_dp_with_strategy(0, RoundingStrategy::ToNegativeInfinity) == d("-3"),
          "negative infinity rounding");
    check(d("1.20").round_dp(28).to_string(r) == "1.20",
          "rounding does not invent additional scale");
    check(d("-1.99").trunc() == d("-1") && d("-1.99").fract() == d("-0.99"),
          "truncation and signed fractional part");
    check(d("-1.01").floor() == d("-2") && d("-1.01").ceil() == d("-1"),
          "floor and ceil of negatives");
    auto zero = -d("0.000");
    check(zero.is_zero() && zero.is_sign_negative(), "negation preserves signed zero");
    check(zero.trunc().is_sign_negative() && !zero.normalize().is_sign_negative() &&
              zero.normalize().scale() == 0,
          "truncation and normalization handle negative zero differently");
    check(d("123.45000").normalize().to_string(r) == "123.45",
          "normalize strips exact decimal factors");
    auto n = d("1.00");
    n.normalize_assign();
    check(n.scale() == 0, "normalize_assign applies result");
    std::pmr::set<Decimal> keys(r);
    keys.emplace(d("1"));
    keys.emplace(d("1.00"));
    keys.emplace(zero);
    keys.emplace(d("0"));
    keys.emplace(d("-0.01"));
    check(keys.size() == 3 && *keys.begin() == d("-0.01"),
          "ordered containers use numeric equality across scale and zero sign");
    check(d("79228162514264337593543950335") > d("0.0000000000000000000000000001"),
          "comparison aligns extreme scales without overflow");
    check(RoundingStrategy::BankersRounding != RoundingStrategy::MidpointNearestEven,
          "deprecated rounding variants remain distinct enum values");
}

static void scale_codec_and_errors() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto amount = d("12.345");
    auto changed = amount;
    check(bool(changed.set_scale(r, 1)) && changed == d("1234.5"),
          "set_scale changes numeric value without changing coefficient");
    auto invalid = changed.set_scale(r, UINT32_MAX);
    check(!invalid && changed == d("1234.5") && invalid.error().scale == UINT32_MAX,
          "invalid set_scale preserves value and error payload");
    check(invalid.error().to_string(r) ==
              "Scale exceeds the maximum precision allowed: 4294967295 > 28",
          "scale error display");
    amount.rescale(2);
    check(amount.to_string(r) == "12.35", "rescale rounds midpoint away from zero");
    check(d("12.345").round_dp(2).to_string(r) == "12.34",
          "round_dp has distinct midpoint behavior");
    check(d("-12.345").trunc_with_scale(2).to_string(r) == "-12.34",
          "trunc_with_scale drops fractional digits");
    check(d("12.3").trunc_with_scale(4).to_string(r) == "12.3000",
          "trunc_with_scale may increase scale");
    auto extended = d("0.0000000000000000000000000001");
    extended.rescale(56);
    check(extended.scale() == 56 && extended == d("1e-28") && extended.normalize() == d("1e-28"),
          "rescale preserves observed extended-scale value");
    check(extended < d("1e-27") && extended > d("0"), "extended-scale comparison");
    bool display = false;
    try {
        (void)extended.to_string(r);
    } catch (const std::length_error&) {
        display = true;
    }
    check(display, "extended display reports locked formatter capacity failure");
    auto zero = -d("0.00");
    zero.rescale(UINT32_MAX);
    check(zero.scale() == 28 && zero.is_sign_negative(),
          "zero rescale clamps scale and preserves sign");
    check(Decimal::deserialize(zero.serialize()).serialize() == zero.serialize(),
          "codec preserves negative zero and scale");
    std::array<std::uint8_t, 16> raw{};
    raw[2] = 30;
    raw[3] = 0x80;
    raw[4] = 5;
    auto decoded = Decimal::deserialize(raw);
    check(decoded.scale() == 28 && decoded.coefficient()[0] == 1 && decoded.is_sign_negative(),
          "decoder scale-30 remainder rule matches Rust");
    raw[2] = 0xff;
    raw[0] = 0xff;
    check(Decimal::deserialize(raw).serialize()[0] == 0, "decoder masks reserved flags");
    check(d("12.00").is_integer() && !d("12.01").is_integer() && zero.is_integer(),
          "integer predicate ignores trailing zeros");
    check(d("1.00").min(d("1")).scale() == 2 && d("1.00").max(d("1")).scale() == 2,
          "min/max preserve self on numeric ties");
    check(Decimal::max_value() == d("79228162514264337593543950335") &&
              Decimal::min_value() == -Decimal::max_value(),
          "numeric limits");
    check(d("123.45").round_sf(3)->to_string(r) == "123" &&
              d("1.2").round_sf(4)->to_string(r) == "1.200",
          "significant figures shrink or extend precision");
    check(!Decimal::max_value().round_sf(1), "significant figure overflow returns None");
    bool overflow = false;
    try {
        (void)d("1.2").round_sf(UINT32_MAX);
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    check(overflow, "significant scale addition overflow reports source panic boundary");
    auto user = global.make_user_context();
    auto& error =
        user->make<DecimalError>(DecimalError::Code::ConversionTo,
                                 "a conversion target longer than the small-string buffer");
    auto& retained = global.make<DecimalError>(error);
    user.reset();
    check(
        retained.memory_resource() == r &&
            retained.to_string(r) ==
                "Error while converting to a conversion target longer than the small-string buffer",
        "explicit error clone survives source Context destruction");
}

int main() {
    try {
        arithmetic();
        rounding_and_order();
        scale_codec_and_errors();
        std::cout << "Decimal contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
