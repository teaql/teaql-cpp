// All nine locked Rust 5.0.6 SQLite request trace tests; owner-thread overlap adaptation.
#include "fixture.hpp"
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/transaction.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <teaql/runtime/purposed_query_debug.hpp>
#include <iostream>
using namespace original_provider;

struct BatchFixture : Fixture {
    BatchFixture() {
        auto r = resource;
        EntityDescriptor entity(std::allocator_arg, Allocator(r), "BatchProbe");
        entity.table("batch_probe");
        entity.property("id", DataType::I64).id();
        entity.property("version", DataType::I64).version();
        entity.property("name", DataType::Text);
        entity.property("password", DataType::Text);
        entity.property("status", DataType::Text);
        entity.audit_mask_fields_declared = true;
        entity.audit_mask_fields.emplace_back("name");
        metadata.register_entity(std::move(entity));
        use_sqlite_provider(*user, *user, transport);
        check(bool(user->ensure_schema().wait()), "native schema setup");
        const auto& begun = executor.begin(r).wait();
        check(bool(begun), "begin seed transaction");
        auto& transaction = begun.value().get_as<MutationExecutor<Executor::Error>>();
        for (std::int64_t id : {9, 10}) {
            InsertCommand command(std::allocator_arg, Allocator(r), "BatchProbe");
            command.value("id", id)
                .value("version", std::int64_t(1))
                .value("name", "before")
                .value("password", "BEFORE-CREDENTIAL")
                .value("status", "ACTIVE");
            check(bool(transaction
                           .mutate(r,
                                   MutationRequest(std::allocator_arg,
                                                   Allocator(r),
                                                   std::move(command),
                                                   "seed native batch probes"))
                           .wait()),
                  "seed batch row");
        }
        check(bool(begun.value().get().commit(r).wait()), "commit seed");
        user->enable_all_sql_log();
        user->clear_sql_logs();
    }

    auto& service() {
        return EntityDataService<Executor>::for_executor(*user, "BatchProbe", *user, executor);
    }
};

static void derived_count() {
    BatchFixture fixture;
    auto r = fixture.resource;
    constexpr auto secret = "COUNT-PRIVATE-DESCENDANT";
    SelectQuery child(std::allocator_arg, Allocator(r), "BatchProbe");
    child.filter(Expr::eq(r, "name", secret));
    SelectQuery query(std::allocator_arg, Allocator(r), "BatchProbe");
    query.filter(Expr::eq(r, "status", "ACTIVE"))
        .relation_query("unused_relation", child)
        .limit(1)
        .comment(String("count ACTIVE including ", r) + secret);
    PurposedSelectQuery source(
        std::allocator_arg, Allocator(r), query, String("page ACTIVE with ", r) + secret);
    auto count = detail::clone_in(r, source);
    count.for_exact_count("count");
    check(debug_string(r, count).find("unused_relation") == String::npos &&
              count.as_query().relations.empty() && !count.as_query().slice,
          "removed relation and slice absent from count/debug");
    auto& repository = fixture.service();
    for (bool owned : {false, true}) {
        fixture.user->clear_sql_logs();
        auto& work = owned ? repository.fetch_all_owned(detail::clone_in(r, count))
                           : repository.fetch_all(count);
        const auto& rows = work.wait();
        check(bool(rows) && rows.value().size() == 1 &&
                  rows.value()[0].get("count")->try_u64() == 2,
              "borrowed/owned exact count");
        auto logs = fixture.user->sql_logs();
        check(logs.size() == 1, "removed relation classified without executing SQL");
        check(debug_string(r, logs[0]).find(secret) == String::npos,
              "removed descendant intent redacted");
        check(logs[0].comment && logs[0].purpose &&
                  logs[0].comment->find("ACTIVE") != String::npos &&
                  logs[0].purpose->find("ACTIVE") != String::npos,
              "ordinary intent remains visible");
        check(logs[0].params.size() == 1 && logs[0].params[0] == Value(r, "ACTIVE"),
              "unchanged physical count binding");
    }
    fixture.user->clear_sql_logs();
    SelectQuery independent(std::allocator_arg, Allocator(r), "BatchProbe");
    independent.limit(1).comment(secret);
    PurposedSelectQuery independent_request(
        std::allocator_arg, Allocator(r), independent, "independent query");
    check(bool(repository.fetch_all(independent_request).wait()), "independent query executes");
    check(fixture.user->sql_logs()[0].comment == secret, "no privacy state leaks to next query");
    fixture.user->clear_sql_logs();
    auto extracted = std::move(detail::clone_in(r, count)).into_query();
    PurposedSelectQuery legacy(std::allocator_arg, Allocator(r), extracted, "legacy probe");
    check(bool(repository.fetch_all(legacy).wait()), "legacy control executes");
    check(fixture.user->sql_logs()[0].comment->find(secret) != String::npos,
          "negative control proves discarded provenance leaks");
}

