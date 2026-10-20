#include <teaql/core/safe_expression.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

struct NoClone {
    int value;
    NoClone(const NoClone&) = delete;
    NoClone(NoClone&&) = default;
};

template <class T>
concept HasSize = requires(const T& expression) { expression.size(); };
template <class T>
concept HasFirst = requires(const T& expression) { expression.first(); };
template <class T>
concept HasGet = requires(const T& expression) { expression.get(0); };
template <class T>
concept HasRoot = requires(const T& expression) { expression.root(); };
template <class T>
concept HasIdentity =
    requires(Memory owner, const T& value) { SafeExpression<T, T>::value(owner, value); };

using NoCloneList = SafeExpression<int, SmartList<NoClone>>;
using CloneList = SafeExpression<int, SmartList<int>>;
static_assert(!HasSize<NoCloneList> && !HasFirst<NoCloneList> && !HasGet<NoCloneList>);
static_assert(HasSize<CloneList> && HasFirst<CloneList> && HasGet<CloneList>);
static_assert(HasIdentity<int> && !HasIdentity<NoClone>);
static_assert(HasRoot<SafeExpression<int, int>>);
static_assert(!HasRoot<SafeExpression<NoClone, NoClone>>);
static_assert(!HasRoot<SafeExpression<int, std::string_view>>);
static_assert(std::is_copy_constructible_v<SafeExpression<NoClone, NoClone>>);

// Counter captures are synchronous borrows and never escape this observation.
template <class T>
static Json inspect(Memory owner, const T& root, bool present) {
    const bool direct = teaql_is_empty(root);
    unsigned evaluations = 0;
    auto expression = SafeExpression<T, T>(
        std::allocator_arg, Allocator(owner), root, [&](const T& input) -> std::optional<T> {
            ++evaluations;
            if (!present) {
                return {};
            }
            return std::optional<T>(std::in_place, detail::clone_in(owner, input));
        });
    const bool empty = expression.is_empty();
    const bool not_empty = expression.is_not_empty();
    const bool null = expression.is_null();
    const bool not_null = expression.is_not_null();
    unsigned empty_callbacks = 0, value_callbacks = 0, null_callbacks = 0, nonnull_callbacks = 0;
    expression.when_is_empty([&] {
        ++empty_callbacks;
    });
    expression.when_not_empty([&](T) {
        ++value_callbacks;
    });
    expression.when_is_null([&] {
        ++null_callbacks;
    });
    expression.when_is_not_null([&](T) {
        ++nonnull_callbacks;
    });
    Json::Object result(owner);
    result.emplace("direct_empty", Json(owner, direct));
    result.emplace("empty", Json(owner, empty));
    result.emplace("not_empty", Json(owner, not_empty));
    result.emplace("null", Json(owner, null));
    result.emplace("not_null", Json(owner, not_null));
    result.emplace("evaluations", Json(owner, std::uint64_t(evaluations)));
    result.emplace("empty_callbacks", Json(owner, std::uint64_t(empty_callbacks)));
    result.emplace("value_callbacks", Json(owner, std::uint64_t(value_callbacks)));
    result.emplace("null_callbacks", Json(owner, std::uint64_t(null_callbacks)));
    result.emplace("nonnull_callbacks", Json(owner, std::uint64_t(nonnull_callbacks)));
    return Json(owner, std::move(result));
}

static Json observe(Memory owner, const Json& input) {
    const bool present = std::get<bool>(field(input, "present").data);
    const auto kind = text(field(input, "kind"));
    constexpr std::array<std::string_view, 8> texts{
        "", " ", "\u2003", "雪😀", std::string_view("a\0b", 3), "\n\r\t", "e\u0301", "%_\\\"'"};
    if (kind == "string" || kind == "borrowed") {
        auto index = std::get<std::uint64_t>(field(input, "index").data);
        if (kind == "borrowed") {
            return inspect(owner, texts.at(index), present);
        }
        return inspect(owner, String(texts.at(index), owner), present);
    }
    if (kind == "vector") {
        Vector<String> items(owner);
        for (const auto& item : std::get<Json::Array>(field(input, "items").data)) {
            items.emplace_back(text(item));
        }
        return inspect(owner, items, present);
    }
    if (kind == "smart_list") {
        SmartList<Value> list(std::allocator_arg, Allocator(owner));
        for (const auto& item : std::get<Json::Array>(field(input, "items").data)) {
            list.data.emplace_back(value(owner, item));
        }
        list.is_loaded = std::get<bool>(field(input, "loaded").data);
        return inspect(owner, list, present);
    }
    if (kind == "option") {
        std::optional<Value> item;
        if (std::get<bool>(field(input, "some").data)) {
            item.emplace(value(owner, field(input, "value")));
        }
        return inspect(owner, item, present);
    }
    if (kind == "value") {
        return inspect(owner, value(owner, field(input, "value")), present);
    }
    throw std::runtime_error("Unknown emptiness fixture kind");
}

struct DisableDefaultResource {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~DisableDefaultResource() {
        std::pmr::set_default_resource(previous);
    }
};

int main() {
    try {
        DisableDefaultResource guard;
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context_owner = global.make_user_context();
            {
                auto owner = user_context_owner->memory_resource();
                auto input = Json::parse(owner, line);
                if (!input) {
                    throw std::runtime_error("Invalid emptiness observation input");
                }
                std::cout << observe(owner, *input).dump(owner) << '\n';
            }
            user_context_owner.reset();
            if (global.live_user_bytes() != 0) {
                throw std::runtime_error("SafeExpression retained User bytes after deletion");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
