// Direct final three pinned provider transaction/collision source contracts.
#include "fixture.hpp"
#include "save_models.hpp"
#include <teaql/runtime/transaction.hpp>
#include <iostream>

using namespace original_provider;
using Scope = TransactionScope<Executor>;
using Output = Result<std::monostate, RuntimeError>;

static EntityDescriptor entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Order");
    result.table("orders");
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version().not_null();
    result.property("name", DataType::Text);
    return result;
}

// This body only sequences the original callback statements. TransactionScope
// owns begin/commit/rollback and the actual SQL executor owns mutation behavior.
struct RawBody {
    using allocator_type = Allocator;
    const Scope& scope;
    bool fail;
    unsigned index = 0;
    AsyncOperation<Result<MutationResult, RuntimeError>>* child = nullptr;
    WakeSignal::Forward forwarding;

    RawBody(std::allocator_arg_t, Allocator, const Scope& scope, bool fail)
        : scope(scope), fail(fail) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto owner = scope.context().memory_resource();
        while (index < 2) {
            if (!child) {
                const auto id = fail ? (index == 0 ? 3u : 1u) : index + 1;
                const auto name = fail ? (index == 0 ? "must roll back" : "duplicate")
                                       : (index == 0 ? "first" : "second");
                InsertCommand insert(std::allocator_arg, Allocator(owner), "Order");
                insert.value("id", std::uint64_t(id))
                    .value("version", std::int64_t(1))
                    .value("name", name);
                child = &scope.mutate(MutationRequest(std::allocator_arg,
                                                      Allocator(owner),
                                                      std::move(insert),
                                                      "insert transaction scope fixture"));
            }
            forwarding.attach(child->wake_signal(), wake);
            auto* result = child->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return Output::err(owner, detail::clone_in(owner, result->error()));
            }
            forwarding.reset();
            child = nullptr;
            ++index;
        }
        return Output::ok(owner, {});
    }

    void finish_and_wait() noexcept {
        forwarding.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding.reset();
        if (child) {
            child->cancel();
        }
    }
};

static void user_context_transaction_scope_commits_and_rolls_back_on_one_connection() {
    Fixture f;
    f.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name TEXT)");
    f.metadata.register_entity(entity(f.resource));
    f.user->register_executor(*f.user, f.executor);
    const auto& committed =
        f.user
            ->execute_in_transaction<Executor, std::monostate>(
                [&](const Scope& scope) -> AsyncOperation<Output>& {
                    return make_operation<Output, RawBody>(f.resource, scope, false);
                })
            .wait();
    check(bool(committed), "original two-insert scope commits");
    const auto& failed =
        f.user
            ->execute_in_transaction<Executor, std::monostate>(
                [&](const Scope& scope) -> AsyncOperation<Output>& {
                    return make_operation<Output, RawBody>(f.resource, scope, true);
                })
            .wait();
    check(!failed, "original duplicate-key scope fails");
    auto rows = f.rows("SELECT id FROM orders ORDER BY id");
    Vector<Value> ids(f.resource);
    for (const auto& row : rows) {
        check(row.get("id") != nullptr, "source committed id exists");
        ids.emplace_back(*row.get("id"));
    }
    check(ids == Vector<Value>(
                     {Value(f.resource, std::int64_t(1)), Value(f.resource, std::int64_t(2))},
                     f.resource),
          "original successful rows remain and pre-failure third insert rolls back");
}

struct AuditedBody {
    using allocator_type = Allocator;
    const Scope& scope;
    TransactionSchool first;
    std::optional<TransactionSchool> second;
    bool fail;
    unsigned index = 0;
    AsyncOperation<Result<TransactionSchool, RuntimeError>>* child = nullptr;
    WakeSignal::Forward forwarding;

    AuditedBody(
        std::allocator_arg_t, Allocator a, const Scope& scope, TransactionSchool first, bool fail)
        : scope(scope), first(std::allocator_arg, a, std::move(first)), fail(fail) {
        if (!fail) {
            second.emplace(
                std::allocator_arg, a, TransactionSchool::fresh(a.resource(), 2, "second"));
        }
    }

    std::optional<Output> poll(WakeSignal& wake) {
        auto owner = scope.context().memory_resource();
        while (index < 2) {
            if (!child) {
                if (index == 0) {
                    child = &scope.save_audited(std::move(first).audit_as(
                        fail ? "create third school" : "create first school"));
                } else {
                    if (fail) {
                        second.emplace(std::allocator_arg,
                                       Allocator(owner),
                                       TransactionSchool::fresh(owner, 1, "duplicate"));
                    }
                    child = &scope.save_audited(std::move(*second).audit_as(
                        fail ? "force duplicate failure" : "create second school"));
                }
            }
            forwarding.attach(child->wake_signal(), wake);
            auto* result = child->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return Output::err(owner, detail::clone_in(owner, result->error()));
            }
            if (!fail) {
                check(result->value().version() == 1,
                      "each original typed save returns authoritative version1");
            }
            forwarding.reset();
            child = nullptr;
            ++index;
        }
        return Output::ok(owner, {});
    }

    void finish_and_wait() noexcept {
        forwarding.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding.reset();
        if (child) {
            child->cancel();
        }
    }
};

