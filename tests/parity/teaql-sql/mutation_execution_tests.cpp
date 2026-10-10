// All fifteen original mutation-execution diagnostic contracts.
#include "execution_fixture.hpp"
#include <iostream>

using namespace original_sql;
using Operation = DataServiceOperation;

static MutationRequest
mutation(Memory r, Operation operation, std::uint64_t id, std::int64_t recover_version = -1) {
    if (operation == Operation::Insert) {
        return insert(r, id);
    }
    if (operation == Operation::Update) {
        UpdateCommand command(std::allocator_arg, Allocator(r), "Customer", id);
        command.expected_version(1).value("name", "Riverside");
        return MutationRequest(std::allocator_arg, Allocator(r), std::move(command));
    }
    if (operation == Operation::Delete) {
        DeleteCommand command(std::allocator_arg, Allocator(r), "Customer", id);
        command.expected_version(1);
        return MutationRequest(std::allocator_arg, Allocator(r), std::move(command));
    }
    RecoverCommand command(std::allocator_arg, Allocator(r), "Customer", id, recover_version);
    return MutationRequest(std::allocator_arg, Allocator(r), std::move(command));
}

static void assert_write(const ExecutionMetadata& metadata,
                         Outcome outcome,
                         std::optional<std::uint64_t> affected) {
    check(
        metadata.operation == Operation::Insert && metadata.sql_log.execution_outcome == outcome &&
            metadata.affected_rows == affected && !metadata.debug_query &&
            std::ranges::find(metadata.sql_log.parameter_policies, SqlParameterLogPolicy::Masked) !=
                metadata.sql_log.parameter_policies.end() &&
            std::ranges::any_of(metadata.trace_chain,
                                [](const TraceNode& node) {
                                    return node.kind == TraceKind::AuditReason &&
                                           node.comment == "audited test";
                                }),
        "original insert outcome and provenance");
}

static void mutation_target_id_provenance() {
    for (bool failure : {false, true}) {
        for (auto operation :
             {Operation::Insert, Operation::Update, Operation::Delete, Operation::Recover}) {
            Fixture f(failure ? Mode::FailWrite : Mode::Success);
            auto request = mutation(f.resource, operation, 1001, -3);
            std::visit(
                [&](auto& command) {
                    if constexpr (!std::same_as<std::decay_t<decltype(command)>,
                                                MutationRequest::Batch>) {
                        command.trace_chain.emplace_back(
                            TraceKind::AuditReason, "Customer", "what: mutate customer 1001", 1001);
                    }
                },
                request.data);
            const auto& result =
                f.executor.mutate_observed(f.resource, std::move(request), f.observer).wait();
            check(bool(result) != failure, "all mutation target fixtures execute");
            if (failure) {
                check(f.entries.size() == 1, "failed target mutation has one outcome");
            } else {
                check(f.entries.empty(), "successful target mutation has no fallback notification");
            }
            const auto& write = failure ? f.entries[0] : result.value().metadata;
            check(std::ranges::find(write.params, Value(f.resource, std::uint64_t(1001))) !=
                          write.params.end() &&
                      std::ranges::any_of(write.trace_chain,
                                          [](const TraceNode& node) {
                                              return node.comment.find("1001") != String::npos;
                                          }),
                  "target remains plain in execution parameters and trace");
            Vector<String> secrets(f.resource);
            write.sql_log.intent_redactions.extend_secrets(false, secrets);
            check(std::ranges::find(secrets, String("1001", f.resource)) != secrets.end(),
                  "target ID is intent provenance");
            f.released_before_observers();
        }
    }
}

static SelectQuery intent_query(Memory r, std::string_view name, bool status) {
    SelectQuery query(std::allocator_arg, Allocator(r), "Customer");
    auto filter = Expr::eq(r, "name", name);
    if (status) {
        filter = filter.and_expr(Expr::eq(r, "status", "ACTIVE"));
    }
    query.filter(filter).limit(10).comment(status ? "what: compile cached intent"
                                                  : "what: transaction cached intent");
    return query;
}

static void cached_query_intent_uses_current_values() {
    Fixture f(Mode::FailRead);
    for (auto name : {"Riverside", "Lakeside"}) {
        auto source = f.executor.query_log_intent(f.resource, intent_query(f.resource, name, true));
        Vector<String> safe(f.resource), debug(f.resource);
        source.extend_secrets(false, safe);
        source.extend_secrets(true, debug);
        check(safe.size() == 1 && safe[0] == name && debug.empty(),
              "cached intent uses current sensitive binding");
    }
    check(f.cache.cached_plans() == 1 && f.controls.writes == 0 && f.controls.reads == 0 &&
              f.entries.empty(),
          "single cached select shape without transport IO or emission");
    f.released_before_observers();
}

