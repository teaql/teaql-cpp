#pragma once
#include <teaql/runtime/data_service.hpp>
#include "move_only_error_fixture.hpp"
#include "relation_hydration_fixture.hpp"

namespace completion_fixture {
using Model = hydration_fixture::Model<0>;

struct Counts {
    unsigned queries = 0, mutations = 0, begins = 0, commits = 0, rollbacks = 0;
    unsigned produced = 0, destroyed = 0, finished = 0, cancelled = 0, abandoned = 0;
    WakeSignal* wake = nullptr;
    bool scope_seen = false, cancel_scope_seen = false;
};

struct Controls {
    unsigned fail_query = 0, fail_mutation = 0;
    bool fail_begin = false, fail_commit = false, fail_rollback = false, pending = false;
    unsigned pending_query = 0, pending_mutation = 0;
    bool pending_begin = false, pending_commit = false, pending_rollback = false;

    bool has_pending() const {
        return pending || pending_query || pending_mutation || pending_begin || pending_commit ||
               pending_rollback;
    }
};

template <class T>
Result<T, MoveOnlyError> failure(Memory r, Counts& counts, std::string_view label = {}) {
    ++counts.produced;
    String text(4096, 'x', r);
    text.replace(0, 14, "native failure");
    if (!label.empty()) {
        text.replace(15, label.size(), label);
    }
    return Result<T, MoveOnlyError>::err(
        r, MoveOnlyError(std::allocator_arg, Allocator(r), text, counts.destroyed));
}

template <class T>
struct Work {
    using allocator_type = Allocator;
    Memory resource;
    Counts& counts;
    Controls& controls;
    bool fail;
    T value;
    std::string_view label;
    bool pending;

    Work(std::allocator_arg_t,
         Allocator a,
         Counts& counts,
         Controls& controls,
         bool fail,
         T value,
         std::string_view label = {},
         bool pending = false)
        : resource(a.resource()), counts(counts), controls(controls), fail(fail),
          value(detail::move_in(a.resource(), std::move(value))), label(label), pending(pending) {}

    std::optional<Result<T, MoveOnlyError>> poll(WakeSignal& wake) {
        using R = Result<T, MoveOnlyError>;
        if (controls.pending || pending) {
            counts.wake = &wake;
            return {};
        }
        if (fail) {
            return failure<T>(resource, counts, label);
        }
        return R::ok(resource, std::move(value));
    }

    void finish_and_wait() noexcept {
        ++counts.finished;
        counts.wake = nullptr;
    }

    void cancel_and_wait() noexcept {
        ++counts.cancelled;
        counts.wake = nullptr;
    }
};

inline auto& query(Memory r, Counts& counts, Controls& controls) {
    const bool fail = ++counts.queries == controls.fail_query;
    auto metadata = ExecutionMetadata::unrecorded_query(r, 1);
    QueryResult result(std::allocator_arg, Allocator(r), std::move(metadata));
    auto& row = result.rows.emplace_back();
    row.insert("id", Value(r, std::uint64_t{7}));
    row.insert("version", Value(r, std::int64_t{3}));
    row.insert("name", Value(r, "stored"));
    return make_operation<Result<QueryResult, MoveOnlyError>, Work<QueryResult>>(
        r,
        counts,
        controls,
        fail,
        std::move(result),
        std::string_view("query"),
        counts.queries == controls.pending_query);
}

inline auto& mutate(Memory r, Counts& counts, Controls& controls) {
    const bool fail = ++counts.mutations == controls.fail_mutation;
    ExecutionMetadata metadata(std::allocator_arg, Allocator(r), DataServiceOperation::Update);
    MutationResult result(std::allocator_arg, Allocator(r), 1, std::move(metadata));
    return make_operation<Result<MutationResult, MoveOnlyError>, Work<MutationResult>>(
        r,
        counts,
        controls,
        fail,
        std::move(result),
        std::string_view("mutation"),
        counts.mutations == controls.pending_mutation);
}

struct Transaction final : DataServiceTransaction<MoveOnlyError> {
    using allocator_type = Allocator;
    Counts& counts;
    Controls& controls;
    bool active = true;

