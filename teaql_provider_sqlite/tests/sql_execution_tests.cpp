#include "../../tests/support/sqlite_execution_fixture.hpp"
#include <iostream>

static void sqlite_execution() {
    GlobalContext global;
    auto& db = global.make_resource<Database>();
    auto& d = global.make_resource<SqliteDialect>();
    auto& e = model(global);
    auto& schema = global.make_resource<Schema>(e);
    auto& t = global.make<Transport>(db);
    auto& cache = global.make<SqlExecutorState>();
    auto& executor = global.make<SqlDataServiceExecutor<String>>(d, t, schema, cache);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& events = user->make<Vector<ExecutionMetadata>>();
    auto observer = ExecutionObserver(std::allocator_arg, Allocator(r), [&](ExecutionMetadata m) {
        events.emplace_back(std::move(m));
    });
    auto& inherited_privacy = user->make<SqlIntentRedactions>();
    inherited_privacy.capture_target_id(Value(r, "INHERITED-SECRET"));
    auto req = update(r);
    req.with_diagnostic_redactions(&inherited_privacy);
    std::get<UpdateCommand>(req.data).expected_version(1);
    auto& lazy = executor.mutate_observed(r, std::move(req), observer);
    check(t.writes == 0, "mutation must be lazy");
    const auto& ordinary = lazy.wait().value();
    check(ordinary.affected_rows == 1 && !ordinary.persisted_snapshot &&
              ordinary.generated_values.fields.empty() && t.reads == 0 && events.empty(),
          "ordinary mutation does not invent generated values or readback");
    check(ordinary.metadata.comment == "Update the fixture record" &&
              ordinary.metadata.trace_chain[0].kind == TraceKind::AuditReason &&
              ordinary.metadata.trace_chain[0].comment == "Update the fixture record",
          "SQL metadata materializes owned mutation intent rather than stale command annotations");
    auto guarded = [&](std::uint64_t tenant, std::int64_t version = 2) {
        auto req = update(r);
        req.with_diagnostic_redactions(&inherited_privacy);
        std::get<UpdateCommand>(req.data).expected_version(version);
        return GuardedMutationRequest(
            std::allocator_arg, Allocator(r), req, Expr::eq(r, "tenant", tenant));
    };
    const auto& denied = executor.mutate_guarded_observed(r, guarded(99), observer).wait().value();
    check(denied.affected_rows == 0 && !denied.persisted_snapshot && t.reads == 0,
          "guard is part of UPDATE and prevents readback when no row changed");
    const auto& success = executor.mutate_guarded_observed(r, guarded(2), observer).wait().value();
    check(success.persisted_snapshot &&
              success.persisted_snapshot->fields.at(String("version", r)) ==
                  Value(r, std::int64_t(3)) &&
              events.empty(),
          "guarded readback returns authoritative SQLite version and success journal is discarded");
    t.fail_read = true;
    auto& failed = executor.mutate_guarded_observed(r, guarded(2, 3), observer);
    check(failed.wait().error().kind() == SqlExecutorError<String>::Kind::Transport,
          "readback transport error stays typed");
    check(events.size() == 2 && events[0].operation == DataServiceOperation::Update &&
              events[0].sql_log.execution_outcome == SqlExecutionOutcome::Success &&
              events[1].operation == DataServiceOperation::Query &&
              events[1].sql_log.execution_outcome == SqlExecutionOutcome::Failure,
          "readback failure flushes successful write then failed query");
    Vector<String> secrets(r);
    events[1].sql_log.intent_redactions.extend_secrets(false, secrets);
    check(
        std::ranges::find(secrets, String("new", r)) != secrets.end() &&
            std::ranges::find(secrets, String("INHERITED-SECRET", r)) == secrets.end(),
        "guarded readback captures current bindings and omits inherited sidecars like the source");
    events.clear();
    auto ordinary_request = update(r);
    ordinary_request.with_diagnostic_redactions(&inherited_privacy);
    const auto& ordinary_failure =
        detail::execute_mutation(
            r, d, t, schema, cache, std::move(ordinary_request), true, observer)
            .wait();
    check(ordinary_failure.error().kind() == SqlExecutorError<String>::Kind::Transport &&
              events.size() == 2 && events[1].operation == DataServiceOperation::Query,
          "ordinary readback failure publishes the write and read diagnostics");
    secrets.clear();
    events[1].sql_log.intent_redactions.extend_secrets(false, secrets);
    check(std::ranges::find(secrets, String("new", r)) != secrets.end() &&
              std::ranges::find(secrets, String("INHERITED-SECRET", r)) != secrets.end(),
          "ordinary readback retains inherited sidecars alongside current binding policies");
    t.fail_read = false;
    events.clear();
    MutationRequest::Batch children(r);
    children.emplace_back(update(r, "batch first"));
    InsertCommand bad(std::allocator_arg, Allocator(r), "Unknown");
    children.emplace_back(std::move(bad), "Reject unknown entity");
    children.emplace_back(update(r, "never executed"));
    auto before = t.writes;
    auto batch = MutationRequest(
        std::allocator_arg, Allocator(r), std::move(children), "SQLite mutation contract");
    check(executor.mutate_observed(r, std::move(batch), observer).wait().error().kind() ==
                  SqlExecutorError<String>::Kind::Compile &&
              t.writes == before + 1 && events.size() == 1,
          "later compile failure flushes earlier leaf without starting following leaf");
    // The same kernel supports transaction-owned ordinary readback. The public
    // transaction wrapper is tested separately when that adapter is connected.
    InsertCommand insert(std::allocator_arg, Allocator(r), "Record");
    insert.value("id", std::uint64_t(8))
        .value("version", std::int64_t(1))
        .value("name", "inserted");
    auto request = MutationRequest(
        std::allocator_arg, Allocator(r), std::move(insert), "SQLite mutation contract");
    const auto& readback =
        detail::execute_mutation(
            r, d, t, schema, cache, std::move(request), true, std::optional<ExecutionObserver>{})
            .wait()
            .value();
    check(readback.persisted_snapshot && readback.persisted_snapshot->fields.at(
                                             String("tenant", r)) == Value(r, std::int64_t(2)),
          "readback includes database defaults, distinct from mutation input");
}