static void transaction_cached_query_intent() {
    Fixture f(Mode::FailRead);
    auto& tx = f.begin();
    Vector<String> safe(f.resource), debug(f.resource);
    tx.query_log_intent(f.resource, intent_query(f.resource, "Riverside", false))
        .extend_secrets(false, safe);
    check(safe.size() == 1 && safe[0] == "Riverside", "transaction known intent");
    SelectQuery unknown(std::allocator_arg, Allocator(f.resource), "Unknown");
    unknown.filter(Expr::eq(f.resource, "field", "UNKNOWN-SECRET"))
        .limit(10)
        .comment("what: unavailable metadata");
    tx.query_log_intent(f.resource, unknown).extend_secrets(true, debug);
    check(debug.size() == 1 && debug[0] == "UNKNOWN-SECRET" && f.controls.writes == 0 &&
              f.controls.reads == 0,
          "unknown transaction metadata remains conservative and IO-free");
    f.released_before_observers();
    check(bool(tx.rollback(f.resource).wait()), "original cached-intent transaction rollback");
}

static void direct_write_failure() {
    Fixture f(Mode::FailWrite);
    const auto& result =
        f.executor.mutate_observed(f.resource, insert(f.resource, 1), f.observer).wait();
    check(!result && f.entries.size() == 1, "one failed write outcome");
    driver_error(result.error(), "original write error");
    assert_write(f.entries[0], Outcome::Failure, {});
    f.released_before_observers();
}

static void partial_batch_failure_and_cancellation() {
    for (auto mode : {Mode::FailWrite, Mode::PendingWrite}) {
        Fixture f(mode, mode == Mode::FailWrite, false, 2);
        auto& operation = f.executor.mutate_observed(f.resource, batch(f.resource), f.observer);
        if (mode == Mode::PendingWrite) {
            check(!operation.poll(), "second batch leaf pending");
            operation.cancel();
        } else {
            const auto& result = operation.wait();
            check(!result, "second batch leaf failed despite sink exception");
            driver_error(result.error(), "original write error");
        }
        check(f.entries.size() == 2 && f.controls.writes == 2,
              "two ordered leaves without third execution");
        assert_write(f.entries[0], Outcome::Success, 1);
        assert_write(
            f.entries[1], mode == Mode::PendingWrite ? Outcome::Cancelled : Outcome::Failure, {});
        f.released_before_observers();
    }
}

static void write_success_then_readback_outcomes() {
    for (auto mode : {Mode::FailRead, Mode::EmptyRead, Mode::PendingRead}) {
        Fixture f(mode);
        auto& tx = f.begin();
        auto& operation = tx.mutate_observed(f.resource, insert(f.resource, 1), f.observer);
        if (mode == Mode::PendingRead) {
            check(!operation.poll(), "write completed and readback pending");
            operation.cancel();
        } else {
            const auto& result = operation.wait();
            check(!result, "original readback contract failure");
            if (mode == Mode::FailRead) {
                driver_error(result.error(), "original readback error");
            } else {
                check(result.error().kind() == Error::Kind::PersistedRecord,
                      "empty successful SELECT is persisted-record failure");
            }
        }
        check(f.entries.size() == 2, "write and readback are two outcomes");
        assert_write(f.entries[0], Outcome::Success, 1);
        const auto& read = f.entries[1];
        auto expected = mode == Mode::FailRead    ? Outcome::Failure
                        : mode == Mode::EmptyRead ? Outcome::Success
                                                  : Outcome::Cancelled;
        check(read.operation == Operation::Query && read.sql_log.execution_outcome == expected &&
                  read.result_count ==
                      (mode == Mode::EmptyRead ? std::optional<std::size_t>(0) : std::nullopt),
              "readback outcome never relabels successful write");
        f.released_before_observers();
    }
}

static void successful_batch_and_later_compile_failure() {
    {
        Fixture f(Mode::Success);
        auto& tx = f.begin();
        const auto& result = tx.mutate_observed(f.resource, batch(f.resource), f.observer).wait();
        check(
            result && result.value().affected_rows == 3 &&
                result.value().metadata.statements.size() == 3 && f.entries.empty(),
            "successful transaction batch returns three metadata leaves without fallback notifications");
        f.released_before_observers();
    }
    {
        Fixture f(Mode::Success);
        MutationRequest::Batch children(f.resource);
        children.emplace_back(insert(f.resource, 1));
        InsertCommand missing(std::allocator_arg, Allocator(f.resource), "Missing");
        children.emplace_back(std::move(missing));
        auto request =
            MutationRequest(std::allocator_arg, Allocator(f.resource), std::move(children));
        const auto& result =
            f.executor.mutate_observed(f.resource, std::move(request), f.observer).wait();
        check(!result && result.error().kind() == Error::Kind::Compile && f.entries.size() == 1 &&
                  f.controls.writes == 1,
              "later compile failure invents no executed statement");
        assert_write(f.entries[0], Outcome::Success, 1);
        f.released_before_observers();
    }
}

