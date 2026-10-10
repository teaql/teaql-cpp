#include "runtime_mutation_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>

using namespace teaql;
using namespace mutation_fixture;
using Access = detail::EntityDataServiceAccess;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Id final : InternalIdGenerator {
    mutable std::uint64_t next = 100;

    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        return Result<std::uint64_t, RuntimeError>::ok(r, next++);
    }
};

static InMemoryMetadataStore& metadata(UserContext& user, bool required = false) {
    auto r = user.memory_resource();
    auto& store = user.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Order");
    descriptor.table("orders");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text);
    if (required) {
        descriptor.properties.back().not_null();
    }
    store.register_entity(std::move(descriptor));
    user.set_metadata(user, store);
    auto& ids = user.make_resource<Id>();
    user.set_internal_id_generator(user, ids);
    user.disable_sql_log();
    return store;
}

static EntityKey key(Memory r, std::uint64_t id) {
    return EntityKey(std::allocator_arg, Allocator(r), "Order", Value(r, id));
}

struct CountingChecker final : teaql::Checker {
    mutable unsigned calls = 0;

    std::string_view entity() const override {
        return "Order";
    }

    void check_and_fix(const UserContext&,
                       EntityValues&,
                       const ObjectLocation&,
                       CheckResults&) const override {
        ++calls;
    }
};

static void required_gate_before_deletes() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user, true);
    auto& state = user->make<State>();
    auto& executor = user->make_resource<Executor>(state);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& root = user->make<EntityRuntimeState>();
    auto deleted = key(r, 7), fresh = key(r, 0);
    root.mark_as_delete(deleted);
    root.set_original_version(deleted, 3);
    root.mark_as_new(fresh);
    root.set(fresh, "other", Value(r, true));
    auto& locations = user->make<detail::LedgerObjectLocations>();
    auto& result = Access::execute_ledger_plan_internal(service, root, locations).wait();
    check(!result && result.error().runtime_error().kind() == RuntimeError::Kind::Check &&
              state.requests.empty() && state.queries.empty(),
          "required CREATE gate rejects all writes, including earlier deletions");
    check(root.is_new(fresh) && root.is_marked_as_delete(deleted),
          "failed execution preserves the pending ledger");
}

static void pending_work_closes() {
    GlobalContext global;
    for (bool read : {true, false}) {
        unsigned cancelled = 0;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        metadata(*user);
        auto& state = user->make<State>();
        state.allow_reads = true;
        state.read_pending = read;
        state.pending = !read;
        state.external_cancel = &cancelled;
        auto& executor = user->make_resource<Executor>(state);
        auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
        auto& root = user->make<EntityRuntimeState>();
        auto identity = key(r, 7);
        root.set(identity, "name", Value(r, "pending"));
        if (!read) {
            root.mark_as_new(identity);
        }
        auto& locations = user->make<detail::LedgerObjectLocations>();
        auto& work = Access::execute_ledger_plan_internal(service, root, locations);
        check(!work.poll(), "ledger child remains pending");
        user.reset();
        check(cancelled == 1,
              "User destruction cancels and joins the suspended child exactly once");
    }
}

static void version_overflow() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<State>();
    state.allow_reads = true;
    state.rows.emplace_back();
    state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
    auto& executor = user->make_resource<Executor>(state);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& root = user->make<EntityRuntimeState>();
    auto identity = key(r, 7);
    root.set(identity, "name", Value(r, "overflow"));
    root.set_original_version(identity, std::numeric_limits<std::int64_t>::max());
    auto& locations = user->make<detail::LedgerObjectLocations>();
    auto& work = Access::execute_ledger_plan_internal(service, root, locations);
#ifndef NDEBUG
    bool rejected = false;
    try {
        (void)work.wait();
    } catch (const std::overflow_error&) {
        rejected = true;
    }
    check(rejected && state.requests.empty(), "debug overflow rejects before update dispatch");
#else
    check(bool(work.wait()) && state.requests.size() == 1 &&
              std::get<UpdateCommand>(state.requests.front().data)
                      .values.fields.at(String("version", r))
                      .try_i64() == std::numeric_limits<std::int64_t>::min(),
          "release version arithmetic wraps without C++ signed overflow");
#endif
}

