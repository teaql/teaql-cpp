#include <teaql/runtime/context.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <atomic>
#include <iostream>
#include <semaphore>
#include <thread>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

template <class F>
void rejected(F&& fn, const char* message) {
    bool failed = false;
    try {
        fn();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

struct Counts {
    unsigned starts = 0, successes = 0, failures = 0, drops = 0, activations = 0, finished = 0,
             cancelled = 0;
    bool fail_start = false, fail_finish = false, active = false, read_before_free = false;
};

struct Scope final : RuntimeTelemetryScope {
    using allocator_type = Allocator;
    Memory r;
    Counts& c;
    String owned;

    Scope(std::allocator_arg_t, Allocator a, Counts& c)
        : r(a.resource()), c(c), owned(8192, 's', r) {}

    ~Scope() {
        ++c.drops;
        check(dynamic_cast<Arena*>(r)->stats().live_blocks > 0,
              "payload destroyed before arena reclaim");
    }

    void with_context(FunctionRef<void()> callback) const override {
        ++c.activations;
        c.active = true;

        struct Guard {
            bool& active;

            ~Guard() {
                active = false;
            }
        } guard{c.active};

        callback();
    }

    void success(const RuntimeAttributes&) override {
        ++c.successes;
        if (c.fail_finish) {
            throw std::runtime_error("adapter finish failed");
        }
    }

    void failure(std::string_view) override {
        ++c.failures;
        if (c.fail_finish) {
            throw std::runtime_error("adapter finish failed");
        }
    }
};

struct Adapter final : RuntimeTelemetry {
    using allocator_type = Allocator;
    Memory r;
    Counts& c;
    Vector<RuntimeOperation> seen;

    Adapter(std::allocator_arg_t, Allocator a, Counts& c) : r(a.resource()), c(c), seen(r) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory target,
                                             const RuntimeOperation& op) const override {
        ++c.starts;
        if (c.fail_start) {
            throw std::runtime_error("adapter start failed");
        }
        const_cast<Adapter*>(this)->seen.emplace_back(op);
        return detail::resource_arena(target).make<ContextBox<RuntimeTelemetryScope>>(
            std::in_place_type<Scope>, c);
    }
};

static RuntimeOperation operation(Memory r) {
    RuntimeOperation op(
        std::allocator_arg, Allocator(r), "query with long name from user arena", "Order.list");
    op.attribute("teaql.entity.id", std::int64_t(7))
        .attribute("teaql.entity.type", "LongEntityNameStoredInTheUserArena")
        .attribute("float", -0.0);
    return op;
}

static void models_and_release() {
    GlobalContext global;
    Counts c;
    auto& adapter = global.make<Adapter>(c);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->with_runtime_telemetry(global, adapter);
    check(!user->runtime_telemetry_is_noop(), "adapter default is not noop");
    {
        auto op = operation(r);
        auto& copied = global.make<RuntimeOperation>(op);
        auto& scope = user->start_runtime_operation(op);
        RuntimeAttributes attrs(r);
        scope.success(attrs);
        scope.failure("late");
        check(c.successes == 1 && c.failures == 0 && c.drops == 1,
              "completion consumes and destroys payload immediately exactly once");
        check(copied.attributes.find(std::string_view("teaql.entity.type"))
                      ->second.memory_resource() == global.memory_resource(),
              "nested attribute allocator propagated");
    }
    {
        auto op = operation(r);
        c.fail_finish = true;
        auto& scope = user->start_runtime_operation(op);
        scope.failure("native_error");
        scope.close();
        check(c.failures == 1 && c.drops == 2,
              "throwing finish is swallowed and still destroys payload once");
    }
    {
        auto op = operation(r);
        c.fail_start = true;
        auto& scope = user->start_runtime_operation(op);
        RuntimeAttributes attrs(r);
        scope.success(attrs);
        check(c.drops == 2, "failed creation leaves empty fail-open scope");
        c.fail_start = false;
    }
    {
        auto op = operation(r);
        auto& scope = user->start_runtime_operation(op);
        scope.close();
        scope.close();
        check(c.drops == 3, "explicit abandonment releases only once");
    }
    {
        RuntimeContextCarrier carrier(r);
        auto& propagated = extract_runtime_context(r, adapter, carrier);
        unsigned calls = 0;
        auto poll = [&] {
            ++calls;
            return 42;
        };
        check(propagated.poll_with_context(poll) == 42 && calls == 1,
              "default context propagation calls business once");
        propagated.close();
    }
    user->close();
    check(c.drops == 3 && user->allocation_stats().live_blocks == 0,
          "closed Context reclaims boxes without repeated payload cleanup");
    check(adapter.seen.size() == 3 &&
              adapter.seen[0].family == "query with long name from user arena",
          "adapter explicitly copied operations to its Global domain");
}

static void poison_and_domains() {
    GlobalContext global, other;
    Counts c;
    auto& adapter = global.make<Adapter>(c);
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    auto foreign = other.make_user_context();
    auto r = user->memory_resource();
    rejected(
        [&] {
            user->set_runtime_telemetry(*foreign, foreign->make<Adapter>(c));
        },
        "foreign application service rejected");
    auto& local = user->make<Adapter>(c);
    auto op = operation(r);
    rejected(
        [&] {
            start_runtime_operation(global.memory_resource(), local, op);
        },
        "Global scope cannot retain User service");
    check(sibling->runtime_telemetry_is_noop(), "default telemetry is explicit Context-owned noop");
    auto& scope = start_runtime_operation(r, adapter, op);
    auto bad = []() -> int {
        throw std::runtime_error("business");
    };
    bool failed = false;
    try {
        scope.poll_with_context(bad);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    check(failed, "business exception propagates");
    auto good = [&] {
        check(!c.active, "poisoned scope bypasses adapter context");
        return 9;
    };
    check(scope.poll_with_context(good) == 9 && c.activations == 1,
          "business exception poisons scope lock semantics");
    RuntimeAttributes attrs(r);
    scope.success(attrs);
    scope.failure("late");
    check(c.successes == 0 && c.failures == 0 && c.drops == 0,
          "poisoned scope suppresses finish and retains delegate until release");
    scope.close();
    check(c.drops == 1, "poisoned delegate still destructs");
    auto& box = user->make<ContextBox<RuntimeTelemetryScope>>(std::in_place_type<Scope>, c);
    auto& owned = user->make<FailOpenRuntimeTelemetryScope>(&box);
    rejected(
        [&] {
            user->make<FailOpenRuntimeTelemetryScope>(&box);
        },
        "Box cannot be consumed by two scopes");
    owned.close();
    rejected(
        [&] {
            global.make<FailOpenRuntimeTelemetryScope>(&box);
        },
        "Box ownership cannot cross allocation domain");
}

struct NativeWork {
    Memory r;
    Counts& c;
    String payload;
    std::thread worker;
    std::binary_semaphore go{0};
    std::atomic<bool> done = false;
    bool released = false;

    NativeWork(Memory r, Counts& c) : r(r), c(c), payload(32768, 'x', r) {}

    ~NativeWork() {
        if (worker.joinable()) {
            std::terminate();
        }
    }

    std::optional<String> poll(WakeSignal& wake) {
        check(c.active, "native poll executes inside telemetry activation");
        if (!worker.joinable() && !done.load()) {
            worker = std::thread([this, &wake] {
                go.acquire();
                c.read_before_free = payload.size() == 32768 && c.drops == 0;
                done.store(true);
                wake.notify();
            });
            return {};
        }
        if (!done.load()) {
            return {};
        }
        return String("scoped native result", r);
    }

    void release() {
        if (!released) {
            released = true;
            go.release();
        }
    }

    void join() noexcept {
        release();
        if (worker.joinable()) {
            worker.join();
        }
    }

    void finish_and_wait() noexcept {
        join();
        ++c.finished;
    }

    void cancel_and_wait() noexcept {
        join();
        ++c.cancelled;
    }
};

static void native_lifecycle(bool cancel) {
    Counts c;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& adapter = user->make<Adapter>(c);
    user->set_runtime_telemetry(*user, adapter);
    auto& op = user->make<RuntimeOperation>("query", "native");
    auto& scope = user->start_runtime_operation(op);
    auto& work = user->make_resource<NativeWork>(r, c);
    auto& child = user->make<AsyncOperation<String>>(work);
    auto& wrapped = scope.run(r, child);
    check(!wrapped.poll(), "native operation initially pending");
    if (cancel) {
        user->close();
        check(c.cancelled == 1 && c.finished == 0 && c.drops == 1 && c.read_before_free,
              "close joins native work before delegate destruction and arena release");
    } else {
        work.release();
        const auto& result = wrapped.wait();
        auto& retained = global.make<String>(result);
        check(result.get_allocator().resource() == r && !c.active,
              "result belongs to caller arena and activation unwinds");
        RuntimeAttributes attrs(r);
        scope.success(attrs);
        user->close();
        check(c.finished == 1 && c.cancelled == 0 && c.drops == 1 && c.read_before_free &&
                  retained == "scoped native result",
              "wake forwarding, deep copy and eager delegate release");
    }
    check(user->allocation_stats().live_blocks == 0,
          "all native operation and telemetry blocks reclaimed");
}

struct Propagation final : RuntimeTelemetryPropagationContext {
    Counts& c;

    explicit Propagation(Counts& c) : c(c) {}

    ~Propagation() {
        ++c.drops;
    }

    void with_context(FunctionRef<void()> callback) const override {
        ++c.activations;
        c.active = true;

        struct Guard {
            bool& active;

            ~Guard() {
                active = false;
            }
        } guard{c.active};

        callback();
    }
};

struct PropagationAdapter final : RuntimeTelemetry {
    using allocator_type = Allocator;
    Memory r;
    Counts& c;

    PropagationAdapter(std::allocator_arg_t, Allocator a, Counts& c) : r(a.resource()), c(c) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory target,
                                             const RuntimeOperation&) const override {
        return detail::resource_arena(target).make<ContextBox<RuntimeTelemetryScope>>(
            std::in_place_type<Scope>, c);
    }

    ContextBox<RuntimeTelemetryPropagationContext>&
    extract_context(Memory target, const RuntimeContextCarrier&) const override {
        return detail::resource_arena(target)
            .make_resource<ContextBox<RuntimeTelemetryPropagationContext>>(
                std::in_place_type<Propagation>, c);
    }
};

struct Pending {
    std::optional<int> poll(WakeSignal&) {
        return {};
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

static void propagation_lifecycle(bool cancel) {
    Counts c;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& adapter = user->make<PropagationAdapter>(c);
    RuntimeContextCarrier carrier(r);
    auto& propagation = extract_runtime_context(r, adapter, carrier);
    {
        auto sibling = global.make_user_context();
        auto& wrong = make_operation<int, Pending>(*sibling);
        rejected(
            [&] {
                propagation.run(r, wrong);
            },
            "propagation refuses a child future from another domain");
        rejected(
            [&] {
                propagation.run(sibling->memory_resource(), wrong);
            },
            "propagation scope cannot lend to a sibling domain");
    }
    auto& work = user->make_resource<NativeWork>(r, c);
    auto& child = user->make<AsyncOperation<String>>(work);
    auto& wrapped = propagation.run(r, child);
    check(!wrapped.poll(), "propagation starts native operation lazily");
    if (cancel) {
        user->close();
        check(c.cancelled == 1 && c.read_before_free && c.drops == 1,
              "propagation cancels and joins before destruction");
    } else {
        work.release();
        check(wrapped.wait() == "scoped native result" && !c.active,
              "propagation forwards native wakes and restores context");
        propagation.close();
        user->close();
        check(c.finished == 1 && c.read_before_free && c.drops == 1,
              "propagation releases delegate exactly once");
    }
    check(user->allocation_stats().live_blocks == 0, "propagation arena reclaimed");
}

int main() {
    try {
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        models_and_release();
        poison_and_domains();
        native_lifecycle(false);
        native_lifecycle(true);
        propagation_lifecycle(false);
        propagation_lifecycle(true);
        std::pmr::set_default_resource(old);
        std::cout << "Runtime telemetry contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