static void failed_count() {
    BatchFixture fixture;
    auto r = fixture.resource;
    fixture.exec("DROP TABLE batch_probe");
    constexpr auto secret = "FAILED-COUNT-PRIVATE";
    SelectQuery child(std::allocator_arg, Allocator(r), "BatchProbe");
    child.filter(Expr::eq(r, "password", secret));
    SelectQuery query(std::allocator_arg, Allocator(r), "BatchProbe");
    query.relation_query("unused", child).comment(String("count ", r) + secret);
    PurposedSelectQuery count(
        std::allocator_arg, Allocator(r), query, String("failure ", r) + secret);
    count.for_exact_count("count");
    check(!fixture.service().fetch_all(count).wait(), "real missing-table error");
    auto logs = fixture.user->sql_logs();
    check(logs.size() == 1 && logs[0].log_context.execution_outcome == SqlExecutionOutcome::Failure,
          "one failure log");
    check(debug_string(r, logs[0]).find(secret) == String::npos,
          "failed count masks removed descendant");
}

static void count_logging_off() {
    BatchFixture fixture;
    auto r = fixture.resource;
    fixture.user->disable_sql_log();
    SelectQuery query(std::allocator_arg, Allocator(r), "BatchProbe");
    PurposedSelectQuery missing(std::allocator_arg, Allocator(r), query, "missing comment probe");
    missing.for_exact_count("count");
    check(!fixture.service().fetch_all(missing).wait() && fixture.user->sql_logs().empty(),
          "logging off still rejects missing intent");
    query.comment("count with logging off");
    PurposedSelectQuery valid(
        std::allocator_arg, Allocator(r), query, "prove unchanged query semantics");
    valid.for_exact_count("count");
    const auto& rows = fixture.service().fetch_all(valid).wait();
    check(bool(rows) && rows.value()[0].get("count")->try_u64() == 2 &&
              fixture.user->sql_logs().empty(),
          "logging off preserves count semantics");
}

static auto& mutate(UserContext& user_context, MutationRequest request) {
    auto* owned = &user_context.make<MutationRequest>(std::move(request));
    return user_context.execute_in_transaction<Executor, MutationResult>(
        [owned](const TransactionScope<Executor>& scope) -> auto& {
            return scope.mutate(std::move(*owned));
        });
}

static auto& query_transaction(UserContext& user_context, SelectQuery query) {
    auto* owned = &user_context.make<QueryRequest>(query, false, true);
    return user_context.execute_in_transaction<Executor, QueryResult>(
        [owned](const TransactionScope<Executor>& scope) -> auto& {
            return scope.query(std::move(*owned));
        });
}

static void leaves(const ExecutionMetadata& metadata, Vector<const ExecutionMetadata*>& output) {
    if (metadata.statements.empty()) {
        output.push_back(&metadata);
    } else {
        for (const auto& statement : metadata.statements) {
            leaves(statement, output);
        }
    }
}

