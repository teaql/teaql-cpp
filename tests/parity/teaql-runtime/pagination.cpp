// Direct pinned root cursor/ID-set contracts through the actual entity runtime.
#include "queue_fixture.hpp"
#include <teaql/core/detail/query_debug.hpp>
#include <teaql/runtime/pagination.hpp>
#include <atomic>
#include <thread>

using namespace original_runtime_queue;

static Record entity_row(Memory owner, std::uint64_t id) {
    char digits[32];
    auto end = std::to_chars(digits, digits + sizeof(digits), id).ptr;
    String name("order-", owner);
    name.append(digits, end);
    return record(owner,
                  {{"id", Value(owner, id)},
                   {"version", Value(owner, std::int64_t(1))},
                   {"name", Value(owner, name)}});
}

struct IdSetQueueExecutor final : QueryExecutor<StubError>, MutationExecutor<StubError> {
    using allocator_type = Allocator;
    Memory owner;
    Vector<Vector<Record>> batches;
    Vector<SelectQuery> queries;
    std::size_t next = 0;

    IdSetQueueExecutor(std::allocator_arg_t, Allocator a)
        : owner(a.resource()), batches(owner), queries(owner) {}

    Memory memory_resource() const noexcept {
        return owner;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, StubError>& query(Memory target,
                                                              QueryRequest request) override {
        queries.emplace_back(request.query);
        const auto now = std::chrono::system_clock::now();
        ExecutionMetadata metadata(
            std::allocator_arg, Allocator(target), DataServiceOperation::Query, now, now);
        metadata.backend = "id-set-queue";
        QueryResult result(std::allocator_arg, Allocator(target), std::move(metadata));
        if (next < batches.size()) {
            for (const auto& row : batches[next]) {
                result.rows.emplace_back(row);
            }
        }
        ++next;
        return make_operation<Result<QueryResult, StubError>, Ready<QueryResult>>(
            target, std::move(result));
    }

    DataServiceOperationResult<MutationResult, StubError>& mutate(Memory,
                                                                  MutationRequest) override {
        throw std::logic_error("original ID-set fixture does not mutate");
    }
};

template <class E>
static auto& enhanced(const EntityDataService<E>& service, const SelectQuery& query) {
    using Output = Result<SmartList<Order>, DataServiceError<StubError>>;
    return make_operation<Output, detail::EnhancedEntitiesOperation<E, Order>>(
        service.memory_resource(), service, query, std::span<const RelationAggregate>{});
}

static SelectQuery query(Memory owner,
                         std::string_view space,
                         std::uint64_t ttl = 60,
                         std::uint64_t max_ids = 100,
                         std::uint64_t offset = 0,
                         std::uint64_t limit = 1,
                         std::string_view order = "id") {
    SelectQuery result(std::allocator_arg, Allocator(owner), "Order");
    result.project("id")
        .project("version")
        .project("name")
        .order_asc(order)
        .page(offset, limit)
        .optimize_pagination_with_id_set_config(space, ttl, max_ids);
    return result;
}

static void plan(const UserContext& user, std::string_view expected) {
    auto actual = user.id_set_plan();
    check(actual && *actual == expected, "original exact ID-set plan diagnostic");
}

static IdSetQueueExecutor& queue(Fixture& f, std::string_view principal) {
    f.user->set_user_identifier(principal);
    auto& executor = f.user->make<IdSetQueueExecutor>();
    f.user->insert_resource(*f.user, executor);
    return executor;
}

static EntityDataService<IdSetQueueExecutor>& service(Fixture& f) {
    auto bound = f.user->entity_data_service<IdSetQueueExecutor>("Order");
    check(bool(bound), "original ID-set queue binding");
    return bound.value().get();
}

static void
id_batch(Memory owner, IdSetQueueExecutor& executor, std::uint64_t first, std::uint64_t last) {
    executor.batches.emplace_back();
    for (auto id = first; id <= last; ++id) {
        executor.batches.back().emplace_back(record(owner, {{"id", Value(owner, id)}}));
    }
}

static void
rows_batch(Memory owner, IdSetQueueExecutor& executor, std::uint64_t first, std::uint64_t last) {
    executor.batches.emplace_back();
    for (auto id = first; id <= last; ++id) {
        executor.batches.back().emplace_back(entity_row(owner, id));
    }
}

static void continuous_page_fetch_uses_id_seek_for_the_next_page() {
    Fixture f;
    f.user->set_user_identifier("tenant-1:user-1");
    auto& executor = f.user->make<QueueExecutor>(true);
    executor.batches.emplace_back();
    for (std::uint64_t id = 100; id >= 91; --id) {
        executor.batches.back().emplace_back(entity_row(f.owner, id));
    }
    f.user->insert_resource(*f.user, executor);
    auto bound = f.user->entity_data_service<QueueExecutor>("Order");
    check(bool(bound), "original continuous capture binding");
    SelectQuery first(std::allocator_arg, Allocator(f.owner), "Order");
    first.order_desc("id").page(0, 10).optimize_for_continuous_page_fetch_with("recent-orders", 60);
    check(bool(Access::fetch_all(bound.value().get(), first).wait()),
          "original first continuous page succeeds");
    auto first_plan = f.user->continuous_page_plan();
    check(first_plan && *first_plan == "OFFSET_FALLBACK:FIRST_PAGE",
          "original first-page offset fallback");
    SelectQuery second(std::allocator_arg, Allocator(f.owner), "Order");
    second.order_desc("id").page(10, 10).optimize_for_continuous_page_fetch_with("recent-orders",
                                                                                 60);
    check(bool(Access::fetch_all(bound.value().get(), second).wait()),
          "original second continuous page succeeds");
    auto second_plan = f.user->continuous_page_plan();
    check(second_plan && *second_plan == "CURSOR_SEEK" && f.user->continuous_page_cursor_id(),
          "original second-page cursor plan and cursor ID");
    check(executor.queries.size() > 1 && executor.queries[1].slice &&
              executor.queries[1].slice->offset == 0 && executor.queries[1].predicate,
          "original cursor query resets offset and has a predicate");
    auto predicate = detail::debug_expr(f.owner, *executor.queries[1].predicate);
    check(predicate.find("Lt") != String::npos && predicate.find("U64(91)") != String::npos,
          "original seek operator and exact last U64 boundary");
}

static void id_set_pagination_reuses_ordered_ids_and_returns_exact_count() {
    Fixture f;
    auto& executor = queue(f, "id-set-test:tenant-1:user-1");
    id_batch(f.owner, executor, 1, 100);
    rows_batch(f.owner, executor, 21, 30);
    rows_batch(f.owner, executor, 51, 60);
    auto& repo = service(f);
    auto first_query = query(f.owner, "orders", 60, 1000, 20, 10, "name");
    const auto& first = enhanced(repo, first_query).wait();
    check(first && first.value().total_count == 100 && !first.value().data.empty() &&
              first.value().data[0].id == 21,
          "original first ID-set page total and first ID");
    plan(*f.user, "ID_SET_BUILD");
    auto second_query = query(f.owner, "orders", 60, 1000, 50, 10, "name");
    const auto& second = enhanced(repo, second_query).wait();
    check(second && second.value().total_count == 100 && !second.value().data.empty() &&
              second.value().data[0].id == 51,
          "original second ID-set page total and first ID");
    plan(*f.user, "ID_SET_HIT");
    check(executor.queries.size() == 3 && executor.queries[0].projection.size() == 1 &&
              executor.queries[0].projection[0] == "id" && executor.queries[0].slice &&
              executor.queries[0].slice->limit == 1001 && !executor.queries[0].order_by.empty() &&
              executor.queries[0].order_by.back().field == "id",
          "original complete ID-only build query shape");
    check(executor.queries[1].slice && executor.queries[1].slice->offset == 0 &&
              executor.queries[1].predicate && executor.queries[2].predicate,
          "original retained-ID page queries reset offset");
    check(detail::debug_expr(f.owner, *executor.queries[1].predicate).find("U64(21)") !=
                  String::npos &&
              detail::debug_expr(f.owner, *executor.queries[2].predicate).find("U64(51)") !=
                  String::npos,
          "original retained-ID slice predicate witnesses");
}

static void id_set_pagination_limit_overflow_falls_back_without_false_count() {
    Fixture f;
    auto& executor = queue(f, "id-set-overflow-test");
    id_batch(f.owner, executor, 1, 4);
    rows_batch(f.owner, executor, 1, 1);
    auto request = query(f.owner, "overflow", 60, 3);
    const auto& rows = enhanced(service(f), request).wait();
    check(rows && !rows.value().total_count, "original overflow fallback has no false exact total");
    plan(*f.user, "ID_SET_FALLBACK_LIMIT_EXCEEDED");
    check(f.user->id_set_count() == 4, "original overflow diagnostic records four candidate IDs");
}

// The original 25ms async delay is native work, not Context access on a worker.
// Only WakeSignal and the state-owned atomic are touched off the owner thread.
struct ConcurrentIdSetExecutor final : QueryExecutor<StubError>, MutationExecutor<StubError> {
    using allocator_type = Allocator;
    Memory owner;
    std::atomic<unsigned>& id_queries;

