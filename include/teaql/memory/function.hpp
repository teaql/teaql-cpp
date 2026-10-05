#pragma once
#include <teaql/memory/arena.hpp>
#include <teaql/memory/types.hpp>
#include <functional>

namespace teaql {
namespace detail {
template <class F>
inline constexpr bool relocatable_callable =
    std::uses_allocator_v<F, Allocator> || std::is_empty_v<F> ||
    (std::is_pointer_v<F> && std::is_function_v<std::remove_pointer_t<F>>);

template <class F>
F copy_callable(Memory r, Memory source, const F& function) {
    if (r != source && !relocatable_callable<F>) {
        throw std::invalid_argument(
            "Cross-context callback captures require allocator-aware state");
    }
    return std::make_obj_using_allocator<F>(Allocator(r), function);
}

inline Arena& context_arena(Memory r) {
    auto* arena = dynamic_cast<Arena*>(r);
    if (!arena) {
        throw std::invalid_argument("Context function requires GlobalContext or UserContext arena");
    }
    return *arena;
}
}
// A callable borrow whose state is registered in the Context arena. Copies inside
// one Context share state; an explicit allocator copy to another Context clones it.
// Captured borrows must obey that same Context boundary; use allocator-aware state
// objects for captures that need to survive an explicit cross-domain copy.
template <class Signature>
class ContextFunction;

template <class R, class... Args>
class ContextFunction<R(Args...)> {
public:
    using allocator_type = Allocator;

    template <class F>
    ContextFunction(std::allocator_arg_t, Allocator a, F&& fn)
        requires(!std::same_as<std::remove_cvref_t<F>, ContextFunction>)
        : resource_(a.resource()) {
        using State = std::remove_cvref_t<F>;
        static_assert(std::uses_allocator_v<State, Allocator> ||
                          std::is_trivially_destructible_v<State>,
                      "Callable captures must be allocator-aware or non-owning");
        static_assert(std::is_invocable_r_v<R, const State&, Args...>);
        auto& state = detail::context_arena(resource_).make<State>(std::forward<F>(fn));
        state_ = &state;
        table_ = &table<State>;
    }

    ContextFunction(std::allocator_arg_t, Allocator a, const ContextFunction& other)
        : resource_(a.resource()), state_(other.state_), table_(other.table_) {
        if (resource_ != other.resource_) {
            state_ = table_->clone(detail::context_arena(resource_), state_);
        }
    }

    ContextFunction(std::allocator_arg_t, Allocator a, ContextFunction&& other)
        : ContextFunction(std::allocator_arg, a, std::as_const(other)) {}

    ContextFunction(const ContextFunction&) = default; // A same-domain borrow, never an owner.
    ContextFunction& operator=(const ContextFunction&) = delete;

    R operator()(Args... args) const {
        return table_->invoke(state_, std::forward<Args>(args)...);
    }

    Memory memory_resource() const {
        return resource_;
    }

private:
    struct Table {
        R (*invoke)(const void*, Args...);
        const void* (*clone)(Arena&, const void*);
    };
    template <class F>
    static constexpr Table table{
        [](const void* state, Args... args) -> R {
            return std::invoke(*static_cast<const F*>(state), std::forward<Args>(args)...);
        },
        [](Arena& arena, const void* state) -> const void* {
            if constexpr (!detail::relocatable_callable<F>) {
                throw std::invalid_argument(
                    "Cross-context callback captures require allocator-aware state");
            } else {
                return &arena.make<F>(*static_cast<const F*>(state));
            }
        }};
    Memory resource_;
    const void* state_;
    const Table* table_;
};
}