static void native_recovery() {
    Fixture fixture;
    auto& user_context = *fixture.user;
    auto r = fixture.resource;
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "RecoveryProbe");
    descriptor.table("recovery_probe");
    descriptor.property("id", DataType::I64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text);
    fixture.metadata.register_entity(std::move(descriptor));
    use_sqlite_provider(user_context, user_context, fixture.transport);
    check(bool(user_context.ensure_schema().wait()), "recovery schema");
    user_context.insert_resource(user_context, fixture.executor);
    user_context.enable_all_sql_log();
    auto& work = user_context.execute_in_transaction<Executor, MutationResult>(
        [](const TransactionScope<Executor>& scope) -> auto& {
            auto r = scope.context().memory_resource();
            InsertCommand insert(std::allocator_arg, Allocator(r), "RecoveryProbe");
            insert.value("id", std::int64_t(7))
                .value("version", std::int64_t(1))
                .value("name", "private recovery fixture");
            check(bool(scope
                           .mutate(MutationRequest(std::allocator_arg,
                                                   Allocator(r),
                                                   std::move(insert),
                                                   "seed recovery fixture"))
                           .wait()),
                  "recovery seed");
            DeleteCommand remove(
                std::allocator_arg, Allocator(r), "RecoveryProbe", std::int64_t(7));
            remove.expected_version(1);
            check(bool(scope
                           .mutate(MutationRequest(std::allocator_arg,
                                                   Allocator(r),
                                                   std::move(remove),
                                                   "archive recovery fixture"))
                           .wait()),
                  "archive");
            return scope.mutate(MutationRequest(
                std::allocator_arg,
                Allocator(r),
                RecoverCommand(
                    std::allocator_arg, Allocator(r), "RecoveryProbe", std::int64_t(7), -2),
                "restore archived object"));
        });
    const auto& result = work.wait();
    check(bool(result) && result.value().affected_rows == 1 &&
              result.value().metadata.operation == DataServiceOperation::Recover &&
              result.value().persisted_snapshot &&
              result.value().persisted_snapshot->fields.at(String("version", r)).try_i64() == 3,
          "native recover identity and version");
    auto logs = user_context.sql_logs();
    bool found = false;
    for (const auto& log : logs) {
        check(debug_string(r, log).find("private recovery fixture") == String::npos,
              "recovery safe logs");
        if (log.operation != SqlLogOperation::Recover) {
            continue;
        }
        found = true;
        check(log.audit_reason == "restore archived object" &&
                  log.trace_path.front().entity_type == "RecoveryProbe" &&
                  log.trace_path.back().entity_type == "recover",
              "recover operation and intent");
        for (const auto& node : log.trace_path) {
            check(node.kind != TraceKind::Comment && node.kind != TraceKind::Purpose &&
                      node.kind != TraceKind::AuditReason,
                  "safe recovery route");
        }
    }
    check(found, "Recover log exists");
    SelectQuery query(std::allocator_arg, Allocator(r), "RecoveryProbe");
    query.limit(1).comment("load restored object").purpose("verify recovered version");
    const auto& rows = query_transaction(user_context, std::move(query)).wait();
    check(bool(rows) && rows.value().rows.size() == 1, "read committed recovered row");
    found = false;
    for (const auto& log : user_context.sql_logs()) {
        if (log.purpose != "verify recovered version") {
            continue;
        }
        found = true;
        check(log.comment == "load restored object" &&
                  log.trace_path.front().entity_type == "RecoveryProbe" &&
                  log.trace_path.back().entity_type == "select",
              "native query intent");
    }
    check(found, "owned native query purpose reaches sink");
}

