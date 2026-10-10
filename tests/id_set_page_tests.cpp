#include <teaql/runtime/detail/id_set_page.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>

using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Executor : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    unsigned calls = 0;
    unsigned* cancelled = nullptr;
    bool pending = false, bad_id = false, fail = false;
    QueryRequest* seen = nullptr;
    WakeSignal* child_wake = nullptr;

    Executor(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Work {
        using allocator_type = Allocator;
        Memory resource;
        Executor& executor;

        Work(std::allocator_arg_t, Allocator a, Executor& executor)
            : resource(a.resource()), executor(executor) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal& wake) {
            executor.child_wake = &wake;
            using R = Result<QueryResult, String>;
            if (executor.pending) {
                return {};
            }
            if (executor.fail) {
                return R::err(resource, String("ID lookup failed", resource));
            }
            QueryResult result(std::allocator_arg,
                               Allocator(resource),
                               ExecutionMetadata::unrecorded_query(resource, 3));
            for (std::uint64_t id : {1, 2, 3}) {
                result.rows.emplace_back();
                result.rows.back().insert(
                    "id", executor.bad_id ? Value(resource, "invalid") : Value(resource, id));
            }
            return R::ok(resource, std::move(result));
        }

        void finish_and_wait() noexcept {
            executor.child_wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            executor.child_wake = nullptr;
            if (executor.cancelled) {
                ++*executor.cancelled;
            }
        }
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        ++calls;
        seen = &detail::resource_arena(r).make<QueryRequest>(std::move(request));
        return make_operation<Result<QueryResult, String>, Work>(r, *this);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

static EntityDataService<Executor>& repo(UserContext& user) {
    auto& executor = user.make<Executor>();
    return EntityDataService<Executor>::for_executor(user, "Item", user, executor);
}

static void root_contracts() {
    GlobalContext global;
    std::optional<ResourceBorrow<const ContextEntityRef>> saved;
    ContextRootError* retained = nullptr;
    auto user = global.make_user_context();
    auto& missing = user->make<decltype(user->require_active_root("Tenant"))>(
        user->require_active_root("Tenant"));
    check(!missing && missing.error().to_string(user->memory_resource()) ==
                          "active root tenant is missing from UserContext",
          "missing active root fails closed");
    user->with_active_root("Tenant", 42);
    auto& valid = user->make<decltype(user->require_active_root("tenant"))>(
        user->require_active_root("tenant"));
    check(valid && valid.value().get().id == 42, "canonical typed root lookup");
    saved.emplace(std::allocator_arg, Allocator(user->memory_resource()), valid.value());
    {
        auto wrong = user->require_active_root("Organization");
        check(!wrong && wrong.error().actual_root->entity_type == "tenant",
              "root error owns the actual typed root");
        retained = &global.make<ContextRootError>(wrong.error());
        check(bool(user->set_generated_bootstrap_active_root("tenant", 42)),
              "identical bootstrap is idempotent");
        check(!user->set_generated_bootstrap_active_root("Tenant", 43),
              "bootstrap cannot replace an existing root");
    }
    bool reassigned = false;
    try {
        user->with_active_root("Tenant", 42);
    } catch (const std::logic_error&) {
        reassigned = true;
    }
    check(reassigned, "ordinary root assignment is exactly once even for the same value");
    user->close();
    user.reset();
    bool expired = false;
    try {
        (void)saved->get();
    } catch (const ReferenceError& error) {
        expired = error.kind() == ReferenceError::Kind::Closed;
    }
    check(expired && retained->actual_root->id == 42 &&
              retained->memory_resource() == global.memory_resource(),
          "root borrows expire and explicit diagnostic copies survive");
}

static void build_and_recheck() {
    GlobalContext global;
    auto user = global.make_user_context();
    user->set_user_identifier("same-reader");
    user->with_active_root("Tenant", 42);
    auto& service = repo(*user);
    auto& executor = Access::context_service(service).executor.get();
    auto& query = user->make<SelectQuery>("Item");
    query.project("label").page(1, 1).optimize_pagination_with_id_set_config("fixture", 600, 3);
    executor.pending = true;
    auto& first = detail::prepare_id_set_page(service, query);
    auto& second = detail::prepare_id_set_page(service, query);
    check(!first.poll() && !second.poll() && executor.calls == 1,
          "same-key misses serialize builds through the real registry");
    auto first_ticket = first.wake_signal().ticket();
    executor.child_wake->notify();
    check(first.wake_signal().ticket() != first_ticket,
          "provider readiness forwards through ID preparation");
    auto second_ticket = second.wake_signal().ticket();
    executor.pending = false;
    const auto& built = first.wait();
    check(built && built.value().total_count == 3 && built.value().query.slice->offset == 0 &&
              !built.value().query.id_set_pagination && user->id_set_plan() == "ID_SET_BUILD",
          "a successful build returns a retained-ID page and authoritative total");
    check(second.wake_signal().ticket() != second_ticket && second.wait() && executor.calls == 1 &&
              user->id_set_plan() == "ID_SET_HIT",
          "a waiter rechecks after lock acquisition and avoids duplicate I/O");
    check(
        executor.seen->query.projection.size() == 1 && executor.seen->query.projection[0] == "id" &&
            executor.seen->query.slice->limit == 4 && executor.seen->query.hard_limit_value == 4 &&
            !query.predicate && query.slice->offset == 1 && query.id_set_pagination,
        "ID build uses max_ids plus one and leaves caller input intact");
    query.page(UINT64_MAX, 0);
    auto& empty = detail::prepare_id_set_page(service, query).wait();
    check(empty && empty.value().total_count == 3 && empty.value().query.slice->limit == 0,
          "empty and overflowing page ranges retain the total without wrapping");
}

static void failures_and_close() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& service = repo(*user);
    auto& executor = Access::context_service(service).executor.get();
    auto& query = user->make<SelectQuery>("Item");
    query.page(0, 1).optimize_pagination_with_id_set_config("limit", 600, 2);
    check(detail::prepare_id_set_page(service, query).wait() &&
              user->id_set_plan() == "ID_SET_FALLBACK_LIMIT_EXCEEDED" && user->id_set_count() == 3,
          "max_ids overflow falls back without admitting retained IDs");
    executor.bad_id = true;
    check(detail::prepare_id_set_page(service, query).wait() &&
              user->id_set_plan() == "ID_SET_FALLBACK_UNSUPPORTED_SHAPE",
          "invalid row IDs fall back");
    executor.bad_id = false;
    executor.fail = true;
    auto& failed = detail::prepare_id_set_page(service, query).wait();
    check(!failed && failed.error().executor_error() == "ID lookup failed",
          "provider errors propagate rather than becoming fallback rows");
    executor.fail = false;
    executor.pending = true;
    unsigned cancelled = 0;
    executor.cancelled = &cancelled;
    auto& holder = detail::prepare_id_set_page(service, query);
    auto& waiter = detail::prepare_id_set_page(service, query);
    check(!holder.poll() && !waiter.poll(), "holder and waiter can remain Pending at close");
    user->close();
    check(cancelled == 1, "close cancels native build work before releasing the held guard");
    auto next = global.make_user_context();
    auto& later_repo = repo(*next);
    auto& later_query = next->make<SelectQuery>("Item");
    later_query.page(0, 1).optimize_pagination_with_id_set_config("limit", 600, 3);
    check(bool(detail::prepare_id_set_page(later_repo, later_query).wait()),
          "shutdown leaves no stranded build guard");

    auto store_ref = next->make_ref<InMemoryIdSetStore>();
    next->set_id_set_store(*next, next->get(store_ref));
    next->destroy(store_ref);
    bool dead = false;
    try {
        detail::prepare_id_set_page(later_repo, later_query).wait();
    } catch (const ReferenceError& error) {
        dead = error.kind() == ReferenceError::Kind::Dead;
    }
    check(dead, "registered store access checks liveness before invoking its virtual interface");
    auto cursor = next->make_ref<InMemoryContinuousPageCursorStore>();
    next->set_continuous_page_cursor_store(*next, next->get(cursor));
    next->destroy(cursor);
    dead = false;
    try {
        (void)detail::ContextPaginationAccess::cursor_store(*next);
    } catch (const ReferenceError& error) {
        dead = error.kind() == ReferenceError::Kind::Dead;
    }
    check(dead, "continuous-page store access uses the same checked registration contract");
}

