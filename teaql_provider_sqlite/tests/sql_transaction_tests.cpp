#include "../../tests/support/sql_transaction_fixture.hpp"

static void commit_rollback_and_cache() {
    Fixture f;
    auto user = f.global.make_user_context();
    auto r = user->memory_resource();
    f.executor.query(r, fixture_query_request(r)).wait();
    auto plans = f.cache.cached_plans();
    auto& begin = f.executor.begin(r);
    check(f.controls.begins == 0, "begin is lazy");
    auto& tx = begin.wait().value().get();
    auto& concrete = dynamic_cast<SqlDataServiceTransaction<String>&>(tx);
    check(&concrete.state == &f.cache, "transaction borrows the executor's explicit shared cache");
    check(saved_name(tx.query(r, fixture_query_request(r)).wait().value()) == "old" &&
              f.cache.cached_plans() == plans,
          "transaction query reuses parent plan");
    const auto& result = tx.mutate(r, update(r, "committed")).wait().value();
    check(result.persisted_snapshot &&
              std::get<String>(result.persisted_snapshot->fields.at(String("name", r)).data) ==
                  "committed",
          "ordinary transaction mutation performs authoritative readback");
    auto& intent_query = user->make<SelectQuery>("Record");
    intent_query.filter(Expr::eq(r, "name", "explicit global diagnostic copy"));
    auto& intent = f.global.make<SqlIntentRedactions>(
        tx.query_log_intent(f.global.memory_resource(), intent_query));
    auto& terminal = tx.commit(r);
    check(tx.consumed() && concrete.transport.consumed() && f.controls.commits == 0,
          "terminal invocation consumes both interfaces before its lazy native effect");
    rejected(
        [&] {
            tx.query(r, fixture_query_request(r));
        },
        "consumed transaction rejects new query");
    check(bool(terminal.wait()), "commit result maps success");
    check(saved_name(f.executor.query(r, fixture_query_request(r)).wait().value()) == "committed",
          "commit persists SQLite changes");
    auto& rollback_tx = f.executor.begin(r).wait().value().get();
    rollback_tx.mutate(r, update(r, "rolled back")).wait();
    check(bool(rollback_tx.rollback(r).wait()), "rollback result maps success");
    check(saved_name(f.executor.query(r, fixture_query_request(r)).wait().value()) == "committed" &&
              f.controls.commits == 1 && f.controls.rollbacks == 1,
          "rollback undoes only its transaction");
    check(!tx.capabilities().transaction && tx.capabilities().batch_mutation,
          "capabilities match source even though begin SPI exists on outer executor");
    user.reset();
    Vector<String> secrets(f.global.memory_resource());
    intent.extend_secrets(false, secrets);
    check(
        secrets.size() == 1 && secrets[0] == "explicit global diagnostic copy",
        "pure intent metadata may be explicitly retained in GlobalContext after transaction Context closes");
}

static void pending_and_guard() {
    Fixture f;
    auto user = f.global.make_user_context();
    auto r = user->memory_resource();
    auto& tx = f.executor.begin(r).wait().value().get();
    auto& pending = tx.query(r, fixture_query_request(r));
    rejected(
        [&] {
            tx.commit(r);
        },
        "even an unpolled query keeps the transaction borrowed");
    check(!tx.consumed(), "pending rejection does not consume transaction");
    pending.cancel();
    f.transport.pause_read = true;
    auto& mutation = tx.mutate(r, update(r, "pending readback"));
    check(!mutation.poll(), "mutation waits after its write");
    rejected(
        [&] {
            tx.rollback(r);
        },
        "pending mutation readback prevents rollback");
    f.transport.pause_read = false;
    mutation.wait();
    auto& guarded = dynamic_cast<GuardedMutationExecutor<SqlExecutorError<String>>&>(tx);
    auto req = update(r, "guard rejected");
    auto& result =
        guarded
            .mutate_guarded(r,
                            GuardedMutationRequest(std::allocator_arg,
                                                   Allocator(r),
                                                   req,
                                                   Expr::eq(r, "tenant", std::uint64_t(99))))
            .wait()
            .value();
    check(result.affected_rows == 0, "transaction guarded path enforces guard atomically");
    auto other = f.global.make_user_context();
    rejected(
        [&] {
            tx.query(other->memory_resource(), fixture_query_request(other->memory_resource()));
        },
        "transaction cannot allocate work in another Context");
    rejected(
        [&] {
            other->make<TransactionHandle<SqlExecutorError<String>>>(tx);
        },
        "transaction handles cannot extend their Context through copying");
    check(bool(tx.rollback(r).wait()), "transaction usable after pending operation completes");
}

