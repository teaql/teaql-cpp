// Original diagnostic cursor tests use the production wrapper and a controlled source.
#include <teaql/sql/detail/stream_execution.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;
using Item = Result<StreamChunk, String>;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Step {
    const char* error = nullptr;
    bool last = false;
};

struct Cursor {
    using allocator_type = Allocator;
    Memory owner;
    Vector<Item> values;
    bool& released;
    unsigned& polls;
    bool pending;
    std::size_t index = 0;

    Cursor(std::allocator_arg_t,
           Allocator a,
           std::initializer_list<Step> steps,
           bool pending,
           bool& released,
           unsigned& polls)
        : owner(a.resource()), values(owner), released(released), polls(polls), pending(pending) {
        for (const auto& step : steps) {
            if (step.error) {
                values.emplace_back(Item::err(owner, String(step.error, owner)));
            } else {
                StreamChunk chunk(std::allocator_arg, a, 0, step.last);
                chunk.rows.emplace_back();
                chunk.rows.emplace_back();
                values.emplace_back(Item::ok(owner, std::move(chunk)));
            }
        }
    }

    std::optional<std::optional<Item>> poll_next(WakeSignal&) {
        ++polls;
        if (pending) {
            return {};
        }
        if (index == values.size()) {
            return std::optional<Item>{};
        }
        return std::optional<Item>(std::move(values[index++]));
    }

    void finish_and_wait() noexcept {
        close();
    }

    void cancel_and_wait() noexcept {
        close();
    }

    void close() noexcept {
        released = true;
        values.clear();
    }
};

struct Fixture {
    Vector<ExecutionMetadata> entries;
    bool released = false;
    bool observed_before_release = false;
    unsigned polls = 0;
    std::unique_ptr<UserContext> user;
    QueryStream<String>* stream = nullptr;

    Fixture(GlobalContext& global,
            std::initializer_list<Step> steps,
            bool pending = false,
            bool panic_sink = false)
        : entries(global.memory_resource()), user(global.make_user_context()) {
        auto owner = user->memory_resource();
        auto& source = make_stream<Item, Cursor>(owner, steps, pending, released, polls);
        ExecutionObserver observer(
            std::allocator_arg, Allocator(owner), [this, panic_sink](ExecutionMetadata metadata) {
                // Record ordering independently: the production wrapper suppresses sink exceptions.
                observed_before_release |= !released;
                entries.emplace_back(std::move(metadata));
                if (panic_sink) {
                    throw std::runtime_error("controlled sink failure");
                }
            });
        stream = &detail::diagnostic_stream(
            owner, source, ExecutionMetadata::unrecorded_query(owner, 0), std::move(observer));
    }

    ~Fixture() {
        user.reset();
    }

    void terminal(std::size_t count, SqlExecutionOutcome outcome) const {
        check(!observed_before_release && released,
              "original observer sees a released provider cursor");
        check(entries.size() == 1 && entries[0].result_count == std::optional<std::size_t>(count) &&
                  entries[0].sql_log.execution_outcome ==
                      std::optional<SqlExecutionOutcome>(outcome) &&
                  !entries[0].debug_query,
              "original single terminal outcome, delivered count and absent debug query");
    }
};

// Context streams are borrows. An explicit scoped consumer adapts Rust owned
// stream drop, invoking the real cancellation path even during stack unwinding.
struct Consumer {
    QueryStream<String>& stream;

    ~Consumer() {
        stream.cancel();
    }
};

static void
partial_transport_failure_preserves_error_and_drops_cursor_first(GlobalContext& global) {
    Fixture f(global, {{nullptr, false}, {"driver canary", false}});
    auto* first = f.stream->wait_next();
    check(first && *first, "original first delivered chunk");
    auto* failure = f.stream->wait_next();
    check(failure && failure->is_err() && failure->error() == "driver canary",
          "original provider error text preserved");
    check(!f.stream->wait_next(), "original failure ends stream");
    f.stream->cancel();
    f.terminal(2, SqlExecutionOutcome::Failure);
}

static void terminal_chunk_closes_without_extra_poll(GlobalContext& global) {
    Fixture f(global, {{nullptr, true}});
    auto* chunk = f.stream->wait_next();
    check(chunk && *chunk && f.released && f.polls == 1,
          "original last chunk releases cursor without another provider poll");
    f.stream->cancel();
    f.terminal(2, SqlExecutionOutcome::Success);
}

static void unpolled_drop_is_cancelled_without_delivery(GlobalContext& global) {
    Fixture f(global, {{nullptr, false}});
    f.stream->cancel();
    check(f.polls == 0, "original unpolled cancellation performs no provider poll");
    f.terminal(0, SqlExecutionOutcome::Cancelled);
}

static void pending_drop_is_cancelled_without_delivery(GlobalContext& global) {
    Fixture f(global, {}, true);
    check(f.stream->poll_next().kind == StreamPoll<Item>::Kind::Pending, "original pending stream");
    f.stream->cancel();
    f.terminal(0, SqlExecutionOutcome::Cancelled);
}

static void consumer_unwind_is_failure_with_delivered_chunk_count(GlobalContext& global) {
    Fixture f(global, {{nullptr, false}});
    auto* first = f.stream->wait_next();
    check(first && *first, "original chunk delivered before consumer panic");
    bool unwound = false;
    try {
        Consumer consumer{*f.stream};
        throw std::runtime_error("controlled consumer failure");
    } catch (const std::runtime_error&) {
        unwound = true;
    }
    check(unwound, "original consumer unwind observed");
    f.terminal(2, SqlExecutionOutcome::Failure);
}

static void sink_panic_does_not_replace_provider_error(GlobalContext& global) {
    Fixture f(global, {{"original driver error", false}}, false, true);
    auto* failure = f.stream->wait_next();
    check(failure && failure->is_err() && failure->error() == "original driver error",
          "original provider error survives the sink exception");
    f.stream->cancel();
    f.terminal(0, SqlExecutionOutcome::Failure);
}

static void sink_panic_on_drop_does_not_escape(GlobalContext& global) {
    Fixture f(global, {}, true, true);
    f.stream->cancel();
    f.terminal(0, SqlExecutionOutcome::Cancelled);
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        {
            GlobalContext global;
            partial_transport_failure_preserves_error_and_drops_cursor_first(global);
            terminal_chunk_closes_without_extra_poll(global);
            unpolled_drop_is_cancelled_without_delivery(global);
            pending_drop_is_cancelled_without_delivery(global);
            consumer_unwind_is_failure_with_delivered_chunk_count(global);
            sink_panic_does_not_replace_provider_error(global);
            sink_panic_on_drop_does_not_escape(global);
            check(global.live_user_bytes() == 0, "all source User contexts are actually reclaimed");
        }
        std::pmr::set_default_resource(previous);
        std::cout << "Original seven diagnostic stream contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
