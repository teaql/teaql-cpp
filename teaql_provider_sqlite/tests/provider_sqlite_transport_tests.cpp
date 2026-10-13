#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <teaql/runtime/context.hpp>
#include <sqlite3.h>
#include <iostream>
#include <thread>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static CompiledQuery query(Memory r, std::string_view sql) {
    CompiledQuery q(std::allocator_arg, Allocator(r));
    q.sql = sql;
    return q;
}

struct Fixture {
    GlobalContext global;
    SqliteConnection& connection = global.make_resource<SqliteConnection>();
    SqliteMutationExecutor& provider = global.make<SqliteMutationExecutor>(connection);

    Fixture() {
        check(
            sqlite3_exec(
                connection.native_handle(),
                "CREATE TABLE records(id INTEGER PRIMARY KEY, version INTEGER, name TEXT, flag BOOLEAN, doc JSON); INSERT INTO records VALUES(1,1,'one',1,'{\"a\":[1]}'),(2,1,'two',0,'{}'),(3,1,'three',1,'{}'),(4,1,'four',1,'{}')",
                nullptr,
                nullptr,
                nullptr) == SQLITE_OK,
            "setup native database");
    }
};

static void calls_and_error_order() {
    Fixture f;
    auto user = f.global.make_user_context();
    auto r = user->memory_resource();
    auto q = query(r, "SELECT id, name, flag, doc FROM records ORDER BY id");
    auto& pending = f.provider.fetch_all_compact_sql(r, q);
    q.sql = "broken after capture";
    const auto& rows = pending.wait().value();
    check(rows.size() == 4 && rows[0].shared_columns() == rows[3].shared_columns(),
          "query captures input and shares result-domain layout");
    check(rows[0].memory_resource() == r && rows[0].shared_columns()->memory_resource() == r &&
              std::get<bool>(rows[0].get("flag")->data),
          "native decoding and layout belong to UserContext");
    auto update = query(r, "UPDATE records SET name=? WHERE id=?");
    update.params.emplace_back("updated");
    update.params.emplace_back(1);
    auto& write = f.provider.execute_sql(r, update);
    check(f.provider.fetch_all_compact(r, query(r, "SELECT name FROM records WHERE id=1"))
                  .value()[0]
                  .get("name")
                  ->try_text() == "one",
          "write remains lazy");
    check(write.wait().value() == 1, "native affected rows");
    check(f.provider.fetch_all_compact(r, query(r, "SELECT name FROM records WHERE id=1"))
                  .value()[0]
                  .get("name")
                  ->try_text() == "updated",
          "native write persists");
    auto invalid = query(r, "NOT SQL");
    invalid.params.emplace_back(Value::Object(r));
    check(f.provider.fetch_all_compact_sql(r, invalid).wait().error().kind() ==
              MutationExecutorError::Kind::UnsupportedValue,
          "ordinary fetch binds before prepare");
    Vector<Value> empty(r);
    check(f.provider.fetch_repeated_compact_sql(r, invalid, 500, empty).wait().error().kind() ==
              MutationExecutorError::Kind::Sqlite,
          "repeated fetch prepares even for empty values");
    auto repeated = query(r, "SELECT id FROM records WHERE id=?");
    repeated.params.emplace_back(Value::Object(r));
    check(f.provider.fetch_repeated_compact_sql(r, repeated, 500, empty).wait().value().empty(),
          "empty repeated values do not bind or index");
    Vector<Value> ids(r);
    ids.emplace_back(3);
    ids.emplace_back(1);
    ids.emplace_back(3);
    const auto& matches = f.provider.fetch_repeated_compact_sql(r, repeated, 0, ids).wait().value();
    check(matches.size() == 3 && matches[0].get("id")->try_i64() == 3 &&
              matches[1].get("id")->try_i64() == 1 && matches[2].get("id")->try_i64() == 3,
          "repeated fetch replaces template before binding and preserves order/duplicates");
    check(f.provider.execute(r, query(r, "SELECT 1")).error().detail() ==
              "Execute returned results - did you mean to call query?",
          "execute rejects result rows");
    check(f.provider.execute(r, query(r, "SELECT 1; SELECT 2")).error().detail() ==
              "Multiple statements provided",
          "multiple statements rejected");
    check(bool(f.provider.fetch_all_compact(r, query(r, "SELECT 1; -- trailing comment"))),
          "trailing comment accepted");
    auto& clone = f.global.make<SqliteMutationExecutor>(f.provider);
    check(&clone.connection() == &f.connection,
          "same-scope provider clone borrows same native resource");
    bool rejected = false;
    try {
        user->make<SqliteMutationExecutor>(f.provider);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "provider clone cannot silently retain another Context");
    const auto capabilities = f.provider.capabilities();
    check(
        capabilities.query && capabilities.mutation && capabilities.transaction &&
            capabilities.schema && capabilities.id_generation && !capabilities.batch_mutation &&
            !capabilities.returning && !capabilities.small_parent_relation_probes,
        "provider retains all eight source capability declarations independently of the SQL adapter");
    bool wrong_thread_rejected = false;
    std::thread worker([&] {
        try {
            f.connection.native_handle();
        } catch (const std::logic_error&) {
            wrong_thread_rejected = true;
        }
    });
    worker.join();
    check(wrong_thread_rejected, "native access is confined to the owning Context thread");
    auto& direct = user->make<SelectQuery>("records");
    direct.comment("Exercise direct transport query");
    direct.purpose("Check compile failure behavior");
    direct.filter(Expr::eq(r, "id", 1));
    auto& invalid_stream = f.provider.query_stream(
        r, QueryRequest(std::allocator_arg, Allocator(r), direct, false, false), 2);
    auto* failure = invalid_stream.wait_next();
    check(failure && !*failure &&
              failure->error().kind() == MutationExecutorError::Kind::SqlCompile,
          "direct provider stream preserves dummy descriptor compile failure");
    check(!invalid_stream.wait_next(), "compile failure is one stream error followed by EOF");
}

