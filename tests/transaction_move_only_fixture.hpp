#pragma once
#include <teaql/runtime/transaction.hpp>
#include "graph_saver_fixture.hpp"

namespace transaction_move_fixture {
using namespace teaql;
using Ambient = saver_fixture::Ambient;
using Scope = TransactionScope<Ambient>;

struct MoveOnlyOutput {
    using allocator_type = Allocator;
    String payload;
    unsigned* destroyed;
    bool active = true;

    MoveOnlyOutput(std::allocator_arg_t, Allocator a, std::string_view text, unsigned& destroyed)
        : payload(text, a.resource()), destroyed(&destroyed) {}

    MoveOnlyOutput(std::allocator_arg_t, Allocator a, MoveOnlyOutput&& other)
        : payload(std::move(other.payload), checked_resource(a, other)), destroyed(other.destroyed),
          active(std::exchange(other.active, false)) {}

    MoveOnlyOutput(MoveOnlyOutput&& other) noexcept
        : payload(std::move(other.payload)), destroyed(other.destroyed),
          active(std::exchange(other.active, false)) {}

    MoveOnlyOutput(const MoveOnlyOutput&) = delete;

    ~MoveOnlyOutput() {
        if (active) {
            ++*destroyed;
        }
    }

    Memory memory_resource() const noexcept {
        return payload.get_allocator().resource();
    }

private:
    static Memory checked_resource(Allocator a, const MoveOnlyOutput& other) {
        if (a.resource() != other.memory_resource()) {
            throw std::invalid_argument("Move-only transaction result cannot change Context");
        }
        return a.resource();
    }
};

using R = Result<MoveOnlyOutput, RuntimeError>;

struct Ready {
    using allocator_type = Allocator;
    Memory owner;
    std::optional<MoveOnlyOutput> value;
    bool fail;

    Ready(std::allocator_arg_t, Allocator a, MoveOnlyOutput&& input, bool fail)
        : owner(a.resource()), value(std::in_place, std::allocator_arg, a, std::move(input)),
          fail(fail) {}

    std::optional<R> poll(WakeSignal&) {
        if (fail) {
            value.reset();
            return R::err(owner, RuntimeError::event(owner, "business failed"));
        }
        return R::ok(owner, std::move(*value));
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        value.reset();
    }
};

struct Callback {
    using allocator_type = Allocator;
    MoveOnlyOutput value;
    bool fail;
    unsigned* invoked;

    Callback(std::allocator_arg_t,
             Allocator a,
             std::string_view payload,
             unsigned& destroyed,
             bool fail,
             unsigned& invoked)
        : value(std::allocator_arg, a, payload, destroyed), fail(fail), invoked(&invoked) {}

    Callback(std::allocator_arg_t, Allocator a, Callback&& other)
        : value(std::allocator_arg, a, std::move(other.value)), fail(other.fail),
          invoked(other.invoked) {}

    Callback(Callback&&) = default;
    Callback(const Callback&) = delete;

    AsyncOperation<R>& operator()(const Scope& scope) {
        ++*invoked;
        return make_operation<R, Ready>(scope.context().memory_resource(), std::move(value), fail);
    }
};
}