    ConcurrentIdSetExecutor(std::allocator_arg_t, Allocator a, std::atomic<unsigned>& count)
        : owner(a.resource()), id_queries(count) {}

    Memory memory_resource() const noexcept {
        return owner;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Work {
        using allocator_type = Allocator;
        Memory owner;
        ConcurrentIdSetExecutor& executor;
        bool id_only, started = false;
        std::atomic<bool> ready{false};
        std::jthread timer;

        Work(std::allocator_arg_t, Allocator a, ConcurrentIdSetExecutor& executor, bool id_only)
            : owner(a.resource()), executor(executor), id_only(id_only) {}

        std::optional<Result<QueryResult, StubError>> poll(WakeSignal& wake) {
            if (id_only && !started) {
                started = true;
                executor.id_queries.fetch_add(1, std::memory_order_seq_cst);
                timer = std::jthread([this, &wake] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(25));
                    ready.store(true, std::memory_order_release);
                    wake.notify();
                });
                return {};
            }
            if (id_only && !ready.load(std::memory_order_acquire)) {
                return {};
            }
            const auto now = std::chrono::system_clock::now();
            ExecutionMetadata metadata(
                std::allocator_arg, Allocator(owner), DataServiceOperation::Query, now, now);
            metadata.backend = "concurrent-id-set";
            QueryResult result(std::allocator_arg, Allocator(owner), std::move(metadata));
            if (id_only) {
                for (std::uint64_t id : {1, 2}) {
                    result.rows.emplace_back(record(owner, {{"id", Value(owner, id)}}));
                }
            } else {
                result.rows.emplace_back(entity_row(owner, 1));
            }
            return Result<QueryResult, StubError>::ok(owner, std::move(result));
        }

