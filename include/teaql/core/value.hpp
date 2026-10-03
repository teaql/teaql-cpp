#pragma once
#include <concepts>
#include <cstdint>
#include <map>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace teaql {
using Memory = std::pmr::memory_resource*;
using String = std::pmr::string;
template<class T> using Vector = std::pmr::vector<T>;
using Allocator = std::pmr::polymorphic_allocator<std::byte>;
enum class DataType { Bool, I64, U64, F64, Decimal, Text, LargeText, Json, Date, Timestamp };
struct TypedNull { DataType type; bool operator==(const TypedNull&) const = default; };
// Exact scalar subset. Dynamic values always carry an explicit allocation domain.
struct Value {
    using allocator_type = Allocator;
    using Storage = std::variant<std::monostate, bool, std::int64_t, std::uint64_t, double, String, TypedNull>;
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
    Value(Memory r, const Value& v) : Value(r) {
        std::visit([&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::same_as<T, String>) data = String(item, r);
            else data = item;
        }, v.data);
    }
    template<class... Args> Value(std::allocator_arg_t, Allocator a, Args&&... args)
        : Value(a.resource(), std::forward<Args>(args)...) {}
    Value(const Value&) = delete;
    Value& operator=(const Value&) = delete;
    Value(Value&&) noexcept = default;
    Value& operator=(Value&& v) { if (this != &v) { Value copy(resource_, v); data = std::move(copy.data); } return *this; }
    bool is_null() const { return std::holds_alternative<std::monostate>(data) || std::holds_alternative<TypedNull>(data); }
    std::optional<std::int64_t> try_i64() const;
    std::optional<std::uint64_t> try_u64() const;
    Memory memory_resource() const { return resource_; }
    bool operator==(const Value& other) const { return data == other.data; }
private:
    Memory resource_;
};
using Record = std::pmr::map<String, Value, std::less<>>;
// These wrappers have separate identities even though their contents share a representation.
struct MutationValues {
    Record fields;
    explicit MutationValues(Memory r) : fields(r) {}
};
struct EntitySnapshot { Record fields; explicit EntitySnapshot(Memory r) : fields(r) {} };
struct GeneratedValues { Record fields; explicit GeneratedValues(Memory r) : fields(r) {} };
}
