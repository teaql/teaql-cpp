#include "context_query_fixture.hpp"
#include <atomic>
#include <iostream>
#include <semaphore>
#include <thread>
using namespace teaql;
using namespace context_query_fixture;

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

static void lazy_scope_and_borrows() {
    GlobalContext g;
    auto c = g.make_user_context();
    auto r = c->memory_resource();
    auto& state = g.make<State>(Config{});
    auto& telemetry = g.make<Telemetry>(state);
    c->set_runtime_telemetry(g, telemetry);
    auto& mock = c->make<Mock>(state);
    auto& repo = service(*c, mock);
    auto& query = c->make<SelectQuery>("Item");
    query.comment_text.emplace("business", r);
    c->set_user_identifier("before");
    auto& result = [&]() -> auto& {
        auto temporary = detail::clone_in(r, repo);
        return temporary.fetch_all(query);
    }();
    check(state.notes.empty(), "Context query is lazy even after wrapper destruction");
    c->set_user_identifier("after");
    c->enable_select_sql_log();
    check(result.wait() && state.active.empty(),
          "nested scopes restore activation before returning");
    check(state.request->comment == "[after] business" && query.comment_text == "business",
          "comment resolution happens on first poll without modifying caller query");
    check(state.operations.size() == 2 && state.operations[0].family == "query" &&
              state.operations[1].family == "provider",
          "independent query/provider operations created");
    check(state.operations[1]
                  .attributes.at(String("teaql.provider.kind", g.memory_resource()))
                  .data == RuntimeAttributeValue(std::allocator_arg,
                                                 Allocator(g.memory_resource()),
                                                 detail::resource_type_name<Mock>())
                               .data,
          "provider type attribute uses actual C++ executor type");
    check(std::count(state.notes.begin(), state.notes.end(), "drop:query") == 1 &&
              std::count(state.notes.begin(), state.notes.end(), "drop:provider") == 1,
          "completed delegates released immediately once");
    rejected(
        [&] {
            g.make<detail::ContextDataService<Mock>>(repo);
        },
        "Context-bound wrapper cannot escape into Global");
    auto& retained = g.make<Vector<CompactRow>>(result.wait().value());
    c->close();
    check(retained.size() == 1 && retained[0].get("id")->try_u64() == 1,
          "explicit result copy survives Context close");
}

struct Ready {
    using allocator_type = Allocator;
    Memory r;
    bool& cancelled;

    Ready(std::allocator_arg_t, Allocator a, bool& flag) : r(a.resource()), cancelled(flag) {}

    std::optional<Result<std::uint64_t, DataServiceError<String>>> poll(WakeSignal&) {
        return Result<std::uint64_t, DataServiceError<String>>::ok(r, 7);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        cancelled = true;
    }
};

static void noop_and_foreign_work() {
    GlobalContext g;
    auto c = g.make_user_context();
    auto other = g.make_user_context();
    auto r = c->memory_resource();
    auto& state = c->make<State>(Config{.noop = true});
    auto& telemetry = c->make<Telemetry>(state);
    c->set_runtime_telemetry(*c, telemetry);
    auto& mock = c->make<Mock>(state);
    auto& repo = service(*c, mock);
    bool cancelled = false;
    using R = Result<std::uint64_t, DataServiceError<String>>;
    auto& work = make_operation<R, Ready>(r, cancelled);
    unsigned names = 0;
    auto name = [&] {
        ++names;
        return String("lazy name", r);
    };
    auto& observed = repo.observe("custom", name, "Item", work);
    check(observed.wait().value() == 7 && names == 0 && state.operations.empty(),
          "noop bypasses operation name construction and both telemetry scopes");
    auto& foreign = make_operation<R, Ready>(*other, cancelled);
    rejected(
        [&] {
            repo.observe("query", name, "Item", foreign);
        },
        "observed operation refuses a sibling result domain");
    check(!cancelled, "domain rejection does not cancel unrelated work");
    foreign.cancel();
    check(cancelled, "foreign operation remains owned by its original Context");
}

