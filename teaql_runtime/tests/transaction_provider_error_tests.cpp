#include <teaql/runtime/transaction.hpp>
#include "../../tests/support/move_only_error_fixture.hpp"
#include <iostream>

using namespace teaql;

struct Counts {
    unsigned errors = 0, commits = 0, rollbacks = 0, abandoned = 0;
};
enum class Failure { Begin, Query, Mutation, Commit, Rollback };

template <class T>
struct NativeFailure {
    using allocator_type = Allocator;
    Memory owner;
    Counts& counts;
    const char* message;

    NativeFailure(std::allocator_arg_t, Allocator a, Counts& counts, const char* message)
        : owner(a.resource()), counts(counts), message(message) {}

    std::optional<Result<T, MoveOnlyError>> poll(WakeSignal&) {
        return Result<T, MoveOnlyError>::err(
            owner, MoveOnlyError(std::allocator_arg, Allocator(owner), message, counts.errors));
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct NativeTx : DataServiceTransaction<MoveOnlyError> {
    using allocator_type = Allocator;
    Counts& counts;
    Failure failure;
    bool active = true;

    NativeTx(std::allocator_arg_t, Allocator a, Counts& counts, Failure failure)
        : DataServiceTransaction<MoveOnlyError>(a.resource()), counts(counts), failure(failure) {}

    ~NativeTx() override {
        cancel();
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, MoveOnlyError>&
    query_transaction(Memory r, QueryRequest) override {
        return make_operation<Result<QueryResult, MoveOnlyError>, NativeFailure<QueryResult>>(
            r, counts, "query failed");
    }

    DataServiceOperationResult<MutationResult, MoveOnlyError>&
    mutate_transaction(Memory r, MutationRequest) override {
        return make_operation<Result<MutationResult, MoveOnlyError>, NativeFailure<MutationResult>>(
            r, counts, "mutation failed");
    }

    void cancel_transaction() noexcept override {
        if (active) {
            active = false;
            ++counts.abandoned;
        }
    }

    struct Terminal {
        using allocator_type = Allocator;
        Memory owner;
        NativeTx& tx;
        bool commit;

        Terminal(std::allocator_arg_t, Allocator a, NativeTx& tx, bool commit)
            : owner(a.resource()), tx(tx), commit(commit) {}

        std::optional<Result<std::monostate, MoveOnlyError>> poll(WakeSignal&) {
            if ((commit && tx.failure == Failure::Commit) ||
                (!commit && tx.failure == Failure::Rollback)) {
                return Result<std::monostate, MoveOnlyError>::err(
                    owner,
                    MoveOnlyError(std::allocator_arg,
                                  Allocator(owner),
                                  commit ? "commit failed" : "rollback failed",
                                  tx.counts.errors));
            }
            tx.active = false;
            return Result<std::monostate, MoveOnlyError>::ok(owner, {});
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            tx.cancel();
        }
    };

    DataServiceOperationResult<std::monostate, MoveOnlyError>&
    commit_transaction(Memory r) override {
        ++counts.commits;
        return make_operation<Result<std::monostate, MoveOnlyError>, Terminal>(r, *this, true);
    }

    DataServiceOperationResult<std::monostate, MoveOnlyError>&
    rollback_transaction(Memory r) override {
        ++counts.rollbacks;
        return make_operation<Result<std::monostate, MoveOnlyError>, Terminal>(r, *this, false);
    }
};

struct NativeProvider : TransactionExecutor<MoveOnlyError> {
    Counts& counts;
    Failure failure;

    NativeProvider(Counts& counts, Failure failure) : counts(counts), failure(failure) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Begin {
        using allocator_type = Allocator;
        Memory owner;
        NativeProvider& provider;

        Begin(std::allocator_arg_t, Allocator a, NativeProvider& provider)
            : owner(a.resource()), provider(provider) {}

        std::optional<Result<TransactionHandle<MoveOnlyError>, MoveOnlyError>> poll(WakeSignal&) {
            if (provider.failure == Failure::Begin) {
                return Result<TransactionHandle<MoveOnlyError>, MoveOnlyError>::err(
                    owner,
                    MoveOnlyError(std::allocator_arg,
                                  Allocator(owner),
                                  "begin failed",
                                  provider.counts.errors));
            }
            auto& tx = detail::async_arena(owner).make_resource<NativeTx>(provider.counts,
                                                                          provider.failure);
            return Result<TransactionHandle<MoveOnlyError>, MoveOnlyError>::ok(
                owner, TransactionHandle<MoveOnlyError>(std::allocator_arg, Allocator(owner), tx));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    DataServiceOperationResult<TransactionHandle<MoveOnlyError>, MoveOnlyError>&
    begin(Memory r) override {
        return make_operation<Result<TransactionHandle<MoveOnlyError>, MoveOnlyError>, Begin>(
            r, *this);
    }
};

struct Ready {
    using allocator_type = Allocator;
    Memory owner;
    bool fail;

    Ready(std::allocator_arg_t, Allocator a, bool fail) : owner(a.resource()), fail(fail) {}

    std::optional<Result<unsigned, RuntimeError>> poll(WakeSignal&) {
        if (fail) {
            return Result<unsigned, RuntimeError>::err(
                owner, RuntimeError::event(owner, "business failed"));
        }
        return Result<unsigned, RuntimeError>::ok(owner, 17);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (auto failure :
         {Failure::Begin, Failure::Query, Failure::Mutation, Failure::Commit, Failure::Rollback}) {
        GlobalContext global;
        auto user = global.make_user_context();
        Counts counts;
        auto executor = user->make_resource_ref<NativeProvider>(counts, failure);
        user->insert_resource(executor);
        using Output = Result<unsigned, RuntimeError>;
        auto& work = user->execute_in_transaction<NativeProvider, unsigned>(
            [failure](const TransactionScope<NativeProvider>& scope) -> AsyncOperation<Output>& {
                auto owner = scope.context().memory_resource();
                if (failure == Failure::Query) {
                    QueryRequest query(std::allocator_arg,
                                       Allocator(owner),
                                       SelectQuery(std::allocator_arg, Allocator(owner), "Item"),
                                       false,
                                       false);
                    const auto& result = scope.query(std::move(query)).wait();
                    check(!result &&
                              result.error().to_string(owner) == "transaction error: query failed",
                          "Move-only query error maps without copying");
                } else if (failure == Failure::Mutation) {
                    InsertCommand command(std::allocator_arg, Allocator(owner), "Item");
                    const auto& result =
                        scope
                            .mutate(MutationRequest(
                                std::allocator_arg, Allocator(owner), std::move(command)))
                            .wait();
                    check(!result && result.error().to_string(owner) ==
                                         "transaction error: mutation failed",
                          "Move-only mutation error maps without copying");
                }
                return make_operation<Output, Ready>(owner, failure == Failure::Rollback);
            });
        const auto& result = work.wait();
        check(bool(result) == (failure == Failure::Query || failure == Failure::Mutation),
              "Provider error mapping preserves scope completion behavior");
        if (failure == Failure::Rollback) {
            check(
                result.error().to_string(user->memory_resource()) ==
                    "transaction error: operation failed (entity event error: business failed); rollback also failed (rollback failed)",
                "Combined rollback error preserves both messages");
        }
        user.reset();
        check(counts.errors == 1 && global.live_user_bytes() == 0,
              "Each native move-only error is destroyed exactly once before User reclamation");
    }
    std::cout << "Move-only transaction provider error contracts pass\n";
}
