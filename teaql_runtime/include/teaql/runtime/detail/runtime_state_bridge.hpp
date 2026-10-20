#pragma once
#include <teaql/runtime/entity_runtime.hpp>
#include <type_traits>

namespace teaql::detail {
template <class>
inline constexpr bool default_runtime_state_hook_owner = false;
template <class Derived>
inline constexpr bool default_runtime_state_hook_owner<EntityDefaults<Derived>> = true;

template <class Owner>
auto runtime_state_hook_owner(std::optional<AnyRef> (Owner::*)()
                                  const) -> std::type_identity<Owner>;
template <class Owner>
auto runtime_state_hook_owner(std::optional<AnyRef> (Owner::*)()
                                  const&) -> std::type_identity<Owner>;

// Explicit core hooks are authoritative, including an empty or wrong-type borrow.
// The inherited default keeps the established typed C++ carrier protocol usable.
template <Entity T>
std::optional<EntityRuntimeState> borrow_entity_runtime_state(const T& entity) {
    if constexpr (requires { entity.runtime_state_any(); }) {
        static_assert(std::same_as<decltype(entity.runtime_state_any()), std::optional<AnyRef>>);
        using Owner = typename decltype(runtime_state_hook_owner(&T::runtime_state_any))::type;
        if constexpr (!default_runtime_state_hook_owner<Owner>) {
            auto borrowed = entity.runtime_state_any();
            const auto* state = borrowed ? borrowed->template get<EntityRuntimeState>() : nullptr;
            if (!state) {
                return std::nullopt;
            }
            return std::optional<EntityRuntimeState>(
                std::in_place, std::allocator_arg, Allocator(state->memory_resource()), *state);
        }
    }
    if constexpr (LedgerEntity<T>) {
        return entity.entity_runtime_state();
    }
    return std::nullopt;
}
}
