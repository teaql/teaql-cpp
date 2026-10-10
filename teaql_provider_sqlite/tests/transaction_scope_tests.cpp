#include <teaql/runtime/transaction.hpp>
#include "../../tests/support/audited_save_fixture.hpp"
#include "../../tests/support/graph_saver_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>

using namespace teaql;
using Ambient = saver_fixture::Ambient;
using Scope = TransactionScope<Ambient>;
using R = Result<std::uint64_t, RuntimeError>;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Ready {
    using allocator_type = Allocator;
    Memory r;
    bool error;

    Ready(std::allocator_arg_t, Allocator a, bool error) : r(a.resource()), error(error) {}

    std::optional<R> poll(WakeSignal&) {
        if (error) {
            return R::err(r, RuntimeError::event(r, "business failed"));
        }
        return R::ok(r, 17);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

static void error_order_and_consumption() {
    for (bool body_error : {false, true}) {
        for (bool commit_error : {false, true}) {
            for (bool rollback_error : {false, true}) {
                GlobalContext global;
                auto user = global.make_user_context();
                auto& state = user->make<mutation_fixture::State>();
                saver_fixture::Controls controls;
                controls.fail_commit = commit_error;
                controls.fail_rollback = rollback_error;
                auto executor = user->make_resource_ref<Ambient>(state, controls);
                user->insert_resource(executor);
                const Scope* scope = nullptr;
                auto& work = user->execute_in_transaction<Ambient, std::uint64_t>(
                    [&, body_error](const Scope& incoming) -> AsyncOperation<R>& {
                        scope = &incoming;
                        return make_operation<R, Ready>(incoming.context().memory_resource(),
                                                        body_error);
                    });
                auto& result = work.wait();
                bool terminal_error = body_error ? rollback_error : commit_error;
                check(bool(result) == !(body_error || terminal_error),
                      "completion follows callback and terminal results");
                if (body_error && rollback_error) {
                    check(
                        result.error().to_string(user->memory_resource()) ==
                            "transaction error: operation failed (entity event error: business failed); rollback also failed (rollback failed)",
                        "rollback failure combines operation and rollback diagnostics");
                } else if (terminal_error) {
                    check(result.error().kind() == RuntimeError::Kind::Transaction,
                          "commit error has Transaction kind");
                } else if (body_error) {
                    check(result.error().kind() == RuntimeError::Kind::Event,
                          "successful rollback preserves the exact original error kind");
                } else {
                    check(result.value() == 17, "successful transaction returns callback data");
                }
                check(
                    state.notes.size() == 2 && state.notes[0] == "begin" &&
                        state.notes[1] == (body_error ? "rollback" : "commit") &&
                        controls.abandoned == unsigned(terminal_error),
                    "one terminal call consumes the native transaction and cleans failed completion");
                bool rejected = false;
                try {
                    auto q = QueryRequest(std::allocator_arg,
                                          Allocator(user->memory_resource()),
                                          SelectQuery(std::allocator_arg,
                                                      Allocator(user->memory_resource()),
                                                      "Order"),
                                          false,
                                          false);
                    (void)scope->query(std::move(q)).wait();
                } catch (const std::logic_error&) {
                    rejected = true;
                }
                check(rejected, "completed scope cannot execute more provider work");
            }
        }
    }
}

static void begin_errors_and_cancellation() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto callback = [](const Scope& scope) -> AsyncOperation<R>& {
        return make_operation<R, Ready>(scope.context().memory_resource(), false);
    };
    auto& missing = user->execute_in_transaction<Ambient, std::uint64_t>(callback).wait();
    check(!missing && missing.error().kind() == RuntimeError::Kind::Transaction,
          "typed lookup failure maps to Transaction before callback execution");
    auto& state = user->make<mutation_fixture::State>();
    saver_fixture::Controls controls;
    auto executor = user->make_resource_ref<Ambient>(state, controls);
    user->insert_resource(executor);
    controls.fail_begin = true;
    auto& failed = user->execute_in_transaction<Ambient, std::uint64_t>(callback).wait();
    check(!failed && failed.error().to_string(user->memory_resource()) ==
                         "transaction error: begin failed",
          "begin error maps to Transaction without a terminal call");
    controls.fail_begin = false;
    controls.pending_begin = true;
    auto& pending = user->execute_in_transaction<Ambient, std::uint64_t>(callback);
    check(!pending.poll(), "begin can suspend");
    pending.cancel();
    check(controls.cancelled_begin == 1 && controls.abandoned == 0,
          "cancelled begin joins without inventing an acquired transaction");
    controls.pending_begin = false;
    controls.pending_terminal = true;
    auto& terminal = user->execute_in_transaction<Ambient, std::uint64_t>(callback);
    check(!terminal.poll(), "commit can suspend");
    terminal.cancel();
    check(controls.abandoned == 1,
          "cancelled commit releases acquired native transaction immediately");
    controls.pending_terminal = false;
    auto borrowed = user->make_resource_ref<Ambient>(state, controls);
    user->insert_resource(borrowed);
    user->destroy(borrowed);
    bool expired = false;
    try {
        (void)user->execute_in_transaction<Ambient, std::uint64_t>(callback).wait();
    } catch (const ReferenceError& error) {
        expired = error.kind() == ReferenceError::Kind::Dead;
    }
    check(expired, "expired executor is rejected before virtual begin");
}

template <class E>
struct NativeBody {
    using allocator_type = Allocator;
    using Output = Result<audited_fixture::Model, RuntimeError>;
    const TransactionScope<E>& scope;
    EntityRuntimeState& root;
    bool abort;
    Audited<audited_fixture::Model> audited;
    std::optional<audited_fixture::Model> entity;
    AsyncOperation<Output>* save = nullptr;
    AsyncOperation<Result<QueryResult, RuntimeError>>* read = nullptr;
    AsyncOperation<Result<MutationResult, RuntimeError>>* write = nullptr;
    WakeSignal::Forward forwarding;

    NativeBody(std::allocator_arg_t,
               Allocator a,
               const TransactionScope<E>& scope,
               EntityRuntimeState& root,
               bool abort)
        : scope(scope), root(root), abort(abort), audited(make_input(a.resource(), root)) {}

    static Audited<audited_fixture::Model> make_input(Memory r, const EntityRuntimeState& root) {
        audited_fixture::Model m(std::allocator_arg, Allocator(r));
        m.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
        m.values.fields.emplace(String("version", r), Value(r, std::int64_t(3)));
        m.values.fields.emplace(String("name", r), Value(r, "input"));
        m.runtime.emplace(std::allocator_arg, Allocator(r), root);
        return Audited<audited_fixture::Model>(
            std::allocator_arg, Allocator(r), std::move(m), "scope reason");
    }

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = scope.context().memory_resource();
        if (!entity) {
            if (!save) {
                check(!scope.entity_data_service("Missing"),
                      "scope reports missing entity data service");
                auto service = scope.entity_data_service("Order");
                check(service && service.value().get().entity() == "Order",
                      "scope binds entity data service to its transaction");
                save = &scope.save_audited(std::move(audited));
            }
            forwarding.attach(save->wake_signal(), wake);
            auto* result = save->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return Output::err(r, detail::clone_in(r, result->error()));
            }
            entity.emplace(std::allocator_arg, Allocator(r), result->value());
            check(!root.current_change_set().changes().empty(),
                  "save leaves intent pending until enclosing commit");
            check(detail::TransactionScopeAccess::pending_ledgers(scope) == 1,
                  "scope retains the flushed ledger");
            scope.context().clear_sql_logs();
        }
        if (!read) {
            SelectQuery query(std::allocator_arg, Allocator(r), "Order");
            read = &scope.query(QueryRequest(std::allocator_arg, Allocator(r), query, true, true));
        }
        forwarding.attach(read->wake_signal(), wake);
        auto* rows = read->poll();
        if (!rows) {
            return {};
        }
        if (!*rows) {
            return Output::err(r, detail::clone_in(r, rows->error()));
        }
        check(rows->value().rows.size() == 1 &&
                  rows->value().rows.front().get("name")->try_text() == "saved" &&
                  rows->value().rows.front().get("version")->try_i64() == 4,
              "provider-neutral scope query sees its own uncommitted audited write");
        check(scope.context().sql_logs().size() == 1,
              "successful SQL query returns metadata without duplicate observer notification");
        if (abort) {
            return Output::err(r, RuntimeError::event(r, "business failed"));
        }
        if (!write) {
            UpdateCommand command(std::allocator_arg, Allocator(r), "Order", std::uint64_t(7));
            command.values.fields.emplace(String("name", r), Value(r, "raw"));
            command.expected_version(4);
            write = &scope.mutate(
                MutationRequest(std::allocator_arg, Allocator(r), std::move(command)));
        }
        forwarding.attach(write->wake_signal(), wake);
        auto* result = write->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return Output::err(r, detail::clone_in(r, result->error()));
        }
        check(result->value().affected_rows == 1,
              "scope provider-neutral mutation uses the active connection");
        check(scope.context().sql_logs().size() == 2,
              "successful SQL mutation returns metadata without duplicate observer notification");
        return Output::ok(r, std::move(*entity));
    }

    void finish_and_wait() noexcept {
        forwarding.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding.reset();
        if (save) {
            save->cancel();
        }
        if (read) {
            read->cancel();
        }
        if (write) {
            write->cancel();
        }
    }
};

