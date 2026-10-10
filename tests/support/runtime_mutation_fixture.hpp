#pragma once
#include <teaql/runtime/data_service.hpp>

namespace mutation_fixture {
using namespace teaql;

struct State {
    using allocator_type = Allocator;
    Memory resource;
    Vector<String> notes;
    Vector<MutationRequest> requests;
    Vector<RawAuditEvent> events;
    Vector<QueryRequest> queries;
    Vector<Record> rows;
    Vector<Vector<Record>> read_batches;
    Vector<std::uint64_t> affected;
    int failure = -1, event_failure = -1;
    bool pending = false;
    bool allow_reads = false, read_pending = false, read_failure = false;
    int read_failure_index = -1;
    unsigned cancelled = 0;
    unsigned* external_cancel = nullptr;
    WakeSignal* child_wake = nullptr;

    State(std::allocator_arg_t, Allocator a)
        : resource(a.resource()), notes(resource), requests(resource), events(resource),
          queries(resource), rows(resource), read_batches(resource), affected(resource) {}

    Memory memory_resource() const {
        return resource;
    }

    void note(std::string_view text) {
        notes.emplace_back(text);
    }
};

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    State& s;

    explicit Executor(State& s) : s(s) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct ReadWork {
        using allocator_type = Allocator;
        Memory r;
        State& s;
        std::size_t index;

        ReadWork(std::allocator_arg_t, Allocator a, State& s, std::size_t index)
            : r(a.resource()), s(s), index(index) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal& wake) {
            s.child_wake = &wake;
            if (s.read_pending) {
                return {};
            }
            if (s.read_failure || s.read_failure_index == int(index)) {
                return Result<QueryResult, String>::err(r, String("read failed", r));
            }
            const auto& rows = index < s.read_batches.size() ? s.read_batches[index] : s.rows;
            QueryResult result(std::allocator_arg,
                               Allocator(r),
                               ExecutionMetadata::unrecorded_query(r, rows.size()));
            for (const auto& row : rows) {
                result.rows.emplace_back(row);
            }
            return Result<QueryResult, String>::ok(r, std::move(result));
        }

        void finish_and_wait() noexcept {
            s.child_wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            s.child_wake = nullptr;
            ++s.cancelled;
            if (s.external_cancel) {
                ++*s.external_cancel;
            }
        }
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        if (!s.allow_reads) {
            throw std::logic_error("audit mutation must not reread old values");
        }
        s.note("query");
        auto index = s.queries.size();
        s.queries.emplace_back(request);
        return make_operation<Result<QueryResult, String>, ReadWork>(r, s, index);
    }

    struct Work {
        using allocator_type = Allocator;
        Memory r;
        State& s;
        std::size_t index;
        std::optional<ExecutionObserver> observer;

        Work(std::allocator_arg_t,
             Allocator a,
             State& s,
             std::size_t i,
             const std::optional<ExecutionObserver>& observer)
            : r(a.resource()), s(s), index(i), observer(detail::clone_in(r, observer)) {}

        std::optional<Result<MutationResult, String>> poll(WakeSignal& wake) {
            s.child_wake = &wake;
            if (s.pending) {
                return {};
            }
            auto count = index < s.affected.size() ? s.affected[index] : 1;
            ExecutionMetadata meta(std::allocator_arg, Allocator(r), DataServiceOperation::Update);
            meta.backend = "fixture";
            meta.affected_rows = count;
            meta.parameterized_query.emplace("UPDATE fixture SET value = ?", r);
            if (observer) {
                s.note("observer");
                (*observer)(detail::clone_in(r, meta));
            }
            if (s.failure == int(index)) {
                return Result<MutationResult, String>::err(r, String("provider failed", r));
            }
            return Result<MutationResult, String>::ok(
                r, MutationResult(std::allocator_arg, Allocator(r), count, std::move(meta)));
        }

        void finish_and_wait() noexcept {
            s.child_wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            s.child_wake = nullptr;
            ++s.cancelled;
            if (s.external_cancel) {
                ++*s.external_cancel;
            }
        }
    };

    DataServiceOperationResult<MutationResult, String>& mutate(Memory r,
                                                               MutationRequest command) override {
        return mutate_observed(r, std::move(command), {});
    }

    DataServiceOperationResult<MutationResult, String>& mutate_observed(
        Memory r, MutationRequest command, std::optional<ExecutionObserver> observer) override {
        auto index = s.requests.size();
        s.requests.emplace_back(command);
        s.note("provider");
        return make_operation<Result<MutationResult, String>, Work>(r, s, index, observer);
    }
};

struct Sink final : RawAuditEventSink {
    State& s;

    explicit Sink(State& s) : s(s) {}

    Result<std::monostate, RuntimeError>
    on_event(Memory r, const UserContext&, const RawAuditEvent& event) const override {
        s.note("event");
        s.events.emplace_back(event);
        if (int(s.events.size() - 1) == s.event_failure) {
            return Result<std::monostate, RuntimeError>::err(
                r, RuntimeError::event(r, "event failed"));
        }
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

struct Cache final : AggregationCacheBackend {
    State& s;

    explicit Cache(State& s) : s(s) {}

    Memory memory_resource() const noexcept override {
        return s.resource;
    }

    std::string_view name_space() const noexcept override {
        return "fixture";
    }

    std::optional<Vector<CompactRow>> get(Memory, std::string_view, std::uint64_t) const override {
        return {};
    }

    void put(std::string_view, std::span<const CompactRow>) override {}

    void invalidate_namespace(std::string_view name) override {
        s.note(name);
    }
};
}