static void streams_and_waiters() {
    Fixture f;
    auto holder = f.global.make_user_context();
    auto r = holder->memory_resource();
    auto& stream = f.provider.stream_sql(r, query(r, "SELECT id FROM records ORDER BY id"), 2);
    const auto* first = stream.wait_next();
    check(first && first->value().rows.size() == 2 && !first->value().is_last,
          "first native chunk");
    check(f.provider.fetch_all_compact(r, query(r, "SELECT 1")).error().kind() ==
              MutationExecutorError::Kind::Lock,
          "synchronous contention returns a lock error without blocking the owner thread");
    auto abandoned = f.global.make_user_context();
    auto ar = abandoned->memory_resource();
    auto& cancelled = f.provider.execute_sql(ar, query(ar, "UPDATE records SET version=99"));
    check(!cancelled.poll(), "other Context waits while stream owns guarded cursor");
    auto follower = f.global.make_user_context();
    auto fr = follower->memory_resource();
    auto& next =
        f.provider.fetch_all_compact_sql(fr, query(fr, "SELECT version FROM records WHERE id=1"));
    check(!next.poll(), "second waiter queues");
    auto ticket = next.wake_signal().ticket();
    abandoned.reset();
    check(!next.poll(), "closing queued Context does not steal active connection");
    const auto* second = stream.wait_next();
    check(second && second->value().chunk_index == 1 && !second->value().is_last,
          "exact alignment never fabricates final chunk");
    check(!stream.wait_next() && next.wake_signal().ticket() != ticket,
          "EOF releases cursor and wakes surviving waiter");
    check(next.wait().value()[0].get("version")->try_i64() == 1,
          "cancelled queued write never executes");
    check(first->value().rows[0].get("id")->try_i64() == 1,
          "earlier chunk remains a Context borrow after later polls");
    auto& all = f.provider.stream_sql(r, query(r, "SELECT id FROM records ORDER BY id"), 0);
    auto* chunk = all.wait_next();
    check(chunk && chunk->value().rows.size() == 4 && chunk->value().is_last,
          "native size zero buffers all rows");
    auto& waiting = f.provider.execute_sql(fr, query(fr, "UPDATE records SET version=2"));
    check(!waiting.poll(), "final yielded chunk still retains cursor until next poll/drop");
    all.cancel();
    check(waiting.wait().value() == 4, "cancel releases connection lease");
    auto& empty = f.provider.stream_sql(r, query(r, "SELECT id FROM records WHERE 0"), 0);
    check(!empty.wait_next(), "empty native stream has no chunks");
    auto& sync = holder->make<Result<Vector<StreamChunk>, MutationExecutorError>>(
        f.provider.fetch_stream(r, query(r, "SELECT id FROM records ORDER BY id"), 2));
    check(sync.value().size() == 3 && sync.value().back().is_last &&
              sync.value().back().rows.empty(),
          "synchronous source helper retains empty final chunk");
    auto& partial = f.provider.stream_sql(r, query(r, "SELECT id FROM records ORDER BY id"), 3);
    partial.wait_next();
    auto& close_waiter = f.provider.fetch_all_compact_sql(fr, query(fr, "SELECT 1"));
    check(!close_waiter.poll(), "pending query before holder close");
    holder.reset();
    check(bool(close_waiter.wait()),
          "closing Context finalizes active cursor before releasing arena");
}

