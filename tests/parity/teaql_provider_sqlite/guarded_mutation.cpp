// Semantic port of the unchanged provider guarded-mutation integration test.
#include "fixture.hpp"

using namespace original_provider;

static void tenant_guard_is_atomic_for_update_delete_and_recover() {
    Fixture fixture;
    auto r = fixture.resource;
    auto entity = order_entity(r);
    fixture.metadata.register_entity(detail::clone_in(r, entity));
    fixture.exec(
        "CREATE TABLE orders(id INTEGER PRIMARY KEY NOT NULL,version INTEGER NOT NULL,tenant_id INTEGER NOT NULL,name TEXT NOT NULL)");

    struct Row {
        std::int64_t id, tenant;
        const char* name;
    };

    for (const auto& row :
         {Row{1, 1, "tenant-one"}, {2, 2, "tenant-two"}, {3, 2, "tenant-two-hard-delete"}}) {
        InsertCommand insert(std::allocator_arg, Allocator(r), "Order");
        insert.value("id", row.id)
            .value("version", std::int64_t(1))
            .value("tenant_id", row.tenant)
            .value("name", row.name);
        fixture.seed(entity, insert);
    }
    auto guarded = [&](auto& executor, auto command, std::int64_t tenant, std::uint64_t affected) {
        GuardedMutationRequest request(std::allocator_arg,
                                       Allocator(r),
                                       MutationRequest(std::allocator_arg,
                                                       Allocator(r),
                                                       std::move(command),
                                                       "original mutation execution fixture"),
                                       Expr::eq(r, "tenant_id", tenant));
        const auto& result = executor.mutate_guarded(r, std::move(request)).wait();
        check(result && result.value().affected_rows == affected,
              "guarded affected-row result matches the original tenant test");
    };
    UpdateCommand cross_update(std::allocator_arg, Allocator(r), "Order", std::int64_t(2));
    cross_update.expected_version(1).value("name", "stolen");
    guarded(fixture.executor, std::move(cross_update), 1, 0);
    UpdateCommand own_update(std::allocator_arg, Allocator(r), "Order", std::int64_t(1));
    own_update.expected_version(1).value("name", "updated");
    guarded(fixture.executor, std::move(own_update), 1, 1);
    DeleteCommand cross_delete(std::allocator_arg, Allocator(r), "Order", std::int64_t(2));
    cross_delete.expected_version(1);
    guarded(fixture.executor, std::move(cross_delete), 1, 0);
    DeleteCommand own_delete(std::allocator_arg, Allocator(r), "Order", std::int64_t(2));
    own_delete.expected_version(1);
    guarded(fixture.executor, std::move(own_delete), 2, 1);
    guarded(fixture.executor,
            RecoverCommand(std::allocator_arg, Allocator(r), "Order", std::int64_t(2), -2),
            1,
            0);
    guarded(fixture.executor,
            RecoverCommand(std::allocator_arg, Allocator(r), "Order", std::int64_t(2), -2),
            2,
            1);
    DeleteCommand cross_hard_delete(std::allocator_arg, Allocator(r), "Order", std::int64_t(3));
    cross_hard_delete.expected_version(1).hard_delete();
    guarded(fixture.executor, std::move(cross_hard_delete), 1, 0);
    DeleteCommand own_hard_delete(std::allocator_arg, Allocator(r), "Order", std::int64_t(3));
    own_hard_delete.expected_version(1).hard_delete();
    guarded(fixture.executor, std::move(own_hard_delete), 2, 1);
    const auto& begun = fixture.executor.begin(r).wait();
    check(begun.is_ok(), "begin real provider transaction");
    auto& transaction = begun.value().get();
    auto& transaction_guard = begun.value().get_as<GuardedMutationExecutor<Executor::Error>>();
    UpdateCommand transaction_cross_update(
        std::allocator_arg, Allocator(r), "Order", std::int64_t(2));
    transaction_cross_update.expected_version(3).value("name", "transactional theft");
    guarded(transaction_guard, std::move(transaction_cross_update), 1, 0);
    check(bool(transaction.rollback(r).wait()), "rollback real provider transaction");
    auto tenant_two = fixture.rows("SELECT tenant_id,version,name FROM orders WHERE id=2");
    check(tenant_two.size() == 1 && tenant_two[0].get("tenant_id") &&
              tenant_two[0].get("version") &&
              *tenant_two[0].get("tenant_id") == Value(r, std::int64_t(2)) &&
              *tenant_two[0].get("version") == Value(r, std::int64_t(3)) &&
              tenant_two[0].get("name")->try_text() == "tenant-two",
          "other tenant survives failed updates and successful soft-delete/recover");
    auto tenant_one = fixture.rows("SELECT name FROM orders WHERE id=1");
    check(tenant_one[0].get("name")->try_text() == "updated", "own update persists");
    auto count = fixture.rows("SELECT COUNT(*) AS count FROM orders WHERE id=3");
    check(count.size() == 1 && count[0].get("count") &&
              *count[0].get("count") == Value(r, std::int64_t(0)),
          "own hard delete removes the row with the original integer count");
}

