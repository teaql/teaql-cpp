#include <teaql/runtime/transaction.hpp>
#include <teaql/runtime/detail/data_service/graph_saver.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;
using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
using Output = Result<std::uint64_t, RuntimeError>;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static int row_count(const SqliteConnection& connection) {
    sqlite3_stmt* statement = nullptr;
    check(sqlite3_prepare_v2(
              connection.native_handle(), "SELECT count(*) FROM orders", -1, &statement, nullptr) ==
              SQLITE_OK,
          "prepare committed row count");
    int status = sqlite3_step(statement);
    int count = status == SQLITE_ROW ? sqlite3_column_int(statement, 0) : -1;
    sqlite3_finalize(statement);
    check(status == SQLITE_ROW, "read committed row count");
    return count;
}

struct Sink final : RawAuditEventSink {
    using allocator_type = Allocator;
    const SqliteConnection& connection;
    mutable Vector<RawAuditEvent> events;
    bool fail_first = false;
    int expected_rows = 2;
    const EntityRuntimeState* root = nullptr;

    Sink(std::allocator_arg_t, Allocator a, const SqliteConnection& connection)
        : connection(connection), events(a.resource()) {}

    Memory memory_resource() const {
        return events.get_allocator().resource();
    }

    Result<std::monostate, RuntimeError>
    on_event(Memory r, const UserContext&, const RawAuditEvent& event) const override {
        check(sqlite3_get_autocommit(connection.native_handle()) != 0,
              "audit delivery happens after physical transaction completion");
        check(row_count(connection) == expected_rows, "sink sees committed database rows");
        if (root) {
            check(root->current_change_set().changes().empty(),
                  "committed ledger is cleared before audit delivery");
        }
        events.emplace_back(event);
        if (fail_first && events.size() == 1) {
            return Result<std::monostate, RuntimeError>::err(
                r, RuntimeError::event(r, "sink rejected first committed fact"));
        }
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

static InMemoryMetadataStore& metadata(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& store = user_context.make<InMemoryMetadataStore>();
    EntityDescriptor entity(std::allocator_arg, Allocator(r), "Order");
    entity.table("orders");
    entity.property("id", DataType::U64).id();
    entity.property("version", DataType::I64).version();
    entity.property("name", DataType::Text);
    store.register_entity(std::move(entity));
    user_context.set_metadata(user_context, store);
    user_context.disable_sql_log();
    return store;
}

struct Body {
    using allocator_type = Allocator;
    const TransactionScope<Executor>& scope;
    Sink& sink;
    bool abort, suspend;
    unsigned index = 0;
    AsyncOperation<Result<MutationResult, RuntimeError>>* pending = nullptr;
    WakeSignal::Forward forwarding;

    Body(std::allocator_arg_t,
         Allocator,
         const TransactionScope<Executor>& scope,
         Sink& sink,
         bool abort,
         bool suspend)
        : scope(scope), sink(sink), abort(abort), suspend(suspend) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto& user_context = scope.context();
        auto r = user_context.memory_resource();
        while (index < 2) {
            if (!pending) {
                InsertCommand command(std::allocator_arg, Allocator(r), "Order");
                command.value("id", std::uint64_t(7 + index));
                command.value("version", std::int64_t(1));
                command.value("name", "committed fact");
                pending = &scope.mutate(MutationRequest(
                    std::allocator_arg, Allocator(r), std::move(command), "write audit fixture"));
            }
            forwarding.attach(pending->wake_signal(), wake);
            auto* result = pending->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return Output::err(r, detail::clone_in(r, result->error()));
            }
            auto event =
                RawAuditEvent::created(r, "Order", result->value().persisted_snapshot->fields);
            check(bool(user_context.send_event(event)), "enqueue explicit transaction fact");
            check(sink.events.empty(), "business facts are invisible during transaction body");
            pending = nullptr;
            ++index;
        }
        if (suspend) {
            return {};
        }
        if (abort) {
            return Output::err(r, RuntimeError::event(r, "business rejected transaction"));
        }
        return Output::ok(r, 2);
    }

    void finish_and_wait() noexcept {
        forwarding.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding.reset();
        if (pending) {
            pending->cancel();
        }
    }
};

enum class Mode { Commit, Rollback, CommitFailure, SinkFailure, Cancel };