static void blank_tails(bool logging) {
    BatchFixture f;
    auto& user_context = *f.user;
    auto r = f.resource;
    user_context.insert_resource(user_context, f.executor);
    if (!logging) {
        user_context.disable_sql_log();
    }
    std::int64_t id = 100;
    for (auto kind : {TraceKind::Entity, TraceKind::Provider, TraceKind::Sql}) {
        user_context.clear_sql_logs();
        TraceNode tail(std::allocator_arg, Allocator(r), kind, "BatchProbe", "", std::uint64_t(id));
        InsertCommand command(std::allocator_arg, Allocator(r), "BatchProbe");
        command.value("id", id)
            .value("version", std::int64_t(1))
            .value("name", "native blank tail")
            .value("status", "ACTIVE");
        command.trace_chain.emplace_back(tail);
        MutationRequest request(
            std::allocator_arg, Allocator(r), std::move(command), " explicit root comment ");
        check(request.trace_chain().back() == tail, "request retains blank typed tail");
        auto original = detail::clone_in(r, request);
        const auto& result = mutate(user_context, std::move(request)).wait();
        check(bool(result) && original.comment() == " explicit root comment " &&
                  original.trace_chain().back() == tail && result.value().affected_rows == 1 &&
                  result.value().persisted_snapshot &&
                  result.value().persisted_snapshot->fields.at(String("id", r)).try_i64() == id,
              "owned request and persisted identity");
        Vector<const ExecutionMetadata*> physical(r);
        leaves(result.value().metadata, physical);
        check(physical.size() == 2 && physical[0]->operation == DataServiceOperation::Insert &&
                  physical[1]->operation == DataServiceOperation::Query,
              "real write/readback");
        for (const auto* statement : physical) {
            check(statement->comment == original.comment() &&
                      statement->sql_log.execution_outcome == SqlExecutionOutcome::Success &&
                      statement->parameterized_query && !statement->parameterized_query->empty(),
                  "physical metadata retains owned intent");
        }
        auto logs = user_context.sql_logs();
        check(logs.size() == (logging ? 2 : 0), "blank tail logging switch");
        for (const auto& log : logs) {
            check(log.audit_reason == original.comment() && log.comment == original.comment(),
                  "blank tail cannot erase intent");
        }
        SelectQuery query(std::allocator_arg, Allocator(r), "BatchProbe");
        query.filter(Expr::eq(r, "id", id))
            .limit(1)
            .comment("load committed blank-tail probe")
            .purpose("verify request-owned intent survives SQLite execution");
        const auto& rows = query_transaction(user_context, std::move(query)).wait();
        check(bool(rows) && rows.value().rows.size() == 1 &&
                  rows.value().rows[0].get("id")->try_i64() == id,
              "separate transaction sees committed row");
        ++id;
    }
}

static MutationRequest batch(Memory r, bool fail) {
    MutationRequest::Batch inner(r), outer(r);
    if (fail) {
        InsertCommand command(std::allocator_arg, Allocator(r), "BatchProbe");
        command.value("id", std::int64_t(10))
            .value("version", std::int64_t(1))
            .value("name", "Riverside")
            .value("password", "PASSWORD-CANARY")
            .value("status", "ACTIVE");
        inner.emplace_back(std::move(command), "update live probe");
    } else {
        UpdateCommand command(std::allocator_arg, Allocator(r), "BatchProbe", std::int64_t(10));
        command.expected_version(1)
            .value("name", "Riverside")
            .value("password", "PASSWORD-CANARY")
            .value("status", "ACTIVE");
        inner.emplace_back(std::move(command), "update live probe");
    }
    DeleteCommand remove(std::allocator_arg, Allocator(r), "BatchProbe", std::int64_t(9));
    remove.expected_version(1);
    outer.emplace_back(std::move(remove), "remove stale probe");
    outer.emplace_back(std::move(inner), "update current probes");
    return MutationRequest(std::allocator_arg,
                           Allocator(r),
                           std::move(outer),
                           "what: synchronize Riverside PASSWORD-CANARY while ACTIVE");
}

