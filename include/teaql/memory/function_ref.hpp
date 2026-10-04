#pragma once
#include <concepts>
#include <functional>
#include <type_traits>

namespace teaql {
// Synchronous callable borrow. It never stores, allocates, or extends callback state.
template<class Signature> class FunctionRef;
template<class R,class... Args> class FunctionRef<R(Args...)> {
public:
    template<class F> requires (!std::same_as<std::remove_cvref_t<F>,FunctionRef> && std::is_object_v<F> && std::is_invocable_r_v<R,F&,Args...>)
    FunctionRef(F& function) : state_(&function), invoke_([](const void* p,Args... args)->R {
        return std::invoke(*static_cast<F*>(const_cast<void*>(p)),std::forward<Args>(args)...);
    }) {}
    R operator()(Args... args) const { return invoke_(state_,std::forward<Args>(args)...); }
private:
    const void* state_;
    R (*invoke_)(const void*,Args...);
};
}
