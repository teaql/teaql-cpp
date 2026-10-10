#include "id_set_build_fixture.hpp"
#include <atomic>
#include <iostream>
#include <semaphore>
#include <thread>
using namespace teaql;
using namespace teaql::detail;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& fn, const char* message) {
    bool failed = false;
    try {
        fn();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

static void identity_and_domains() {
    GlobalContext global;
    auto first = global.make_user_context();
    auto second = global.make_user_context();
    auto& key = first->make<String>("a long query key owned by the first UserContext");
    auto& a = ContextPaginationAccess::build_lock(*first, key);
    auto& b = ContextPaginationAccess::build_lock(*second, key);
    check(IdSetBuildAccess::same(a, b) && IdSetBuildAccess::leases(a) == 2,
          "same application and key reuse one mutex");
    check(a.memory_resource() == first->memory_resource() &&
              IdSetBuildAccess::entry_memory(a) == global.memory_resource(),
          "lease and mutex directory use explicit User/Global domains");
    key = "changed caller key";
    auto& same = ContextPaginationAccess::build_lock(
        *second, "a long query key owned by the first UserContext");
    check(IdSetBuildAccess::same(a, same), "directory retains its own key copy");
    auto& other = ContextPaginationAccess::build_lock(*second, "another query");
    check(!IdSetBuildAccess::same(a, other), "distinct keys use distinct mutexes");
    GlobalContext unrelated;
    auto foreign = unrelated.make_user_context();
    auto& independent = ContextPaginationAccess::build_lock(
        *foreign, "a long query key owned by the first UserContext");
    check(!IdSetBuildAccess::same(a, independent),
          "applications have independent explicit registries");
    auto& local = first->make<IdSetBuildRegistry>();
    rejects(
        [&] {
            local.borrow(second->memory_resource(), "invalid");
        },
        "User-owned registry cannot lend into a sibling");
    first->close();
    check(first->allocation_stats().live_blocks == 0 && IdSetBuildAccess::leases(b) == 2,
          "first Context closes without retaining its arena");
    b.reset();
    same.reset();
    other.reset();
    other.reset();
    check(!b.active(), "explicit lease reset is idempotent");
    rejects(
        [&] {
            b.lock();
        },
        "released lease cannot start work");
    auto& fresh = ContextPaginationAccess::build_lock(*second, "fresh");
    check(IdSetBuildAccess::registry_size(fresh) == 1, "next lookup prunes inactive entries");
    rejects(
        [&] {
            ContextPaginationAccess::build_lock(*first, "closed");
        },
        "closed Context rejects build leases");
}

static void waiting_and_cancellation(bool close_waiter) {
    GlobalContext global;
    auto owner = global.make_user_context();
    auto w1 = global.make_user_context();
    auto w2 = global.make_user_context();
    auto& a = ContextPaginationAccess::build_lock(*owner, "q");
    auto& b = ContextPaginationAccess::build_lock(*w1, "q");
    auto& c = ContextPaginationAccess::build_lock(*w2, "q");
    auto& holder = a.lock().wait().get();
    auto& first = b.lock();
    auto& second = c.lock();
    check(!first.poll() && !second.poll(), "other contexts suspend behind a held build guard");
    rejects(
        [&] {
            a.reset();
        },
        "held guard prevents lease reset");
    rejects(
        [&] {
            b.reset();
        },
        "pending future prevents lease reset");
    auto t1 = first.wake_signal().ticket(), t2 = second.wake_signal().ticket();
    holder.release();
    holder.release();
    check(first.wake_signal().ticket() == t1 + 1 && second.wake_signal().ticket() == t2,
          "unlock wakes one registered waiter");
    if (close_waiter) {
        w1->close();
        check(w1->allocation_stats().live_blocks == 0,
              "woken waiter close reclaims every arena block");
    } else {
        first.cancel();
    }
    check(second.wake_signal().ticket() == t2 + 1,
          "cancel or close after wake transfers notification to another waiter");
    auto& guard = second.wait().get();
    check(guard.owns_lock(), "notified waiter can acquire");
    guard.release();
    if (!close_waiter) {
        b.reset();
    }
    c.reset();
    a.reset();
    auto& reuse = ContextPaginationAccess::build_lock(*owner, "q");
    auto& not_polled = reuse.lock();
    not_polled.cancel();
    check(reuse.lock().wait().get().owns_lock(), "unpolled cancellation never holds mutex");
}

static void slot_reuse_and_overtaking() {
    GlobalContext global;
    auto ctx = global.make_user_context();
    auto& lock = ContextPaginationAccess::build_lock(*ctx, "q");
    auto& initial = lock.lock().wait().get();
    auto& a = lock.lock();
    auto& b = lock.lock();
    auto& c = lock.lock();
    check(!a.poll() && !b.poll() && !c.poll(), "three pending waiters register");
    a.cancel();
    c.cancel();
    auto& reused = lock.lock();
    check(!reused.poll(), "new waiter can reuse a freed slot");
    auto b_before = b.wake_signal().ticket(), r_before = reused.wake_signal().ticket();
    initial.release();
    check(b.wake_signal().ticket() == b_before + 1 && reused.wake_signal().ticket() == r_before,
          "wake order follows occupied slot order, not FIFO");
    auto& newcomer = lock.lock().wait().get();
    check(newcomer.owns_lock() && !b.poll(),
          "source mutex allows a new poller to overtake an already woken waiter");
    auto retry = b.wake_signal().ticket();
    newcomer.release();
    check(b.wake_signal().ticket() == retry + 1,
          "repolled pending waiter registers a fresh notification");
    auto& bg = b.wait().get();
    bg.release();
    check(reused.wake_signal().ticket() == r_before + 1, "next remaining slot is notified");
    reused.cancel();
    lock.reset();
}

struct NativeState {
    using allocator_type = Allocator;
    String input;
    ResourceBorrow<IdSetBuildGuard> guard;
    std::thread worker;
    std::binary_semaphore go{0};
    std::atomic<bool> done = false;
    bool& stopped;
    bool& read;

    NativeState(
        std::allocator_arg_t, Allocator a, IdSetBuildGuard& guard, bool& stopped, bool& read)
        : input(32768, 'n', a.resource()),
          guard(std::allocator_arg, a, guard, resource_arena(guard.memory_resource())),
          stopped(stopped), read(read) {}

    ~NativeState() {
        if (worker.joinable()) {
            std::terminate();
        }
    }

    std::optional<std::uint64_t> poll(WakeSignal& wake) {
        if (!worker.joinable() && !done.load()) {
            // Resolve on the owner; registered work borrows this host object until join.
            auto* native_guard = &guard.get();
            worker = std::thread([this, &wake, native_guard] {
                go.acquire();
                read = input.size() == 32768 && input.back() == 'n' && native_guard->owns_lock();
                done.store(true);
                wake.notify();
            });
            return {};
        }
        if (done.load()) {
            return 1;
        }
        return {};
    }

    void stop() noexcept {
        go.release();
        if (worker.joinable()) {
            worker.join();
        }
        stopped = true;
    }

    void finish_and_wait() noexcept {
        stop();
    }

    void cancel_and_wait() noexcept {
        stop();
    }
};

static void close_holders_and_waiters() {
    GlobalContext global;
    auto holder_context = global.make_user_context();
    auto waiting_context = global.make_user_context();
    auto final_context = global.make_user_context();
    auto& holder = ContextPaginationAccess::build_lock(*holder_context, "q");
    auto& waiting = ContextPaginationAccess::build_lock(*waiting_context, "q");
    auto& final = ContextPaginationAccess::build_lock(*final_context, "q");
    auto& held_guard = holder.lock().wait().get();
    auto& first = waiting.lock();
    auto& last = final.lock();
    check(!first.poll() && !last.poll(), "waiters registered before holder shutdown");
    bool stopped = false, read = false;
    auto& native = make_resource_operation<std::uint64_t, NativeState>(
        *holder_context, held_guard, stopped, read);
    check(!native.poll(), "native build work can suspend while holding guard");
    auto observed = last.wake_signal().ticket();
    waiting_context->close();
    check(last.wake_signal().ticket() == observed &&
              waiting_context->allocation_stats().live_blocks == 0,
          "closing unwoken waiter removes its borrowed node before reclaiming arena");
    holder_context->close();
    check(stopped && read && holder_context->allocation_stats().live_blocks == 0 &&
              last.wake_signal().ticket() == observed + 1,
          "native work joins before holder arena reclamation and remaining waiter wakes");
    auto& acquired = last.wait().get();
    check(acquired.owns_lock() && IdSetBuildAccess::leases(final) == 1,
          "surviving context can acquire after prior context shutdown");
    rejects(
        [&] {
            global.make<ResourceBorrow<IdSetBuildGuard>>(last.wait());
        },
        "guard borrow cannot escape to longer Global lifetime");
    final_context->close();
    check(final_context->allocation_stats().live_blocks == 0,
          "held guard releases during Context quiescence");
    auto survivor = global.make_user_context();
    auto& renewed = ContextPaginationAccess::build_lock(*survivor, "q");
    check(IdSetBuildAccess::registry_size(renewed) == 1 && renewed.lock().wait().get().owns_lock(),
          "registry is reusable after every previous Context closes");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        identity_and_domains();
        waiting_and_cancellation(false);
        waiting_and_cancellation(true);
        slot_reuse_and_overtaking();
        close_holders_and_waiters();
        std::pmr::set_default_resource(previous);
        std::cout << "ID set build coordination and Context lifetime contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
