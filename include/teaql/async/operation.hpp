#pragma once
#include <teaql/memory/arena.hpp>
#include <teaql/memory/clone.hpp>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>

namespace teaql {
namespace detail {
inline Arena& async_arena(Memory r) {
    auto* arena = dynamic_cast<Arena*>(r);
    if (!arena)
        throw std::invalid_argument("Async work requires a Context arena");
    if (arena->closed() || arena->closing())
        throw std::logic_error("Cannot start work in a closing Context");
    return *arena;
}
}

// May be signalled by a native completion thread. Only the Context owner polls
// operations and allocates model data; notify() does not resume user code.
class WakeSignal final {
public:
    class Forward;
    using Ticket = std::uint64_t;

    Ticket ticket() const {
        std::lock_guard lock(mutex_);
        return generation_;
    }

    void notify() noexcept {
        {
            std::lock_guard lock(mutex_);
            ++generation_;
            if (parent_)
                parent_->notify();
        }
        changed_.notify_all();
    }

    void wait(Ticket observed) {
        std::unique_lock lock(mutex_);
        changed_.wait(lock, [&] {
            return generation_ != observed;
        });
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    Ticket generation_ = 0;
    // Forwarding topology is configured only by the Context owner. Native
    // threads only notify; the source lock keeps detach waiting until an
    // in-flight notification has finished touching the borrowed parent.
    WakeSignal* parent_ = nullptr;
};

// One pending future has one awaiting parent, as with an owned Rust future.
// Reset before destroying either signal. Context-owned composition states reset
// during cancellation/quiescence, while all signals are still alive.
class WakeSignal::Forward final {
public:
    Forward() = default;
    Forward(const Forward&) = delete;
    Forward& operator=(const Forward&) = delete;

    ~Forward() {
        reset();
    }

    void attach(WakeSignal& source, WakeSignal& parent) {
        if (source_ == &source && parent_ == &parent)
            return;
        reset();
        // Only the owner changes graph links; worker notifications cannot race
        // with these reads. Reject cycles before installing a locking edge.
        for (auto* node = &parent; node; node = node->parent_)
            if (node == &source)
                throw std::logic_error("Cyclic operation wake forwarding");
        std::lock_guard lock(source.mutex_);
        if (source.parent_)
            throw std::logic_error("Operation already has an awaiting parent");
        source.parent_ = &parent;
        source_ = &source;
        parent_ = &parent;
    }

    void reset() noexcept {
        if (!source_)
            return;
        {
            std::lock_guard lock(source_->mutex_);
            source_->parent_ = nullptr;
        }
        source_ = nullptr;
        parent_ = nullptr;
    }

private:
    WakeSignal* source_ = nullptr;
    WakeSignal* parent_ = nullptr;
};
enum class AsyncStatus { Pending, Ready, Cancelled, Failed };

class OperationCancelled : public std::exception {
public:
    const char* what() const noexcept override {
        return "Context operation was cancelled";
    }
};

// A lazy, pollable future owned by one Context. Returned T references remain
// borrows until that Context closes. cancel()/close quiesce native work before
// reclaiming its host memory. No scheduler or global thread pool is installed.
// State contract: optional<T> poll(WakeSignal&), finish_and_wait() noexcept and
// cancel_and_wait() noexcept. Both terminal hooks detach every native callback;
// cancel must also join outstanding host-memory access. Hooks run only once.
template <class T>
class AsyncOperation final {
public:
    using allocator_type = Allocator;

    template <class State>
    AsyncOperation(std::allocator_arg_t, Allocator a, State& state)
        : resource_(a.resource()), state_(&state), table_(&table<State>) {
        quiescence_.attach(detail::async_arena(resource_), this, [](void* self) noexcept {
            static_cast<AsyncOperation*>(self)->cancel();
        });
    }

    AsyncOperation(const AsyncOperation&) = delete;
    AsyncOperation& operator=(const AsyncOperation&) = delete;

    ~AsyncOperation() {
        cancel();
    }

