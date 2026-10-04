#pragma once
#include <teaql/core/decimal.hpp>
#include <teaql/core/json.hpp>
#include <teaql/core/time.hpp>
#include <concepts>
#include <cstdint>
#include <map>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace teaql {
enum class DataType { Bool, I64, U64, F64, Decimal, Text, LargeText, Json, Date, Timestamp };
struct TypedNull { DataType type; bool operator==(const TypedNull&) const = default; };
// Dynamic values always carry an explicit allocation domain.
struct Value {
    using allocator_type = Allocator;
    using List = Vector<Value>;
    using Object = std::pmr::map<String, Value, std::less<>>;
    using Storage = std::variant<std::monostate, bool, std::int64_t, std::uint64_t, double, String, TypedNull, List, Object, Decimal, Json, Date, Timestamp>;
    Storage data;
    explicit Value(Memory r) : resource_(r) {}
    Value(Memory r, std::nullptr_t) : Value(r) {}
    Value(Memory r, bool v) : data(v), resource_(r) {}
    template<std::signed_integral T> Value(Memory r, T v) : data(std::int64_t(v)), resource_(r) {}
    template<std::unsigned_integral T> requires (!std::same_as<T, bool>)
    Value(Memory r, T v) : data(std::uint64_t(v)), resource_(r) {}
    Value(Memory r, double v) : data(v), resource_(r) {}
    Value(Memory r, std::string_view v) : data(String(v, r)), resource_(r) {}
    Value(Memory r, const char* v) : Value(r, std::string_view(v)) {}
    Value(Memory r, const String& v) : Value(r, std::string_view(v)) {}
    Value(Memory r, TypedNull v) : data(v), resource_(r) {}
    Value(Memory r, Decimal v) : data(v), resource_(r) {}
    Value(Memory r, Date v) : data(v), resource_(r) {}
    Value(Memory r, Timestamp v) : data(v), resource_(r) {}
    Value(Memory r, const Json& v) : data(Json(r, v)), resource_(r) {}
    Value(Memory r, Json&& v) : data(Json(r, std::move(v))), resource_(r) {}
    Value(Memory, const List&);
    Value(Memory, const Object&);
    Value(Memory, const Value&);
    Value(Memory, Value&&);
    template<class... Args> Value(std::allocator_arg_t, Allocator a, Args&&... args)
        : Value(a.resource(), std::forward<Args>(args)...) {}
    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;
    Value(Value&&) noexcept = default;
    Value& operator=(Value&&);
    bool is_null() const { return std::holds_alternative<std::monostate>(data) || std::holds_alternative<TypedNull>(data); }
    std::optional<std::int64_t> try_i64() const;
    std::optional<std::uint64_t> try_u64() const;
    std::optional<double> try_f64() const;
    std::optional<Decimal> try_decimal() const;
    std::optional<Date> try_date() const;
    std::optional<Timestamp> try_timestamp() const;
    Json to_json_value(Memory destination) const;
    std::optional<bool> try_bool() const;
    std::optional<std::string_view> try_text() const;
    const Object* as_object() const { return std::get_if<Object>(&data); }
    const List* as_list() const { return std::get_if<List>(&data); }
    Memory memory_resource() const { return resource_; }
    bool operator==(const Value& other) const { return data == other.data; }
private:
    Memory resource_;
};
using Record = Value::Object;
Record clone_record(Memory, const Record&);
Json record_to_json_value(Memory destination, const Record&);
}
