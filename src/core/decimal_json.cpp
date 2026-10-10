#include <teaql/core/decimal.hpp>
#include <teaql/core/json.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <cmath>

namespace teaql {
namespace {
std::optional<Decimal> decode(const Json& value, Memory r) {
    if (auto* integer = std::get_if<std::int64_t>(&value.data)) {
        return Decimal::from_i64(*integer);
    }
    if (auto* integer = std::get_if<std::uint64_t>(&value.data)) {
        return Decimal::from_u64(*integer);
    }
    if (auto* text = std::get_if<String>(&value.data)) {
        auto result = Decimal::parse(*text);
        if (result) {
            return result;
        }
        auto scientific = Decimal::from_scientific(r, *text);
        return scientific ? std::optional(scientific.value()) : std::nullopt;
    }
    if (auto* floating = std::get_if<double>(&value.data)) {
        String text(r);
        detail::append_float(text, *floating, false);
        return Decimal::parse(text);
    }
    return {};
}
}

Json Decimal::to_json_value(Memory r) const {
    auto text = to_string(r);
    // Default serde uses a signed ArrayString<32>, unlike Display's external sign.
    if (text.size() > 32) {
        throw std::length_error("Decimal JSON string exceeds the locked Rust serializer capacity");
    }
    return Json(r, text);
}

std::optional<Decimal> Decimal::from_json_value(const Json& value) {
    return decode(value, value.memory_resource());
}

Result<Decimal, DecimalJsonError> Decimal::from_json_value_result(Memory r, const Json& value) {
    if (auto result = decode(value, r)) {
        return Result<Decimal, DecimalJsonError>::ok(r, *result);
    }
    using Code = DecimalJsonError::Code;
    auto code =
        std::holds_alternative<String>(value.data) || std::holds_alternative<double>(value.data)
            ? Code::InvalidValue
            : Code::InvalidType;
    String message(code == Code::InvalidValue ? "invalid value: " : "invalid type: ", r);
    if (auto* text = std::get_if<String>(&value.data)) {
        message += "string ";
        message += detail::debug_text(r, *text);
    } else if (auto* floating = std::get_if<double>(&value.data)) {
        message += "floating point `";
        String text(r);
        // serde_json overrides serde's generic float diagnostic with zmij text.
        detail::append_float(text, *floating, std::isfinite(*floating));
        message += text;
        message += '`';
    } else if (auto* boolean = std::get_if<bool>(&value.data)) {
        message += *boolean ? "boolean `true`" : "boolean `false`";
    } else if (std::holds_alternative<Json::Array>(value.data)) {
        message += "sequence";
    } else if (std::holds_alternative<Json::Object>(value.data)) {
        message += "map";
    } else {
        message += "null";
    }
    message += ", expected a Decimal type representing a fixed-point number";
    return Result<Decimal, DecimalJsonError>::err(
        r, DecimalJsonError(std::allocator_arg, Allocator(r), code, message));
}
}
