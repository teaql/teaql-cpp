#include <teaql/core/safe_expression.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
#include "../../tests/support/safe_expression_empty_fixture.hpp"

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Owned {
    using allocator_type = Allocator;
    String text;

    Owned(std::allocator_arg_t, Allocator a, std::string_view text) : text(text, a.resource()) {}

    Owned(std::allocator_arg_t, Allocator a, Owned&& other) : text(a.resource()) {
        if (a.resource() != other.text.get_allocator().resource()) {
            throw std::invalid_argument("Uncopyable payload cannot change owner");
        }
        text = std::move(other.text);
    }

    Owned(const Owned&) = delete;
    Owned(Owned&&) noexcept = default;
    Owned& operator=(Owned&&) = delete;
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

struct ScalarCapture : Capture {
    using Capture::Capture;

    ScalarCapture(std::allocator_arg_t, Allocator a, ScalarCapture&& other)
        : Capture(std::allocator_arg, a, static_cast<Capture&&>(other)) {}

    ScalarCapture(ScalarCapture&&) noexcept = default;
    ScalarCapture(const ScalarCapture&) = delete;

    std::optional<Owned> operator()(const int& root, Memory owner) const {
        Owned value(std::allocator_arg, Allocator(owner), std::to_string(root));
        return Capture::operator()(value, owner);
    }
};

template <class T>
concept HasIdentityFactory =
    requires(Memory owner, const T& value) { SafeExpression<T, T>::value(owner, value); };
static_assert(HasIdentityFactory<int>);
static_assert(!HasIdentityFactory<Owned>);
static_assert(!std::is_constructible_v<SafeExpression<Owned, Owned>,
                                       std::allocator_arg_t,
                                       Allocator,
                                       const Owned&,
                                       Capture>);

template <class Expression>
concept HasIdentityRoot = requires(const Expression& expression) { expression.root(); };
template <class Expression>
concept HasListAdapters = requires(const Expression& expression) {
    expression.size();
    expression.first();
    expression.get(0);
};
static_assert(HasIdentityRoot<SafeExpression<int, int>>);
static_assert(!HasIdentityRoot<SafeExpression<Owned, Owned>>);
static_assert(!HasIdentityRoot<SafeExpression<int, Owned>>);
static_assert(HasListAdapters<SafeExpression<int, SmartList<int>>>);
static_assert(!HasListAdapters<SafeExpression<int, SmartList<Owned>>>);

static void consuming_capabilities() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    Owned root(std::allocator_arg, Allocator(owner), "root");
    using Expression = SafeExpression<Owned, Owned>;
    auto& expression = user->make<Expression>(
        std::move(root), Capture(std::allocator_arg, Allocator(owner), "-capture"));
    auto copied = Expression(std::allocator_arg, Allocator(owner), expression);
    auto result = copied.eval();
    check(result && result->text == "root-capture" && root.text.empty(),
          "new consumes non-copyable root and callback; same-domain Clone shares state");
    auto mapped = copied
                      .apply([](Owned value, Memory owner) {
                          value.text += "-mapped";
                          return Owned(std::allocator_arg, Allocator(owner), std::move(value));
                      })
                      .apply_optional([](Owned value, Memory) -> std::optional<Owned> {
                          return std::move(value);
                      });
    auto mapped_value = mapped.eval();
    check(mapped_value && mapped_value->text == "root-capture-mapped",
          "apply and apply_optional preserve owned outputs without Clone");
    auto present = mapped.or_else(Owned(std::allocator_arg, Allocator(owner), "unused"));
    check(present.text == "root-capture-mapped", "or_else supports an owned non-copyable result");
    auto absent = SafeExpression<int, Owned>(
        std::allocator_arg, Allocator(owner), 7, [](const int&) -> std::optional<Owned> {
            return {};
        });
    auto fallback = absent.or_else_with([owner] {
        return Owned(std::allocator_arg, Allocator(owner), "fallback");
    });
    check(fallback.text == "fallback", "lazy fallback need not be Clone");
    auto failure = absent.or_else_throw([owner] {
        return Owned(std::allocator_arg, Allocator(owner), "error");
    });
    check(failure.is_err() && failure.error().text == "error",
          "or_else_throw preserves an owned non-copyable error");
    bool rejected = false;
    try {
        (void)Expression(std::allocator_arg, Allocator(global.memory_resource()), expression);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && expression.eval()->text == "root-capture",
          "foreign uncopyable root rejection leaves the live expression usable");
    auto captured =
        SafeExpression<int, Owned>(std::allocator_arg,
                                   Allocator(owner),
                                   7,
                                   ScalarCapture(std::allocator_arg, Allocator(owner), "-capture"));
    rejected = false;
    try {
        (void)SafeExpression<int, Owned>(
            std::allocator_arg, Allocator(global.memory_resource()), captured);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && captured.eval()->text == "7-capture",
          "foreign non-copyable callback state is rejected even when the root is copyable");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        consuming_capabilities();
        {
            GlobalContext global;
            auto user = global.make_user_context();
            for (bool present : {false, true}) {
                for (bool empty : {false, true}) {
                    auto result =
                        application_empty_fixture::observe(user->memory_resource(), present, empty);
                    bool expected_empty = !present || empty;
                    check(
                        result.empty == expected_empty && result.not_empty != expected_empty &&
                            result.evaluations == 4 && result.predicates == (present ? 4u : 0u) &&
                            result.empty_callbacks == unsigned(expected_empty) &&
                            result.nonempty_callbacks == unsigned(!expected_empty),
                        "custom TeaqlEmpty uses ADL once per present evaluation and preserves branches");
                }
            }
        }
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
