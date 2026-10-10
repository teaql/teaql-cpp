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

static void metadata(UserContext& user) {
    auto r = user.memory_resource();
    auto& store = user.make<InMemoryMetadataStore>();
    for (auto name : {"Order", "Child"}) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), name);
        d.property("id", DataType::U64).id();
        d.property("version", DataType::I64).version();
        d.property("name", DataType::Text);
        store.register_entity(std::move(d));
    }
    user.set_metadata(user, store);
}

static GraphMutationPlan plan(Memory r, GraphMutationKind kind = GraphMutationKind::Create) {
    GraphMutationPlan p(std::allocator_arg, Allocator(r));
    p.planned_root.emplace(std::allocator_arg, Allocator(r), "Order");
    p.planned_root->value("id", std::uint64_t(7)).value("name", "input").with_comment("reason");
    GraphNode child(std::allocator_arg, Allocator(r), "Child");
    child.value("id", std::uint64_t(8)).value("name", "child input");
    p.planned_root->relation("children", child);
    MutationValues values(r);
    values.fields = clone_record(r, p.planned_root->values.fields);
    values.fields.emplace(String("version", r), Value(r, std::int64_t(1)));
    Vector<String> fields(r);
    fields.emplace_back("name");
    p.push("Order", kind, values, fields);
    p.rebuild_batches();
    return p;
}

static void execution_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->disable_sql_log();
    metadata(*user);
    auto& state = user->make<State>();
    state.allow_reads = true;
    state.rows.emplace_back();
    state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(7)));
    state.rows.back().emplace(String("version", r), Value(r, std::int64_t(9)));
    state.rows.back().emplace(String("name", r), Value(r, "database"));
    auto& executor = user->make_resource<Executor>(state);
    auto& sink = user->make_resource<Sink>(state);
    user->set_event_sink(*user, sink);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& future = Access::execute_graph_plan_internal(service, plan(r));
    auto& result = future.wait();
    check(result && result.value().values.fields.at(String("name", r)).try_text() == "database",
          "authoritative root read replaces planned values");
    check(state.notes.size() == 3 && state.notes[0] == "provider" && state.notes[1] == "event" &&
              state.notes[2] == "query",
          "event precedes root reread");
    check(result.value()
                  .relations.at(String("children", r))[0]
                  .values.fields.at(String("name", r))
                  .try_text() == "child input",
          "root reread preserves supplied child graph");
    auto& retained = global.make<GraphNode>(result.value());
    user.reset();
    check(retained.values.fields.at(String("name", global.memory_resource())).try_text() ==
              "database",
          "Global root deep-copy survives User destruction");
}

static void pending_read_and_failure() {
    GlobalContext global;
    unsigned cancellations = 0;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    user->disable_sql_log();
    auto& state = user->make<State>();
    state.allow_reads = true;
    state.read_pending = true;
    state.external_cancel = &cancellations;
    auto& executor = user->make_resource<Executor>(state);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& work = Access::execute_graph_plan_internal(service, plan(r));
    check(!work.poll() && state.requests.size() == 1 && state.queries.size() == 1,
          "write completes before pending reread");
    user.reset();
    check(cancellations == 1, "Context close joins suspended root reread");
    auto other = global.make_user_context();
    auto m = other->memory_resource();
    metadata(*other);
    auto& failed = other->make<State>();
    failed.allow_reads = true;
    auto& provider = other->make_resource<Executor>(failed);
    auto& repo = EntityDataService<Executor>::for_executor(*other, "Order", *other, provider);
    auto& error = Access::execute_graph_plan_internal(repo, plan(m)).wait();
    check(!error && error.error().runtime_error().message() ==
                        "persisted Order record could not be read back",
          "missing authoritative row fails after writing");
}

struct Deny final : MutationPolicy {
    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(std::allocator_arg, Allocator(r), "deny", "1", "fingerprint");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan&) const override {
        return MutationDecision::deny(r, "DENIED", "denied", {});
    }
};

static void governance_before_write() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<State>();
    state.allow_reads = true;
    auto& executor = user->make_resource<Executor>(state);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& deny = user->make_resource<Deny>();
    auto registry = user->make_ref<FunctionMutationPolicyRegistry>(
        [&deny, r](Memory dst, std::string_view) -> std::optional<MutationPolicyBorrow> {
            return MutationPolicyBorrow(
                std::allocator_arg, Allocator(dst), deny, detail::resource_arena(r));
        });
    user->set_mutation_policy_registry(registry);
    auto& root = user->make<GraphNode>("Order");
    root.value("id", std::uint64_t(7)).with_operation(GraphOperation::Create);
    auto& denied = Access::save_graph_internal(service, detail::clone_in(r, root)).wait();
    check(!denied && state.requests.empty() && state.queries.empty(),
          "denied forced-create plan performs no provider work");
}

static void transaction_owned_sqlite_reread() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->disable_sql_log();
    auto& store = user->make<InMemoryMetadataStore>();
    EntityDescriptor d(std::allocator_arg, Allocator(r), "Order");
    d.table("orders");
    d.property("id", DataType::U64).id();
    d.property("version", DataType::I64).version();
    d.property("name", DataType::Text);
    store.register_entity(std::move(d));
    user->set_metadata(*user, store);
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (
        auto statement :
        {"CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
         "CREATE TRIGGER fill_order AFTER INSERT ON orders BEGIN UPDATE orders SET name='triggered',version=5 WHERE id=NEW.id; END"}) {
        sql.sql = statement;
        check(bool(transport.execute(r, sql)), "SQLite execution setup");
    }
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& cache = user->make<SqlExecutorState>();
    auto& executor =
        user->make<SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>>(
            dialect, transport, store, cache);
    auto& tx = executor.begin(r).wait().value().get();
    using Tx = std::remove_reference_t<decltype(tx)>;
    auto& repo = EntityDataService<Tx>::for_executor(*user, "Order", *user, tx);
    auto& result = Access::execute_graph_plan_internal(repo, plan(r)).wait();
    check(result && result.value().values.fields.at(String("name", r)).try_text() == "triggered" &&
              result.value().values.fields.at(String("version", r)).try_i64() == 5,
          "transaction-owned root reread captures database trigger before completion");
    check(bool(tx.rollback(r).wait()), "runtime graph write remains rollbackable");
    sql.sql = "SELECT id FROM orders";
    auto& rows = transport.fetch_all_compact_sql(r, sql).wait();
    check(rows && rows.value().empty(),
          "rollback removes graph mutation after authoritative reread");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        execution_and_retention();
        pending_read_and_failure();
        governance_before_write();
        transaction_owned_sqlite_reread();
        std::pmr::set_default_resource(previous);
        std::cout << "Graph execution contracts passed\n";
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
}
