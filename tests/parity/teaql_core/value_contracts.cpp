// Exact scalar conversion fixtures from the pinned Value module.
#include <teaql/core/value.hpp>
#include <teaql/memory/arena.hpp>
#include <chrono>
#include <iostream>
#include <limits>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void value_try_i64_accepts_representable_numeric_variants(Memory owner) {
    check(Value(owner, INT64_MIN).try_i64() == std::optional<std::int64_t>(INT64_MIN) &&
              Value(owner, INT64_MAX).try_i64() == std::optional<std::int64_t>(INT64_MAX) &&
              Value(owner, std::uint64_t(INT64_MAX)).try_i64() ==
                  std::optional<std::int64_t>(INT64_MAX) &&
              Value(owner, Decimal::from_i64(-42)).try_i64() == std::optional<std::int64_t>(-42),
          "original representable signed conversions");
}

static void value_try_i64_rejects_unsigned_overflow_and_unrelated_variants(Memory owner) {
    check(!Value(owner, std::uint64_t(INT64_MAX) + 1).try_i64() &&
              !Value(owner, UINT64_MAX).try_i64() && !Value(owner, 42.0).try_i64() &&
              !Value(owner, "42").try_i64() && !Value(owner).try_i64(),
          "all original signed conversion failures");
}

static void value_try_u64_accepts_representable_numeric_variants(Memory owner) {
    check(Value(owner, std::uint64_t(0)).try_u64() == std::optional<std::uint64_t>(0) &&
              Value(owner, UINT64_MAX).try_u64() == std::optional<std::uint64_t>(UINT64_MAX) &&
              Value(owner, INT64_MAX).try_u64() == std::optional<std::uint64_t>(INT64_MAX) &&
              Value(owner, Decimal::from_i64(42)).try_u64() == std::optional<std::uint64_t>(42),
          "original representable unsigned conversions");
}

static void value_try_u64_rejects_negative_and_unrelated_variants(Memory owner) {
    check(!Value(owner, std::int64_t(-1)).try_u64() &&
              !Value(owner, Decimal::from_i64(-1)).try_u64() && !Value(owner, 42.0).try_u64() &&
              !Value(owner, "42").try_u64() && !Value(owner).try_u64(),
          "all original unsigned conversion failures");
}

static void value_try_decimal_accepts_decimal_numeric_and_text_variants(Memory owner) {
    auto decimal = Decimal::parse("123.450");
    check(decimal.has_value(), "original decimal fixture");
    check(Value(owner, *decimal).try_decimal() == decimal &&
              Value(owner, INT64_MIN).try_decimal() ==
                  std::optional<Decimal>(Decimal::from_i64(INT64_MIN)) &&
              Value(owner, UINT64_MAX).try_decimal() ==
                  std::optional<Decimal>(Decimal::from_u64(UINT64_MAX)) &&
              Value(owner, "123.450").try_decimal() == decimal &&
              Value(owner, 136.25).try_decimal() == Decimal::from_f64_retain(136.25),
          "original decimal, integer, text and representable float conversions");
}

static void value_try_decimal_rejects_invalid_text_and_unrelated_variants(Memory owner) {
    check(!Value(owner, "not-a-decimal").try_decimal() && !Value(owner, true).try_decimal() &&
              !Value(owner, std::numeric_limits<double>::quiet_NaN()).try_decimal() &&
              !Value(owner).try_decimal(),
          "all original decimal conversion failures");
}

static void value_try_decimal_uses_human_decimal_for_f64(Memory owner) {
    auto expected = Decimal::parse("129.95");
    check(expected && Value(owner, 129.95).try_decimal() == expected,
          "original human decimal avoids binary floating expansion");
}

static void value_try_f64_accepts_supported_numeric_variants(Memory owner) {
    check(Value(owner, 1.25).try_f64() == std::optional<double>(1.25) &&
              Value(owner, std::int64_t(-2)).try_f64() == std::optional<double>(-2.0) &&
              Value(owner, std::uint64_t(2)).try_f64() == std::optional<double>(2.0) &&
              Value(owner, *Decimal::parse("1.5")).try_f64() == std::optional<double>(1.5),
          "original supported floating conversions");
}

static void value_try_f64_rejects_unrelated_variants(Memory owner) {
    check(!Value(owner, "1.5").try_f64() && !Value(owner, true).try_f64() &&
              !Value(owner).try_f64(),
          "all original floating conversion failures");
}

static std::int64_t utc_millis(int year,
                               unsigned month,
                               unsigned day,
                               unsigned hour = 0,
                               unsigned minute = 0,
                               unsigned second = 0) {
    auto instant = std::chrono::sys_days(std::chrono::year(year) / month / day) +
                   std::chrono::hours(hour) + std::chrono::minutes(minute) +
                   std::chrono::seconds(second);
    return std::chrono::duration_cast<std::chrono::milliseconds>(instant.time_since_epoch())
        .count();
}

