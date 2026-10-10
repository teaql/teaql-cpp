#include <teaql/core/safe_expression.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>
#include "../safe_expression_empty_fixture.hpp"

static Json shown(Memory owner, std::optional<Value> value) {
    return value ? Json(owner, detail::debug_value(owner, *value)) : Json(owner);
}

struct Owned {
    using allocator_type = Allocator;
    String text;

    Owned(std::allocator_arg_t, Allocator a, std::string_view text) : text(text, a.resource()) {}

    Owned(std::allocator_arg_t, Allocator a, Owned&& other)
        : text(std::move(other.text), a.resource()) {}

    Owned(Owned&&) noexcept = default;
    Owned(const Owned&) = delete;
};

struct Capture {
    using allocator_type = Allocator;
    Owned state;

    Capture(std::allocator_arg_t, Allocator a, std::string_view text)
        : state(std::allocator_arg, a, text) {}

    Capture(std::allocator_arg_t, Allocator a, Capture&& other)
        : state(std::allocator_arg, a, std::move(other.state)) {}

    Capture(Capture&&) noexcept = default;
    Capture(const Capture&) = delete;

    std::optional<Owned> operator()(const Owned& root, Memory owner) const {
        String result(root.text, owner);
        result += state.text;
        return Owned(std::allocator_arg, Allocator(owner), result);
    }
};

static Json owned(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto expression = SafeExpression<Owned, Owned>(
        std::allocator_arg,
        Allocator(owner),
        Owned(std::allocator_arg, Allocator(owner), text(field(input, "root"))),
        Capture(std::allocator_arg, Allocator(owner), text(field(input, "capture"))));
    Json::Object out(owner);
    out.emplace("initial", Json(owner, expression.eval()->text));
    auto mapped = expression
                      .apply([](Owned value, Memory owner) {
                          value.text += "-mapped";
                          return Owned(std::allocator_arg, Allocator(owner), std::move(value));
                      })
                      .apply_optional([](Owned value, Memory) -> std::optional<Owned> {
                          return std::move(value);
                      });
    out.emplace("mapped", Json(owner, mapped.eval()->text));
    out.emplace(
        "eager",
        Json(owner, mapped.or_else(Owned(std::allocator_arg, Allocator(owner), "unused")).text));
    auto absent = SafeExpression<int, Owned>(
        std::allocator_arg, Allocator(owner), 7, [](const int&) -> std::optional<Owned> {
            return {};
        });
    out.emplace("fallback",
                Json(owner,
                     absent
                         .or_else_with([owner] {
                             return Owned(std::allocator_arg, Allocator(owner), "fallback");
                         })
                         .text));
    auto error = absent.or_else_throw([owner] {
        return Owned(std::allocator_arg, Allocator(owner), "error");
    });
    out.emplace("error", Json(owner, error.error().text));
    return Json(owner, std::move(out));
}

struct Item : BaseEntityDefaults<Item> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "SafeItem";
    BaseEntityData data;

    Item(std::allocator_arg_t, Allocator a) : data(std::allocator_arg, a) {}

    Item(std::allocator_arg_t, Allocator a, const Item& other)
        : data(std::allocator_arg, a, other.data) {}

    Item(Item&&) noexcept = default;

    Memory memory_resource() const {
        return data.memory_resource();
    }

    const BaseEntityData& base() const {
        return data;
    }

    BaseEntityData& base_mut() {
        return data;
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        return EntityDescriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
    }

    static Result<Item, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("Fixture does not decode");
    }

    MutationValues into_values() && {
        return MutationValues(memory_resource(), data.to_values_map(memory_resource()));
    }
};

static_assert(BaseEntity<Item>);

static Json entity(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    Item item(std::allocator_arg, Allocator(owner));
    item.set_id(number<std::uint64_t>(field(input, "id")));
    item.set_version(number<std::int64_t>(field(input, "version")));
    auto expression = SafeExpression<Item, Item>::value(owner, item);
    auto changed = expression.update_entity_id(number<std::uint64_t>(field(input, "next"))).eval();
    Json::Object out(owner);
    out.emplace("id", Json(owner, std::to_string(*expression.entity_id().eval())));
    out.emplace("version", Json(owner, std::to_string(*expression.entity_version().eval())));
    out.emplace("changed_id", Json(owner, std::to_string(changed->id())));
    out.emplace("changed_version", Json(owner, std::to_string(changed->version_value())));
    out.emplace("original_id", Json(owner, std::to_string(expression.root().id())));
    return Json(owner, std::move(out));
}

