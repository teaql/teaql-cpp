#include "audited_save_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>
using namespace teaql;
using namespace audited_fixture;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static InMemoryMetadataStore& metadata(UserContext& user) {
    auto& store = user.make<InMemoryMetadataStore>();
    auto descriptor = Model::entity_descriptor(user.memory_resource());
    descriptor.table("orders");
    store.register_entity(std::move(descriptor));
    user.set_metadata(user, store);
    user.disable_sql_log();
    return store;
}

static Model model(Memory r, const EntityRuntimeState* root = nullptr) {
    Model m(std::allocator_arg, Allocator(r));
    m.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    m.values.fields.emplace(String("version", r), Value(r, std::int64_t(3)));
    m.values.fields.emplace(String("name", r), Value(r, "input"));
    if (root) {
        m.runtime.emplace(std::allocator_arg, Allocator(r), *root);
    }
    return m;
}

static void ledger_pipeline_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.rows.emplace_back();
    state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
    state.rows.back().emplace(String("name", r), Value(r, "database"));
    auto& checker = user->make_resource<audited_fixture::Checker>(state, "fill");
    auto& registry = user->make<InMemoryCheckerRegistry>();
    registry.register_checker(*user, checker);
    user->set_checker_registry(*user, registry);
    auto executor = user->make_resource_ref<mutation_fixture::Executor>(state);
    auto& root = user->make<EntityRuntimeState>();
    auto identity = user->make_ref<EntityKey>("Order", Value(r, std::uint64_t(7)));
    root.mark_as_new(user->get(identity));
    root.set(user->get(identity), "name", Value(r, "pending"));
    auto audited = Audited<Model>(std::allocator_arg, Allocator(r), model(r, &root), "reason");
    auto& result =
        save_audited_ledger_entity_with_executor(std::move(audited), *user, executor).wait();
    check(result && result.value().ledger && !result.value().entity.restored &&
              result.value().entity.values.fields.at(String("name", r)).try_text() == "database",
          "typed result uses authoritative row and invalidates relations for CREATE");
    check(checker.calls == 1 && user->last_fix_evidence().size() == 1,
          "complete graph preflight and evidence scope run once on the ledger path");
    check(root.is_new(user->get(identity)) &&
              root.get(user->get(identity), "name")->try_text() == "fixed",
          "Fix merges into the pending ledger without clearing it");
    check(state.requests.size() == 1 && std::get<InsertCommand>(state.requests[0].data)
                                                .values.fields.at(String("name", r))
                                                .try_text() == "fixed",
          "exact fixed payload reaches the executor");
    auto& retained = global.make<AuditedLedgerResult<Model>>(result.value());
    user.reset();
    check(retained.entity.values.fields.at(String("name", global.memory_resource())).try_text() ==
                  "database" &&
              retained.ledger->new_keys().size() == 1,
          "explicit Global result copies include entity and retained mutation ledger");
}

static void pending_and_failure() {
    GlobalContext global;
    unsigned cancelled = 0;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.read_pending = true;
    state.external_cancel = &cancelled;
    auto executor = user->make_resource_ref<mutation_fixture::Executor>(state);
    auto& root = user->make<EntityRuntimeState>();
    EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
    root.set(key, "name", Value(r, "pending"));
    root.set_original_version(key, 3);
    auto audited = Audited<Model>(std::allocator_arg, Allocator(r), model(r, &root), "reason");
    auto& work = save_audited_ledger_entity_with_executor(std::move(audited), *user, executor);
    check(!work.poll(), "typed save suspends during ledger classification");
    user.reset();
    check(cancelled == 1, "closing User joins suspended typed-save descendants");
    auto next = global.make_user_context();
    auto target = next->memory_resource();
    metadata(*next);
    auto& failed = next->make<mutation_fixture::State>();
    auto& checker = next->make_resource<audited_fixture::Checker>(failed, "error");
    auto& registry = next->make<InMemoryCheckerRegistry>();
    registry.register_checker(*next, checker);
    next->set_checker_registry(*next, registry);
    auto provider = next->make_resource_ref<mutation_fixture::Executor>(failed);
    auto input = Audited<Model>(std::allocator_arg, Allocator(target), model(target), "reason");
    auto& rejected =
        save_audited_ledger_entity_with_executor(std::move(input), *next, provider).wait();
    check(!rejected && failed.requests.empty() && next->last_fix_evidence().size() == 1,
          "typed preflight failure precedes I/O and still publishes Fix evidence");
}

static void sqlite_outer_transaction() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& store = metadata(*user);
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (auto statement : {"CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
                           "INSERT INTO orders VALUES(7,3,'original')"}) {
        sql.sql = statement;
        check(bool(transport.execute(r, sql)), "SQLite audited fixture setup");
    }
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& cache = user->make<SqlExecutorState>();
    auto& executor =
        user->make<SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>>(
            dialect, transport, store, cache);
    auto& transaction = executor.begin(r).wait().value().get();
    using Transaction = std::remove_reference_t<decltype(transaction)>;
    ResourceBorrow<Transaction> borrow(
        std::allocator_arg, Allocator(r), transaction, detail::resource_arena(r));
    auto& root = user->make<EntityRuntimeState>();
    auto identity = user->make_ref<EntityKey>("Order", Value(r, std::uint64_t(7)));
    root.set_original_version(user->get(identity), 3);
    root.set(user->get(identity), "name", Value(r, "updated"));
    auto audited =
        Audited<Model>(std::allocator_arg, Allocator(r), model(r, &root), "outer transaction");
    auto& result =
        save_audited_ledger_entity_with_executor(std::move(audited), *user, borrow).wait();
    check(result && result.value().ledger && result.value().entity.restored &&
              result.value().entity.values.fields.at(String("version", r)).try_i64() == 4 &&
              root.get(user->get(identity), "name")->try_text() == "updated" &&
              !transaction.consumed(),
          "typed save uses outer transaction and leaves both transaction and intent pending");
    auto& retained = global.make<AuditedLedgerResult<Model>>(result.value());
    check(retained.entity.runtime && retained.ledger &&
              *retained.entity.runtime == *retained.ledger,
          "one Global copy session preserves entity and pending-ledger identity");
    check(bool(transaction.rollback(r).wait()), "outer owner can roll back typed save");
    sql.sql = "SELECT version,name FROM orders WHERE id=7";
    auto& rows = transport.fetch_all_compact_sql(r, sql).wait();
    check(rows && rows.value()[0].get("version")->try_i64() == 3 &&
              rows.value()[0].get("name")->try_text() == "original" &&
              root.current_change_set().changes().size() == 1,
          "rollback restores the database while preserving retryable in-memory intent");
    user.reset();
    EntityKey kept(std::allocator_arg,
                   Allocator(global.memory_resource()),
                   "Order",
                   Value(global.memory_resource(), std::uint64_t(7)));
    retained.ledger->set(kept, "name", Value(global.memory_resource(), "retained change"));
    check(retained.entity.runtime->get(kept, "name")->try_text() == "retained change",
          "retained entity and ledger still share destination state after User destruction");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ledger_pipeline_and_retention();
        pending_and_failure();
        sqlite_outer_transaction();
        std::pmr::set_default_resource(previous);
        std::cout << "Audited save contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