static void close_and_resume() {
    GlobalContext global;
    auto& db = global.make_resource<Database>();
    auto& d = global.make_resource<SqliteDialect>();
    auto& e = model(global);
    auto& schema = global.make_resource<Schema>(e);
    auto& t = global.make<Transport>(db);
    auto& cache = global.make<SqlExecutorState>();
    auto& executor = global.make<SqlDataServiceExecutor<String>>(d, t, schema, cache);
    auto& events = global.make<Vector<ExecutionMetadata>>();
    t.pause_read = true;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto observer =
            ExecutionObserver(std::allocator_arg, Allocator(r), [&](ExecutionMetadata m) {
                check(t.cancelled == 1, "close stops pending transport before delivering journal");
                events.emplace_back(std::move(m));
            });
        auto req = update(r);
        auto& pending = executor.mutate_guarded_observed(
            r,
            GuardedMutationRequest(
                std::allocator_arg, Allocator(r), req, Expr::eq(r, "tenant", std::uint64_t(2))),
            observer);
        check(!pending.poll() && t.writes == 1 && t.reads == 1 && events.empty(),
              "success journal waits while readback is pending");
    }
    check(events.size() == 2 &&
              events[0].sql_log.execution_outcome == SqlExecutionOutcome::Success &&
              events[1].sql_log.execution_outcome == SqlExecutionOutcome::Cancelled,
          "Context close emits each statement once before reclamation");
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    DataServiceOperationResult<MutationResult, SqlExecutorError<String>>* pending = nullptr;
    {
        auto source = global.make_user_context();
        auto sr = source->memory_resource();
        auto req = update(sr, "copied across source Context");
        pending = &executor.mutate_guarded(
            r,
            GuardedMutationRequest(
                std::allocator_arg, Allocator(sr), req, Expr::eq(sr, "tenant", std::uint64_t(2))));
    }
    check(!pending->poll(), "request deep copy survives source close and waits on readback");
    auto writes = t.writes;
    t.pause_read = false;
    const auto& result = pending->wait().value();
    check(t.writes == writes && result.persisted_snapshot,
          "resuming readback never re-executes the write");
    auto& retained = global.make<MutationResult>(result);
    user.reset();
    check(retained.persisted_snapshot->fields.at(String("name", global.memory_resource())) ==
              Value(global.memory_resource(), "copied across source Context"),
          "explicit result copy preserves snapshot across destination close");
}