    const T* poll() {
        if (status_ == AsyncStatus::Cancelled)
            throw OperationCancelled{};
        if (status_ == AsyncStatus::Failed)
            std::rethrow_exception(failure_);
        if (status_ == AsyncStatus::Ready)
            return &*result_;
        if (polling_)
            throw std::logic_error("Recursive poll of the same operation");
        polling_ = true;
        try {
            auto value = table_->poll(state_, wake_);
            if (value) {
                result_.emplace(detail::move_in(resource_, std::move(*value)));
                table_->finish(state_);
                status_ = AsyncStatus::Ready;
                quiescence_.reset();
            }
            polling_ = false;
            return result_ ? &*result_ : nullptr;
        } catch (...) {
            failure_ = std::current_exception();
            status_ = AsyncStatus::Failed;
            table_->cancel(state_);
            polling_ = false;
            quiescence_.reset();
            throw;
        }
    }

    const T& wait() {
        for (;;) {
            auto observed = wake_.ticket();
            if (auto* value = poll())
                return *value;
            wake_.wait(observed);
        }
    }

    void cancel() noexcept {
        // Poll/cancel/Context close are owner-thread operations. Reentrant close
        // from an active provider callback would free its own stack and is invalid.
        if (polling_)
            std::terminate();
        if (status_ != AsyncStatus::Pending)
            return;
        polling_ = true;
        status_ = AsyncStatus::Cancelled;
        table_->cancel(state_);
        polling_ = false;
        quiescence_.reset();
        wake_.notify();
    }

    AsyncStatus status() const noexcept {
        return status_;
    }

    WakeSignal& wake_signal() noexcept {
        return wake_;
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

private:
    struct Table {
        std::optional<T> (*poll)(void*, WakeSignal&);
        void (*finish)(void*) noexcept;
        void (*cancel)(void*) noexcept;
    };
    template <class State>
    static constexpr Table table{
        [](void* state, WakeSignal& wake) -> std::optional<T> {
            return static_cast<State*>(state)->poll(wake);
        },
        [](void* state) noexcept {
            static_assert(noexcept(std::declval<State&>().finish_and_wait()));
            static_cast<State*>(state)->finish_and_wait();
        },
        [](void* state) noexcept {
            static_assert(noexcept(std::declval<State&>().cancel_and_wait()));
            static_cast<State*>(state)->cancel_and_wait();
        }};
    Memory resource_;
    void* state_;
    const Table* table_;
    WakeSignal wake_;
    Arena::Quiescence quiescence_;
    AsyncStatus status_ = AsyncStatus::Pending;
    bool polling_ = false;
    std::optional<T> result_;
    std::exception_ptr failure_;
};

// Managed state and native-resource state have intentionally separate factories.
// Operation handles themselves are Context borrows; cancel explicitly when a
// caller abandons an operation before Context close (the Rust future-drop edge).
template <class T, class State, class... Args>
AsyncOperation<T>& make_operation(Memory r, Args&&... args) {
    static_assert(std::uses_allocator_v<State, Allocator> ||
                      std::is_trivially_destructible_v<State>,
                  "Managed operation state must propagate its allocator");
    auto& arena = detail::async_arena(r);
    auto& state = arena.template make<State>(std::forward<Args>(args)...);
    try {
        return arena.template make<AsyncOperation<T>>(state);
    } catch (...) {
        state.cancel_and_wait();
        throw;
    }
}

template <class T, class State, class... Args>
AsyncOperation<T>& make_resource_operation(Memory r, Args&&... args) {
    auto& arena = detail::async_arena(r);
    auto& state = arena.template make<State>(std::forward<Args>(args)...);
    try {
        return arena.template make<AsyncOperation<T>>(state);
    } catch (...) {
        state.cancel_and_wait();
        throw;
    }
}

template <class T, class State, class Context, class... Args>
    requires requires(Context& c) { c.memory_resource(); }
AsyncOperation<T>& make_operation(Context& context, Args&&... args) {
    return make_operation<T, State>(context.memory_resource(), std::forward<Args>(args)...);
}

template <class T, class State, class Context, class... Args>
    requires requires(Context& c) { c.memory_resource(); }
AsyncOperation<T>& make_resource_operation(Context& context, Args&&... args) {
    return make_resource_operation<T, State>(context.memory_resource(),
                                             std::forward<Args>(args)...);
}
}
