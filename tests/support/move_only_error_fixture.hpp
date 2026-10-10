#pragma once
#include <teaql/runtime/context.hpp>
#include <utility>

using namespace teaql;

struct MoveOnlyError {
    using allocator_type = Allocator;
    String message;
    unsigned* destroyed;
    bool active = true;

    MoveOnlyError(std::allocator_arg_t, Allocator a, std::string_view message, unsigned& destroyed)
        : message(message, a.resource()), destroyed(&destroyed) {}

    static Memory checked_resource(Allocator a, const MoveOnlyError& other) {
        if (a.resource() != other.memory_resource()) {
            throw std::invalid_argument("Native error cannot change Context");
        }
        return a.resource();
    }

    MoveOnlyError(std::allocator_arg_t, Allocator a, MoveOnlyError&& other)
        : message(std::move(other.message), checked_resource(a, other)), destroyed(other.destroyed),
          active(std::exchange(other.active, false)) {}

    MoveOnlyError(MoveOnlyError&& other) noexcept
        : message(std::move(other.message)), destroyed(other.destroyed),
          active(std::exchange(other.active, false)) {}

    MoveOnlyError(const MoveOnlyError&) = delete;

    ~MoveOnlyError() {
        if (active) {
            ++*destroyed;
        }
    }

    Memory memory_resource() const {
        return message.get_allocator().resource();
    }

    String to_string(Memory owner) const {
        return String(message, owner);
    }
};