static void batch_lineage_and_future_privacy() {
    GlobalContext global;
    auto& db = global.make_resource<Database>();
    auto& dialect = global.make_resource<SqliteDialect>();
    auto& entity = model(global);
    auto& schema = global.make_resource<Schema>(entity);
    auto& transport = global.make<Transport>(db);
    auto& cache = global.make<SqlExecutorState>();
    auto& executor = global.make<SqlDataServiceExecutor<String>>(dialect, transport, schema, cache);
    ExecutionMetadata* retained = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        for (unsigned mode = 0; mode < 4; ++mode) {
            const auto middle = mode == 1 ? "root" : "middle";
            const auto local = mode < 2 ? "root" : (mode == 2 ? "middle" : "leaf");
            UpdateCommand command(std::allocator_arg, Allocator(r), "Record", std::uint64_t{7});
            command.value("name", "nested");
            command.trace_chain.emplace_back(TraceKind::AuditReason, "Record", "old annotation", 7);
            MutationRequest::Batch children(r), outer(r);
            children.emplace_back(command, local);
            outer.emplace_back(std::move(children), middle);
            MutationRequest request(std::allocator_arg, Allocator(r), std::move(outer), "root");
            const auto& result = executor.mutate(r, std::move(request)).wait().value();
            const auto& nested = result.metadata.statements[0];
            const auto& leaf = nested.statements[0];
            check(result.metadata.comment == "root" && nested.comment == "root" &&
                      leaf.comment == "root",
                  "top-level intent remains the metadata comment at every native batch level");
            const auto count = mode == 1 ? 1u : (mode == 3 ? 3u : 2u);
            check(leaf.trace_chain.size() == count && leaf.trace_chain[0].comment == "root",
                  "adjacent repeated batch scope is coalesced without dropping distinct ancestry");
            const auto identity_index = mode < 2 ? 0u : (mode == 2 ? 1u : 2u);
            check(
                leaf.trace_chain[identity_index].entity_id == 7 &&
                    command.trace_chain[0].comment == "old annotation",
                "the repeated request reason retains its real item ID without mutating command frames");
        }
        UpdateCommand with_readback(std::allocator_arg, Allocator(r), "Record", std::uint64_t{7});
        with_readback.value("name", "readback pair");
        MutationRequest::Batch read_children(r);
        read_children.emplace_back(with_readback, "read child");
        MutationRequest read_root(
            std::allocator_arg, Allocator(r), std::move(read_children), "read root");
        const auto& saved = detail::execute_mutation(r,
                                                     dialect,
                                                     transport,
                                                     schema,
                                                     cache,
                                                     std::move(read_root),
                                                     true,
                                                     std::optional<ExecutionObserver>{})
                                .wait()
                                .value();
        const auto& summary = saved.metadata.statements[0];
        check(
            summary.statements.size() == 2 &&
                summary.statements[0].operation == DataServiceOperation::Update &&
                summary.statements[1].operation == DataServiceOperation::Query &&
                summary.statements[1].result_count == 1 &&
                summary.statements[0].statements.empty() &&
                summary.statements[1].statements.empty() &&
                summary.statements[1].trace_chain[1].comment ==
                    "verify the persisted mutation result",
            "native batch items retain their ordered physical write/readback pair without an observer");
        auto& events = user_context->make<Vector<ExecutionMetadata>>();
        auto observer =
            ExecutionObserver(std::allocator_arg, Allocator(r), [&](ExecutionMetadata metadata) {
                events.emplace_back(std::move(metadata));
            });
        transport.fail_read = true;
        auto before = transport.writes;
        UpdateCommand future(std::allocator_arg, Allocator(r), "Record", std::uint64_t{8});
        future.value("tenant", "PLAIN-FUTURE");
        future.old_values.emplace(r);
        future.old_values->fields.emplace(String("name", r), Value(r, "OLD-NAME-FUTURE"));
        future.old_values->fields.emplace(String("api_token", r),
                                          Value(r, "OLD-CREDENTIAL-FUTURE"));
        MutationRequest::Batch children(r);
        children.emplace_back(update(r));
        children.emplace_back(future, "future child");
        MutationRequest request(std::allocator_arg,
                                Allocator(r),
                                std::move(children),
                                "root OLD-NAME-FUTURE OLD-CREDENTIAL-FUTURE PLAIN-FUTURE");
        const auto& failed =
            detail::execute_mutation(r,
                                     dialect,
                                     transport,
                                     schema,
                                     cache,
                                     std::move(request),
                                     true,
                                     std::optional<ExecutionObserver>(std::move(observer)))
                .wait();
        check(
            !failed && transport.writes == before + 1 && events.size() == 2,
            "privacy precollection does not execute or compile a future sibling before first readback failure");
        for (const auto& metadata : events) {
            Vector<String> safe(r), debug(r);
            metadata.sql_log.intent_redactions.extend_secrets(false, safe);
            metadata.sql_log.intent_redactions.extend_secrets(true, debug);
            auto contains = [&](const auto& values, std::string_view value) {
                return std::any_of(values.begin(), values.end(), [&](const auto& item) {
                    return item == value;
                });
            };
            check(
                contains(safe, "OLD-NAME-FUTURE") && contains(safe, "OLD-CREDENTIAL-FUTURE") &&
                    !contains(safe, "PLAIN-FUTURE") && !contains(debug, "OLD-NAME-FUTURE") &&
                    contains(debug, "OLD-CREDENTIAL-FUTURE"),
                "complete future-sibling old values retain masked/plain/credential provenance before physical I/O");
        }
        check(events[1].trace_chain[0].kind == TraceKind::Comment &&
                  events[1].trace_chain[1].kind == TraceKind::Purpose &&
                  events[1].trace_chain[1].comment == "verify the persisted mutation result",
              "readback carries canonical derived-query intent before statement lineage");
        transport.fail_read = false;
        retained = &global.make<ExecutionMetadata>(events[1]);
    }
    Vector<String> secrets(global.memory_resource());
    retained->sql_log.intent_redactions.extend_secrets(false, secrets);
    check(
        retained->comment->find("OLD-CREDENTIAL-FUTURE") != String::npos && !secrets.empty(),
        "explicit Global batch diagnostics retain their canonical strings and privacy after User deletion");
}

