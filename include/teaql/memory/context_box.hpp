#pragma once
#include <teaql/memory/arena.hpp>
#include <teaql/memory/types.hpp>
#include <concepts>

namespace teaql {
// An arena-owned box with eager, exactly-once payload destruction. The box is a
// Context borrow; reset releases native resources without reclaiming arena bytes.
// Allocate the box through the owning Context's make()/make_resource().
template <class T>
class ContextBox final {
public:
    using allocator_type = Allocator;

    template <class D, class... Args>
        requires std::derived_from<D, T>
    ContextBox(std::allocator_arg_t, Allocator a, std::in_place_type_t<D>, Args&&... args)
        : resource_(a.resource()) {
        static_assert(std::is_nothrow_destructible_v<D>,
                      "Context resources must destruct without throwing");
        auto* arena = dynamic_cast<Arena*>(resource_);
        if (!arena || arena->closed()) {
            throw std::invalid_argument("ContextBox requires a live Context arena");
        }
        auto* p = static_cast<D*>(resource_->allocate(sizeof(D), alignof(D)));
        std::uninitialized_construct_using_allocator(p, a, std::forward<Args>(args)...);
        object_ = p;
        payload_ = p;
        destroy_ = [](void* raw) noexcept {
            std::destroy_at(static_cast<D*>(raw));
        };
    }

    ContextBox(const ContextBox&) = delete;
    ContextBox& operator=(const ContextBox&) = delete;

    ~ContextBox() {
        reset();
    }

    // Payload borrows expire at reset(), possibly before Context close.
    T* get() const noexcept {
        return object_;
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

    // Taking ownership does not extend the Context lifetime or allow reuse.
    bool available() const noexcept {
        return !claimed_ && object_;
    }

    void claim() {
        if (claimed_ || !object_) {
            throw std::logic_error("ContextBox payload was already consumed");
        }
        claimed_ = true;
    }

    void reset() noexcept {
        if (object_) {
            object_ = nullptr;
            destroy_(payload_);
            payload_ = nullptr;
        }
    }

private:
    Memory resource_;
    T* object_ = nullptr;
    void* payload_ = nullptr;
    void (*destroy_)(void*) noexcept = nullptr;
    bool claimed_ = false;
};
}
