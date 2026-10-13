#pragma once
#include <teaql/memory/arena.hpp>
#include <teaql/memory/types.hpp>

namespace teaql {
// A checked, non-owning managed-object binding. ContextState remains in Global
// diagnostics after actual User deletion; generic arenas still require their
// owner to remain alive. No payload destructor touches a captured allocator.
template <class T>
class ArenaBorrow {
public:
    using allocator_type = Allocator;

    ArenaBorrow(std::allocator_arg_t, Allocator a, const T& value)
        : destination_(&as_arena(a.resource())), origin_(&as_arena(value.memory_resource())),
          destination_state_(destination_->context_state()),
          origin_state_(origin_->context_state()), slot_(origin_->slot_for_address(&value)),
          identity_(origin_->allocation_identity(slot_)) {
        if (!origin_->can_lend_to(*destination_)) {
            throw std::invalid_argument(
                "Managed object borrow requires an explicit destination copy");
        }
        if (!slot_) {
            throw std::invalid_argument(
                "Managed object borrow requires a registered Context object");
        }
        (void)get();
    }

    ArenaBorrow(std::allocator_arg_t, Allocator a, const ArenaBorrow& source)
        : ArenaBorrow(std::allocator_arg, a, source.get()) {}

    ArenaBorrow(std::allocator_arg_t, Allocator a, ArenaBorrow&& source)
        : ArenaBorrow(std::allocator_arg, a, std::as_const(source)) {}

    ArenaBorrow(const ArenaBorrow&) = default;
    ArenaBorrow(ArenaBorrow&&) noexcept = default;
    ArenaBorrow& operator=(const ArenaBorrow&) = default;
    ArenaBorrow& operator=(ArenaBorrow&&) noexcept = default;

    const T& get(std::source_location access = std::source_location::current()) const {
        check_state(destination_state_, access);
        check_state(origin_state_, access);
        destination_->check_thread();
        if (destination_->closed() || destination_->closing()) {
            throw ReferenceError(ReferenceError::Kind::Closed, identity_, access);
        }
        return *static_cast<const T*>(origin_->resolve(slot_, typeid(T), identity_, access));
    }

    Memory memory_resource() const {
        (void)get();
        return destination_;
    }

    std::uint64_t identity() const noexcept {
        return identity_;
    }

    bool operator==(const ArenaBorrow& other) const noexcept {
        return identity_ == other.identity_;
    }

private:
    static Arena& as_arena(Memory resource) {
        auto* result = dynamic_cast<Arena*>(resource);
        if (!result) {
            throw std::invalid_argument("Managed object borrows require a Context arena");
        }
        return *result;
    }

    void check_state(const detail::ContextState* state, std::source_location access) const {
        if (state && state->phase != detail::ContextState::Phase::Open) {
            throw ReferenceError(ReferenceError::Kind::Closed, identity_, access, state->closed_at);
        }
    }

    Arena* destination_;
    Arena* origin_;
    const detail::ContextState* destination_state_;
    const detail::ContextState* origin_state_;
    std::uint32_t slot_;
    std::uint64_t identity_;
};
}