static bool cached(SqliteConnection& connection, std::string_view sql) {
    for (auto* s = sqlite3_next_stmt(connection.native_handle(), nullptr); s;
         s = sqlite3_next_stmt(connection.native_handle(), s)) {
        if (sqlite3_sql(s) == sql) {
            return true;
        }
    }
    return false;
}

static void cache_contracts() {
    Fixture f;
    auto user = f.global.make_user_context();
    auto r = user->memory_resource();
    for (unsigned i = 0; i < 70; ++i) {
        auto q = query(r, "SELECT id FROM records");
        q.comment.emplace(std::to_string(i), r);
        check(bool(f.provider.fetch_all_compact(r, q)), "commented query");
    }
    check(f.connection.cached_statement_count() == 64 &&
              f.provider.cached_column_layout_count() == 1,
          "statement cache includes comment; layout cache excludes it");
    f.provider.clear_query_caches();
    for (unsigned i = 0; i < 64; ++i) {
        auto sql = "SELECT " + std::to_string(i);
        check(bool(f.provider.fetch_all_compact(r, query(r, sql))), "fill cache");
    }
    check(f.provider.cached_column_layout_count() == 64, "64 column layouts");
    f.provider.fetch_all_compact(r, query(r, "SELECT 0"));
    f.provider.fetch_all_compact(r, query(r, "SELECT 64"));
    check(cached(f.connection, "SELECT 0") && !cached(f.connection, "SELECT 1") &&
              cached(f.connection, "SELECT 64"),
          "prepared statement cache evicts least recently used entry");
    check(f.provider.cached_column_layout_count() == 1, "column cache clears entirely at capacity");
    f.provider.clear_query_caches();
    f.provider.execute(r, query(r, "CREATE TABLE changing(v BOOLEAN)"));
    f.provider.execute(r, query(r, "INSERT INTO changing VALUES(1)"));
    auto& old = user->make<CompactRow>(
        f.provider.fetch_all_compact(r, query(r, "SELECT v FROM changing")).value()[0]);
    f.provider.execute(r, query(r, "DROP TABLE changing"));
    f.provider.execute(r, query(r, "CREATE TABLE changing(v INTEGER)"));
    f.provider.execute(r, query(r, "INSERT INTO changing VALUES(1)"));
    check(std::holds_alternative<bool>(
              f.provider.fetch_all_compact(r, query(r, "SELECT v FROM changing"))
                  .value()[0]
                  .get("v")
                  ->data),
          "layout cache preserves source schema-stale behavior until explicit clear");
    f.provider.clear_query_caches();
    check(std::holds_alternative<std::int64_t>(
              f.provider.fetch_all_compact(r, query(r, "SELECT v FROM changing"))
                  .value()[0]
                  .get("v")
                  ->data) &&
              std::get<bool>(old.get("v")->data),
          "clearing caches changes future decoding without invalidating borrowed rows");
}