        void finish_and_wait() noexcept {
            if (timer.joinable()) {
                timer.join();
            }
        }

        void cancel_and_wait() noexcept {
            finish_and_wait();
        }
    };

    DataServiceOperationResult<QueryResult, StubError>& query(Memory target,
                                                              QueryRequest request) override {
        const bool id_only =
            request.query.projection.size() == 1 && request.query.projection[0] == "id";
        return make_operation<Result<QueryResult, StubError>, Work>(target, *this, id_only);
    }

    DataServiceOperationResult<MutationResult, StubError>& mutate(Memory,
                                                                  MutationRequest) override {
        throw std::logic_error("original concurrent ID-set fixture does not mutate");
    }
};

struct SharedFixture {
    GlobalContext global;
    InMemoryIdSetStore& store = global.make<InMemoryIdSetStore>();
    std::atomic<unsigned>& id_queries = global.make<std::atomic<unsigned>>(0);

    std::unique_ptr<UserContext> user(std::string_view principal) {
        auto context = global.make_user_context();
        auto owner = context->memory_resource();
        context->set_user_identifier(principal);
        auto& metadata = context->make<InMemoryMetadataStore>();
        metadata.with_entity(entity(owner));
        context->with_metadata(*context, metadata);
        auto& registry = context->make<InMemoryEntityRegistry>();
        registry.with_entity("Order");
        context->with_entity_registry(*context, registry);
        context->set_id_set_store(global, store);
        auto& dialect = context->make_resource<TestDialect>();
        context->insert_resource(*context, dialect);
        auto& executor = context->make<ConcurrentIdSetExecutor>(id_queries);
        context->insert_resource(*context, executor);
        return context;
    }
};

