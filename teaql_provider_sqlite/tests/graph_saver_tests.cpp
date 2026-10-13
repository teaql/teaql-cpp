#include "../../tests/support/graph_saver_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;
using namespace saver_fixture;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static InMemoryMetadataStore& metadata(UserContext& user) {
    auto r = user.memory_resource();
    auto& store = user.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Order");
    descriptor.table("orders");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text);
    store.register_entity(std::move(descriptor));
    user.set_metadata(user, store);
    user.disable_sql_log();
    return store;
}

static GraphNode input(Memory r) {
    GraphNode node(std::allocator_arg, Allocator(r), "Order");
    node.with_operation(GraphOperation::Create)
        .value("id", std::uint64_t(7))
        .value("name", "planned")
        .with_comment("Save order fixture");
    return node;
}

static void cancellation_and_late_lookup() {
    GlobalContext global;
    for (int stage = 0; stage < 4; ++stage) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        metadata(*user);
        auto& state = user->make<mutation_fixture::State>();
        state.allow_reads = true;
        state.pending = stage == 1;
        state.read_pending = stage == 2;
        state.rows.emplace_back();
        state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
        state.rows.back().emplace(String("version", r), Value(r, std::int64_t(3)));
        state.rows.back().emplace(String("name", r), Value(r, "database"));
        auto& controls = user->make<Controls>();
        controls.pending_begin = stage == 0;
        controls.pending_terminal = stage == 3;
        auto executor = user->make_resource_ref<Ambient>(state, controls);
        user->register_executor(executor);
        auto saver = user->require_resource<detail::DynGraphSaver>();
        auto& work = saver.value().get().save_graph_dyn(*user, input(r));
        check(!work.poll(), "saver suspends in the selected stage");
        work.cancel();
        work.cancel();
        if (stage == 0) {
            check(controls.cancelled_begin == 1 && controls.abandoned == 0,
                  "pending begin cancellation starts no transaction");
        } else {
            check(controls.abandoned == 1 && user->get(executor).transaction->consumed(),
                  "explicit cancellation consumes and abandons a live transaction immediately");
        }
        check(!user->closed() && !detail::current_mutation_governance(*user),
              "cancellation releases work without closing User or retaining governance");
    }
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<mutation_fixture::State>();
    auto& controls = user->make<Controls>();
    auto executor = user->make_resource_ref<Ambient>(state, controls);
    user->register_executor(executor);
    auto& marker = user->make<detail::GraphSaverFor<Ambient>>();
    auto& work = marker.save_graph_dyn(*user, input(r));
    user->destroy(executor);
    bool rejected = false;
    try {
        (void)work.poll();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Dead;
    }
    check(rejected && state.notes.empty(),
          "executor is checked at call-time before virtual dispatch");
}

static void ledger_completion_and_errors() {
    GlobalContext global;
    for (bool fail_commit : {false, true}) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        metadata(*user);
        auto& state = user->make<mutation_fixture::State>();
        state.allow_reads = true;
        state.rows.emplace_back();
        state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
        state.rows.back().emplace(String("version", r), Value(r, std::int64_t(3)));
        state.rows.back().emplace(String("name", r), Value(r, "database"));
        auto& controls = user->make<Controls>();
        controls.fail_commit = fail_commit;
        auto executor = user->make_resource_ref<Ambient>(state, controls);
        user->register_executor(executor);
        auto& root = user->make<EntityRuntimeState>();
        auto identity = user->make_ref<EntityKey>("Order", Value(r, std::uint64_t(7)));
        root.mark_as_new(user->get(identity));
        root.set(user->get(identity), "name", Value(r, "changed"));
        auto saver = user->require_resource<detail::DynGraphSaver>();
        auto& result = saver.value().get().save_ledger_dyn(*user, input(r), root).wait();
        check(bool(result) != fail_commit && root.is_new(user->get(identity)) == fail_commit,
              "ledger clears only after successful commit");
        check(!state.notes.empty() && state.notes.back() == "commit",
              "commit follows the transaction-owned authoritative root read");
        check(controls.abandoned == unsigned(fail_commit),
              "failed consumed commit still releases its active transaction");
    }
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.failure = 0;
    auto& controls = user->make<Controls>();
    controls.fail_rollback = true;
    auto executor = user->make_resource_ref<Ambient>(state, controls);
    user->register_executor(executor);
    auto saver = user->require_resource<detail::DynGraphSaver>();
    auto& result = saver.value().get().save_graph_dyn(*user, input(r)).wait();
    check(!result && result.error().message() == "rollback failed" && controls.abandoned == 1,
          "rollback failure replaces the write error and releases the active transaction");
}