static void expired_metadata_after_read() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->disable_sql_log();
    auto metadata = user->make_ref<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Order");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    user->get(metadata).register_entity(std::move(descriptor));
    user->set_metadata(*user, user->get(metadata));
    auto& state = user->make<State>();
    state.allow_reads = true;
    state.read_pending = true;
    state.rows.emplace_back();
    state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
    auto& executor = user->make_resource<Executor>(state);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& root = user->make<EntityRuntimeState>();
    auto identity = key(r, 7);
    root.set(identity, "name", Value(r, "pending"));
    root.set_original_version(identity, 1);
    auto& locations = user->make<detail::LedgerObjectLocations>();
    auto& work = Access::execute_ledger_plan_internal(service, root, locations);
    check(!work.poll(), "classification suspends while metadata remains live");
    user->destroy(metadata);
    state.read_pending = false;
    bool rejected = false;
    try {
        (void)work.poll();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Dead;
    }
    check(rejected && state.requests.empty(),
          "metadata owner expiry rejects the resumed descriptor view before a mutation");
}

static void sqlite_execution_and_rollback() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& store = metadata(*user);
    auto& state = user->make<State>();
    auto& sink = user->make_resource<Sink>(state);
    user->set_event_sink(*user, sink);
    auto& checker = user->make_resource<CountingChecker>();
    auto& registry = user->make<InMemoryCheckerRegistry>();
    registry.register_checker(*user, checker);
    user->set_checker_registry(*user, registry);
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (auto statement : {"CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
                           "INSERT INTO orders VALUES(7,3,'loaded'),(9,2,'remove')"}) {
        sql.sql = statement;
        check(bool(transport.execute(r, sql)), "SQLite ledger setup");
    }
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& cache = user->make<SqlExecutorState>();
    auto& executor =
        user->make<SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>>(
            dialect, transport, store, cache);
    auto& transaction = executor.begin(r).wait().value().get();
    using Transaction = std::remove_reference_t<decltype(transaction)>;
    auto& service =
        EntityDataService<Transaction>::for_executor(*user, "Order", *user, transaction);
    auto& root = user->make<EntityRuntimeState>();
    auto changed = key(r, 7), removed = key(r, 9), fresh = key(r, 0);
    root.set(changed, "name", Value(r, "updated"));
    root.set_original_version(changed, 3);
    root.mark_as_delete(removed);
    root.set_original_version(removed, 2);
    root.mark_as_new(fresh);
    root.set(fresh, "name", Value(r, "inserted"));
    root.set_comment("ledger reason");
    auto& locations = user->make<detail::LedgerObjectLocations>();
    auto& result = Access::execute_ledger_plan_internal(service, root, locations).wait();
    check(result && result.value().at(fresh).try_u64() == 100,
          "ledger returns its generated identity mapping");
    check(checker.calls == 0, "sparse execution does not repeat typed Checker/Fix");
    check(state.events.size() == 3 && state.events[0].kind == RawAuditEventKind::Deleted &&
              state.events[1].kind == RawAuditEventKind::Created &&
              state.events[2].kind == RawAuditEventKind::Updated,
          "events follow Delete, Insert and Update source call sites");
    check(root.is_new(fresh) && root.is_marked_as_delete(removed) &&
              root.get_original_version(changed) == 3,
          "executor leaves ledger completion to its transaction owner");
    sql.sql = "SELECT id,version,name FROM orders ORDER BY id";
    auto& rows = transport.fetch_all_compact_sql(r, sql).wait();
    check(rows && rows.value().size() == 3 && rows.value()[0].get("version")->try_i64() == 4 &&
              rows.value()[1].get("version")->try_i64() == -3 &&
              rows.value()[2].get("id")->try_u64() == 100,
          "real SQLite executes guarded update, soft delete and generated insert");
    auto& retained = global.make<detail::LedgerGeneratedIds>(result.value());
    check(bool(transaction.rollback(r).wait()), "caller can roll back all ledger writes");
    auto& original = transport.fetch_all_compact_sql(r, sql).wait();
    check(original && original.value().size() == 2 &&
              original.value()[0].get("version")->try_i64() == 3 &&
              original.value()[1].get("version")->try_i64() == 2,
          "rollback restores every version and removes the generated row");
    user.reset();
    check(retained.size() == 1 && retained.begin()->second.try_u64() == 100,
          "explicit Global identity map survives actual User destruction");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        required_gate_before_deletes();
        pending_work_closes();
        version_overflow();
        expired_metadata_after_read();
        sqlite_execution_and_rollback();
        std::pmr::set_default_resource(previous);
        std::cout << "Ledger execution contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