static GuardedMutationRequest guarded(Fixture& f) {
    auto request = mutation(f.resource, Operation::Update, 1);
    return GuardedMutationRequest(std::allocator_arg,
                                  Allocator(f.resource),
                                  request,
                                  Expr::eq(f.resource, "status", "ACTIVE"));
}

static void guarded_readback_failure() {
    Fixture f(Mode::FailRead);
    const auto& result =
        f.executor.mutate_guarded_observed(f.resource, guarded(f), f.observer).wait();
    check(!result && f.entries.size() == 2, "guarded write/readback failure");
    driver_error(result.error(), "original readback error");
    check(f.entries[0].operation == Operation::Update &&
              f.entries[0].sql_log.execution_outcome == Outcome::Success &&
              f.entries[1].sql_log.execution_outcome == Outcome::Failure,
          "guarded statement outcomes");
    Vector<String> inherited(f.resource);
    f.entries[1].sql_log.intent_redactions.extend_secrets(false, inherited);
    check(std::ranges::find(inherited, String("Riverside", f.resource)) != inherited.end(),
          "readback inherited write intent");
    for (const auto& metadata : f.entries) {
        check(metadata.parameterized_query &&
                  metadata.parameterized_query->find("status") != String::npos &&
                  std::ranges::find(metadata.params, Value(f.resource, "ACTIVE")) !=
                      metadata.params.end(),
              "guard remains on both write and authoritative read");
    }
    f.released_before_observers();
}

static void guarded_transaction_write_failure() {
    Fixture f(Mode::FailWrite);
    const auto& begin = f.executor.begin(f.resource).wait();
    check(bool(begin), "begin guarded transaction");
    auto& tx = begin.value().get_as<GuardedMutationExecutor<Error>>();
    const auto& result = tx.mutate_guarded_observed(f.resource, guarded(f), f.observer).wait();
    check(!result && f.entries.size() == 1 && f.entries[0].operation == Operation::Update &&
              f.entries[0].sql_log.execution_outcome == Outcome::Failure &&
              !f.entries[0].affected_rows,
          "guarded transaction failed write has one outcome and unknown count");
    f.released_before_observers();
}

static void nested_batch_failure() {
    Fixture f(Mode::FailWrite, false, false, 2);
    MutationRequest::Batch inner(f.resource), outer(f.resource);
    inner.emplace_back(insert(f.resource, 2));
    inner.emplace_back(insert(f.resource, 3));
    outer.emplace_back(insert(f.resource, 1));
    outer.emplace_back(std::move(inner));
    auto request = MutationRequest(std::allocator_arg, Allocator(f.resource), std::move(outer));
    const auto& result =
        f.executor.mutate_observed(f.resource, std::move(request), f.observer).wait();
    check(!result && f.entries.size() == 2, "nested batch does not duplicate prior leaf");
    assert_write(f.entries[0], Outcome::Success, 1);
    assert_write(f.entries[1], Outcome::Failure, {});
    f.released_before_observers();
}

static void all_mutation_kinds_preserve_failure_kind() {
    for (auto operation :
         {Operation::Insert, Operation::Update, Operation::Delete, Operation::Recover}) {
        Fixture f(Mode::FailWrite);
        const auto& result =
            f.executor.mutate_observed(f.resource, mutation(f.resource, operation, 1), f.observer)
                .wait();
        check(!result && result.error().kind() == Error::Kind::Transport && f.entries.size() == 1 &&
                  f.entries[0].operation == operation &&
                  f.entries[0].sql_log.execution_outcome == Outcome::Failure &&
                  !f.entries[0].affected_rows,
              "all mutation variants preserve unknown failed affected count");
        f.released_before_observers();
    }
}

int main() {
    try {
        mutation_target_id_provenance();
        cached_query_intent_uses_current_values();
        transaction_cached_query_intent();
        direct_write_failure();
        partial_batch_failure_and_cancellation();
        write_success_then_readback_outcomes();
        successful_batch_and_later_compile_failure();
        guarded_readback_failure();
        guarded_transaction_write_failure();
        nested_batch_failure();
        all_mutation_kinds_preserve_failure_kind();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