static void value_try_date_accepts_date_and_iso_date_text(Memory owner) {
    auto leap_day = Date::from_ymd(2024, 2, 29);
    check(leap_day.has_value(), "original leap-day fixture");
    auto millis = utc_millis(2024, 2, 29);
    check(Value(owner, *leap_day).try_date() == leap_day &&
              Value(owner, "2024-02-29").try_date() == leap_day &&
              Value(owner, millis).try_date() == leap_day &&
              Value(owner, std::uint64_t(millis)).try_date() == leap_day,
          "original Date, ISO date text and signed/unsigned epoch conversions");
}

static void value_try_date_rejects_invalid_dates_and_unrelated_variants(Memory owner) {
    check(!Value(owner, "2023-02-29").try_date() &&
              !Value(owner, "2024-02-29T00:00:00Z").try_date() && !Value(owner).try_date(),
          "all original date conversion failures");
}

static void value_try_timestamp_accepts_timestamp_and_supported_text_formats(Memory owner) {
    auto millis = utc_millis(2024, 1, 2, 3, 4, 5);
    auto timestamp = std::optional<Timestamp>(Timestamp(millis));
    auto offset = std::optional<Timestamp>(Timestamp(millis - 8 * 60 * 60 * 1000));
    auto midnight = std::optional<Timestamp>(Timestamp(utc_millis(2024, 1, 2)));
    check(Value(owner, *timestamp).try_timestamp() == timestamp &&
              Value(owner, "2024-01-02T03:04:05+08:00").try_timestamp() == offset &&
              Value(owner, "2024-01-02 03:04:05").try_timestamp() == timestamp &&
              Value(owner, "2024-01-02").try_timestamp() == midnight &&
              Value(owner, millis).try_timestamp() == timestamp &&
              Value(owner, std::uint64_t(millis)).try_timestamp() == timestamp,
          "all original timestamp, offset, naive/date text and integer conversions");
}

static void value_try_timestamp_normalizes_offsets_and_rejects_invalid_input(Memory owner) {
    auto expected = std::optional<Timestamp>(Timestamp(utc_millis(2024, 1, 1, 19, 4, 5)));
    check(Value(owner, "2024-01-02T03:04:05+08:00").try_timestamp() == expected &&
              !Value(owner, "2024-13-40 25:61:61").try_timestamp() &&
              !Value(owner, true).try_timestamp() && !Value(owner).try_timestamp(),
          "original offset normalization and invalid timestamp inputs");
}

static void typed_null_keeps_type_without_numeric_or_calendar_conversion(Memory owner) {
    for (auto type : {DataType::Bool,
                      DataType::I64,
                      DataType::U64,
                      DataType::F64,
                      DataType::Decimal,
                      DataType::Text,
                      DataType::LargeText,
                      DataType::Json,
                      DataType::Date,
                      DataType::Timestamp}) {
        Value value(owner, TypedNull{type});
        check(value.is_null() && std::get<TypedNull>(value.data).type == type && !value.try_i64() &&
                  !value.try_u64() && !value.try_decimal() && !value.try_f64() &&
                  !value.try_text() && !value.try_bool() && !value.try_date() &&
                  !value.try_timestamp() && value.to_json_value(owner) == Json(owner),
              "all typed-null variants retain type and project null without inventing a scalar");
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        {
            Arena arena(4096, std::pmr::new_delete_resource());
            Memory owner = &arena;
            value_try_i64_accepts_representable_numeric_variants(owner);
            value_try_i64_rejects_unsigned_overflow_and_unrelated_variants(owner);
            value_try_u64_accepts_representable_numeric_variants(owner);
            value_try_u64_rejects_negative_and_unrelated_variants(owner);
            value_try_decimal_accepts_decimal_numeric_and_text_variants(owner);
            value_try_decimal_rejects_invalid_text_and_unrelated_variants(owner);
            value_try_decimal_uses_human_decimal_for_f64(owner);
            value_try_f64_accepts_supported_numeric_variants(owner);
            value_try_f64_rejects_unrelated_variants(owner);
            value_try_date_accepts_date_and_iso_date_text(owner);
            value_try_date_rejects_invalid_dates_and_unrelated_variants(owner);
            value_try_timestamp_accepts_timestamp_and_supported_text_formats(owner);
            value_try_timestamp_normalizes_offsets_and_rejects_invalid_input(owner);
            typed_null_keeps_type_without_numeric_or_calendar_conversion(owner);
        }
        std::pmr::set_default_resource(previous);
        std::cout << "Original thirteen Value contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
