#pragma once
#include <teaql/async/operation.hpp>

namespace teaql {
enum class StreamStatus { Open, Ended, Cancelled, Failed };

template <class T>
struct StreamPoll {
    enum class Kind { Pending, Item, End };
    Kind kind;
    const T* item = nullptr;
};

// State's poll_next returns nullopt when pending, optional(nullopt) at EOF,
// or optional(optional(item)). Native callbacks use this stream's stable wake
// signal. Items are separately Context-owned; polling never invalidates them.
template <class T>
class AsyncStream final {
public:
    using allocator_type = Allocator;

    template <class State>
    AsyncStream(std::allocator_arg_t, Allocator a, State& state)
        : resource_(a.resource()), state_(&state), table_(&table<State>) {
        quiescence_.attach(detail::async_arena(resource_), this, [](void* self) noexcept {
            static_cast<AsyncStream*>(self)->cancel();
        });
    }

    AsyncStream(const AsyncStream&) = delete;
    AsyncStream& operator=(const AsyncStream&) = delete;

    ~AsyncStream() {
        cancel();
    }

    StreamPoll<T> poll_next() {
        using Kind = typename StreamPoll<T>::Kind;
        if (status_ == StreamStatus::Cancelled) {
            throw OperationCancelled{};
        }
        if (status_ == StreamStatus::Failed) {
            std::rethrow_exception(failure_);
        }
        if (status_ == StreamStatus::Ended) {
            return {Kind::End};
        }
        if (polling_) {
            throw std::logic_error("Recursive poll of the same stream");
        }
        polling_ = true;
        try {
            auto next = table_->poll(state_, wake_);
            const T* item = nullptr;
            if (next && *next) {
                item = &static_cast<Arena*>(resource_)->template make<T>(std::move(**next));
            } else if (next) {
                table_->finish(state_);
                status_ = StreamStatus::Ended;
                quiescence_.reset();
            }
            polling_ = false;
            return item ? StreamPoll<T>{Kind::Item, item}
                        : StreamPoll<T>{next ? Kind::End : Kind::Pending};
        } catch (...) {
            failure_ = std::current_exception();
            status_ = StreamStatus::Failed;
            table_->cancel(state_);
            polling_ = false;
            quiescence_.reset();
            throw;
        }
    }

    const T* wait_next() {
        using Kind = typename StreamPoll<T>::Kind;
        for (;;) {
            auto observed = wake_.ticket();
            auto next = poll_next();
            if (next.kind != Kind::Pending) {
                return next.item;
            }
            wake_.wait(observed);
        }
    }

    void cancel() noexcept {
        if (polling_) {
            std::terminate();
        }
        if (status_ != StreamStatus::Open) {
            return;
        }
        polling_ = true;
        status_ = StreamStatus::Cancelled;
        table_->cancel(state_);
        polling_ = false;
        quiescence_.reset();
        wake_.notify();
    }

    StreamStatus status() const noexcept {
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
        std::optional<std::optional<T>> (*poll)(void*, WakeSignal&);
        void (*finish)(void*) noexcept;
        void (*cancel)(void*) noexcept;
    };
    template <class State>
    static constexpr Table table{
        [](void* state, WakeSignal& wake) -> std::optional<std::optional<T>> {
            return static_cast<State*>(state)->poll_next(wake);
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
    StreamStatus status_ = StreamStatus::Open;
    bool polling_ = false;
    std::exception_ptr failure_;
};

template <class T, class State, class... Args>
AsyncStream<T>& make_stream(Memory r, Args&&... args) {
    static_assert(std::uses_allocator_v<State, Allocator> ||
                      std::is_trivially_destructible_v<State>,
                  "Managed stream state must propagate its allocator");
    auto& arena = detail::async_arena(r);
    auto& state = arena.template make<State>(std::forward<Args>(args)...);
    try {
        return arena.template make<AsyncStream<T>>(state);
    } catch (...) {
        state.cancel_and_wait();
        throw;
    }
}

template <class T, class State, class... Args>
AsyncStream<T>& make_resource_stream(Memory r, Args&&... args) {
    auto& arena = detail::async_arena(r);
    auto& state = arena.template make<State>(std::forward<Args>(args)...);
    try {
        return arena.template make<AsyncStream<T>>(state);
    } catch (...) {
        state.cancel_and_wait();
        throw;
    }
}

template <class T, class State, class Context, class... Args>
    requires requires(Context& c) { c.memory_resource(); }
AsyncStream<T>& make_stream(Context& context, Args&&... args) {
    return make_stream<T, State>(context.memory_resource(), std::forward<Args>(args)...);
}

template <class T, class State, class Context, class... Args>
    requires requires(Context& c) { c.memory_resource(); }
AsyncStream<T>& make_resource_stream(Context& context, Args&&... args) {
    return make_resource_stream<T, State>(context.memory_resource(), std::forward<Args>(args)...);
}
}
