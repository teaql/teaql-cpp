#pragma once
#include <teaql/memory/types.hpp>
#include <teaql/core/result.hpp>
#include <concepts>
#include <map>
#include <optional>
#include <string_view>
#include <variant>

namespace teaql {
struct JsonParseError {
    // Message views refer to static parser diagnostics, never input memory.
    std::string_view message;
    std::size_t line = 0, column = 0;
    String to_string(Memory) const;
};

// Distinct from Value's Object/List. JSON numbers retain integer/float identity.
struct Json {
    using allocator_type = Allocator;
    using Array = Vector<Json>;
    using Object = std::pmr::map<String, Json, std::less<>>;
    using Storage = std::
        variant<std::monostate, bool, std::int64_t, std::uint64_t, double, String, Array, Object>;
    Storage data;

    explicit Json(Memory r) : resource_(r) {}

    Json(Memory r, bool v) : data(v), resource_(r) {}

    template <std::signed_integral T>
    Json(Memory r, T v) : resource_(r) {
        if (v < 0)
            data = std::int64_t(v);
        else
            data = std::uint64_t(v);
    }

    template <std::unsigned_integral T>
        requires(!std::same_as<T, bool>)
    Json(Memory r, T v) : data(std::uint64_t(v)), resource_(r) {}

    Json(Memory, double);

    Json(Memory r, std::string_view v) : data(String(v, r)), resource_(r) {}

    Json(Memory r, const char* v) : Json(r, std::string_view(v)) {}

    Json(Memory r, const String& v) : Json(r, std::string_view(v)) {}

    Json(Memory, const Array&);
    Json(Memory, const Object&);
    Json(Memory, const Json&);
    Json(Memory, Json&&);

    template <class... Args>
    Json(std::allocator_arg_t, Allocator a, Args&&... args)
        : Json(a.resource(), std::forward<Args>(args)...) {}

    Json(const Json&) = delete;
    Json& operator=(const Json&) = delete;
    Json(Json&&) noexcept = default;
    Json& operator=(Json&&);
    static std::optional<Json> parse(Memory, std::string_view);
    static Result<Json, JsonParseError> parse_result(Memory, std::string_view);
    String dump(Memory) const;

    Memory memory_resource() const {
        return resource_;
    }

    bool operator==(const Json& v) const {
        return data == v.data;
    }

private:
    Memory resource_;
};
}
