#pragma once
#include <memory>
#include <memory_resource>
#include <type_traits>
#include <utility>

namespace teaql::detail {
// Nested ownership never extends the containing Context's lifetime.
template<class T> struct ScopedDelete {
    std::pmr::memory_resource* resource = nullptr;
    void operator()(T* p) const noexcept {
        std::pmr::polymorphic_allocator<T>(resource).delete_object(p);
    }
};
template<class T> using ScopedBox = std::unique_ptr<T, ScopedDelete<T>>;
template<class T, class... Args>
ScopedBox<T> make_box(std::pmr::memory_resource* resource, Args&&... args) {
    auto a = std::pmr::polymorphic_allocator<T>(resource);
    return ScopedBox<T>(a.template new_object<T>(std::forward<Args>(args)...), ScopedDelete<T>{resource});
}
template<class T> T move_to(std::pmr::memory_resource* target, T&& source) {
    if (target == source.memory_resource()) return std::move(source);
    return std::make_obj_using_allocator<T>(std::pmr::polymorphic_allocator<T>(target), std::as_const(source));
}
// Prepare first: allocation failure leaves the destination alive and unchanged.
template<class T> T& assign_scoped(T& target, T&& source) {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    if (&target != &source) {
        auto replacement = move_to(target.memory_resource(), std::move(source));
        std::destroy_at(&target);
        std::construct_at(&target, std::move(replacement));
    }
    return target;
}
}
