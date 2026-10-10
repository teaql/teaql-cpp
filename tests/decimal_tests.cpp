#include <teaql/core/decimal.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
#include <set>
#include <bit>
#include <unordered_set>
#include <ranges>
using namespace teaql;

// Actual source const-fn responsibilities must remain usable for static data.
constexpr auto constant_decimal = Decimal::from_parts({123, 0, 0}, true, 2);
static_assert(constant_decimal.has_value() && constant_decimal->scale() == 2);
static_assert(constant_decimal->is_sign_negative() && !constant_decimal->is_sign_positive());
static_assert(constant_decimal->unpack() == UnpackedDecimal{true, 2, 0, 0, 123});
static_assert(constant_decimal->mantissa() == Int128::from_i64(-123));
constexpr auto constant_bytes = constant_decimal->serialize();
static_assert(constant_bytes ==
              std::array<std::uint8_t, 16>{0, 0, 2, 128, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
static_assert(Decimal::from_parts({0, 0, 0}, true, 28)->is_zero());
static_assert(Decimal::from_parts({0, 0, 0}, true, 28)->is_sign_positive());
static_assert(!Decimal::from_parts({1, 0, 0}, false, 29));
static_assert(Decimal::max_value().unpack() ==
              UnpackedDecimal{false, 0, UINT32_MAX, UINT32_MAX, UINT32_MAX});
static_assert(Decimal::min_value().unpack() ==
              UnpackedDecimal{true, 0, UINT32_MAX, UINT32_MAX, UINT32_MAX});
static_assert(Decimal::ZERO.is_zero() && Decimal::ONE.unpack().lo == 1);

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

static void ordinal_defaults() {
    const auto left = d("1.00");
    const auto right = d("1");
    check(left.max(right).scale() == 2 && left.ord_max(right).scale() == 0,
          "inherent max and inherited Ord max preserve distinct equal-value representations");
    check(left.ord_min(right).scale() == 2,
          "inherited Ord min retains its first equal-valued argument");
    auto negative_zero = d("0.000");
    negative_zero.set_sign_negative(true);
    check(negative_zero.max(Decimal::ZERO).is_sign_negative() &&
              !negative_zero.ord_max(Decimal::ZERO).is_sign_negative(),
          "numeric zero ties preserve the selected operand sign");
    const auto minimum = d("1.000");
    const auto maximum = d("2.00");
    check(d("0.5").clamp(minimum, maximum).scale() == 3 &&
              d("3").clamp(minimum, maximum).scale() == 2 &&
              left.clamp(minimum, maximum).scale() == 2,
          "clamp retains bound representation outside and caller representation inside");
    bool rejected = false;
    try {
        (void)left.clamp(maximum, minimum);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && left.scale() == 2 && minimum.scale() == 3 && maximum.scale() == 2,
          "reversed clamp bounds fail without mutating caller or bounds");
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

static void reciprocal_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    const auto divisor = d("2");
    auto previous = divisor.serialize();
    auto inverse = divisor.inv();
    check(inverse.to_string(r) == "0.50" && inverse.scale() == 2 && divisor.serialize() == previous,
          "reciprocal preserves original division scale without changing its operand");
    check(d("-4").inv() == d("-0.25") && d("1e-28").inv() == d("1e28"),
          "reciprocal retains sign and coefficient boundaries");
    for (auto zero : {Decimal::ZERO, -d("0.000")}) {
        previous = zero.serialize();
        bool rejected = false;
        try {
            (void)zero.inv();
        } catch (const std::domain_error&) {
            rejected = true;
        }
        check(rejected && zero.serialize() == previous,
              "reciprocal rejects both zero signs without mutating its operand");
    }
}

static void identity_defaults() {
    NoDefaultAllocator guard;
    auto value = d("1.000");
    check(value.is_one() && value.scale() == 3, "one predicate compares numeric identity");
    auto* address = &value;
    value.set_zero();
    check(&value == address && value.serialize() == Decimal::ZERO.serialize() && !value.is_one(),
          "zero default replaces raw scale and sign in place");
    value = -d("0.000");
    value.set_one();
    check(&value == address && value.serialize() == Decimal::ONE.serialize() && value.is_one(),
          "one default resets signed scaled zero to canonical identity in place");
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
    auto scale29 = extended;
    scale29.rescale(29);
    auto sum = extended.checked_add(scale29);
    auto difference = extended.checked_sub(scale29);
    auto remainder29 = d("1").checked_rem(scale29);
    check(sum && sum->scale() == 29 && sum->coefficient()[0] == 20,
          "addition preserves extended result scale rather than forcing scale 28");
    check(difference && difference->scale() == 29 && difference->is_zero() &&
              !difference->is_sign_negative(),
          "extended subtraction preserves scale and canonical zero sign");
    check(remainder29 && remainder29->scale() == 29 && remainder29->is_zero(),
          "remainder preserves extended result scale even for zero");
    bool division_scale = false;
    try {
        (void)scale29.checked_div(d("1"));
    } catch (const std::domain_error&) {
        division_scale = true;
    }
    check(division_scale, "exact division preserves original checked-constructor scale failure");
    extended.rescale(56);
    bool alignment_overflow = false;
    try {
        (void)extended.checked_sub(Decimal::MIN);
    } catch (const std::domain_error&) {
        alignment_overflow = true;
    }
    check(alignment_overflow, "extended alignment preserves original intermediate-buffer failure");
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

static void exact_and_radix_parsing() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    DecimalError* retained = nullptr;
    DecimalError* retained_underflow = nullptr;
    {
        auto exact = Decimal::from_str_exact(r, "-001.2300");
        check(exact && exact.value().to_string(r) == "-1.2300",
              "exact parsing preserves coefficient, sign and scale");
        auto small = Decimal::from_str_exact(r, "0.0000000000000000000000000001");
        check(small && small.value().scale() == 28, "exact parser admits scale 28");
        auto underflow = Decimal::from_str_exact(r, "0.00000000000000000000000000010");
        check(!underflow && underflow.error().code == DecimalError::Code::Underflow,
              "exact parser rejects discarded digits even when zero");
        check(!Decimal::from_str_exact(r, "0.0000000000000000000000000001_"),
              "exact parser treats trailing separators at precision as underflow");
        check(!Decimal::from_str_exact(r, "1e2") && !Decimal::from_str_radix(r, "1e2", 10),
              "exact and radix-ten parsing do not fall back to scientific notation");
        check(Decimal::parse_result(r, "1e2").value() == d("100"),
              "ordinary parsing retains scientific fallback");
        check(Decimal::from_str_radix(r, "Ff", 16).value() == d("255"),
              "radix parser accepts either case");
        check(Decimal::from_str_radix(r, "101.101", 2).value().to_string(r) == "0.045",
              "radix coefficient retains the locked decimal-scale interpretation");
        check(!Decimal::from_str_radix(r, "2", 2) && !Decimal::from_str_radix(r, "1", 37),
              "invalid digit and unsupported radix produce structured errors");
        auto invalid = Decimal::from_str_radix(r, "1", 1);
        retained = &global.make<DecimalError>(invalid.error());
        retained_underflow = &global.make<DecimalError>(underflow.error());
    }
    user.reset();
    check(retained->memory_resource() == global.memory_resource() &&
              retained->to_string(global.memory_resource()) == "Unsupported radix < 2" &&
              retained_underflow->code == DecimalError::Code::Underflow,
          "parse errors explicitly copy to Global and survive User destruction");
    bool capacity = false, scale = false;
    try {
        (void)Decimal::from_str_radix(global.memory_resource(), std::string(97, '1'), 2);
    } catch (const std::length_error&) {
        capacity = true;
    }
    try {
        (void)Decimal::from_str_radix(global.memory_resource(), "0." + std::string(29, '1'), 2);
    } catch (const std::out_of_range&) {
        scale = true;
    }
    check(capacity && scale, "locked radix panic boundaries become checked C++ exceptions");
}

static void primitive_numeric_conversions() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto maximum = UInt128{{UINT32_MAX, UINT32_MAX, UINT32_MAX, 0}};
    auto too_large = UInt128{{0, 0, 0, 1}};
    auto signed_maximum = Int128::from_magnitude(maximum, false);
    auto signed_minimum = Int128::from_magnitude(maximum, true);
    check(Decimal::from_u128(maximum) == Decimal::max_value() && !Decimal::from_u128(too_large),
          "unsigned conversion enforces the full 96-bit coefficient range");
    check(Decimal::from_i128(signed_minimum) == Decimal::min_value() &&
              !Decimal::from_i128(Int128{{0, 0, 0, 0x80000000u}}),
          "signed conversion rejects i128 minimum without signed overflow");
    check(Decimal::max_value().to_i128() == signed_maximum &&
              Decimal::min_value().to_i128() == signed_minimum &&
              Decimal::max_value().to_u128() == maximum,
          "wide integral conversion round trips extremes");
    check(d("-12.90").to_i128() == Int128::from_i64(-12) &&
              d("-12.90").mantissa() == Int128::from_i64(-1290),
          "mantissa differs from fractional truncation");
    auto scaled = Decimal::try_from_i128_with_scale(r, signed_maximum, 28);
    check(scaled && scaled.value().scale() == 28 && scaled.value().mantissa() == signed_maximum,
          "checked scaled construction preserves all coefficient bits");
    auto overflow = Decimal::try_from_i128_with_scale(r, Int128{too_large.words}, 0);
    auto invalid_scale = Decimal::try_from_i128_with_scale(r, Int128{too_large.words}, 29);
    check(!overflow && overflow.error().code == DecimalError::Code::ExceedsMaximumPossibleValue &&
              !invalid_scale &&
              invalid_scale.error().code == DecimalError::Code::ScaleExceedsMaximumPrecision,
          "scale validation precedes coefficient validation");
    check(Decimal::from_i64_with_scale(INT64_MIN, 2).mantissa() == Int128::from_i64(INT64_MIN),
          "scaled i64 minimum avoids negation overflow");
    check(!Decimal::try_new(r, 1, UINT32_MAX) && Decimal::try_new(r, 0, 28).value().scale() == 28,
          "checked i64 construction preserves scale and rejects overflow");
    check(Decimal::from_integer(std::uint32_t(UINT32_MAX)) == d("4294967295") &&
              Decimal::from_integer(std::int8_t(-128)) == d("-128"),
          "native primitive factories preserve signedness");
    check(d("127.99").to_integer<std::int8_t>() == 127 && !d("128").to_integer<std::int8_t>() &&
              d("65535.9").to_integer<std::uint16_t>() == 65535 &&
              !d("65536").to_integer<std::uint16_t>(),
          "narrow conversions truncate before checking bounds");
    auto negative_zero = -d("0");
    check(!negative_zero.to_u128() && !negative_zero.to_integer<std::uint8_t>() &&
              negative_zero.to_integer<std::int8_t>() == 0,
          "negative zero remains rejected by unsigned conversions");
    check(d("42").to_integer<char>() == 42 && d("42").to_integer<std::intptr_t>() == 42,
          "native character and pointer-sized conversions use target ranges");
    check(Decimal::from_f32(0.1f)->to_string(r) == "0.1" &&
              Decimal::from_f32_retain(0.1f)->to_string(r) == "0.100000001490116119384765625",
          "binary32 standard and retained precision remain distinct");
    check(Decimal::from_f64_retain(0.1)->to_string(r) == "0.1000000000000000055511151231" &&
              Decimal::from_f64(0.1)->to_string(r) == "0.1",
          "binary64 retained conversion keeps locked precision");
    check(Decimal::from_f32_retain(1.0f)->scale() == 0 &&
              Decimal::from_f64_retain(1.0)->scale() == 0,
          "exact binary integers do not acquire fractional scale");
    check(Decimal::from_f32_retain(std::bit_cast<float>(std::uint32_t(725904025)))->to_string(r) ==
              "0.0000000000006978112206991460",
          "retained conversion does not normalize trailing zeros");
    check(Decimal::from_f32(-0.0f)->is_sign_negative() &&
              Decimal::from_f64_retain(-0.0)->is_sign_negative() &&
              std::bit_cast<std::uint32_t>(d("-0.00").to_f32()) == 0,
          "parsed zero normalizes sign while floating constructors retain it");
    check(!Decimal::from_f32(std::numeric_limits<float>::infinity()) &&
              !Decimal::from_f32_retain(std::numeric_limits<float>::quiet_NaN()) &&
              Decimal::from_f32(std::numeric_limits<float>::denorm_min())->is_zero(),
          "binary32 nonfinite rejection and subnormal underflow");
    bool range = false;
    try {
        (void)Int128::from_magnitude({{0, 0, 0, 0x80000000u}}, false);
    } catch (const std::out_of_range&) {
        range = true;
    }
    check(range && Int128::from_magnitude({{0, 0, 0, 0x80000000u}}, true).negative(),
          "portable signed magnitude validates its asymmetric range");
    auto bad_float = Decimal::try_from_f64(r, std::numeric_limits<double>::infinity());
    auto bad_integer = d("128").try_to_integer<std::int8_t>(r);
    auto bad_unsigned = negative_zero.try_to_u128(r);
    check(!bad_float && bad_float.error().code == DecimalError::Code::ConversionTo &&
              bad_float.error().text == "Decimal" && !bad_integer &&
              bad_integer.error().text == "i8" && !bad_unsigned &&
              bad_unsigned.error().text == "u128",
          "structured conversion failures preserve the Rust target names");
    check(negative_zero.try_to_usize(r).error().text == "usize" &&
              d("42").try_to_isize(r).value() == 42,
          "pointer-sized conversion wrappers retain their distinct error labels");
    check(bool(d("2147483648").to_integer<std::intptr_t>()) == (sizeof(std::intptr_t) == 8),
          "pointer-sized conversion checks the actual native target width");
    bool primitive_overflow = false;
    try {
        (void)Decimal::from_integer(too_large);
    } catch (const std::overflow_error&) {
        primitive_overflow = true;
    }
    check(primitive_overflow && Decimal::from_integer(signed_minimum) == Decimal::min_value(),
          "infallible wide factories preserve source overflow panics and valid values");
}

static void scientific_and_foundational_traits() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto strict = Decimal::from_scientific(r, "2.5e-28");
    auto lossy = Decimal::from_scientific_lossy(r, "2.5e-28");
    check(!strict && strict.error().code == DecimalError::Code::ScaleExceedsMaximumPrecision &&
              strict.error().scale == 29 && lossy &&
              lossy.value().to_string(r) == "0.0000000000000000000000000003",
          "strict and lossy scientific constructors preserve distinct scale behavior");
    check(Decimal::from_scientific_lossy(r, "-2.5e-28").value() == -lossy.value() &&
              Decimal::from_scientific(r, "1.00e-28").error().scale == 30,
          "lossy rounding is away from zero; strict error includes the combined scale");
    check(Decimal::from_scientific(r, "7922816251426433759354395033.5e1").value() == Decimal::MAX &&
              d("7922816251426433759354395033.5e1") == Decimal::MAX,
          "scientific positive scaling uses Decimal multiplication before normalization");
    check(Decimal::from_scientific(r, "1.2e28").value() == d("12000000000000000000000000000"),
          "positive scaling preserves the original 19-digit chunking");
    check(Decimal::from_scientific(r, "1").error().to_string(r) == "Failed to parse" &&
              Decimal::from_scientific(r, "1e4294967296").error().to_string(r) == "Failed to parse",
          "missing separator and u32 exponent overflow remain parse errors");
    auto& payload = global.make<DecimalError>(
        DecimalError::Code::ScaleExceedsMaximumPrecision, "unused string", 29);
    check(strict.error() == payload, "error equality compares only the enum's active payload");
    check(Decimal::ZERO == d("0") && Decimal::ONE == d("1") && Decimal::NEGATIVE_ONE == d("-1") &&
              Decimal::TWO == d("2") && Decimal::TEN == d("10") &&
              Decimal::ONE_HUNDRED == d("100") && Decimal::ONE_THOUSAND == d("1000") &&
              Decimal::MIN == Decimal::min_value() && Decimal::MAX == Decimal::max_value(),
          "the baseline's active constants retain exact scalar values");
    auto negative_zero = -d("0.00");
    check(negative_zero.signum() == Decimal::ZERO && !negative_zero.signum().is_sign_negative() &&
              negative_zero.is_negative() && !negative_zero.is_positive() &&
              d("-12.3").signum() == Decimal::NEGATIVE_ONE,
          "Signed signum and sign predicates differ for zero");
    check(d("1.00").abs_sub(d("2")) == Decimal::ZERO && d("3.5").abs_sub(d("1.25")) == d("2.25"),
          "Signed abs_sub returns zero for a nonpositive difference");
    auto unpacked = negative_zero.unpack();
    check(unpacked.negative && unpacked.scale == 2 &&
              !Decimal::from_unpacked(unpacked).is_sign_negative(),
          "unpacking retains negative zero while repacking normalizes it");
    auto extended = d("0.0000000000000000000000000001");
    extended.rescale(56);
    bool invalid_scale = false;
    try {
        (void)Decimal::from_unpacked(extended.unpack());
    } catch (const std::out_of_range&) {
        invalid_scale = true;
    }
    check(invalid_scale, "extended unpacking retains scale, but from_parts cannot repack it");
    std::pmr::unordered_set<Decimal> values(r);
    values.insert(d("1"));
    values.insert(d("1.00"));
    values.insert(negative_zero);
    values.insert(d("0"));
    values.insert(extended);
    values.insert(d("1e-28"));
    check(values.size() == 3 && negative_zero.hash_words() == Decimal::ZERO.hash_words() &&
              d("-1.20").hash_words() == d("-1.2000").hash_words(),
          "normalized hash input agrees with numeric equality across signs and scales");
    std::array<Decimal, 0> empty{};
    std::array factors{d("1.20"), d("2"), d("3")};
    check(Decimal::sum(empty) == Decimal::ZERO && Decimal::product(empty) == Decimal::ONE &&
              Decimal::sum(factors) == d("6.2") && Decimal::product(factors) == d("7.2"),
          "folds use their distinct identities and preserve source order");
    auto generated = std::views::iota(1, 4) | std::views::transform([](int n) {
                         return Decimal::from_i64(n);
                     });
    check(Decimal::sum(generated) == d("6") && Decimal::product(generated) == d("6"),
          "numeric folds accept lazily generated input ranges");
    bool overflow = false;
    std::array overflowing{Decimal::MAX, Decimal::ONE, Decimal::NEGATIVE_ONE};
    try {
        (void)Decimal::sum(overflowing);
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    check(overflow, "sum reports intermediate overflow before any later cancellation");
}

static void json_and_numeric_formatting() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context();
    DecimalJsonError* retained_error = nullptr;
    Json* retained_json = nullptr;
    {
        auto r = user->memory_resource();
        auto numeric = Json(r, 0.1);
        auto decoded = Decimal::from_json_value_result(r, numeric);
        check(decoded && decoded.value().to_string(r) == "0.1",
              "default serde parses a float's Display text");
        auto signed_zero = Json(r, -0.0);
        check(!Decimal::from_json_value(signed_zero)->is_sign_negative() &&
                  Decimal::from_f64(-0.0)->is_sign_negative(),
              "serde float decoding and binary float conversion differ for zero sign");
        auto text = Json(r, "12.3400");
        auto amount = *Decimal::from_json_value(text);
        auto output = amount.to_json_value(r);
        check(std::get<String>(output.data) == "12.3400" && output.memory_resource() == r,
              "default serde emits an explicitly scoped string preserving scale");
        retained_json = &global.make<Json>(output);
        auto invalid = Json(r, "invalid decimal text longer than the small-string buffer");
        auto error = Decimal::from_json_value_result(r, invalid);
        check(!error && error.error().code == DecimalJsonError::Code::InvalidValue &&
                  error.error().message.starts_with("invalid value: string "),
              "invalid strings retain the serde error category and diagnostic");
        retained_error = &global.make<DecimalJsonError>(error.error());
        auto wrong_type = Decimal::from_json_value_result(r, Json(r, true));
        check(!wrong_type && wrong_type.error().code == DecimalJsonError::Code::InvalidType,
              "bool is not a Decimal serde number");
        DecimalFormat options;
        options.precision = 2;
        check(d("1.239").format(r, options) == "1.23",
              "Display precision truncates rather than rounds");
        options.notation = DecimalNotation::LowerExp;
        check(d("12.3000").format(r, options) == "1.23e1" &&
                  d("0.1").format(r, options) == "1.00e-1",
              "scientific precision covers multi-digit and single-digit coefficients");
        options.precision.reset();
        check(d("12.3000").format(r, options) == "1.23000e1",
              "default scientific formatting keeps significant trailing coefficient zeros");
        options.notation = DecimalNotation::Display;
        options.width = 10;
        options.sign_plus = true;
        options.zero_pad = true;
        check(d("12.30").format(r, options) == "+000012.30", "sign precedes zero padding");
        options.zero_pad = false;
        options.width = 8;
        options.fill = U'\U0001f980';
        options.alignment = DecimalAlignment::Center;
        check(d("1.20").format(r, options) == "🦀+1.20🦀🦀",
              "Unicode padding uses scalar width and odd center padding goes right");
        options.notation = DecimalNotation::UpperExp;
        check((-d("0.00")).format(r, options) == "0e0",
              "scientific zero retains source's flag-independent lowercase spelling");
        // A nonzero value can retain the extended scale needed for this edge.
        auto extended = d("0.0000000000000000000000000001");
        extended.rescale(30);
        extended = -extended;
        check(extended.to_string(r).size() == 33,
              "Display's sign is outside its fixed string buffer");
        bool serialize_capacity = false;
        try {
            (void)extended.to_json_value(r);
        } catch (const std::length_error&) {
            serialize_capacity = true;
        }
        check(serialize_capacity, "serde's signed fixed string capacity remains distinct");
        options.fill = char32_t(0xd800);
        bool invalid_fill = false;
        try {
            (void)amount.format(r, options);
        } catch (const std::invalid_argument&) {
            invalid_fill = true;
        }
        check(invalid_fill, "a format fill must be a valid Unicode scalar");
    }
    user.reset();
    check(retained_error->memory_resource() == global.memory_resource() &&
              retained_error->message.find("longer than the small-string buffer") != String::npos &&
              std::get<String>(retained_json->data) == "12.3400",
          "explicit Global copies retain JSON and diagnostics after User destruction");
}

int main() {
    try {
        arithmetic();
        ordinal_defaults();
        reciprocal_contracts();
        identity_defaults();
        rounding_and_order();
        scale_codec_and_errors();
        exact_and_radix_parsing();
        primitive_numeric_conversions();
        scientific_and_foundational_traits();
        json_and_numeric_formatting();
        std::cout << "Decimal contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
