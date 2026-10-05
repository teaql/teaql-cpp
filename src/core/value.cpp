#include <teaql/core/value.hpp>
#include <teaql/memory/scoped.hpp>

namespace teaql {
Value::Value(Memory r, const List& values) : Value(r) {
    List copy(r);
    copy.reserve(values.size());
    for (const auto& value : values)
        copy.emplace_back(value);
    data = std::move(copy);
}

Value::Value(Memory r, const Object& values) : Value(r) {
    data = clone_record(r, values);
}

Value::Value(Memory r, const Value& value) : Value(r) {
    std::visit(
        [&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::same_as<T, String>)
                data = String(item, r);
            else if constexpr (std::same_as<T, Json>)
                data = Json(r, item);
            else if constexpr (std::same_as<T, List>) {
                List copy(r);
                copy.reserve(item.size());
                for (const auto& nested : item)
                    copy.emplace_back(nested);
                data = std::move(copy);
            } else if constexpr (std::same_as<T, Object>)
                data = clone_record(r, item);
            else
                data = item;
        },
        value.data);
}

Value::Value(Memory r, Value&& other) : Value(r) {
    if (r == other.memory_resource())
        data = std::move(other.data);
    else {
        Value copy(r, static_cast<const Value&>(other));
        data = std::move(copy.data);
    }
}

Value& Value::operator=(Value&& other) {
    return detail::assign_scoped(*this, std::move(other));
}

std::optional<double> Value::try_f64() const {
    if (const auto* v = std::get_if<double>(&data))
        return *v;
    if (const auto* v = std::get_if<std::int64_t>(&data))
        return static_cast<double>(*v);
    if (const auto* v = std::get_if<std::uint64_t>(&data))
        return static_cast<double>(*v);
    if (const auto* v = std::get_if<Decimal>(&data))
        return v->to_f64();
    return {};
}

std::optional<bool> Value::try_bool() const {
    if (const auto* v = std::get_if<bool>(&data))
        return *v;
    return {};
}

std::optional<std::string_view> Value::try_text() const {
    if (const auto* v = std::get_if<String>(&data))
        return *v;
    return {};
}

std::optional<Decimal> Value::try_decimal() const {
    if (const auto* v = std::get_if<Decimal>(&data))
        return *v;
    if (const auto* v = std::get_if<std::int64_t>(&data))
        return Decimal::from_i64(*v);
    if (const auto* v = std::get_if<std::uint64_t>(&data))
        return Decimal::from_u64(*v);
    if (const auto* v = std::get_if<double>(&data))
        return Decimal::from_f64(*v);
    if (const auto* v = std::get_if<String>(&data))
        return Decimal::parse(*v);
    return {};
}

std::optional<Date> Value::try_date() const {
    if (const auto* v = std::get_if<Date>(&data))
        return *v;
    if (const auto* v = std::get_if<String>(&data))
        return Date::parse(*v);
    // Timestamp and Decimal are deliberately not accepted by Rust's try_date.
    std::optional<std::int64_t> millis;
    if (const auto* v = std::get_if<std::int64_t>(&data))
        millis = *v;
    if (const auto* v = std::get_if<std::uint64_t>(&data); v && *v <= std::uint64_t(INT64_MAX))
        millis = std::int64_t(*v);
    if (!millis)
        return {};
    return Date::from_epoch_days(*millis / 86400000 - (*millis % 86400000 < 0));
}

std::optional<Timestamp> Value::try_timestamp() const {
    if (const auto* v = std::get_if<Timestamp>(&data))
        return *v;
    if (const auto* v = std::get_if<String>(&data))
        return Timestamp::parse(*v);
    if (const auto* v = std::get_if<std::int64_t>(&data))
        return Timestamp(*v);
    if (const auto* v = std::get_if<std::uint64_t>(&data); v && *v <= std::uint64_t(INT64_MAX))
        return Timestamp(std::int64_t(*v));
    return {};
}

Json Value::to_json_value(Memory r) const {
    return std::visit(
        [&](const auto& v) -> Json {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, std::monostate> || std::same_as<T, TypedNull>)
                return Json(r);
            else if constexpr (std::same_as<T, Decimal> || std::same_as<T, Date>)
                return Json(r, v.to_string(r));
            else if constexpr (std::same_as<T, Timestamp>)
                return Json(r, v.as_millis());
            else if constexpr (std::same_as<T, List>) {
                Json result(r);
                Json::Array array(r);
                array.reserve(v.size());
                for (const auto& item : v)
                    array.emplace_back(item.to_json_value(r));
                result.data = std::move(array);
                return result;
            } else if constexpr (std::same_as<T, Object>) {
                Json result(r);
                Json::Object object(r);
                for (const auto& [key, item] : v)
                    object.emplace(key, item.to_json_value(r));
                result.data = std::move(object);
                return result;
            } else
                return Json(r, v);
        },
        data);
}
}