struct Schema final : SchemaProvider {
    const EntityDescriptor& entity;

    explicit Schema(const EntityDescriptor& e) : entity(e) {}

    const EntityDescriptor* get_entity(std::string_view name) const override {
        return name == entity.name ? &entity : nullptr;
    }
};

static void executor_integration() {
    Fixture f;
    auto& entity = f.global.make<EntityDescriptor>("Record");
    entity.table("records");
    entity.property("id", DataType::U64).id();
    entity.property("version", DataType::I64).version();
    entity.property("name", DataType::Text);
    auto& schema = f.global.make_resource<Schema>(entity);
    auto& dialect = f.global.make_resource<SqliteDialect>();
    auto& cache = f.global.make<SqlExecutorState>();
    auto& executor =
        f.global.make<SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>>(
            dialect, f.provider, schema, cache);
    auto user = f.global.make_user_context();
    auto r = user->memory_resource();
    auto& q = user->make<SelectQuery>("Record");
    q.comment("Exercise production SQL transport");
    q.purpose("Check query, stream and transaction behavior");
    q.filter(Expr::eq(r, "id", 1));
    auto& events = user->make<Vector<ExecutionMetadata>>();
    auto& observer = user->make<ExecutionObserver>([&](ExecutionMetadata m) {
        events.emplace_back(std::move(m));
    });
    check(executor.query_observed(
                      r, QueryRequest(std::allocator_arg, Allocator(r), q, false, true), observer)
                  .wait()
                  .value()
                  .rows.size() == 1,
          "production transport drives SQL query executor");
    UpdateCommand command(std::allocator_arg, Allocator(r), "Record", std::uint64_t(1));
    command.value("name", "guarded");
    GuardedMutationRequest request(std::allocator_arg,
                                   Allocator(r),
                                   MutationRequest(std::allocator_arg,
                                                   Allocator(r),
                                                   std::move(command),
                                                   "Exercise guarded transport mutation"),
                                   Expr::eq(r, "version", 1));
    const auto& mutated = executor.mutate_guarded_observed(r, std::move(request), observer).wait();
    check(mutated && mutated.value().affected_rows == 1 && mutated.value().persisted_snapshot,
          "production guarded update performs authoritative snapshot read");
    auto& stream = executor.query_stream_observed(
        r, QueryRequest(std::allocator_arg, Allocator(r), q, false, true), 2, observer);
    auto* chunk = stream.wait_next();
    check(chunk && chunk->value().is_last &&
              chunk->value().rows[0].get("name")->try_text() == "guarded",
          "SQL native stream adapter uses production provider");
    check(!events.empty() &&
              events.back().sql_log.execution_outcome == SqlExecutionOutcome::Success,
          "production flow reports terminal diagnostics");
    auto& tx = executor.begin(r).wait().value().get();
    UpdateCommand update(std::allocator_arg, Allocator(r), "Record", std::uint64_t(1));
    update.value("name", "transactional");
    const auto& saved = tx.mutate(r,
                                  MutationRequest(std::allocator_arg,
                                                  Allocator(r),
                                                  std::move(update),
                                                  "Exercise transaction transport mutation"))
                            .wait();
    check(saved && saved.value().persisted_snapshot,
          "production SQL transaction performs ordinary mutation readback");
    check(tx.query(r, QueryRequest(std::allocator_arg, Allocator(r), q, false, true))
                  .wait()
                  .value()
                  .rows[0]
                  .get("name")
                  ->try_text() == "transactional",
          "production SQL transaction query observes its mutation");
    check(bool(tx.rollback(r).wait()), "production SQL transaction rollback adapter");
    check(executor.query(r, QueryRequest(std::allocator_arg, Allocator(r), q, false, false))
                  .wait()
                  .value()
                  .rows[0]
                  .get("name")
                  ->try_text() == "guarded",
          "production rollback undoes transaction mutation");
}