static void global_store_isolation() {
    GlobalContext global;
    auto& store = global.make<InMemoryIdSetStore>();
    for (unsigned pass = 0; pass != 3; ++pass) {
        auto user = global.make_user_context();
        user->set_user_identifier("same-reader");
        user->with_active_root("Tenant", pass == 2 ? 43 : 42);
        user->set_id_set_store(global, store);
        auto& service = repo(*user);
        auto& query = user->make<SelectQuery>("Item");
        query.page(0, 1).optimize_pagination_with_id_set_config("retained", 600, 3);
        auto& result = detail::prepare_id_set_page(service, query).wait();
        auto& executor = Access::context_service(service).executor.get();
        check(result && result.value().total_count == 3 &&
                  executor.calls == (pass == 1 ? 0u : 1u) &&
                  result.value().memory_resource() == user->memory_resource(),
              "Global retained copies survive User destruction and are isolated by active root");
    }
}

static void sqlite_page_execution() {
    GlobalContext global;
    auto user = global.make_user_context();
    user->disable_sql_log();
    auto r = user->memory_resource();
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (auto text : {"CREATE TABLE items(id INTEGER PRIMARY KEY,label TEXT)",
                      "INSERT INTO items VALUES(1,'a'),(2,'a'),(3,'b')"}) {
        sql.sql = text;
        check(bool(transport.execute(r, sql)), "SQLite ID-set fixture setup");
    }
    auto& metadata = user->make<InMemoryMetadataStore>();
    EntityDescriptor item(std::allocator_arg, Allocator(r), "Item");
    item.table("items");
    item.property("id", DataType::U64).is_id = true;
    item.property("label", DataType::Text);
    metadata.register_entity(std::move(item));
    user->set_metadata(*user, metadata);
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& state = user->make<SqlExecutorState>();
    auto& executor = user->make<SqlDataServiceExecutor<MutationExecutorError>>(
        dialect, transport, metadata, state);
    using E = std::remove_reference_t<decltype(executor)>;
    auto& service = EntityDataService<E>::for_executor(*user, "Item", *user, executor);
    auto& query = user->make<SelectQuery>("Item");
    query.projects({"id", "label"})
        .order_desc("label")
        .page(1, 1)
        .optimize_pagination_with_id_set_config("sql", 600, 3);
    auto& page = detail::prepare_id_set_page(service, query).wait();
    check(page && page.value().total_count == 3,
          "SQLite enumerates retained IDs through the real executor");
    auto& rows = Access::context_service(service).fetch_all(page.value().query).wait();
    check(rows && rows.value().size() == 1 && rows.value()[0].get("id")->try_u64() == 1,
          "the rewritten predicate and deterministic ID tie-breaker execute in SQLite");
    sql.sql = "INSERT INTO items VALUES(4,'z')";
    check(bool(transport.execute(r, sql)), "modify backing rows after ID admission");
    query.page(2, 1);
    auto& next = detail::prepare_id_set_page(service, query).wait();
    auto& result = Access::context_service(service).fetch_all(next.value().query).wait();
    check(next && next.value().total_count == 3 && user->id_set_plan() == "ID_SET_HIT" && result &&
              result.value()[0].get("id")->try_u64() == 2,
          "retained pagination preserves its admitted IDs and total despite a later insert");
}

int main() {
    try {
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        root_contracts();
        build_and_recheck();
        failures_and_close();
        global_store_isolation();
        sqlite_page_execution();
        std::pmr::set_default_resource(old);
        std::cout << "Active root and ID-set page contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
