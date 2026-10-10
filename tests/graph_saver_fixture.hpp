#pragma once
#include "runtime_mutation_fixture.hpp"

namespace saver_fixture {
using namespace teaql;

struct Controls {
    bool pending_begin = false, pending_terminal = false;
    bool fail_begin = false, fail_commit = false, fail_rollback = false;
    unsigned abandoned = 0, cancelled_begin = 0;
};

struct Transaction final : DataServiceTransaction<String> {
    using allocator_type = Allocator;
    mutation_fixture::State& state;
    Controls& controls;
    mutation_fixture::Executor inner;
    bool active = true;

    Transaction(std::allocator_arg_t,
                Allocator a,
                mutation_fixture::State& state,
                Controls& controls)
        : DataServiceTransaction<String>(a.resource()), state(state), controls(controls),
          inner(state) {}

    DataServiceCapabilities capabilities() const override {
        return inner.capabilities();
    }

    struct Terminal {
        using allocator_type = Allocator;
        Memory r;
        Transaction& transaction;
        bool commit;

        Terminal(std::allocator_arg_t, Allocator a, Transaction& transaction, bool commit)
            : r(a.resource()), transaction(transaction), commit(commit) {}

        std::optional<Result<std::monostate, String>> poll(WakeSignal&) {
            if (transaction.controls.pending_terminal) {
                return {};
            }
            bool fail =
                commit ? transaction.controls.fail_commit : transaction.controls.fail_rollback;
            if (fail) {
                return Result<std::monostate, String>::err(
                    r, String(commit ? "commit failed" : "rollback failed", r));
            }
            transaction.active = false;
            return Result<std::monostate, String>::ok(r, {});
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            transaction.cancel();
        }
    };

protected:
    void cancel_transaction() noexcept override {
        if (active) {
            active = false;
            ++controls.abandoned;
        }
    }

    DataServiceOperationResult<QueryResult, String>& query_transaction(Memory r,
                                                                       QueryRequest q) override {
        state.note("tx");
        return inner.query(r, std::move(q));
    }

    DataServiceOperationResult<MutationResult, String>&
    mutate_transaction(Memory r, MutationRequest q) override {
        return inner.mutate(r, std::move(q));
    }

    DataServiceOperationResult<std::monostate, String>& commit_transaction(Memory r) override {
        state.note("commit");
        return make_operation<Result<std::monostate, String>, Terminal>(r, *this, true);
    }

    DataServiceOperationResult<std::monostate, String>& rollback_transaction(Memory r) override {
        state.note("rollback");
        return make_operation<Result<std::monostate, String>, Terminal>(r, *this, false);
    }
};

struct Ambient final : QueryExecutor<String>,
                       MutationExecutor<String>,
                       TransactionExecutor<String> {
    mutation_fixture::State& state;
    Controls& controls;
    mutation_fixture::Executor inner;
    Vector<Record> ambient_rows;
    Transaction* transaction = nullptr;

    Ambient(mutation_fixture::State& state, Controls& controls)
        : state(state), controls(controls), inner(state), ambient_rows(state.memory_resource()) {}

    DataServiceCapabilities capabilities() const override {
        return inner.capabilities();
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory r, QueryRequest q) override {
        state.note("ambient");
        auto index = state.queries.size();
        if (index >= state.read_batches.size()) {
            state.read_batches.resize(index + 1);
            for (const auto& row : ambient_rows) {
                state.read_batches[index].emplace_back(row);
            }
        }
        return inner.query(r, std::move(q));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("transactional saver must not mutate on the ambient executor");
    }

    struct Begin {
        using allocator_type = Allocator;
        Memory r;
        Ambient& owner;

        Begin(std::allocator_arg_t, Allocator a, Ambient& owner) : r(a.resource()), owner(owner) {}

        std::optional<Result<TransactionHandle<String>, String>> poll(WakeSignal&) {
            if (owner.controls.pending_begin) {
                return {};
            }
            if (owner.controls.fail_begin) {
                return Result<TransactionHandle<String>, String>::err(r, String("begin failed", r));
            }
            owner.transaction =
                &detail::async_arena(r).make_resource<Transaction>(owner.state, owner.controls);
            return Result<TransactionHandle<String>, String>::ok(
                r, TransactionHandle<String>(std::allocator_arg, Allocator(r), *owner.transaction));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            ++owner.controls.cancelled_begin;
        }
    };

    DataServiceOperationResult<TransactionHandle<String>, String>& begin(Memory r) override {
        state.note("begin");
        return make_operation<Result<TransactionHandle<String>, String>, Begin>(r, *this);
    }
};
}