template <class F>
static void rejected(F&& action, const char* message) {
    bool failed = false;
    try {
        action();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

static void native_transactions() {
    Fixture f;
    auto user = f.global.make_user_context();
    auto r = user->memory_resource();
    auto* db = f.connection.native_handle();
    auto& blocker = f.provider.stream_sql(r, query(r, "SELECT id FROM records"), 1);
    blocker.wait_next();
    auto queued = f.global.make_user_context();
    auto qr = queued->memory_resource();
    check(!f.provider.begin_sql(qr).poll(), "begin queues behind an existing cursor");
    queued.reset();
    blocker.cancel();
    check(sqlite3_get_autocommit(db),
          "closing a queued begin Context does not start or roll back a transaction");
    auto& lazy = f.provider.begin_sql(r);
    check(sqlite3_get_autocommit(db), "native begin stays lazy");
    lazy.cancel();
    check(sqlite3_get_autocommit(db), "unpolled begin cancellation has no SQL effect");
    check(bool(f.provider.begin_transaction(r)), "synchronous begin");
    check(!f.provider.begin_transaction(r), "nested begin keeps SQLite error");
    f.provider.execute(r, query(r, "UPDATE records SET version=8"));
    check(bool(f.provider.rollback_transaction(r)), "synchronous rollback");
    check(!f.provider.commit_transaction(r), "commit outside transaction keeps SQLite error");
    auto& tx = f.provider.begin_sql(r).wait().value().get();
    auto& pending = tx.execute_sql(r, query(r, "UPDATE records SET version=9"));
    rejected(
        [&] {
            tx.commit_sql(r);
        },
        "pending SQL operation blocks transaction consumption");
    pending.cancel();
    tx.execute_sql(r, query(r, "UPDATE records SET name='committed' WHERE id=1")).wait();
    auto& commit = tx.commit_sql(r);
    check(tx.consumed() && !sqlite3_get_autocommit(db),
          "terminal call consumes before native commit");
    rejected(
        [&] {
            tx.fetch_all_compact_sql(r, query(r, "SELECT 1"));
        },
        "consumed native transaction rejects work");
    check(bool(commit.wait()) && sqlite3_get_autocommit(db), "native commit completes");
    auto second = f.global.make_user_context();
    auto sr = second->memory_resource();
    auto& newer = f.provider.begin_sql(sr).wait().value().get();
    newer.execute_sql(sr, query(sr, "UPDATE records SET name='later' WHERE id=1")).wait();
    user.reset();
    check(!sqlite3_get_autocommit(db),
          "old committed Context cannot roll back a newer transaction");
    second.reset();
    check(sqlite3_get_autocommit(db), "abandoned transaction rolls back on Context close");
    auto reader = f.global.make_user_context();
    auto rr = reader->memory_resource();
    check(f.provider.fetch_all_compact(rr, query(rr, "SELECT name,version FROM records WHERE id=1"))
                  .value()[0]
                  .get("name")
                  ->try_text() == "committed",
          "Context rollback preserves committed predecessor");
    auto owner = f.global.make_user_context();
    auto or_ = owner->memory_resource();
    auto& cancelled = f.provider.begin_sql(or_).wait().value().get();
    cancelled.execute_sql(or_, query(or_, "UPDATE records SET version=22")).wait();
    auto& cursor = f.provider.stream_sql(rr, query(rr, "SELECT id FROM records ORDER BY id"), 1);
    using CursorPoll = StreamPoll<Result<StreamChunk, MutationExecutorError>>;
    check(cursor.poll_next().kind == CursorPoll::Kind::Pending,
          "foreign Context cursor waits for the transaction lease");
    auto& terminal = cancelled.commit_sql(or_);
    check(terminal.status() == AsyncStatus::Pending, "unpolled commit remains cancellable");
    terminal.cancel();
    check(sqlite3_get_autocommit(db),
          "cancelled pending commit rolls back without waiting on foreign host memory");
    check(cursor.wait_next() != nullptr, "rollback wakes the queued foreign cursor");
    cursor.cancel();
    check(f.provider.fetch_all_compact(rr, query(rr, "SELECT version FROM records WHERE id=1"))
                  .value()[0]
                  .get("version")
                  ->try_i64() == 1,
          "cancelled commit did not persist writes");
    check(
        sqlite3_exec(
            db,
            "PRAGMA foreign_keys=ON; CREATE TABLE parent(id INTEGER PRIMARY KEY); CREATE TABLE child(id INTEGER REFERENCES parent(id) DEFERRABLE INITIALLY DEFERRED)",
            nullptr,
            nullptr,
            nullptr) == SQLITE_OK,
        "deferred constraint setup");
    auto failure = f.global.make_user_context();
    auto fr = failure->memory_resource();
    auto& failed = f.provider.begin_sql(fr).wait().value().get();
    failed.execute_sql(fr, query(fr, "INSERT INTO child VALUES(999)")).wait();
    const auto& result = failed.commit_sql(fr).wait();
    check(!result && result.error().sqlite_extended_code() == SQLITE_CONSTRAINT_FOREIGNKEY &&
              failed.consumed() && sqlite3_get_autocommit(db),
          "failed consumed native commit preserves the error and rolls back before returning");
    failure.reset();
    check(sqlite3_get_autocommit(db),
          "failed commit is rolled back before its Context is reclaimed");
}

static void native_cleanup() {
    GlobalContext global;
    bool released = false;
    auto user = global.make_user_context();

    struct Evidence {
        UserContext* context;
        bool* released;
    };

    Evidence evidence{user.get(), &released};
    auto& connection = user->make_resource<SqliteConnection>();
    auto& provider = user->make<SqliteMutationExecutor>(connection);
    auto r = user->memory_resource();
    auto target = global.make_user_context();
    auto global_query = query(global.memory_resource(), "SELECT 1");
    for (auto destination : {global.memory_resource(), target->memory_resource()}) {
        bool rejected = false;
        try {
            provider.fetch_all_compact_sql(destination, global_query);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "asynchronous work cannot retain a shorter or sibling connection Context");
    }
    check(sqlite3_create_function_v2(
              connection.native_handle(),
              "cleanup_probe",
              0,
              SQLITE_UTF8,
              &evidence,
              [](sqlite3_context* c, int, sqlite3_value**) {
                  sqlite3_result_int(c, 1);
              },
              nullptr,
              nullptr,
              [](void* p) {
                  auto& e = *static_cast<Evidence*>(p);
                  *e.released = e.context->allocation_stats().live_blocks > 0;
              }) == SQLITE_OK,
          "native cleanup hook");
    auto& stream = provider.stream_sql(
        r, query(r, "SELECT cleanup_probe() UNION ALL SELECT cleanup_probe()"), 1);
    stream.wait_next();
    user->close();
    check(released && user->allocation_stats().live_blocks == 0,
          "native SQLite cleanup runs before arena blocks return");
    bool failed = false;
    try {
        GlobalContext temporary;
        temporary.make_resource<SqliteConnection>("/dev/null/teaql-impossible.db");
    } catch (const std::runtime_error& e) {
        failed = std::string_view(e.what()).size() > 0;
    }
    check(failed, "open failure survives creating Context unwinding");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        calls_and_error_order();
        streams_and_waiters();
        cache_contracts();
        executor_integration();
        native_transactions();
        native_cleanup();
        std::pmr::set_default_resource(previous);
        std::cout << "Production SQLite transport contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
