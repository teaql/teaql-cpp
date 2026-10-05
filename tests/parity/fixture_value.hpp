#pragma once
#include <teaql/core/value.hpp>
#include <charconv>
#include <stdexcept>
using namespace teaql;

static const Json& field(const Json& input, std::string_view key) {
    return std::get<Json::Object>(input.data).find(key)->second;
}

static const Json* optional(const Json& input, std::string_view key) {
    auto& object = std::get<Json::Object>(input.data);
    auto p = object.find(key);
    return p == object.end() ? nullptr : &p->second;
}

static std::string_view text(const Json& input) {
    return std::get<String>(input.data);
}

template <class T>
static T number(const Json& input) {
    auto s = text(input);
    T result{};
    auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), result);
    if (error != std::errc{} || end != s.data() + s.size())
        throw std::runtime_error("invalid number");
    return result;
}

static Value value(Memory r, const Json& input) {
    auto kind = text(field(input, "kind"));
    auto* data = optional(input, "value");
    if (kind == "null")
        return Value(r);
    if (kind == "typed_null")
        return Value(r, TypedNull{DataType::Text});
    if (kind == "text")
        return Value(r, text(*data));
    if (kind == "bool")
        return Value(r, std::get<bool>(data->data));
    if (kind == "i64")
        return Value(r, number<std::int64_t>(*data));
    if (kind == "u64")
        return Value(r, number<std::uint64_t>(*data));
    if (kind == "f64")
        return Value(r, number<double>(*data));
    if (kind == "decimal")
        return Value(r, *Decimal::parse(text(*data)));
    if (kind == "date")
        return Value(r, *Date::parse(text(*data)));
    if (kind == "timestamp")
        return Value(r, Timestamp(number<std::int64_t>(*data)));
    if (kind == "json")
        return Value(r, *data);
    if (kind == "list") {
        Value::List out(r);
        for (const auto& item : std::get<Json::Array>(data->data))
            out.emplace_back(value(r, item));
        return Value(r, out);
    }
    if (kind == "object") {
        Value::Object out(r);
        for (const auto& [key, item] : std::get<Json::Object>(data->data))
            out.emplace(String(key, r), value(r, item));
        return Value(r, out);
    }
    throw std::runtime_error("unknown fixture value");
}