static EntityDataService<ConcurrentIdSetExecutor>& service(UserContext& context) {
    auto bound = context.entity_data_service<ConcurrentIdSetExecutor>("Order");
    check(bool(bound), "original concurrent executor binding");
    return bound.value().get();
}

static void id_set_pagination_coalesces_concurrent_cache_misses() {
    SharedFixture f;
    auto first = f.user("id-set-single-flight-user");
    auto second = f.user("id-set-single-flight-user");
    auto request = query(first->memory_resource(), "single-flight");
    auto& first_work = enhanced(service(*first), request);
    auto& second_work = enhanced(service(*second), request);
    check(!first_work.poll() && !second_work.poll(),
          "both original readers overlap while the ID build is pending");
    const auto& first_rows = first_work.wait();
    const auto& second_rows = second_work.wait();
    check(first_rows && first_rows.value().total_count == 2 && second_rows &&
              second_rows.value().total_count == 2 &&
              f.id_queries.load(std::memory_order_seq_cst) == 1,
          "original concurrent misses share exactly one ID-only build and exact total2");
}

static void id_set_pagination_rebuilds_after_ttl_expiry() {
    SharedFixture f;
    auto context = f.user("id-set-ttl-user");
    auto request = query(context->memory_resource(), "ttl", 1);
    auto& repo = service(*context);
    check(bool(enhanced(repo, request).wait()), "original first TTL page succeeds");
    std::this_thread::sleep_for(std::chrono::milliseconds(1050));
    check(bool(enhanced(repo, request).wait()) && f.id_queries.load(std::memory_order_seq_cst) == 2,
          "original 1050ms wait expires the one-second ID set and triggers a second build");
    plan(*context, "ID_SET_BUILD");
}

static void id_set_pagination_isolates_principals_in_a_shared_store() {
    SharedFixture f;
    auto first = f.user("tenant-1:user-1");
    auto second = f.user("tenant-1:user-2");
    auto request = query(first->memory_resource(), "principal-isolation");
    check(bool(enhanced(service(*first), request).wait()) &&
              bool(enhanced(service(*second), request).wait()) &&
              f.id_queries.load(std::memory_order_seq_cst) == 2,
          "original different principals build separate retained ID sets in a shared store");
}

template <class T>
struct PaginationReady {
    using allocator_type = Allocator;
    T result;

    PaginationReady(std::allocator_arg_t, Allocator a, T result)
        : result(detail::move_in(a.resource(), std::move(result))) {}

