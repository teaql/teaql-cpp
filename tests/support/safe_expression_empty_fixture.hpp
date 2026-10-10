#pragma once
#include <teaql/core/safe_expression.hpp>

namespace application_empty_fixture {
struct Flag {
    bool empty;
    unsigned* predicate_calls;
};

inline bool teaql_is_empty(const Flag& value) {
    ++*value.predicate_calls;
    return value.empty;
}

struct Observations {
    bool empty;
    bool not_empty;
    unsigned evaluations;
    unsigned predicates;
    unsigned empty_callbacks;
    unsigned nonempty_callbacks;
};

inline Observations observe(teaql::Memory owner, bool present, bool empty) {
    unsigned evaluations = 0, predicates = 0, empty_callbacks = 0, nonempty_callbacks = 0;
    auto expression = teaql::SafeExpression<int, Flag>(
        std::allocator_arg, teaql::Allocator(owner), 7, [&](const int&) -> std::optional<Flag> {
            ++evaluations;
            return present ? std::optional(Flag{empty, &predicates}) : std::nullopt;
        });
    bool is_empty = expression.is_empty();
    bool is_not_empty = expression.is_not_empty();
    expression.when_is_empty([&] {
        ++empty_callbacks;
    });
    expression.when_not_empty([&](Flag) {
        ++nonempty_callbacks;
    });
    return {is_empty, is_not_empty, evaluations, predicates, empty_callbacks, nonempty_callbacks};
}

static_assert(teaql::TeaqlEmpty<Flag>);
}