static void batch_cases(bool fail) {
    BatchFixture f;
    auto& user_context = *f.user;
    auto r = f.resource;
    user_context.insert_resource(user_context, f.executor);
    auto request = batch(r, fail);
    auto original = detail::clone_in(r, request);
    const auto& result = mutate(user_context, std::move(request)).wait();
    check(bool(result) == !fail, "native batch result");
    if (!fail) {
        check(result.value().affected_rows == 2 &&
                  result.value().metadata.comment == original.comment(),
              "root batch intent");
        Vector<const ExecutionMetadata*> physical(r);
        leaves(result.value().metadata, physical);
        check(physical.size() == 4, "two writes and readbacks");
        for (unsigned i = 0; i < 4; ++i) {
            const auto& statement = *physical[i];
            check(statement.comment == original.comment(), "root physical comment");
            check((statement.operation == DataServiceOperation::Query) == bool(i % 2),
                  "alternating writes and readbacks");
            if (i % 2) {
                check(statement.result_count == 1, "readback count");
            }
            Vector<String> expected(r), actual(r);
            expected.emplace_back(original.comment());
            if (i < 2) {
                expected.emplace_back("remove stale probe");
            } else {
                expected.emplace_back("update current probes");
                expected.emplace_back("update live probe");
            }
            for (const auto& node : statement.trace_chain) {
                if (node.kind == TraceKind::AuditReason) {
                    actual.emplace_back(node.comment);
                }
            }
            check(actual == expected, "physical nested audit lineage");
        }
        for (auto secret : {"Riverside", "PASSWORD-CANARY"}) {
            check(std::find(physical[2]->params.begin(),
                            physical[2]->params.end(),
                            Value(r, secret)) != physical[2]->params.end(),
                  "sensitive physical bindings unchanged");
        }
        const auto& children = std::get<MutationRequest::Batch>(original.data);
        check(children[0].comment() == "remove stale probe" &&
                  children[1].comment() == "update current probes",
              "caller child reasons intact");
    }
    auto logs = user_context.sql_logs();
    if (!fail) {
        check(logs.size() == 4 && std::count_if(logs.begin(),
                                                logs.end(),
                                                [](const auto& log) {
                                                    return log.operation == SqlLogOperation::Select;
                                                }) == 2,
              "one log per physical statement");
    }
    bool saw_delete = false, saw_failure = false;
    for (const auto& log : logs) {
        auto debug = debug_string(r, log);
        check(debug.find("Riverside") == String::npos &&
                  debug.find("PASSWORD-CANARY") == String::npos,
              "future sibling secrets excluded from every safe log");
        if (fail) {
            check(log.comment && log.comment->starts_with("what: synchronize"),
                  "failed batch root intent");
            saw_delete |= log.operation == SqlLogOperation::Delete;
            saw_failure |= log.operation == SqlLogOperation::Insert &&
                           log.log_context.execution_outcome == SqlExecutionOutcome::Failure;
            if (log.operation == SqlLogOperation::Select) {
                check(log.purpose == "verify the persisted mutation result",
                      "failure readback purpose");
            }
        } else {
            check(log.comment && log.comment->find("ACTIVE") != String::npos &&
                      log.trace_path.back().kind == TraceKind::Sql,
                  "plain intent and SQL route");
            for (const auto& node : log.trace_path) {
                check(node.kind != TraceKind::Comment && node.kind != TraceKind::Purpose &&
                          node.kind != TraceKind::AuditReason,
                      "safe batch route");
            }
        }
    }
    if (fail) {
        check(saw_delete && saw_failure, "earlier delete and failing insert both logged");
    }
    SelectQuery query(std::allocator_arg, Allocator(r), "BatchProbe");
    query.limit(10)
        .comment(fail ? "inspect rollback" : "load Riverside after batch")
        .purpose(fail ? "verify native batch rollback retained the prior version"
                      : "verify unchanged bindings and request isolation");
    const auto& rows = query_transaction(user_context, std::move(query)).wait();
    check(bool(rows), "post-transaction query");
    if (fail) {
        check(rows.value().rows.size() == 2, "rollback retains both rows");
        for (const auto& row : rows.value().rows) {
            check(row.get("version")->try_i64() == 1, "rollback retains version");
        }
    } else {
        auto found =
            std::find_if(rows.value().rows.begin(), rows.value().rows.end(), [](const auto& row) {
                return row.get("id")->try_i64() == 10;
            });
        check(found != rows.value().rows.end() && found->get("name")->try_text() == "Riverside" &&
                  found->get("password")->try_text() == "PASSWORD-CANARY" &&
                  found->get("version")->try_i64() == 2 &&
                  user_context.sql_logs().back().comment == "load Riverside after batch",
              "persisted values and subsequent request isolation");
    }
}

struct BatchGate {
    bool released = false;
    unsigned entered = 0;
    WakeSignal* wake = nullptr;

    void release() {
        released = true;
        if (wake) {
            wake->notify();
        }
    }
};

struct GatedMutation {
    using allocator_type = Allocator;
    using Output = Result<MutationResult, RuntimeError>;
    Memory resource;
    const TransactionScope<Executor>& scope;
    MutationRequest& request;
    BatchGate& gate;
    AsyncOperation<Output>* child = nullptr;

    GatedMutation(std::allocator_arg_t,
                  Allocator a,
                  const TransactionScope<Executor>& scope,
                  MutationRequest& request,
                  BatchGate& gate)
        : resource(a.resource()), scope(scope), request(request), gate(gate) {
        ++gate.entered;
    }

    std::optional<Output> poll(WakeSignal& wake) {
        gate.wake = &wake;
        if (!gate.released) {
            return {};
        }
        if (!child) {
            child = &scope.mutate(std::move(request));
        }
        if (const auto* result = child->poll()) {
            return detail::clone_in(resource, *result);
        }
        return {};
    }

