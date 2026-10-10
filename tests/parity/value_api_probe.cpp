#include <teaql/runtime/context.hpp>
#include <teaql/core/debug.hpp>
#include "fixture_value.hpp"
#include <bit>
#include <cmath>
#include <iostream>

template <class T>
static T hex_number(const Json& input) {
    T result{};
    auto source = text(input);
    auto [end, error] = std::from_chars(source.data(), source.data() + source.size(), result, 16);
    if (error != std::errc{} || end != source.data() + source.size()) {
        throw std::runtime_error("Invalid float bits");
    }
    return result;
}

static Value make(Memory owner, const Json& input) {
    auto kind = text(field(input, "kind"));
    auto data = optional(input, "value");
    if (kind == "from_i16") {
        return Value(owner, number<std::int16_t>(*data));
    }
    if (kind == "from_i32") {
        return Value(owner, number<std::int32_t>(*data));
    }
    if (kind == "from_i64") {
        return Value(owner, number<std::int64_t>(*data));
    }
    if (kind == "from_u16") {
        return Value(owner, number<std::uint16_t>(*data));
    }
    if (kind == "from_u32") {
        return Value(owner, number<std::uint32_t>(*data));
    }
    if (kind == "from_u64") {
        return Value(owner, number<std::uint64_t>(*data));
    }
    if (kind == "from_f32_bits") {
        return Value(owner, std::bit_cast<float>(hex_number<std::uint32_t>(*data)));
    }
    if (kind == "from_f64_bits") {
        return Value(owner, std::bit_cast<double>(hex_number<std::uint64_t>(*data)));
    }
    if (kind == "from_borrowed_text") {
        return Value(owner, text(*data));
    }
    if (kind == "from_owned_text") {
        return Value(owner, String(text(*data), owner));
    }
    if (kind == "from_json") {
        return Value(owner, Json(owner, *data));
    }
    if (kind == "from_bool") {
        return Value(owner, std::get<bool>(data->data));
    }
    if (kind == "from_decimal") {
        return Value(owner, *Decimal::parse(text(*data)));
    }
    if (kind == "from_date") {
        return Value(owner, *Date::parse(text(*data)));
    }
    if (kind == "from_timestamp") {
        return Value(owner, Timestamp(number<std::int64_t>(*data)));
    }
    if (kind == "typed_null") {
        return Value(owner,
                     TypedNull{DataType(std::get<std::uint64_t>(field(input, "type").data))});
    }
    return value(owner, input);
}

static Json evaluate(Memory owner, const Json& input) {
    auto value = make(owner, field(input, "value"));
    Json::Object output(owner);
    Json::Array equalities(owner);
    for (const auto& other : std::get<Json::Array>(field(input, "others").data)) {
        equalities.emplace_back(value == make(owner, other));
    }
    auto put_integer = [&](std::string_view name, auto optional) {
        output.emplace(name, optional ? Json(owner, std::to_string(*optional)) : Json(owner));
    };
    put_integer("i64", value.try_i64());
    put_integer("u64", value.try_u64());
    auto decimal = value.try_decimal();
    output.emplace("decimal", decimal ? Json(owner, decimal->to_string(owner)) : Json(owner));
    auto floating = value.try_f64();
    Json bits(owner);
    if (floating && text(field(field(input, "value"), "kind")) == "from_f32_bits" &&
        std::isnan(*floating)) {
        bits = Json(owner, "NaN");
    } else if (floating) {
        auto integer = std::bit_cast<std::uint64_t>(*floating);
        char buffer[16];
        auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), integer, 16);
        if (error != std::errc{}) {
            throw std::runtime_error("Cannot format float bits");
        }
        String padded(16 - (end - buffer), '0', owner);
        padded.append(buffer, end);
        bits = Json(owner, padded);
    }
    output.emplace("f64_bits", std::move(bits));
    auto boolean = value.try_bool();
    auto text = value.try_text();
    auto date = value.try_date();
    auto timestamp = value.try_timestamp();
    output.emplace("bool", boolean ? Json(owner, *boolean) : Json(owner));
    output.emplace("text", text ? Json(owner, *text) : Json(owner));
    output.emplace("date", date ? Json(owner, date->to_string(owner)) : Json(owner));
    put_integer("timestamp", timestamp ? std::optional(timestamp->as_millis()) : std::nullopt);
    output.emplace("debug", Json(owner, debug_string(owner, value)));
    output.emplace("json", value.to_json_value(owner));
    output.emplace("self_equal", Json(owner, value == value));
    output.emplace("clone_equal", Json(owner, value == Value(owner, value)));
    output.emplace("equalities", Json(owner, std::move(equalities)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid Value API fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