static void query_route_and_close() {
    Fixture f;
    auto user = f.global.make_user_context();
    auto r = user->memory_resource();
    auto& q = user->make<SelectQuery>("Record");
    q.comment("Exercise transaction query routing");
    q.purpose("Check partition execution and diagnostics");
    auto& values = user->make<Vector<Value>>();
    values.emplace_back(std::uint64_t(2));
    values.emplace_back(std::uint64_t(3));
    q.filter(Expr::in_list(r, "tenant", values)).partition_by("tenant").limit(1).order_asc("id");
    f.executor.query(r, QueryRequest(std::allocator_arg, Allocator(r), q, false, false)).wait();
    check(f.transport.reads == 2, "outer query uses small-parent repeated probes");
    auto& tx = f.executor.begin(r).wait().value().get();
    tx.query(r, QueryRequest(std::allocator_arg, Allocator(r), q, false, false)).wait();
    check(f.transport.reads == 3, "transaction preserves single partition query path");
    auto& events = f.global.make<Vector<ExecutionMetadata>>();
    f.transport.pause_read = true;
    auto& observer = user->make<ExecutionObserver>([&](ExecutionMetadata m) {
        check(f.transport.cancelled == 1 && f.controls.abandoned == 0,
              "pending transport stops before diagnostics and native transaction rollback");
        events.emplace_back(std::move(m));
    });
    check(!tx.mutate_observed(r, update(r, "abandoned"), observer).poll(),
          "close fixture waits in readback");
    user.reset();
    check(events.size() == 2 &&
              events[1].sql_log.execution_outcome == SqlExecutionOutcome::Cancelled &&
              f.controls.abandoned == 1,
          "context closes nested work, then diagnoses, then releases native transaction");
    f.transport.pause_read = false;
    auto next = f.global.make_user_context();
    check(saved_name(
              f.executor
                  .query(next->memory_resource(), fixture_query_request(next->memory_resource()))
                  .wait()
                  .value()) == "old",
          "abandoned native transaction rolls back before Context memory is reclaimed");
}

static void terminal_failures() {
    Fixture f;
    {
        auto user = f.global.make_user_context();
        auto r = user->memory_resource();
        f.controls.fail_begin = true;
        auto& failure = f.executor.begin(r).wait().error();
        check(failure.kind() == SqlExecutorError<String>::Kind::Transport &&
                  failure.transport_error() == "begin failure",
              "begin failure keeps typed provider error");
        f.controls.fail_begin = false;
    }
    {
        auto user = f.global.make_user_context();
        auto r = user->memory_resource();
        f.controls.pending_begin = true;
        auto& begin = f.executor.begin(r);
        check(!begin.poll(), "begin pending");
    }
    check(f.controls.cancelled_begin == 1, "closing Context cancels pending native begin");
    f.controls.pending_begin = false;
    {
        auto user = f.global.make_user_context();
        auto r = user->memory_resource();
        auto& tx = f.executor.begin(r).wait().value().get();
        f.controls.fail_terminal = true;
        auto& error = tx.commit(r).wait().error();
        check(error.transport_error() == "terminal failure" && tx.consumed(),
              "failed commit still consumes access");
        rejected(
            [&] {
                tx.rollback(r);
            },
            "failed commit cannot reuse transaction");
    }
    check(f.controls.abandoned == 1, "failed commit resource cleaned when Context closes");
    f.controls.fail_terminal = false;
    {
        auto user = f.global.make_user_context();
        auto r = user->memory_resource();
        auto& tx = f.executor.begin(r).wait().value().get();
        f.controls.pending_terminal = true;
        auto& commit = tx.commit(r);
        check(!commit.poll(), "commit pending");
        commit.cancel();
        check(f.controls.abandoned == 2, "cancelling terminal joins/releases provider work");
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        commit_rollback_and_cache();
        pending_and_guard();
        query_route_and_close();
        terminal_failures();
        std::pmr::set_default_resource(previous);
        std::cout << "SQL transaction execution contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
