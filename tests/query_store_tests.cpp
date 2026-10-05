#include "query_store_fixture.hpp"
#include <atomic>
#include <iostream>
#include <semaphore>
#include <thread>
using namespace teaql;
using Access = detail::ContextPaginationAccess;
static_assert(!std::is_copy_constructible_v<InMemoryAggregationCache> &&
              !std::is_copy_constructible_v<InMemoryContinuousPageCursorStore> &&
              !std::is_copy_constructible_v<InMemoryIdSetStore>);

static void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

template <class F>
static void rejected(F&& fn, const char* message) {
    bool failed = false;
    try {
        fn();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

static ContinuousPageCursor
cursor(Memory r, std::string_view key = "query", std::uint64_t offset = 10) {
    ContinuousPageCursor c(std::allocator_arg, Allocator(r));
    c.query_key = key;
    c.cursor_id = "a cursor identifier with context-owned storage";
    c.entity = "Parent";
    Record boundary(r);
    boundary.emplace(String("id", r), Value(r, std::uint64_t(7)));
    boundary.emplace(String("label", r), Value(r, "nested boundary"));
    c.boundary = Value(r, boundary);
    c.next_offset = offset;
    c.page_size = 10;
    c.expires_at = SystemTime::from_unix_seconds(4102444800, 123);
    return c;
}

static RetainedIdSet ids(Memory r, std::string_view key = "query") {
    RetainedIdSet out(std::allocator_arg, Allocator(r));
    out.query_key = key;
    out.ids = {9, 1, 9, 0, std::numeric_limits<std::uint64_t>::max()};
    out.expires_at = SystemTime::from_unix_seconds(4102444800, 456);
    return out;
}

static void cache_copy_precedes_retention_age() {
    struct StampedResource : std::pmr::memory_resource {
        std::chrono::steady_clock::time_point last;

        void* do_allocate(std::size_t n, std::size_t alignment) override {
            auto* p = std::pmr::new_delete_resource()->allocate(n, alignment);
            last = std::chrono::steady_clock::now();
            return p;
        }

        void do_deallocate(void* p, std::size_t n, std::size_t alignment) override {
            std::pmr::new_delete_resource()->deallocate(p, n, alignment);
        }

        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
            return this == &other;
        }
    } upstream;

    GlobalContext g(&upstream, 128);
    auto c = g.make_user_context(128);
    auto r = c->memory_resource();
    auto& cache = g.make<InMemoryAggregationCache>();
    auto& rows = c->make<Vector<CompactRow>>();
    auto& payload = c->make<String>(262144, 'x');
    Record fields(r);
    fields.emplace(String("payload", r), Value(r, payload));
    rows.emplace_back(fields);
    cache.put("query", rows);
    auto copied_at = upstream.last;
    check(detail::QueryStoreAccess::timestamp(cache, "query") >= copied_at,
          "large ownership copy finishes before initial cache age starts");
    cache.put("query", rows);
    copied_at = upstream.last;
    check(detail::QueryStoreAccess::timestamp(cache, "query") >= copied_at,
          "replacement ownership copy finishes before cache age resets");
}

static void retention_and_defaults() {
    GlobalContext g;
    auto& cache = g.make<InMemoryAggregationCache>("tenant");
    auto& cursors = g.make<InMemoryContinuousPageCursorStore>();
    auto& sets = g.make<InMemoryIdSetStore>();
    auto first = g.make_user_context();
    auto second = g.make_user_context();
    auto r = first->memory_resource();
    auto s = second->memory_resource();
    check(Access::cursor_store(*first).memory_resource() == r &&
              Access::id_set_store(*first).memory_resource() == r,
          "default stores are owned by requesting UserContext");
    check(&Access::cursor_store(*first) != &Access::cursor_store(*second) &&
              &Access::id_set_store(*first) != &Access::id_set_store(*second),
          "independent UserContexts have independent defaults");
    first->set_continuous_page_cursor_store(g, cursors);
    first->set_id_set_store(g, sets);
    second->set_continuous_page_cursor_store(g, cursors);
    second->set_id_set_store(g, sets);
    check(&Access::cursor_store(*first) == &cursors && &Access::id_set_store(*first) == &sets,
          "explicit Global store registration preserves service identity");
    auto& input = first->make<ContinuousPageCursor>(cursor(r));
    auto& id_input = first->make<RetainedIdSet>(ids(r));
    auto& put = cursors.put(r, input);
    auto& put_ids = sets.put(r, id_input);
    input.query_key = "caller changed key";
    std::get<Value::Object>(input.boundary.data).at(String("label", r)) =
        Value(r, "caller changed payload");
    id_input.ids.clear();
    check(put.wait() && put_ids.wait(), "lazy put snapshots values before caller mutation");
    auto& fetched = cursors.get(r, "query", 10).wait();
    auto& fetched_ids = sets.get(r, "query").wait();
    check(fetched && fetched.value() && fetched.value()->memory_resource() == r &&
              fetched.value()->boundary.memory_resource() == r,
          "Global cursor copied into request domain");
    check(fetched_ids && fetched_ids.value()->ids.size() == 5 &&
              fetched_ids.value()->ids.get_allocator().resource() == r,
          "retained ID values preserve order/duplicates with explicit result ownership");
    auto& mutable_copy = first->make<RetainedIdSet>(*fetched_ids.value());
    mutable_copy.ids[0] = 99;
    check(sets.get(r, "query").wait().value()->ids[0] == 9,
          "mutating returned ID copy does not mutate store");
    auto& retained = g.make<ContinuousPageCursor>(*fetched.value());
    auto& retained_ids = g.make<RetainedIdSet>(*fetched_ids.value());
    auto& rows = first->make<Vector<CompactRow>>();
    Record record(r);
    record.emplace(String("value", r), detail::clone_in(r, fetched.value()->boundary));
    rows.emplace_back(record);
    record.clear();
    cache.put("tenant::q", rows);
    rows.front().insert("value", Value(r, "changed"));
    auto cached = cache.get(r, "tenant::q", 0);
    check(cached && cached->front().get("value")->memory_resource() == r,
          "aggregation cache returns nested data in requested domain");
    cached.reset();
    auto& unpolled = cursors.put(r, cursor(r, "cancelled"));
    auto& unpolled_ids = sets.put(r, ids(r, "cancelled"));
    (void)unpolled;
    (void)unpolled_ids;
    first->close();
    check(first->allocation_stats().live_blocks == 0,
          "all User snapshots and future states reclaimed");
    check(retained.boundary.memory_resource() == g.memory_resource() &&
              retained_ids.ids.size() == 5,
          "explicit Global copies survive original Context close");
    check(cursors.get(s, "query", 10).wait().value().has_value() &&
              sets.get(s, "query").wait().value()->ids[0] == 9,
          "shared Global stores retain copies after source User destruction");
    check(!cursors.get(s, "cancelled", 10).wait().value() &&
              !sets.get(s, "cancelled").wait().value(),
          "close cancels unpolled puts without committing store mutations");
    auto survived = cache.get(s, "tenant::q", 0);
    check(survived && std::get<Value::Object>(survived->front().get("value")->data)
                              .at(String("label", s))
                              .try_text() == "nested boundary",
          "aggregation payload remains independent of overwritten and destroyed User values");
    auto& local = second->make<InMemoryIdSetStore>();
    rejected(
        [&] {
            local.get(g.memory_resource(), "q");
        },
        "User store cannot create a future in longer Global domain");
    auto unrelated = g.make_user_context();
    rejected(
        [&] {
            unrelated->set_id_set_store(*second, local);
        },
        "User store cannot be registered in sibling");
    rejected(
        [&] {
            second->set_id_set_store(g, local);
        },
        "store ownership declaration must match allocator");
    rejected(
        [&] {
            first->continuous_page_plan();
        },
        "closed Context observation access rejected");
    rejected(
        [&] {
            Access::id_set_store(*first);
        },
        "closed Context default store access rejected");
}

static void expiration_and_models() {
    GlobalContext g;
    auto c = g.make_user_context();
    auto r = c->memory_resource();
    auto& cache = c->make<InMemoryAggregationCache>();
    auto& rows = c->make<Vector<CompactRow>>();
    rows.emplace_back();
    cache.put("expired", rows);
    detail::QueryStoreAccess::age(cache, "expired", 50000);
    check(!cache.get(r, "expired", 1) && cache.get(r, "expired", 0) &&
              detail::QueryStoreAccess::keys(r, cache).size() == 1,
          "expired aggregation entry remains stored; zero age means unlimited");
    auto& cursors = c->make<InMemoryContinuousPageCursorStore>();
    auto past = cursor(r);
    past.expires_at = SystemTime::from_unix_seconds(-1, 999999999);
    check(bool(cursors.put(r, past).wait()), "expired cursor can be inserted");
    check(detail::QueryStoreAccess::keys(r, cursors).size() == 1 &&
              !cursors.get(r, "query", 10).wait().value() &&
              detail::QueryStoreAccess::keys(r, cursors).empty(),
          "cursor get removes expired target");
    auto& sets = c->make<InMemoryIdSetStore>();
    detail::QueryStoreAccess::capacity(sets, 2, 16);
    auto a = ids(r, "old");
    a.ids.clear();
    a.expires_at = SystemTime::from_unix_seconds(0);
    check(bool(sets.put(r, a).wait()), "insert expired empty ID set");
    auto b = ids(r, "large");
    check(!sets.put(r, b).wait() && detail::QueryStoreAccess::keys(r, sets).size() == 1,
          "oversized ID rejection precedes expiry purge");
    b.ids = {1, 2};
    check(bool(sets.put(r, b).wait()) && detail::QueryStoreAccess::keys(r, sets).size() == 1,
          "accepted ID insertion purges expired entries");
    auto min = SystemTime::from_unix_seconds(std::numeric_limits<std::int64_t>::min(), 999999999);
    auto max = min.checked_add_seconds(std::numeric_limits<std::uint64_t>::max());
    check(max && max->unix_seconds() == std::numeric_limits<std::int64_t>::max() &&
              max->subsec_nanos() == 999999999 && !max->checked_add_seconds(1),
          "SystemTime addition preserves complete signed-second range and detects overflow");
    check(SystemTime::from_unix_seconds(-1, 999999999) < SystemTime{} &&
              SystemTime{} < SystemTime::from_unix_seconds(0, 1),
          "SystemTime preserves subsecond ordering around Unix epoch");
    rejected(
        [&] {
            SystemTime::from_unix_seconds(0, 1000000000);
        },
        "invalid SystemTime nanoseconds rejected");
    auto old = cursor(r, "old-store");
    auto& original = Access::cursor_store(*c);
    auto& waiting = original.put(r, old);
    c->set_continuous_page_cursor_store(*c, cursors);
    check(waiting.wait() && original.get(r, "old-store", 10).wait().value() &&
              !cursors.get(r, "old-store", 10).wait().value(),
          "replacement does not redirect outstanding operations from previous store");
    Access::observe_continuous_page(*c, "SEEK", "cursor");
    Access::observe_id_set(*c, "ID_SET_HIT", 9);
    auto copied = c->continuous_page_plan();
    Access::observe_continuous_page(*c, "DISABLED", {});
    check(copied == "SEEK" && c->id_set_count() == 9 && !c->continuous_page_cursor_id(),
          "observation snapshots and two independent channels retained");
}

struct NativeCounts {
    bool read = false, destroyed_after_stop = false;
    unsigned stops = 0;
};

class NativeIdStore : public IdSetStore {
public:
    using allocator_type = Allocator;
    Memory r;
    NativeCounts& counts;
    String payload;

    NativeIdStore(std::allocator_arg_t, Allocator a, NativeCounts& counts)
        : r(a.resource()), counts(counts), payload(32768, 's', r) {}

    ~NativeIdStore() {
        counts.destroyed_after_stop = counts.stops == 1;
    }

    Memory memory_resource() const noexcept override {
        return r;
    }

    struct Work {
        using allocator_type = Allocator;
        Memory r;
        ResourceBorrow<NativeIdStore> store;
        String key;
        std::thread worker;
        std::binary_semaphore go{0};
        std::atomic<bool> done = false;
        bool released = false;

        Work(std::allocator_arg_t, Allocator a, NativeIdStore& s, std::string_view key)
            : r(a.resource()), store(std::allocator_arg, a, s, detail::resource_arena(s.r)),
              key(key, r) {}

        ~Work() {
            if (worker.joinable())
                std::terminate();
        }

        std::optional<PaginationResult<std::optional<RetainedIdSet>>> poll(WakeSignal& wake) {
            if (!worker.joinable() && !done.load()) {
                worker = std::thread([this, &wake] {
                    go.acquire();
                    store.get().counts.read = key == "native-query" &&
                                              store.get().payload.size() == 32768 &&
                                              store.get().payload.back() == 's';
                    done.store(true);
                    wake.notify();
                });
                return {};
            }
            if (!done.load())
                return {};
            using R = PaginationResult<std::optional<RetainedIdSet>>;
            auto result = ids(r, key);
            return R::ok(r, std::move(result));
        }

        void release() {
            if (!released) {
                released = true;
                go.release();
            }
        }

        void stop() noexcept {
            release();
            if (worker.joinable())
                worker.join();
            ++store.get().counts.stops;
        }

        void finish_and_wait() noexcept {
            stop();
        }

        void cancel_and_wait() noexcept {
            stop();
        }
    };

    Work* work = nullptr;

    AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>&
    get(Memory result, std::string_view key) override {
        auto& arena = detail::resource_arena(result);
        work = &arena.make_resource<Work>(*this, key);
        return arena.make<AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>>(*work);
    }

    AsyncOperation<PaginationWriteResult>& put(Memory, const RetainedIdSet&) override {
        throw std::logic_error("unused native put");
    }

    AsyncOperation<PaginationWriteResult>& invalidate(Memory, std::string_view) override {
        throw std::logic_error("unused native invalidate");
    }
};

static void native_lifetime(unsigned action) {
    NativeCounts counts;
    GlobalContext g;
    auto c = g.make_user_context();
    auto r = c->memory_resource();
    auto& store = c->make_resource<NativeIdStore>(counts);
    c->set_id_set_store(*c, store);
    auto& operation = Access::id_set_store(*c).get(r, "native-query");
    check(!operation.poll(), "asynchronous store SPI can suspend native work");
    RetainedIdSet* retained = nullptr;
    if (action == 0) {
        store.work->release();
        auto& result = operation.wait();
        check(result && result.value() && result.value()->ids[0] == 9,
              "native store wakes owner and constructs scoped result");
        retained = &g.make<RetainedIdSet>(*result.value());
    } else if (action == 1)
        operation.cancel();
    c->close();
    check(counts.read && counts.stops == 1 && counts.destroyed_after_stop &&
              c->allocation_stats().live_blocks == 0,
          "native store access finishes before store destructor and arena reclamation");
    if (retained)
        check(retained->ids[0] == 9 &&
                  retained->ids.get_allocator().resource() == g.memory_resource(),
              "native result retained only through explicit Global copy");
}

int main() {
    try {
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        cache_copy_precedes_retention_age();
        retention_and_defaults();
        expiration_and_models();
        for (unsigned i = 0; i < 3; ++i)
            native_lifetime(i);
        std::pmr::set_default_resource(old);
        std::cout << "Query stores, deadlines and Context lifetime contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