static Json observe(UserContext& user, const Json& input) {
    if (auto* kind = optional(input, "kind")) {
        if (text(*kind) == "custom_empty") {
            auto result =
                application_empty_fixture::observe(user.memory_resource(),
                                                   std::get<bool>(field(input, "present").data),
                                                   std::get<bool>(field(input, "empty").data));
            Json::Object out(user.memory_resource());
            out.emplace("empty", Json(user.memory_resource(), result.empty));
            out.emplace("not_empty", Json(user.memory_resource(), result.not_empty));
            out.emplace("evaluations",
                        Json(user.memory_resource(), std::uint64_t(result.evaluations)));
            out.emplace("predicates",
                        Json(user.memory_resource(), std::uint64_t(result.predicates)));
            out.emplace("empty_callbacks",
                        Json(user.memory_resource(), std::uint64_t(result.empty_callbacks)));
            out.emplace("nonempty_callbacks",
                        Json(user.memory_resource(), std::uint64_t(result.nonempty_callbacks)));
            return Json(user.memory_resource(), std::move(out));
        }
        if (text(*kind) == "owned") {
            return owned(user, input);
        }
        if (text(*kind) == "entity") {
            return entity(user, input);
        }
    }
    auto owner = user.memory_resource();
    auto root = value(owner, field(input, "root"));
    auto alternate = value(owner, field(input, "alternate"));
    auto fallback = value(owner, field(input, "fallback"));
    Vector<String> calls(owner);
    auto note = [&](std::string_view operation, const Value& item) {
        String entry(operation, owner);
        entry += ':';
        entry += detail::debug_value(owner, item);
        calls.emplace_back(entry);
    };
    bool present = std::get<bool>(field(input, "present").data);
    auto base = SafeExpression<Value, Value>(std::allocator_arg,
                                             Allocator(owner),
                                             root,
                                             [&](const Value& input) -> std::optional<Value> {
                                                 note("evaluate", input);
                                                 if (!present) {
                                                     return {};
                                                 }
                                                 return detail::clone_in(owner, input);
                                             });
    auto mode = text(field(input, "mode"));
    auto expression = base.apply_optional([&](Value item) -> std::optional<Value> {
                              note("optional", item);
                              if (mode == "missing") {
                                  return {};
                              }
                              return std::move(item);
                          })
                          .apply([&](Value item) {
                              note("map", item);
                              if (mode == "wrap") {
                                  Value::List list(owner);
                                  list.emplace_back(std::move(item));
                                  return Value(owner, list);
                              }
                              return std::move(item);
                          });
    auto copied = base;
    Json::Object out(owner);
    out.emplace("shared_root", Json(owner, &base.root() == &copied.root()));
    out.emplace("eval", shown(owner, expression.eval()));
    out.emplace("supplied", shown(owner, expression.eval_with(alternate)));
    out.emplace("clone_eval", shown(owner, decltype(expression)(expression).eval()));
    out.emplace(
        "eager",
        Json(owner,
             detail::debug_value(owner, expression.or_else(detail::clone_in(owner, fallback)))));
    out.emplace("lazy", Json(owner, detail::debug_value(owner, expression.or_else_with([&] {
                                 calls.emplace_back("fallback");
                                 return detail::clone_in(owner, fallback);
                             }))));
    auto outcome = expression.or_else_throw([&] {
        calls.emplace_back("error");
        return String("missing value", owner);
    });
    Json::Object result(owner);
    if (outcome) {
        result.emplace("ok", Json(owner, detail::debug_value(owner, outcome.value())));
    } else {
        result.emplace("error", Json(owner, outcome.error()));
    }
    out.emplace("outcome", Json(owner, std::move(result)));
    out.emplace("null", Json(owner, expression.is_null()));
    out.emplace("not_null", Json(owner, expression.is_not_null()));
    out.emplace("empty", Json(owner, expression.is_empty()));
    out.emplace("not_empty", Json(owner, expression.is_not_empty()));
    expression.when_is_null([&] {
        calls.emplace_back("null");
    });
    expression.when_is_not_null([&](Value item) {
        note("value", item);
    });
    expression.when_is_empty([&] {
        calls.emplace_back("empty");
    });
    expression.when_not_empty([&](Value item) {
        note("nonempty", item);
    });
    auto identity = SafeExpression<Value, Value>::value(owner, root);
    out.emplace("root", Json(owner, detail::debug_value(owner, identity.root())));
    out.emplace("identity", shown(owner, identity.eval()));
    out.emplace("direct_empty", Json(owner, teaql_is_empty(root)));
    out.emplace("none_empty", Json(owner, teaql_is_empty(std::optional<Value>{})));
    std::optional<Value> some_null(std::in_place, owner);
    out.emplace("some_null_empty", Json(owner, teaql_is_empty(some_null)));
    SmartList<Value> list(std::allocator_arg, Allocator(owner));
    list.loaded();
    for (const auto& item : std::get<Json::Array>(field(input, "list").data)) {
        list.push(value(owner, item));
    }
    auto list_expression = SafeExpression<SmartList<Value>, SmartList<Value>>::value(owner, list);
    auto size = list_expression.size().eval();
    out.emplace("size", size ? Json(owner, *size) : Json(owner));
    out.emplace("first", shown(owner, list_expression.first().eval()));
    out.emplace(
        "get",
        shown(owner,
              list_expression.get(std::get<std::uint64_t>(field(input, "index").data)).eval()));
    Json::Array trace(owner);
    for (const auto& entry : calls) {
        trace.emplace_back(entry);
    }
    out.emplace("calls", Json(owner, std::move(trace)));
    return Json(owner, std::move(out));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid SafeExpression fixture");
            }
            std::cout << observe(*user, *input).dump(user->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
