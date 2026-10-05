#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/provider_sqlite/detail/schema.hpp>
#include <teaql/runtime/module.hpp>
#include <sqlite3.h>
#include <iostream>
using namespace teaql;

static void check(bool v, const char* message) {
    if (!v) {
        throw std::runtime_error(message);
    }
}

static EntityDescriptor
descriptor(Memory r, std::string_view name = "Order", std::string_view table = "orders") {
    EntityDescriptor e(std::allocator_arg, Allocator(r), name);
    e.table(table);
    e.property("id", DataType::I64).id().not_null();
    e.property("version", DataType::I64).version().not_null();
    e.property("name", DataType::Text);
    return e;
}

struct Sink final : RawAuditEventSink {
    using allocator_type = Allocator;
    sqlite3* db;
    Vector<RawAuditEvent> events;
    bool fail = false, panic = false;
    bool before_commit = true, normal = true;

    Sink(std::allocator_arg_t, Allocator a, sqlite3* db) : db(db), events(a.resource()) {}

    Result<std::monostate, RuntimeError>
    on_event(Memory r, const UserContext& context, const RawAuditEvent& e) const override {
        auto& self = const_cast<Sink&>(*this);
        self.events.emplace_back(e);
        self.before_commit &= sqlite3_get_autocommit(db) == 0;
        self.normal &= !detail::is_generated_schema_bootstrap(context);
        if (panic) {
            throw std::runtime_error("sink panic");
        }
        if (fail) {
            return Result<std::monostate, RuntimeError>::err(
                r, RuntimeError::event(r, "ignored schema audit sink error"));
        }
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

struct Fixture {
    GlobalContext global;
    SqliteConnection& connection = global.make_resource<SqliteConnection>();
    SqliteMutationExecutor& executor = global.make_resource<SqliteMutationExecutor>(connection);
    std::unique_ptr<UserContext> user = global.make_user_context();
    Sink& sink = global.make<Sink>(connection.native_handle());

    Fixture() {
        use_sqlite_provider(*user, global, executor);
        user->set_event_sink(global, sink);
    }

    void metadata(std::span<const EntityDescriptor> descriptors) {
        auto& store = user->make<InMemoryMetadataStore>();
        for (const auto& e : descriptors) {
            store.register_entity(detail::clone_in(user->memory_resource(), e));
        }
        user->set_metadata(*user, store);
    }

    void one(const EntityDescriptor& e) {
        metadata({&e, 1});
    }

    void exec(std::string_view sql) {
        CompiledQuery q(std::allocator_arg, Allocator(user->memory_resource()));
        q.sql = sql;
        auto result = executor.execute(user->memory_resource(), q);
        if (!result) {
            throw std::runtime_error(
                std::string(result.error().to_string(user->memory_resource())));
        }
    }

    void prime() {
        CompiledQuery q(std::allocator_arg, Allocator(user->memory_resource()));
        q.sql = "SELECT id FROM orders";
        check(bool(executor.fetch_all_compact(user->memory_resource(), q)),
              "prime provider caches");
        check(executor.cached_column_layout_count() > 0 && connection.cached_statement_count() > 0,
              "both provider caches populated");
    }
};

static void lifecycle() {
    Fixture f;
    auto r = f.user->memory_resource();
    auto& entity = f.user->make<EntityDescriptor>(descriptor(r));
    entity.property("İΣ", DataType::Text);
    f.one(entity);
    f.sink.fail = true;
    check(bool(f.user->ensure_schema().wait()), "schema ignores sink Result errors");
    check(f.sink.events.size() == 2 && f.sink.events[0].kind == RawAuditEventKind::SchemaCreated &&
              f.sink.events[1].kind == RawAuditEventKind::SchemaVerified,
          "granular source event order");
    check(f.sink.before_commit && f.sink.normal,
          "DDL events are inside transaction and outside bootstrap mode");
    f.prime();
    f.sink.fail = false;
    check(bool(f.user->ensure_schema().wait()), "idempotent schema");
    check(f.executor.cached_column_layout_count() == 0 &&
              f.connection.cached_statement_count() == 0,
          "successful schema invalidates both caches");
    {
        auto columns = f.executor.table_columns(r, "orders");
        check(bool(columns) && columns.value().contains(std::string_view("i\xcc\x87\xcf\x82")),
              "table columns use Unicode lowercase and final sigma");
        auto& kept = f.global.make<SqliteMutationExecutor::TableColumns>(columns.value());
        check(kept.begin()->first.get_allocator().resource() == f.global.memory_resource() &&
                  kept.begin()->second.first.get_allocator().resource() ==
                      f.global.memory_resource(),
              "column map nested PMR propagation");
    }
    entity.property("required", DataType::Bool).not_null();
    f.one(entity);
    check(bool(f.user->ensure_schema().wait()), "required column can be added to empty table");
    f.exec("INSERT INTO orders(id, version, required) VALUES(1, 1, 0)");
    entity.property("later", DataType::Text).not_null();
    f.one(entity);
    {
        auto& result = f.user->ensure_schema().wait();
        check(!result &&
                  result.error().message().find("deterministic backfill") != std::string_view::npos,
              "required column on populated table rejected");
        auto columns = f.executor.table_columns(r, "orders");
        check(!columns.value().contains(std::string_view("later")),
              "failed evolution leaves no required column");
    }
    f.user->close();
    check(f.user->allocation_stats().live_blocks == 0 && f.sink.events[0].entity == "Order",
          "Global audit retention survives User reclamation");
}

static void rollback_and_commit_order() {
    Fixture f;
    auto r = f.user->memory_resource();
    auto e = descriptor(r);
    f.one(e);
    check(bool(f.user->ensure_schema().wait()), "base schema");
    f.prime();
    auto cached = f.connection.cached_statement_count();
    f.sink.events.clear();
    EntityDescriptor bad(std::allocator_arg, Allocator(r), "Order");
    bad.table("orders");
    bad.property("added", DataType::Text);
    bad.property("name", DataType::I64);
    f.one(bad);
    auto& failed = f.user->ensure_schema().wait();
    check(!failed &&
              failed.error().message().find("incompatible column type") != std::string_view::npos,
          "type mismatch aborts evolution");
    auto columns = f.executor.table_columns(r, "orders");
    check(!columns.value().contains(std::string_view("added")) &&
              sqlite3_get_autocommit(f.connection.native_handle()),
          "earlier DDL rolled back");
    check(f.sink.events.size() == 1 && f.sink.events[0].kind == RawAuditEventKind::FieldAdded,
          "already emitted event remains visible after rollback");
    check(f.connection.cached_statement_count() == cached &&
              f.executor.cached_column_layout_count() > 0,
          "failed schema retains caches");
    f.one(e);
    f.sink.events.clear();

    struct Hook {
        sqlite3* db;

        ~Hook() {
            sqlite3_commit_hook(db, nullptr, nullptr);
        }
    } hook{f.connection.native_handle()};

    sqlite3_commit_hook(
        hook.db,
        [](void*) {
            return 1;
        },
        nullptr);
    auto& commit = f.user->ensure_schema().wait();
    check(!commit && sqlite3_get_autocommit(hook.db) && f.sink.events.size() == 1,
          "failed COMMIT rolls back after verification event");
    check(f.executor.cached_column_layout_count() > 0, "failed commit retains layout cache");
    sqlite3_commit_hook(hook.db, nullptr, nullptr);
    GraphNode root(std::allocator_arg, Allocator(r), "Order");
    f.user->set_root_graphs({&root, 1});
    auto& legacy = f.user->ensure_schema().wait();
    check(
        !legacy &&
            legacy.error().message() ==
                "rusqlite bind error: generated root/constant bootstrap must use the typed RuntimeModule callback",
        "legacy graph rejection after physical schema");
    check(f.executor.cached_column_layout_count() == 0 &&
              f.connection.cached_statement_count() == 0 && sqlite3_get_autocommit(hook.db),
          "legacy rejection follows commit and cache invalidation");
}

static void relations_and_indexes() {
    Fixture f;
    auto r = f.user->memory_resource();
    Vector<EntityDescriptor> entities(r);
    entities.emplace_back(descriptor(r, "Parent", "parents"));
    entities.emplace_back(descriptor(r, "Child", "children"));
    entities.back().property("parentId", DataType::I64);
    entities.back().relation("parent", "Parent").local("parentId").foreign("id");
    f.metadata(entities);
    check(bool(f.user->ensure_schema().wait()), "foreign key DDL creates both entities");
    check(bool(f.user->ensure_schema().wait()), "FK and indexes verify idempotently");
    CompiledQuery q(std::allocator_arg, Allocator(r));
    q.sql = "INSERT INTO children(id,version,parentId) VALUES(1,1,999)";
    auto missing = f.executor.execute(r, q);
    check(!missing, "physical foreign key enforced");
    f.exec("DROP INDEX IDX_CHILDREN_PARENTID");
    f.exec("CREATE INDEX IDX_CHILDREN_PARENTID ON children(parentId DESC)");
    auto& mismatch = f.user->ensure_schema().wait();
    check(!mismatch && mismatch.error().message().find("ascending BINARY full-column keys") !=
                           std::string_view::npos,
          "colliding descending index rejected");
}

static void connection_wait_and_close() {
    Fixture f;
    auto r = f.user->memory_resource();
    auto& e = f.user->make<EntityDescriptor>(descriptor(r));
    f.one(e);
    check(bool(f.user->ensure_schema().wait()), "base schema for stream");
    f.exec("INSERT INTO orders(id,version) VALUES(1,1),(2,1)");
    auto holder = f.global.make_user_context();
    CompiledQuery q(std::allocator_arg, Allocator(holder->memory_resource()));
    q.sql = "SELECT id FROM orders";
    auto& stream = f.executor.stream_sql(holder->memory_resource(), std::move(q), 1);
    check(stream.wait_next() != nullptr, "stream holds connection lease");
    auto& schema = f.user->ensure_schema();
    check(schema.poll() == nullptr, "schema waits on whole connection lease");
    auto ticket = schema.wake_signal().ticket();
    stream.cancel();
    check(ticket != schema.wake_signal().ticket() && bool(schema.wait()),
          "stream release wakes schema operation");
    CompiledQuery again(std::allocator_arg, Allocator(holder->memory_resource()));
    again.sql = "SELECT id FROM orders";
    auto& second = f.executor.stream_sql(holder->memory_resource(), std::move(again), 1);
    second.wait_next();
    check(f.user->ensure_schema().poll() == nullptr, "second schema queued");
    f.user->close();
    second.cancel();
    check(f.user->allocation_stats().live_blocks == 0,
          "pending schema withdraws native lease before reclaim");
    auto next = f.global.make_user_context();
    use_sqlite_provider(*next, f.global, f.executor);
    check(bool(next->ensure_schema().wait()), "connection reusable after pending User close");
}

static void exceptions_and_missing() {
    Fixture f;
    auto r = f.user->memory_resource();
    auto e = descriptor(r);
    f.one(e);
    f.sink.panic = true;
    bool threw = false;
    try {
        f.user->ensure_schema().wait();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw && sqlite3_get_autocommit(f.connection.native_handle()) == 0,
          "sink panic propagates and preserves Rust unwinding transaction state");
    auto locked = f.executor.table_exists(r, "orders");
    check(!locked && locked.error().kind() == MutationExecutorError::Kind::Lock,
          "panic poisons native connection lease"); // native connection teardown rolls back
    GlobalContext global;
    auto user = global.make_user_context();
    auto& provider = user->make<SqliteSchemaProvider>();
    user->set_schema_provider(*user, provider);
    auto& missing = user->ensure_schema().wait();
    check(!missing && missing.error().message() ==
                          "rusqlite bind error: missing typed resource: SqliteDialect",
          "typed dialect required first");
    auto& dialect = user->make_resource<SqliteDialect>();
    user->insert_resource(*user, dialect);
    auto& missing_executor = user->ensure_schema().wait();
    check(!missing_executor &&
              missing_executor.error().message() ==
                  "rusqlite bind error: missing typed resource: SqliteMutationExecutor",
          "typed executor required second");
}

int main() {
    try {
        setenv("TEAQL_LOG_ENDPOINT", "off", 1);
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        lifecycle();
        rollback_and_commit_order();
        relations_and_indexes();
        connection_wait_and_close();
        exceptions_and_missing();
        std::pmr::set_default_resource(old);
        std::cout << "SQLite schema provider contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
