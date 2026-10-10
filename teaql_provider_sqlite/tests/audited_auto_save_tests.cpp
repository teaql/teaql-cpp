#include "../../tests/support/audited_save_fixture.hpp"
#include "../../tests/support/graph_saver_fixture.hpp"
#include "../../tests/support/save_preparation_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static InMemoryMetadataStore& metadata(UserContext& user) {
    auto& store = user.make<InMemoryMetadataStore>();
    auto d = audited_fixture::Model::entity_descriptor(user.memory_resource());
    d.table("orders");
    store.register_entity(std::move(d));
    store.register_entity(save_fixture::Model::entity_descriptor(user.memory_resource()));
    user.set_metadata(user, store);
    user.disable_sql_log();
    return store;
}

static audited_fixture::Model input(Memory r, const EntityRuntimeState* root = nullptr) {
    audited_fixture::Model m(std::allocator_arg, Allocator(r));
    m.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    m.values.fields.emplace(String("version", r), Value(r, std::int64_t(3)));
    m.values.fields.emplace(String("name", r), Value(r, "input"));
    m.fresh = true;
    if (root) {
        m.runtime.emplace(std::allocator_arg, Allocator(r), *root);
    }
    return m;
}

static void entry_order_and_evidence() {
    GlobalContext global;
    for (bool ledger : {false, true}) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        metadata(*user);
        auto& state = user->make<mutation_fixture::State>();
        auto& checker = user->make_resource<audited_fixture::Checker>(state, "error");
        auto& registry = user->make<InMemoryCheckerRegistry>();
        registry.register_checker(*user, checker);
        user->set_checker_registry(*user, registry);
        auto audited =
            Audited<audited_fixture::Model>(std::allocator_arg, Allocator(r), input(r), "reason");
        auto& work = ledger
                         ? save_audited_ledger_entity(std::move(audited), *user)
                         : AuditedSaveExt<audited_fixture::Model>::save(std::move(audited), *user);
        auto& result = work.wait();
        check(!result && checker.calls == unsigned(!ledger),
              "plain preflight precedes missing saver; ledger lookup precedes preflight");
        check(user->last_fix_evidence().empty(),
              "missing saver/plain entry does not publish ledger Fix evidence");
    }
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    auto& controls = user->make<saver_fixture::Controls>();
    auto executor = user->make_resource_ref<saver_fixture::Ambient>(state, controls);
    user->register_executor(executor);
    auto& checker = user->make_resource<audited_fixture::Checker>(state, "error");
    auto& registry = user->make<InMemoryCheckerRegistry>();
    registry.register_checker(*user, checker);
    user->set_checker_registry(*user, registry);
    auto audited =
        Audited<audited_fixture::Model>(std::allocator_arg, Allocator(r), input(r), "reason");
    auto& result = save_audited_ledger_entity(std::move(audited), *user).wait();
    check(!result && user->last_fix_evidence().size() == 1 && !user->get(executor).transaction,
          "automatic ledger preflight failure publishes evidence before opening a transaction");
}

static void cancelled_automatic_entry() {
    GlobalContext global;
    unsigned cancelled = 0;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.read_pending = true;
    state.external_cancel = &cancelled;
    auto& controls = user->make<saver_fixture::Controls>();
    auto executor = user->make_resource_ref<saver_fixture::Ambient>(state, controls);
    user->register_executor(executor);
    auto& root = user->make<EntityRuntimeState>();
    EntityKey identity(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
    root.mark_as_new(identity);
    root.set(identity, "name", Value(r, "pending"));
    auto audited = Audited<audited_fixture::Model>(
        std::allocator_arg, Allocator(r), input(r, &root), "reason");
    auto& work = save_audited_ledger_entity(std::move(audited), *user);
    check(!work.poll(), "automatic audited entry suspends at authoritative read");
    work.cancel();
    check(cancelled == 1 && controls.abandoned == 1 && root.is_new(identity),
          "cancellation joins read, releases transaction and preserves uncommitted intent");
    check(!detail::current_mutation_governance(*user), "cancel restores governance activation");
}

static void sqlite_commit_and_reconstruction() {
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
        check(bool(transport.execute(r, sql)), "SQLite automatic audited setup");
    }
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& cache = user->make<SqlExecutorState>();
    using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
    auto executor = user->make_ref<Executor>(dialect, transport, store, cache);
    user->register_executor(executor);
    auto& root = user->make<EntityRuntimeState>();
    auto identity = user->make_ref<EntityKey>("Order", Value(r, std::uint64_t(7)));
    root.set_original_version(user->get(identity), 3);
    root.set(user->get(identity), "name", Value(r, "committed"));
    auto& model = user->make<audited_fixture::Model>(input(r, &root));
    model.fresh = false;
    auto& audited = user->make<Audited<audited_fixture::Model>>(std::move(model), "reason");
    auto& result = save_audited_ledger_entity(std::move(audited), *user).wait();
    check(result && result.value().restored &&
              result.value().values.fields.at(String("version", r)).try_i64() == 4 &&
              root.current_change_set().changes().empty(),
          "automatic ledger entry commits authoritative row and restores safe runtime state");
    auto& retained = global.make<audited_fixture::Model>(result.value());
    // The ordinary extension must support an Entity that has no ledger API.
    static_assert(!LedgerEntity<save_fixture::Model>);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.rows.emplace_back();
    state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(1)));
    auto& controls = user->make<saver_fixture::Controls>();
    auto fake = user->make_resource_ref<saver_fixture::Ambient>(state, controls);
    user->register_executor(fake);
    auto& plain = user->make<save_fixture::Model>();
    plain.fresh = true;
    plain.values.fields.emplace(String("id", r), Value(r, std::uint64_t(1)));
    auto& normal = user->make<Audited<save_fixture::Model>>(std::move(plain), "plain reason");
    check(bool(AuditedSaveExt<save_fixture::Model>::save(std::move(normal), *user).wait()),
          "plain extension preserves the non-ledger Entity abstraction");
    user.reset();
    check(retained.values.fields.at(String("name", global.memory_resource())).try_text() ==
                  "committed" &&
              retained.runtime,
          "explicit retained typed result survives actual User destruction");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        entry_order_and_evidence();
        cancelled_automatic_entry();
        sqlite_commit_and_reconstruction();
        std::pmr::set_default_resource(previous);
        std::cout << "Automatic audited contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