static void sqlite_scope_and_retention() {
    for (bool abort : {false, true}) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& metadata = user->make<InMemoryMetadataStore>();
        auto descriptor = audited_fixture::Model::entity_descriptor(r);
        descriptor.table("orders");
        metadata.register_entity(std::move(descriptor));
        user->set_metadata(*user, metadata);
        user->enable_all_sql_log();
        auto& connection = user->make_resource<SqliteConnection>();
        auto& transport = user->make<SqliteMutationExecutor>(connection);
        auto& sql = user->make<CompiledQuery>();
        for (auto statement :
             {"CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
              "INSERT INTO orders VALUES(7,3,'original')"}) {
            sql.sql = statement;
            check(bool(transport.execute(r, sql)), "SQLite scope fixture setup");
        }
        auto& dialect = user->make_resource<SqliteDialect>();
        auto& cache = user->make<SqlExecutorState>();
        using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
        auto executor = user->make_ref<Executor>(dialect, transport, metadata, cache);
        user->insert_resource(executor);
        auto& root = user->make<EntityRuntimeState>();
        EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
        root.set_original_version(key, 3);
        root.set(key, "name", Value(r, "saved"));
        auto& result =
            user->execute_in_transaction<Executor, audited_fixture::Model>(
                    [&](const TransactionScope<Executor>& scope)
                        -> AsyncOperation<Result<audited_fixture::Model, RuntimeError>>& {
                        return make_operation<Result<audited_fixture::Model, RuntimeError>,
                                              NativeBody<Executor>>(r, scope, root, abort);
                    })
                .wait();
        check(bool(result) == !abort, "scope commits or rolls back the entire body");
        check(root.current_change_set().changes().empty() == !abort,
              "only a successful enclosing commit clears the saved ledger");
        sql.sql = "SELECT version,name FROM orders WHERE id=7";
        auto& rows = transport.fetch_all_compact_sql(r, sql).wait();
        check(rows && rows.value()[0].get("version")->try_i64() == (abort ? 3 : 5) &&
                  rows.value()[0].get("name")->try_text() == (abort ? "original" : "raw"),
              "audited and raw mutations share one physical SQLite transaction");
        check(!user->sql_logs().empty(), "transaction metadata reaches Context SQL logs");
        if (!abort) {
            auto kept = user->make_global<audited_fixture::Model>(result.value());
            check(result.value().entity_runtime_state()->current_change_set().changes().empty(),
                  "returned entity observes its original ledger being cleared at commit");
            user.reset();
            auto next = global.make_user_context();
            auto rebound = next->rebind_global(kept);
            check(next->get(rebound)
                              .values.fields.at(String("name", next->memory_resource()))
                              .try_text() == "saved" &&
                      next->get(rebound).runtime->current_change_set().changes().empty(),
                  "explicit Global copy survives actual User destruction");
        }
    }
}

