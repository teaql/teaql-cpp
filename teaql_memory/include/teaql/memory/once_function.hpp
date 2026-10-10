#pragma once
#include <teaql/memory/function.hpp>

namespace teaql {
// A one-shot Context borrow. Same-domain copies share one invocation state;
// they neither duplicate the callback nor extend its lifetime. Mutable captures
// are permitted, unlike the shared Fn-style ContextFunction contract.
template <class Signature>
class ContextOnceFunction;

template <class R, class... Args>
class ContextOnceFunction<R(Args...)> {
public:
    using allocator_type = Allocator;

    template <class F>
        requires(!std::same_as<std::remove_cvref_t<F>, ContextOnceFunction>)
    ContextOnceFunction(std::allocator_arg_t, Allocator a, F&& callback) : resource_(a.resource()) {
        using Callable = std::remove_cvref_t<F>;
        static_assert(std::uses_allocator_v<Callable, Allocator> ||
                          std::is_trivially_destructible_v<Callable>,
                      "One-shot captures must be allocator-aware or non-owning");
        static_assert(std::is_invocable_r_v<R, Callable&, Args...>);
        state_ = &detail::context_arena(resource_).template make<State<Callable>>(
            std::forward<F>(callback));
        invoke_ = [](void* raw, Args... args) -> R {
            auto& state = *static_cast<State<Callable>*>(raw);
            if (state.called) {
                throw std::logic_error("Context callback has already been consumed");
            }
            state.called = true;
            return std::invoke(state.callback, std::forward<Args>(args)...);
        };
    }

    ContextOnceFunction(std::allocator_arg_t, Allocator a, const ContextOnceFunction& other)
        : resource_(a.resource()), state_(other.state_), invoke_(other.invoke_) {
        if (resource_ != other.resource_) {
            throw std::invalid_argument("One-shot callback cannot escape its owning Context");
        }
    }

    ContextOnceFunction(std::allocator_arg_t, Allocator a, ContextOnceFunction&& other)
        : ContextOnceFunction(std::allocator_arg, a, std::as_const(other)) {}

    ContextOnceFunction(const ContextOnceFunction&) = delete;
    ContextOnceFunction(ContextOnceFunction&&) noexcept = default;

    R operator()(Args... args) const {
        auto& arena = detail::context_arena(resource_);
        arena.check_thread();
        if (arena.closed() || arena.closing()) {
            throw std::logic_error("Cannot invoke a callback in a closing Context");
        }
        return invoke_(state_, std::forward<Args>(args)...);
    }

private:
    template <class F>
    struct State {
        using allocator_type = Allocator;
        F callback;
        bool called = false;

        template <class C>
        State(std::allocator_arg_t, Allocator a, C&& callback)
            : callback(std::make_obj_using_allocator<F>(a, std::forward<C>(callback))) {}
    };

    Memory resource_;
    void* state_;
    R (*invoke_)(void*, Args...);
};
}
