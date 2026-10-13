#include <teaql/runtime/detail/generated_query_execution.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct State {
    bool ready = false;
    unsigned cancelled = 0, finished = 0;
    WakeSignal* wake = nullptr;
};

struct RowsWork {
    using allocator_type = Allocator;
    using Output = Result<Vector<CompactRow>, DataServiceError<String>>;
    Memory resource;
    State& state;
    Output result;
    String buffer;

    RowsWork(std::allocator_arg_t, Allocator a, State& state, const Output& result)
        : resource(a.resource()), state(state), result(std::allocator_arg, a, result),
          buffer(32768, 'x', resource) {}

    std::optional<Output> poll(WakeSignal& wake) {
        state.wake = &wake;
        if (!state.ready) {
            return std::nullopt;
        }
        return std::optional<Output>(std::in_place, std::move(result));
    }

    void finish_and_wait() noexcept {
        state.wake = nullptr;
        ++state.finished;
    }

    void cancel_and_wait() noexcept {
        // Cleanup still runs before the host buffer's arena blocks are reclaimed.
        if (buffer.size() != 32768 || buffer.back() != 'x') {
            std::terminate();
        }
        state.wake = nullptr;
        ++state.cancelled;
    }
};

static void pending_and_deleted_contexts() {
    GlobalContext global;
    State state;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    Vector<CompactRow> rows(r);
    rows.emplace_back();
    rows.back().insert("count", Value(r, std::int64_t(9)));
    auto result = RowsWork::Output::ok(r, std::move(rows));
    auto& input = make_operation<RowsWork::Output, RowsWork>(r, state, result);
    using Scalar = detail::GeneratedScalarQueryOperation<String, true>;
    auto& count = make_operation<Scalar::Output, Scalar>(r, input, "Record");
    check(!count.poll(), "deferred scalar rows must keep their parent pending");
    auto ticket = count.wake_signal().ticket();
    state.ready = true;
    state.wake->notify();
    check(count.wake_signal().ticket() != ticket, "scalar child wake reaches its awaiting parent");
    check(count.wait().value() == 9 && state.finished == 1,
          "scalar completion consumes the source rows and completes cleanup once");
    State cancelled;
    {
        auto next_user_context = global.make_user_context();
        auto nr = next_user_context->memory_resource();
        auto empty = RowsWork::Output::ok(nr, Vector<CompactRow>(nr));
        auto& pending = make_operation<RowsWork::Output, RowsWork>(nr, cancelled, empty);
        auto& parent = make_operation<Scalar::Output, Scalar>(nr, pending, "Record");
        check(!parent.poll(), "cancellation fixture must actually be pending");
    }
    check(cancelled.cancelled == 1 && !cancelled.wake,
          "actual Context deletion cancels suspended work before releasing its host memory");
}

static void scalar_failures() {
    for (unsigned kind = 0; kind < 7; ++kind) {
        GlobalContext global;
        State state;
        state.ready = true;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        Vector<CompactRow> rows(r);
        if (kind != 0) {
            rows.emplace_back();
            if (kind != 1) {
                Value count = kind == 2   ? Value(r)
                              : kind == 3 ? Value(r, -1)
                              : kind == 4 ? Value(r, "9")
                              : kind == 5 ? Value(r, true)
                                          : Value(r, 9.0);
                rows.back().insert("count", count);
            }
        }
        auto input_result = RowsWork::Output::ok(r, std::move(rows));
        auto& input = make_operation<RowsWork::Output, RowsWork>(r, state, input_result);
        using Scalar = detail::GeneratedScalarQueryOperation<String, true>;
        auto& count = make_operation<Scalar::Output, Scalar>(r, input, "Record");
        const auto& result = count.wait();
        check(
            !result && result.error().to_string(r) ==
                           "graph write error: count result for Record is missing or not numeric",
            "missing, negative, null, text, bool and floating count values fail with source Graph diagnostics");
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        pending_and_deleted_contexts();
        scalar_failures();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated execution Context contracts passed\n";
}