static void pending_save_and_ledger_completion() {
    for (unsigned phase = 0; phase < 3; ++phase) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& metadata = user->make<InMemoryMetadataStore>();
        metadata.register_entity(audited_fixture::Model::entity_descriptor(r));
        user->set_metadata(*user, metadata);
        user->disable_sql_log();
        auto& state = user->make<mutation_fixture::State>();
        state.allow_reads = true;
        state.rows.emplace_back();
        state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
        state.rows.back().emplace(String("version", r), Value(r, std::int64_t(3)));
        state.rows.back().emplace(String("name", r), Value(r, "database"));
        state.read_pending = phase == 0;
        state.pending = phase == 1;
        saver_fixture::Controls controls;
        controls.pending_terminal = phase == 2;
        auto executor = user->make_resource_ref<Ambient>(state, controls);
        user->insert_resource(executor);
        auto& root = user->make<EntityRuntimeState>();
        EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
        root.set_original_version(key, 3);
        root.set(key, "name", Value(r, "pending"));
        auto& work = user->execute_in_transaction<Ambient, audited_fixture::Model>(
            [&](const Scope& scope)
                -> AsyncOperation<Result<audited_fixture::Model, RuntimeError>>& {
                return scope.save_audited(NativeBody<Ambient>::make_input(r, root));
            });
        check(!work.poll(), "scope suspends in ledger read, ledger mutation or commit");
        work.cancel();
        check(controls.abandoned == 1 && state.cancelled == unsigned(phase != 2) &&
                  root.current_change_set().changes().size() == 1 &&
                  !detail::current_mutation_governance(*user),
              "cancellation joins native work, abandons transaction and preserves pending ledger");
    }
    for (bool body_error : {false, true}) {
        for (bool terminal_error : {false, true}) {
            GlobalContext global;
            auto user = global.make_user_context();
            auto r = user->memory_resource();
            auto& metadata = user->make<InMemoryMetadataStore>();
            metadata.register_entity(audited_fixture::Model::entity_descriptor(r));
            user->set_metadata(*user, metadata);
            user->disable_sql_log();
            auto& state = user->make<mutation_fixture::State>();
            state.allow_reads = true;
            state.rows.emplace_back();
            state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
            state.rows.back().emplace(String("version", r), Value(r, std::int64_t(3)));
            state.rows.back().emplace(String("name", r), Value(r, "database"));
            saver_fixture::Controls controls;
            controls.fail_commit = terminal_error;
            controls.fail_rollback = terminal_error;
            auto executor = user->make_resource_ref<Ambient>(state, controls);
            user->insert_resource(executor);
            auto& root = user->make<EntityRuntimeState>();
            EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
            root.set_original_version(key, 3);
            root.set(key, "name", Value(r, "pending"));
            auto& result =
                user->execute_in_transaction<Ambient, std::uint64_t>([&](const Scope& scope)
                                                                         -> AsyncOperation<R>& {
                        for (unsigned save = 0; save < 2; ++save) {
                            check(bool(scope.save_audited(NativeBody<Ambient>::make_input(r, root))
                                           .wait()),
                                  "repeated audited save can flush the same ledger");
                            check(
                                detail::TransactionScopeAccess::pending_ledgers(scope) == 1 &&
                                    root.current_change_set().changes().size() == 1,
                                "repeated flush deduplicates ledger identity and keeps intent pending");
                        }
                        return make_operation<R, Ready>(r, body_error);
                    })
                    .wait();
            check(
                bool(result) == !(body_error || terminal_error) &&
                    root.current_change_set().changes().empty() == !(body_error || terminal_error),
                "only successful commit clears the repeated ledger; every failure preserves retry intent");
        }
    }
}