static void typed_audited_saves_share_commit_and_rollback_boundary() {
    Fixture f;
    f.exec(
        "CREATE TABLE transaction_school (id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name TEXT NOT NULL)");
    f.metadata.register_entity(TransactionSchool::entity_descriptor(f.resource));
    f.user->register_executor(*f.user, f.executor);
    const auto& committed =
        f.user
            ->execute_in_transaction<Executor, std::monostate>(
                [&](const Scope& scope) -> AsyncOperation<Output>& {
                    return make_operation<Output, AuditedBody>(
                        f.resource, scope, TransactionSchool::fresh(f.resource, 1, "first"), false);
                })
            .wait();
    check(bool(committed), "original two typed audited saves commit together");
    auto& third =
        f.user->make<TransactionSchool>(TransactionSchool::fresh(f.resource, 3, "must roll back"));
    auto third_ledger = detail::clone_in(f.resource, third.root);
    const auto& failed = f.user
                             ->execute_in_transaction<Executor, std::monostate>(
                                 [&](const Scope& scope) -> AsyncOperation<Output>& {
                                     return make_operation<Output, AuditedBody>(
                                         f.resource, scope, std::move(third), true);
                                 })
                             .wait();
    check(!failed, "original duplicate audited save fails the enclosing scope");
    check(!third_ledger.new_keys().empty(),
          "original rolled-back ledger retains retryable new intent");
    auto rows = f.rows("SELECT id, name FROM transaction_school ORDER BY id");
    check(rows.size() == 2 && *rows[0].get("id") == Value(f.resource, std::int64_t(1)) &&
              *rows[0].get("name") == Value(f.resource, "first") &&
              *rows[1].get("id") == Value(f.resource, std::int64_t(2)) &&
              *rows[1].get("name") == Value(f.resource, "second"),
          "complete original committed ID/name tuples remain after audited rollback");
}

static void sqlite_relation_name_collision_hydrates_only_fk_and_updates_existing_parent() {
    Fixture f;
    auto channel = PaymentChannelCollision::entity_descriptor(f.resource);
    auto order = PaymentOrderCollision::entity_descriptor(f.resource);
    f.metadata.register_entity(detail::clone_in(f.resource, channel));
    f.metadata.register_entity(detail::clone_in(f.resource, order));
    use_sqlite_provider(*f.user, *f.user, f.transport);
    check(bool(f.user->ensure_schema().wait()), "original collision schemas installed");
    InsertCommand channel_insert(
        std::allocator_arg, Allocator(f.resource), "PaymentChannelCollision");
    channel_insert.value("id", std::uint64_t(1002))
        .value("version", std::int64_t(1))
        .value("label", "Primary");
    f.seed(channel, channel_insert);
    InsertCommand order_insert(std::allocator_arg, Allocator(f.resource), "PaymentOrderCollision");
    order_insert.value("id", std::uint64_t(9))
        .value("version", std::int64_t(1))
        .value("channel_id", std::uint64_t(1002))
        .value("name", "before");
    f.seed(order, order_insert);
    SelectQuery query(std::allocator_arg, Allocator(f.resource), "PaymentOrderCollision");
    query.filter(Expr::eq(f.resource, "id", std::uint64_t(9)));
    auto select = f.dialect.compile_select(f.resource, order, query);
    check(select.sql.find("channel AS channel_id") != String::npos,
          "original scalar physical-column projection preserves its logical alias");
    auto rows = f.transport.fetch_all_compact(f.resource, select);
    check(rows && rows.value().size() == 1, "original collision row exists");
    auto loaded = PaymentOrderCollision::from_compact_row(
        f.resource, detail::clone_in(f.resource, rows.value()[0]));
    check(loaded && loaded.value().channel_id == 1002 && !loaded.value().channel &&
              loaded.value().version() == 1,
          "original typed row has scalar FK and no synthesized relation entity");
    UpdateCommand update(
        std::allocator_arg, Allocator(f.resource), "PaymentOrderCollision", loaded.value().id);
    update.expected_version(loaded.value().version()).value("name", "after");
    auto affected =
        f.transport.execute(f.resource, f.dialect.compile_update(f.resource, order, update));
    check(affected && affected.value() == 1, "original update affects existing parent once");
    auto reread = f.transport.fetch_all_compact(f.resource, select);
    check(reread && !reread.value().empty(), "original updated collision row reread succeeds");
    auto reloaded = PaymentOrderCollision::from_compact_row(
        f.resource, detail::clone_in(f.resource, reread.value()[0]));
    check(reloaded && reloaded.value().channel_id == 1002 && !reloaded.value().channel &&
              reloaded.value().version() == 2 && reloaded.value().name == "after",
          "all original reloaded FK/relation/version/name assertions");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        user_context_transaction_scope_commits_and_rolls_back_on_one_connection();
        typed_audited_saves_share_commit_and_rollback_boundary();
        sqlite_relation_name_collision_hydrates_only_fk_and_updates_existing_parent();
        std::cout << "Original final three SQLite provider contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