    Transaction(std::allocator_arg_t, Allocator a, Counts& counts, Controls& controls)
        : DataServiceTransaction<MoveOnlyError>(a.resource()), counts(counts), controls(controls) {}

    ~Transaction() override {
        cancel();
    }

    DataServiceCapabilities capabilities() const override {
        return {.query = true, .mutation = true};
    }

protected:
    DataServiceOperationResult<QueryResult, MoveOnlyError>&
    query_transaction(Memory r, QueryRequest) override {
        return completion_fixture::query(r, counts, controls);
    }

    DataServiceOperationResult<MutationResult, MoveOnlyError>&
    mutate_transaction(Memory r, MutationRequest) override {
        return completion_fixture::mutate(r, counts, controls);
    }

    DataServiceOperationResult<std::monostate, MoveOnlyError>&
    commit_transaction(Memory r) override {
        ++counts.commits;
        active = false;
        return make_operation<Result<std::monostate, MoveOnlyError>, Work<std::monostate>>(
            r,
            counts,
            controls,
            controls.fail_commit,
            std::monostate{},
            std::string_view("commit"),
            controls.pending_commit);
    }

    DataServiceOperationResult<std::monostate, MoveOnlyError>&
    rollback_transaction(Memory r) override {
        ++counts.rollbacks;
        active = false;
        return make_operation<Result<std::monostate, MoveOnlyError>, Work<std::monostate>>(
            r,
            counts,
            controls,
            controls.fail_rollback,
            std::monostate{},
            std::string_view("rollback"),
            controls.pending_rollback);
    }

    void cancel_transaction() noexcept override {
        if (active) {
            active = false;
            ++counts.abandoned;
        }
    }
};

struct Executor final : QueryExecutor<MoveOnlyError>,
                        MutationExecutor<MoveOnlyError>,
                        TransactionExecutor<MoveOnlyError> {
    using allocator_type = Allocator;
    Memory resource;
    Counts& counts;
    Controls& controls;

    Executor(std::allocator_arg_t, Allocator a, Counts& counts, Controls& controls)
        : resource(a.resource()), counts(counts), controls(controls) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {.query = true, .mutation = true, .transaction = true};
    }

    DataServiceOperationResult<QueryResult, MoveOnlyError>& query(Memory r, QueryRequest) override {
        return completion_fixture::query(r, counts, controls);
    }

    DataServiceOperationResult<MutationResult, MoveOnlyError>& mutate(Memory r,
                                                                      MutationRequest) override {
        return completion_fixture::mutate(r, counts, controls);
    }

    struct Begin {
        using allocator_type = Allocator;
        Memory resource;
        Executor& executor;

        Begin(std::allocator_arg_t, Allocator a, Executor& executor)
            : resource(a.resource()), executor(executor) {}

        std::optional<Result<TransactionHandle<MoveOnlyError>, MoveOnlyError>>
        poll(WakeSignal& wake) {
            if (executor.controls.pending || executor.controls.pending_begin) {
                executor.counts.wake = &wake;
                return {};
            }
            if (executor.controls.fail_begin) {
                return failure<TransactionHandle<MoveOnlyError>>(
                    resource, executor.counts, "begin");
            }
            auto& transaction = detail::async_arena(resource).make_resource<Transaction>(
                executor.counts, executor.controls);
            return Result<TransactionHandle<MoveOnlyError>, MoveOnlyError>::ok(
                resource,
                TransactionHandle<MoveOnlyError>(
                    std::allocator_arg, Allocator(resource), transaction));
        }

        void finish_and_wait() noexcept {
            ++executor.counts.finished;
            executor.counts.wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            ++executor.counts.cancelled;
            executor.counts.wake = nullptr;
        }
    };

    DataServiceOperationResult<TransactionHandle<MoveOnlyError>, MoveOnlyError>&
    begin(Memory r) override {
        ++counts.begins;
        return make_operation<Result<TransactionHandle<MoveOnlyError>, MoveOnlyError>, Begin>(
            r, *this);
    }
};

inline void metadata(UserContext& user) {
    auto r = user.memory_resource();
    user.disable_sql_log();
    auto& store = user.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Parent");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text);
    store.register_entity(std::move(descriptor));
    user.set_metadata(user, store);
}
}