static void allocation_failure_after_begin() {
    GlobalContext global;
    MemoryLimits limits;
    limits.max_objects = 256;
    auto user = global.make_user_context(limits);
    auto& state = user->make<mutation_fixture::State>();
    saver_fixture::Controls controls;
    auto executor = user->make_resource_ref<Ambient>(state, controls);
    user->insert_resource(executor);
    auto& work = user->execute_in_transaction<Ambient, std::uint64_t>(
        [](const Scope& scope) -> AsyncOperation<R>& {
            return make_operation<R, Ready>(scope.context().memory_resource(), false);
        });
    while (user->allocation_stats().issued_objects < limits.max_objects - 3) {
        (void)user->make<int>();
    }
    bool exhausted = false;
    try {
        (void)work.poll();
    } catch (const MemoryLimitError& error) {
        exhausted = error.kind() == MemoryLimitError::Kind::Objects;
    }
    check(
        exhausted && controls.abandoned == 1 && state.notes.size() == 1 &&
            state.notes[0] == "begin",
        "failed scope allocation after acquiring transaction performs native cleanup without new allocation");
    user->close();
}

template <class E>
struct QueryOnly {
    using allocator_type = Allocator;
    const TransactionScope<E>& scope;
    AsyncOperation<Result<QueryResult, RuntimeError>>* child = nullptr;
    WakeSignal::Forward forwarding;

    QueryOnly(std::allocator_arg_t, Allocator, const TransactionScope<E>& scope) : scope(scope) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = scope.context().memory_resource();
        if (!child) {
            SelectQuery q(std::allocator_arg, Allocator(r), "Order");
            child = &scope.query(QueryRequest(std::allocator_arg, Allocator(r), q, true, true));
        }
        forwarding.attach(child->wake_signal(), wake);
        auto* result = child->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(r, detail::clone_in(r, result->error()));
        }
        return R::ok(r, 17);
    }

    void finish_and_wait() noexcept {
        forwarding.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding.reset();
        if (child) {
            child->cancel();
        }
    }
};