    std::optional<T> poll(WakeSignal&) {
        return detail::clone_in(result.memory_resource(), result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct UnavailableIdSetStore final : IdSetStore {
    using allocator_type = Allocator;
    Memory owner;

    UnavailableIdSetStore(std::allocator_arg_t, Allocator a) : owner(a.resource()) {}

    Memory memory_resource() const noexcept override {
        return owner;
    }

    AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>& get(Memory target,
                                                                        std::string_view) override {
        using Output = PaginationResult<std::optional<RetainedIdSet>>;
        return make_operation<Output, PaginationReady<Output>>(
            target, Output::err(target, String("unavailable", target)));
    }

    AsyncOperation<PaginationWriteResult>& put(Memory target, const RetainedIdSet&) override {
        return make_operation<PaginationWriteResult, PaginationReady<PaginationWriteResult>>(
            target, PaginationWriteResult::err(target, String("unavailable", target)));
    }

    AsyncOperation<PaginationWriteResult>& invalidate(Memory target, std::string_view) override {
        return make_operation<PaginationWriteResult, PaginationReady<PaginationWriteResult>>(
            target, PaginationWriteResult::err(target, String("unavailable", target)));
    }
};

static void id_set_pagination_retains_empty_exact_result() {
    Fixture f;
    auto& executor = queue(f, "id-set-empty-user");
    executor.batches.emplace_back();
    executor.batches.emplace_back();
    auto request = query(f.owner, "empty", 60, 100, 0, 10);
    const auto& rows = enhanced(service(f), request).wait();
    check(rows && rows.value().data.empty() && rows.value().total_count == 0,
          "original empty ID set has exact zero total");
    plan(*f.user, "ID_SET_BUILD");
}

static void id_set_pagination_store_failure_falls_back_without_changing_rows() {
    Fixture f;
    auto& executor = queue(f, "id-set-store-failure-user");
    rows_batch(f.owner, executor, 7, 7);
    auto& unavailable = f.user->make<UnavailableIdSetStore>();
    f.user->set_id_set_store(*f.user, unavailable);
    auto request = query(f.owner, "unavailable", 60, 100, 0, 10);
    const auto& rows = enhanced(service(f), request).wait();
    check(rows && !rows.value().data.empty() && rows.value().data[0].id == 7 &&
              !rows.value().total_count,
          "original store failure preserves row7 and omits a false total");
    plan(*f.user, "ID_SET_FALLBACK_STORE_UNAVAILABLE");
}

static void id_set_pagination_does_not_shift_page_when_an_entity_disappears() {
    Fixture f;
    auto& executor = queue(f, "id-set-delete-user");
    id_batch(f.owner, executor, 1, 2);
    rows_batch(f.owner, executor, 2, 2);
    auto request = query(f.owner, "delete", 60, 100, 0, 2);
    const auto& rows = enhanced(service(f), request).wait();
    check(rows && rows.value().total_count == 2 && rows.value().data.size() == 1 &&
              rows.value().data[0].id == 2,
          "original missing entity does not shift the retained page or reduce its exact ID total");
}

static void id_set_pagination_unsupported_shape_falls_back_visibly() {
    Fixture f;
    auto& executor = queue(f, "id-set-unsupported-user");
    rows_batch(f.owner, executor, 9, 9);
    auto request = query(f.owner, "unsupported", 60, 100, 0, 10);
    request.order_by.clear();
    request.order_expr_asc(Expr::column(f.owner, "name"));
    const auto& rows = enhanced(service(f), request).wait();
    check(rows && !rows.value().data.empty() && rows.value().data[0].id == 9,
          "original unsupported shape retains row9");
    plan(*f.user, "ID_SET_FALLBACK_UNSUPPORTED_SHAPE");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    struct Restore {
        Memory previous;

        ~Restore() {
            std::pmr::set_default_resource(previous);
        }
    } restore{previous};

    try {
        continuous_page_fetch_uses_id_seek_for_the_next_page();
        id_set_pagination_reuses_ordered_ids_and_returns_exact_count();
        id_set_pagination_limit_overflow_falls_back_without_false_count();
        id_set_pagination_coalesces_concurrent_cache_misses();
        id_set_pagination_rebuilds_after_ttl_expiry();
        id_set_pagination_isolates_principals_in_a_shared_store();
        id_set_pagination_retains_empty_exact_result();
        id_set_pagination_store_failure_falls_back_without_changing_rows();
        id_set_pagination_does_not_shift_page_when_an_entity_disappears();
        id_set_pagination_unsupported_shape_falls_back_visibly();
        std::cout << "Ten original runtime-root pagination contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