static void sqlite_transactional_save_and_cancel() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& store = metadata(*user);
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (
        auto statement :
        {"CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
         "CREATE TRIGGER fill_order AFTER INSERT ON orders BEGIN UPDATE orders SET name='triggered',version=5 WHERE id=NEW.id; END"}) {
        sql.sql = statement;
        check(bool(transport.execute(r, sql)), "SQLite saver setup");
    }
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& cache = user->make<SqlExecutorState>();
    using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
    auto executor = user->make_ref<Executor>(dialect, transport, store, cache);
    user->register_executor(executor);
    auto saver = user->require_resource<detail::DynGraphSaver>();
    auto& graph = saver.value().get().save_graph_dyn(*user, input(r)).wait();
    check(graph && graph.value().values.fields.at(String("name", r)).try_text() == "triggered" &&
              graph.value().values.fields.at(String("version", r)).try_i64() == 5,
          "graph saver commits trigger-derived values read on its own transaction");
    auto& root = user->make<EntityRuntimeState>();
    auto identity = user->make_ref<EntityKey>("Order", Value(r, std::uint64_t(7)));
    root.set_original_version(user->get(identity), 5);
    root.set(user->get(identity), "name", Value(r, "ledger update"));
    auto& ledger = saver.value().get().save_ledger_dyn(*user, input(r), root).wait();
    check(ledger && ledger.value().values.fields.at(String("version", r)).try_i64() == 6 &&
              root.current_change_set().changes().empty(),
          "ledger saver commits guarded update and clears only the committed ledger");
    // Explicit transaction cancellation must release SQLite before User close.
    auto& transaction = user->get(executor).begin(r).wait().value().get();
    InsertCommand cancelled(std::allocator_arg, Allocator(r), "Order");
    cancelled.values.fields.emplace(String("id", r), Value(r, std::uint64_t(8)));
    cancelled.values.fields.emplace(String("version", r), Value(r, std::int64_t(1)));
    cancelled.values.fields.emplace(String("name", r), Value(r, "cancelled"));
    check(bool(transaction
                   .mutate(r,
                           MutationRequest(std::allocator_arg,
                                           Allocator(r),
                                           std::move(cancelled),
                                           "verify transaction cancellation"))
                   .wait()),
          "uncommitted cancellation fixture write uses the transaction capability");
    transaction.cancel();
    transaction.cancel();
    sql.sql = "SELECT id FROM orders ORDER BY id";
    auto& rows = transport.fetch_all_compact_sql(r, sql).wait();
    check(rows && rows.value().size() == 1 && rows.value()[0].get("id")->try_u64() == 7,
          "allocation-free cancellation rolls back immediately while User remains open");
    auto& replacement = user->get(executor).begin(r).wait().value().get();
    check(bool(replacement.rollback(r).wait()), "released connection accepts another transaction");
    auto& retained = global.make<GraphNode>(ledger.value());
    user.reset();
    check(retained.values.fields.at(String("name", global.memory_resource())).try_text() ==
              "ledger update",
          "explicit Global saved result survives actual User destruction");
}

static void begin_handoff_exhaustion() {
    GlobalContext global;
    MemoryLimits limits;
    limits.max_objects = 256;
    auto user = global.make_user_context(limits);
    auto r = user->memory_resource();
    auto& store = metadata(*user);
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& cache = user->make<SqlExecutorState>();
    auto& executor =
        user->make<SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>>(
            dialect, transport, store, cache);
    bool began = false;
    sqlite3_trace_v2(
        connection.native_handle(),
        SQLITE_TRACE_STMT,
        [](unsigned, void* target, void* statement, void*) -> int {
            auto sql = std::string_view(sqlite3_sql(static_cast<sqlite3_stmt*>(statement)));
            if (sql.starts_with("BEGIN")) {
                *static_cast<bool*>(target) = true;
            }
            return 0;
        },
        &began);
    auto& work = executor.begin(r);
    // The provider needs exactly its transaction, begin state and operation;
    // leave no identity for the data-service adapter after BEGIN succeeds.
    while (user->allocation_stats().issued_objects < limits.max_objects - 3) {
        (void)user->make_ref<int>(0);
    }
    bool rejected = false;
    try {
        (void)work.poll();
    } catch (const MemoryLimitError& error) {
        rejected = error.kind() == MemoryLimitError::Kind::Objects;
    }
    check(rejected && began && sqlite3_get_autocommit(connection.native_handle()),
          "failed adapter handoff rolls back acquired SQLite transaction without allocation");
    sqlite3_trace_v2(connection.native_handle(), 0, nullptr, nullptr);
}

static void independent_terminal_cancellation() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& state = user->make<mutation_fixture::State>();
    auto& controls = user->make<Controls>();
    controls.pending_terminal = true;
    auto& executor = user->make_resource<Ambient>(state, controls);
    auto& transaction = executor.begin(r).wait().value().get();
    auto& terminal = transaction.commit(r);
    check(!terminal.poll(), "independent commit remains pending");
    transaction.cancel();
    transaction.cancel();
    check(terminal.status() == AsyncStatus::Cancelled && controls.abandoned == 1,
          "transaction cancellation joins its terminal work without recursive cancellation");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        cancellation_and_late_lookup();
        ledger_completion_and_errors();
        sqlite_transactional_save_and_cancel();
        begin_handoff_exhaustion();
        independent_terminal_cancellation();
        std::pmr::set_default_resource(previous);
        std::cout << "Transactional graph saver contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