static void sqlite_failure_diagnostics() {
    for (bool enabled : {false, true}) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& metadata = user->make<InMemoryMetadataStore>();
        auto descriptor = audited_fixture::Model::entity_descriptor(r);
        descriptor.table("missing_table");
        metadata.register_entity(std::move(descriptor));
        user->set_metadata(*user, metadata);
        user->set_sql_log_options(enabled ? SqlLogOptions::select_only()
                                          : SqlLogOptions::disabled());
        auto& connection = user->make_resource<SqliteConnection>();
        auto& transport = user->make<SqliteMutationExecutor>(connection);
        auto& dialect = user->make_resource<SqliteDialect>();
        auto& cache = user->make<SqlExecutorState>();
        using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
        auto executor = user->make_ref<Executor>(dialect, transport, metadata, cache);
        user->insert_resource(executor);
        auto& result = user->execute_in_transaction<Executor, std::uint64_t>(
                               [&](const TransactionScope<Executor>& scope) -> AsyncOperation<R>& {
                                   return make_operation<R, QueryOnly<Executor>>(r, scope);
                               })
                           .wait();
        check(!result && result.error().kind() == RuntimeError::Kind::Transaction &&
                  user->sql_logs().size() == unsigned(enabled),
              "failed native query emits exactly one observer diagnostic only when enabled");
        auto& reuse = user->get(executor).begin(r).wait();
        check(reuse && bool(reuse.value().get().rollback(r).wait()),
              "failed scope releases the SQLite connection for another transaction");
    }
}

static void close_and_callback_failures() {
    GlobalContext global;
    saver_fixture::Controls controls;
    unsigned cancelled = 0;
    auto user = global.make_user_context();
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.read_pending = true;
    state.external_cancel = &cancelled;
    auto executor = user->make_resource_ref<Ambient>(state, controls);
    user->insert_resource(executor);
    auto& work = user->execute_in_transaction<Ambient, std::uint64_t>(
        [&](const Scope& scope) -> AsyncOperation<R>& {
            return make_operation<R, QueryOnly<Ambient>>(user->memory_resource(), scope);
        });
    check(!work.poll(), "query-only callback can suspend");
    user.reset();
    check(cancelled == 1 && controls.abandoned == 1,
          "actual User destruction joins callback query and releases its transaction");
    auto next = global.make_user_context();
    auto& next_state = next->make<mutation_fixture::State>();
    auto registered = next->make_resource_ref<Ambient>(next_state, controls);
    next->insert_resource(registered);
    bool threw = false;
    try {
        (void)next
            ->execute_in_transaction<Ambient, std::uint64_t>(
                [](const Scope&) -> AsyncOperation<R>& {
                    throw std::runtime_error("callback exception");
                })
            .wait();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw && controls.abandoned == 2,
          "callback exception abandons the native transaction before propagating");
    auto foreign = global.make_user_context();
    auto& foreign_work = make_operation<R, Ready>(foreign->memory_resource(), false);
    bool rejected = false;
    try {
        (void)next
            ->execute_in_transaction<Ambient, std::uint64_t>(
                [&](const Scope&) -> AsyncOperation<R>& {
                    return foreign_work;
                })
            .wait();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && controls.abandoned == 3 && foreign_work.status() == AsyncStatus::Cancelled,
          "foreign callback future is cancelled and its native transaction is abandoned");
    unsigned called = 0;
    auto& mutable_result = next->execute_in_transaction<Ambient, std::uint64_t>(
        [counter = 0, &called](const Scope& scope) mutable -> AsyncOperation<R>& {
            ++counter;
            ++called;
            check(counter == 1, "mutable transaction callback is invoked only once");
            return make_operation<R, Ready>(scope.context().memory_resource(), false);
        });
    check(bool(mutable_result.wait()) && bool(mutable_result.wait()) && called == 1,
          "cached transaction result never repeats its FnOnce callback");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        error_order_and_consumption();
        begin_errors_and_cancellation();
        sqlite_scope_and_retention();
        pending_save_and_ledger_completion();
        allocation_failure_after_begin();
        sqlite_failure_diagnostics();
        close_and_callback_failures();
        std::pmr::set_default_resource(previous);
        std::cout << "TransactionScope contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