static void tenant_changes_at_the_write_boundary() {
    Fixture fixture;
    auto r = fixture.resource;
    fixture.metadata.register_entity(order_entity(r));
    fixture.exec(
        "CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,tenant_id INTEGER,name TEXT)");
    fixture.exec("INSERT INTO orders VALUES(1,1,1,'original')");

    struct ChangingTransport final : SqlTransport<MutationExecutorError> {
        using allocator_type = Allocator;
        SqliteMutationExecutor& native;
        bool changed = false;

        ChangingTransport(std::allocator_arg_t, Allocator, SqliteMutationExecutor& native)
            : native(native) {}

        DataServiceOperationResult<Rows, MutationExecutorError>&
        fetch_all_compact_sql(Memory r, const CompiledQuery& query) override {
            return native.fetch_all_compact_sql(r, query);
        }

        DataServiceOperationResult<std::uint64_t, MutationExecutorError>&
        execute_sql(Memory r, const CompiledQuery& query) override {
            if (!changed && query.sql.starts_with("UPDATE")) {
                CompiledQuery concurrent(std::allocator_arg, Allocator(r));
                concurrent.sql = "UPDATE orders SET tenant_id=2 WHERE id=1";
                check(bool(native.execute(r, concurrent)),
                      "fixture changes tenant immediately before the write");
                changed = true;
            }
            return native.execute_sql(r, query);
        }
    };

    auto& transport = fixture.user->make<ChangingTransport>(fixture.transport);
    auto& state = fixture.user->make<SqlExecutorState>();
    auto& executor =
        fixture.user->make<SqlDataServiceExecutor<MutationExecutorError, ChangingTransport>>(
            fixture.dialect, transport, fixture.metadata, state);
    UpdateCommand update(std::allocator_arg, Allocator(r), "Order", std::int64_t(1));
    update.expected_version(1).value("name", "stolen");
    GuardedMutationRequest guarded(
        std::allocator_arg,
        Allocator(r),
        MutationRequest(
            std::allocator_arg, Allocator(r), std::move(update), "reject concurrent tenant change"),
        Expr::eq(r, "tenant_id", std::int64_t(1)));
    const auto& result = executor.mutate_guarded(r, std::move(guarded)).wait();
    check(result && transport.changed && result.value().affected_rows == 0 &&
              !result.value().persisted_snapshot,
          "the write's own guard rejects a tenant change and leaks no foreign snapshot");
    auto rows = fixture.rows("SELECT tenant_id,version,name FROM orders WHERE id=1");
    check(rows.size() == 1 && rows[0].get("tenant_id") && rows[0].get("version") &&
              *rows[0].get("tenant_id") == Value(r, std::int64_t(2)) &&
              *rows[0].get("version") == Value(r, std::int64_t(1)) &&
              rows[0].get("name")->try_text() == "original",
          "a stale pre-write view cannot authorize mutation of the new tenant's row");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    tenant_guard_is_atomic_for_update_delete_and_recover();
    tenant_changes_at_the_write_boundary();
}