    void finish_and_wait() noexcept {
        gate.wake = nullptr;
    }

    void cancel_and_wait() noexcept {
        if (child) {
            child->cancel();
        }
        gate.wake = nullptr;
    }
};

static void independent_batches() {
    BatchFixture f;
    auto& user_context = *f.user;
    auto r = f.resource;
    user_context.insert_resource(user_context, f.executor);
    auto* gate = &user_context.make<BatchGate>();
    auto start = [&](std::string_view label, std::int64_t id, std::string_view name) -> auto& {
        MutationRequest::Batch children(r);
        for (auto value : {id, id + 1}) {
            InsertCommand command(std::allocator_arg, Allocator(r), "BatchProbe");
            command.value("id", value)
                .value("version", std::int64_t(1))
                .value("name", name)
                .value("status", "ACTIVE");
            children.emplace_back(std::move(command),
                                  String("create ", r) + String(label, r) + " probe");
        }
        auto* request = &user_context.make<MutationRequest>(
            std::move(children),
            String("save native batch ", r) + String(label, r) + " " + String(name, r));
        return user_context.execute_in_transaction<Executor, MutationResult>(
            [request, gate](const TransactionScope<Executor>& scope) -> auto& {
                return make_operation<GatedMutation::Output, GatedMutation>(
                    scope.context().memory_resource(), scope, *request, *gate);
            });
    };
    auto& alpha = start("A", 20, "ALPHA-PRIVATE");
    auto& beta = start("B", 30, "BETA-PRIVATE");
    check(!alpha.poll(), "first real transaction remains pending at gate");
    check(!beta.poll(), "second transaction remains live while lease is owned");
    check(gate->entered == 1 && user_context.sql_logs().empty(),
          "second callback awaits native lease; neither request publishes yet");
    gate->release();
    auto validate = [&](const auto& result, std::string_view label, std::string_view name) {
        check(bool(result) && result.value().affected_rows == 2,
              "independent batch affects two rows");
        auto reason = String("save native batch ", r) + String(label, r) + " " + String(name, r);
        auto local = String("create ", r) + String(label, r) + " probe";
        Vector<const ExecutionMetadata*> physical(r);
        leaves(result.value().metadata, physical);
        for (const auto* leaf : physical) {
            check(leaf->comment == reason, "operation-local physical intent");
            Vector<String> lineage(r);
            for (const auto& node : leaf->trace_chain) {
                if (node.kind == TraceKind::AuditReason) {
                    lineage.emplace_back(node.comment);
                }
            }
            check(lineage.size() == 2 && lineage[0] == reason && lineage[1] == local,
                  "operation-local physical lineage");
        }
    };
    validate(alpha.wait(), "A", "ALPHA-PRIVATE");
    validate(beta.wait(), "B", "BETA-PRIVATE");
    check(gate->entered == 2, "both transaction callbacks execute once");
    auto logs = user_context.sql_logs();
    check(logs.size() == 8, "four writes and four readbacks");
    unsigned selects = 0, a = 0, b = 0;
    for (const auto& log : logs) {
        selects += log.operation == SqlLogOperation::Select;
        a += log.comment && log.comment->starts_with("save native batch A");
        b += log.comment && log.comment->starts_with("save native batch B");
        auto debug = debug_string(r, log);
        check(debug.find("ALPHA-PRIVATE") == String::npos &&
                  debug.find("BETA-PRIVATE") == String::npos,
              "independent safe logs redact both secrets");
    }
    check(selects == 4 && a == 4 && b == 4, "exact per-operation log counts");
    SelectQuery query(std::allocator_arg, Allocator(r), "BatchProbe");
    query.limit(10)
        .comment("inspect native batches")
        .purpose("verify both independent transactions committed");
    const auto& rows = query_transaction(user_context, std::move(query)).wait();
    check(bool(rows) && rows.value().rows.size() == 6, "both independent batches committed");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    derived_count();
    failed_count();
    count_logging_off();
    native_recovery();
    blank_tails(false);
    blank_tails(true);
    batch_cases(false);
    batch_cases(true);
    independent_batches();
    std::cout << "REQUEST_TRACE_SOURCE_TESTS=9;passed\n";
}