static void moved_requests_reject_before_sql() {
    struct CountingSchema final : SchemaProvider {
        const EntityDescriptor& entity;
        mutable std::size_t lookups = 0;

        explicit CountingSchema(const EntityDescriptor& entity) : entity(entity) {}

        const EntityDescriptor* get_entity(std::string_view name) const override {
            ++lookups;
            return name == entity.name ? &entity : nullptr;
        }
    };

    GlobalContext global;
    auto& db = global.make_resource<Database>();
    auto& dialect = global.make_resource<SqliteDialect>();
    auto& entity = model(global);
    auto& schema = global.make_resource<CountingSchema>(entity);
    auto& transport = global.make<Transport>(db);
    auto& cache = global.make<SqlExecutorState>();
    auto& executor = global.make<SqlDataServiceExecutor<String>>(dialect, transport, schema, cache);
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& query = user_context->make<SelectQuery>("Record");
    query.comment("entry").purpose("inspect");
    auto query_request = QueryRequest::from_query(r, query);
    auto consumed_query_intent = std::move(query_request.value().intent);
    bool query_rejected = false;
    try {
        executor.query(r, std::move(query_request).value()).wait();
    } catch (const std::invalid_argument&) {
        query_rejected = true;
    }
    InsertCommand insert(std::allocator_arg, Allocator(r), "Record");
    insert.value("id", std::uint64_t{7});
    auto request = MutationRequest::create(r, insert, "write entry");
    auto consumed_request = std::move(request).value();
    bool write_rejected = false;
    try {
        executor.mutate(r, std::move(request).value()).wait();
    } catch (const std::invalid_argument&) {
        write_rejected = true;
    }
    MutationRequest::Batch children(r);
    children.emplace_back(insert, "first");
    children.emplace_back(insert, "second");
    auto batch = MutationRequest::create(r, children, "batch entry");
    auto& stored = std::get<MutationRequest::Batch>(batch.value().data);
    auto consumed_second = std::move(stored[1]);
    bool batch_rejected = false;
    try {
        executor.mutate(r, std::move(batch).value()).wait();
    } catch (const std::invalid_argument&) {
        batch_rejected = true;
    }
    check(
        query_rejected && write_rejected && batch_rejected && schema.lookups == 0 &&
            transport.writes == 0 && transport.reads == 0,
        "consumed query/mutation intents and later invalid children reject before schema lookup or physical SQL");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        sqlite_execution();
        moved_requests_reject_before_sql();
        batch_lineage_and_future_privacy();
        close_and_resume();
        std::pmr::set_default_resource(previous);
        std::cout << "SQL mutation execution contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