static void explicit_scope(Mode mode) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& store = metadata(*user_context);
    auto& connection = user_context->make_resource<SqliteConnection>();
    auto& transport = user_context->make<SqliteMutationExecutor>(connection);
    CompiledQuery sql(std::allocator_arg, Allocator(r));
    auto execute = [&](std::string_view text) {
        sql.sql = text;
        check(bool(transport.execute(r, sql)), "setup transaction audit fixture");
    };
    execute("PRAGMA foreign_keys=ON");
    execute("CREATE TABLE parents(id INTEGER PRIMARY KEY)");
    execute(
        mode == Mode::CommitFailure
            ? "CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT,parent_id INTEGER DEFAULT 99 REFERENCES parents(id) DEFERRABLE INITIALLY DEFERRED)"
            : "CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)");
    auto& dialect = user_context->make_resource<SqliteDialect>();
    auto& cache = user_context->make<SqlExecutorState>();
    auto executor = user_context->make_ref<Executor>(dialect, transport, store, cache);
    user_context->insert_resource(executor);
    auto& sink = user_context->make<Sink>(connection);
    sink.fail_first = mode == Mode::SinkFailure;
    user_context->set_event_sink(*user_context, sink);
    auto& work = user_context->execute_in_transaction<Executor, std::uint64_t>(
        [&](const TransactionScope<Executor>& scope) -> AsyncOperation<Output>& {
            return make_operation<Output, Body>(
                r, scope, sink, mode == Mode::Rollback, mode == Mode::Cancel);
        });
    if (mode == Mode::Cancel) {
        check(work.poll() == nullptr && row_count(connection) == 2 && sink.events.empty(),
              "suspended operation has unpublished uncommitted writes");
        work.cancel();
        check(row_count(connection) == 0 && sink.events.empty(),
              "cancellation rolls back writes and discards pending facts");
    } else {
        auto& result = work.wait();
        bool committed = mode == Mode::Commit || mode == Mode::SinkFailure;
        check(row_count(connection) == (committed ? 2 : 0), "database follows terminal result");
        check(sink.events.size() == (committed ? 2 : 0),
              "failed commit and rollback discard all facts; committed facts all attempt delivery");
        if (mode == Mode::Commit) {
            check(result && result.value() == 2, "successful commit returns callback result");
        } else {
            auto expected = mode == Mode::SinkFailure ? RuntimeError::Kind::AuditAfterCommit
                            : mode == Mode::Rollback  ? RuntimeError::Kind::Event
                                                      : RuntimeError::Kind::Transaction;
            check(!result && result.error().kind() == expected,
                  "terminal error preserves commit status");
        }
    }
    check(sqlite3_get_autocommit(connection.native_handle()) != 0,
          "every terminal path releases the physical transaction");
}

static void automatic_ledger_save(bool fail_sink) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& store = metadata(*user_context);
    auto& connection = user_context->make_resource<SqliteConnection>();
    auto& transport = user_context->make<SqliteMutationExecutor>(connection);
    CompiledQuery sql(std::allocator_arg, Allocator(r));
    sql.sql = "CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)";
    check(bool(transport.execute(r, sql)), "setup automatic save table");
    sql.sql = "INSERT INTO orders VALUES(7,3,'original')";
    check(bool(transport.execute(r, sql)), "setup automatic save row");
    auto& dialect = user_context->make_resource<SqliteDialect>();
    auto& cache = user_context->make<SqlExecutorState>();
    auto executor = user_context->make_ref<Executor>(dialect, transport, store, cache);
    user_context->register_executor(executor);
    auto& root = user_context->make<EntityRuntimeState>();
    EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
    root.set_original_version(key, 3);
    root.set(key, "name", Value(r, "saved"));
    auto& sink = user_context->make<Sink>(connection);
    sink.fail_first = fail_sink;
    sink.expected_rows = 1;
    sink.root = &root;
    user_context->set_event_sink(*user_context, sink);
    GraphNode node(std::allocator_arg, Allocator(r), "Order");
    node.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    node.comment.emplace("automatic ledger audit fixture", r);
    auto saver = user_context->require_resource<detail::DynGraphSaver>();
    auto& result = saver.value().get().save_ledger_dyn(*user_context, std::move(node), root).wait();
    check(bool(result) == !fail_sink && sink.events.size() == 1,
          "automatic graph saver delivers one committed native fact");
    if (fail_sink) {
        check(result.error().kind() == RuntimeError::Kind::AuditAfterCommit,
              "automatic saver preserves committed audit failure kind");
    }
    check(root.current_change_set().changes().empty(),
          "audit failure cannot restore committed ledger");
    sql.sql = "SELECT name,version FROM orders WHERE id=7";
    auto& rows = transport.fetch_all_compact_sql(r, sql).wait();
    check(rows && rows.value()[0].get("name")->try_text() == "saved" &&
              rows.value()[0].get("version")->try_i64() == 4,
          "automatic save stays committed after sink failure");
}

int main() {
    for (auto mode :
         {Mode::Commit, Mode::Rollback, Mode::CommitFailure, Mode::SinkFailure, Mode::Cancel}) {
        explicit_scope(mode);
    }
    automatic_ledger_save(false);
    automatic_ledger_save(true);
    std::cout << "SQLite commit audit transaction contracts passed\n";
}