struct NativeCounts {
    bool read = false;
    unsigned stopped = 0;
};

struct NativeExecutor : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    State& state;
    NativeCounts& counts;

    NativeExecutor(std::allocator_arg_t, Allocator a, State& state, NativeCounts& counts)
        : r(a.resource()), state(state), counts(counts) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Work {
        using allocator_type = Allocator;
        Memory r;
        State& state;
        NativeCounts& counts;
        String payload;
        std::thread worker;
        std::binary_semaphore go{0};
        std::atomic<bool> done = false;
        bool released = false;

        Work(std::allocator_arg_t, Allocator a, State& state, NativeCounts& counts)
            : r(a.resource()), state(state), counts(counts), payload(32768, 'h', r) {}

        ~Work() {
            if (worker.joinable())
                std::terminate();
        }

        std::optional<Result<QueryResult, String>> poll(WakeSignal& wake) {
            check(state.path() == "query/provider", "native query polls under both scopes");
            if (!worker.joinable() && !done.load()) {
                worker = std::thread([this, &wake] {
                    go.acquire();
                    counts.read = payload.size() == 32768 && payload.back() == 'h';
                    done.store(true);
                    wake.notify();
                });
                return {};
            }
            if (!done.load())
                return {};
            QueryResult result(
                std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, 0));
            return Result<QueryResult, String>::ok(r, std::move(result));
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
            ++counts.stopped;
            state.note("native.stop");
        }

        void finish_and_wait() noexcept {
            stop();
        }

        void cancel_and_wait() noexcept {
            stop();
        }
    };

    Work* work = nullptr;

    DataServiceOperationResult<QueryResult, String>& query(Memory target, QueryRequest) override {
        auto& arena = detail::resource_arena(target);
        work = &arena.make_resource<Work>(state, counts);
        return arena.make<AsyncOperation<Result<QueryResult, String>>>(*work);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unused");
    }
};

static void native_lifetime(bool cancel, bool close) {
    NativeCounts counts;
    GlobalContext g;
    auto& state = g.make<State>(Config{});
    auto& telemetry = g.make<Telemetry>(state);
    auto c = g.make_user_context();
    auto r = c->memory_resource();
    c->set_runtime_telemetry(g, telemetry);
    auto& executor = c->make<NativeExecutor>(state, counts);
    ResourceBorrow<NativeExecutor> borrow(
        std::allocator_arg, Allocator(r), executor, detail::resource_arena(r));
    auto& repo = c->make<detail::ContextDataService<NativeExecutor>>(*c, borrow);
    auto& query = c->make<SelectQuery>("Item");
    auto& result = repo.fetch_smart_list(query);
    check(!result.poll() && state.active.empty(),
          "pending native query leaves no active telemetry scope");
    if (close)
        c->close();
    else if (cancel) {
        result.cancel();
        c->close();
    } else {
        executor.work->release();
        check(result.wait() && result.wait().value().is_loaded,
              "native completion wakes all adapter layers");
        c->close();
    }
    auto stopped = std::find(state.notes.begin(), state.notes.end(), "native.stop");
    auto dropped = std::find(state.notes.begin(), state.notes.end(), "drop:provider");
    check(counts.read && counts.stopped == 1 && stopped < dropped,
          "native host access stops before provider scope destruction");
    check(c->allocation_stats().live_blocks == 0,
          "Context reclaims all query/telemetry/native memory blocks");
    if (cancel || close)
        check(std::none_of(state.notes.begin(),
                           state.notes.end(),
                           [](const auto& n) {
                               return n.starts_with("success:") || n.starts_with("failure:");
                           }),
              "cancellation does not invent a terminal business outcome");
}

int main() {
    try {
        setenv("TEAQL_LOG_ENDPOINT", "off", 1);
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        lazy_scope_and_borrows();
        noop_and_foreign_work();
        native_lifetime(false, false);
        native_lifetime(true, false);
        native_lifetime(false, true);
        std::pmr::set_default_resource(old);
        std::cout << "Context query and telemetry contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
