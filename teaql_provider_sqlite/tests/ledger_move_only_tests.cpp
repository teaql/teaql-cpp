#include "../../tests/support/ledger_move_fixture.hpp"
#include <teaql/runtime/transaction.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <algorithm>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static_assert(!detail::domain_copyable<ledger_move_fixture::Model>);
static_assert(!detail::domain_copyable<AuditedLedgerResult<ledger_move_fixture::Model>>);

template <class E>
struct NativeBody {
    using allocator_type = Allocator;
    using Output = Result<ledger_move_fixture::Model, RuntimeError>;
    const TransactionScope<E>& scope;
    EntityRuntimeState& root;
    bool abort;
    Audited<ledger_move_fixture::Model> audited;
    std::optional<ledger_move_fixture::Model> entity;
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

    static Audited<ledger_move_fixture::Model> make_input(Memory r,
                                                          const EntityRuntimeState& root) {
        ledger_move_fixture::Model m(std::allocator_arg, Allocator(r));
        m.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
        m.values.fields.emplace(String("version", r), Value(r, std::int64_t(3)));
        m.values.fields.emplace(String("name", r), Value(r, "input"));
        m.runtime.emplace(std::allocator_arg, Allocator(r), root);
        return Audited<ledger_move_fixture::Model>(
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
            entity.emplace(std::move(save->take_result()).value());
            check(!root.current_change_set().changes().empty(),
                  "save leaves intent pending until enclosing commit");
            check(detail::TransactionScopeAccess::pending_ledgers(scope) == 1,
                  "scope retains the flushed ledger");
            scope.context().clear_sql_logs();
        }
        if (!read) {
            SelectQuery query(std::allocator_arg, Allocator(r), "Order");
            query.comment("Inspect move-only transaction save")
                .purpose("Verify uncommitted ledger row");
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
            write = &scope.mutate(MutationRequest(std::allocator_arg,
                                                  Allocator(r),
                                                  std::move(command),
                                                  "raw update inside transaction scope"));
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
        auto logs = scope.context().sql_logs();
        check(
            logs.size() == 3 &&
                std::ranges::count_if(logs,
                                      [](const auto& log) {
                                          return log.operation == SqlLogOperation::Select;
                                      }) == 2 &&
                std::ranges::count_if(logs,
                                      [](const auto& log) {
                                          return log.operation == SqlLogOperation::Update;
                                      }) == 1 &&
                result->value().persisted_snapshot,
            "mutation plus authoritative readback add two distinct logs without duplicate notification");
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
        auto descriptor = ledger_move_fixture::Model::entity_descriptor(r);
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
            user->execute_in_transaction<Executor, ledger_move_fixture::Model>(
                    [&](const TransactionScope<Executor>& scope)
                        -> AsyncOperation<Result<ledger_move_fixture::Model, RuntimeError>>& {
                        return make_operation<Result<ledger_move_fixture::Model, RuntimeError>,
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
            check(result.value().entity_runtime_state()->current_change_set().changes().empty(),
                  "Move-only returned entity retains the committed ledger identity");
        }
        user.reset();
        check(global.live_user_bytes() == 0,
              "Move-only audited transaction returns every User block");
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    sqlite_scope_and_retention();
    std::cout << "Move-only audited SQLite transaction contracts pass\n";
}
