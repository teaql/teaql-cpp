#include "../../tests/support/locking_fixture.hpp"
#include <teaql/runtime/detail/context/locking.hpp>
#include <iostream>
#include <charconv>
#include <thread>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void local_ownership_and_lifetime() {
    GlobalContext global;
    auto first = global.make_user_context();
    auto second = global.make_user_context();
    auto& key = first->make<String>("a long User-owned lock key copied into Global storage");
    auto original = String(key, global.memory_resource());
    check(first->try_local_lock(key, 0, 0),
          "zero timeout is a successful single attempt on a free key");
    key = "modified caller data";
    check(!second->try_local_lock(original, 0, 0),
          "caller mutation does not change stored lock key");
    check(first->try_local_lock(original, 0, 0),
          "same owner can reacquire without a recursion count");
    second->unlock_local(original);
    check(!second->try_local_lock(original, 0, 0), "wrong-owner unlock does nothing");
    first->unlock_local(original);
    check(second->try_local_lock(original, 0, 0), "one unlock releases repeated ownership");
    first->unlock_local(original);
    check(!first->try_local_lock(original, 0, 0), "old owner cannot release a newer owner's entry");
    second->unlock_local(original);
    check(first->try_local_lock("retained", 0, 0), "acquire a non-expiring local lease");
    first.reset();
    check(!second->try_local_lock("retained", 0, 0) && global.live_user_bytes() != 0,
          "User destruction preserves a non-expiring lease without retaining User payloads");
    second.reset();
    check(global.live_user_bytes() == 0, "local lock directory retains no User blocks");
    auto next = global.make_user_context();
    check(!next->try_local_lock("retained", 0, 0),
          "Context identities are not reused as lock owners");
    GlobalContext independent;
    auto other = independent.make_user_context();
    check(other->try_local_lock("retained", 0, 0),
          "explicit applications have independent local directories");
    next->close();
    bool closed = false;
    try {
        (void)next->try_local_lock("q", 0, 0);
    } catch (const ReferenceError& error) {
        closed = error.kind() == ReferenceError::Kind::Closed;
    }
    check(closed, "closed Context cannot acquire local locks");
}

static void expiry_and_timeout() {
    GlobalContext global;
    auto a = global.make_user_context();
    auto b = global.make_user_context();
    check(a->try_local_lock("q", 0, 400), "finite lease");
    check(!b->try_local_lock("q", 10, 0), "contender times out before lease expiry");
    check(b->try_local_lock("q", 1000, 0), "blocking contender acquires after expiry");
    a->unlock_local("q");
    check(!a->try_local_lock("q", 0, 0), "expired owner cannot release replacement lease");
    b->unlock_local("q");
    check(a->try_local_lock("renew", 0, 1), "short first lease");
    check(a->try_local_lock("renew", 0, 0), "reacquisition removes automatic expiry");
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    check(!b->try_local_lock("renew", 0, 0), "zero expiry renewal does not expire");
    auto huge = std::numeric_limits<std::uint64_t>::max();
    check(a->try_local_lock("huge", huge, huge) && !b->try_local_lock("huge", 0, huge),
          "u64 deadlines are not narrowed to chrono nanosecond range");
    a->unlock_local("huge");
}

static void budgets_and_thread_boundary() {
    ContextLimits limits;
    limits.global.max_bytes = 64 * 1024;
    GlobalContext global(std::pmr::new_delete_resource(), 128, limits);
    auto user = global.make_user_context();
    check(user->try_local_lock("existing", 0, 0), "acquire before Global quota exhaustion");
    auto& key = user->make<String>(2048, 'k');
    bool exhausted = false;
    for (unsigned i = 0; i < 1000; ++i) {
        char suffix[16];
        auto end = std::to_chars(suffix, suffix + sizeof(suffix), i).ptr;
        key.resize(2048);
        key.append(suffix, end);
        try {
            (void)user->try_local_lock(key, 0, 0);
        } catch (const MemoryLimitError& error) {
            exhausted = error.kind() == MemoryLimitError::Kind::Bytes;
            break;
        }
    }
    check(exhausted && global.allocation_stats().live_bytes <= limits.global.max_bytes,
          "lock keys and directory obey the Global byte budget");
    user->unlock_local("existing");
    bool wrong_thread = false;
    std::thread caller([&] {
        try {
            (void)user->try_local_lock("foreign", 0, 0);
        } catch (const std::logic_error&) {
            wrong_thread = true;
        }
    });
    caller.join();
    check(wrong_thread, "Context lock acquisition remains on its owner thread");
    user->close();
    check(user->allocation_stats().live_bytes == 0, "exhaustion never prevents User teardown");
}

static void remote_optional_owner_and_replacement() {
    GlobalContext global;
    auto first = global.make_user_context();
    auto second = global.make_user_context();
    check(first->try_remote_lock("optional", 123, 456).wait() &&
              first->unlock_remote("optional").wait(),
          "missing provider is optional success");
    auto provider = first->make_global_resource<lock_fixture::Provider>();
    first->set_remote_lock_provider(provider);
    auto second_access = second->rebind_global(provider);
    second->set_remote_lock_provider(second_access);
    auto& p = first->get(provider);
    check(first->try_remote_lock("key", 17, 29).wait() &&
              !second->try_remote_lock("key", 18, 30).wait() &&
              !second->unlock_remote("key").wait() && first->unlock_remote("key").wait(),
          "remote locks preserve owner-specific release");
    check(p.calls[0].timeout == 17 && p.calls[0].expiry == 29 &&
              p.calls[0].owner != p.calls[1].owner && p.calls[0].owner == p.calls[3].owner,
          "opaque owner is stable per Context and timeout/expiry are passed unchanged");
    first->set_user_identifier("changed identifier");
    first->set_trace_id("changed trace");
    check(first->try_remote_lock("retained", 0, 0).wait() &&
              p.calls.back().owner == p.calls[0].owner,
          "business identifiers do not change lock ownership");
    first.reset();
    check(!second->unlock_remote("retained").wait() &&
              !second->try_remote_lock("retained", 0, 0).wait(),
          "closing User does not silently release provider-backed lease");
    auto late = second->try_remote_lock("replacement", 0, 0).status();
    check(late == AsyncStatus::Pending, "remote delegation is lazy");
    auto replacement = second->make_resource_ref<lock_fixture::Provider>();
    auto& future = second->try_remote_lock("new", 0, 0);
    second->set_remote_lock_provider(replacement);
    check(future.wait() && second->get(replacement).owners.contains("new"),
          "provider is selected when the operation is first polled");
    auto expired = second->make_resource_ref<lock_fixture::Provider>();
    second->set_remote_lock_provider(expired);
    second->destroy(expired);
    bool rejected = false;
    try {
        (void)second->try_remote_lock("dead", 0, 0).wait();
    } catch (const ReferenceError& e) {
        rejected = e.kind() == ReferenceError::Kind::Dead;
    }
    check(rejected, "destroyed provider is rejected before virtual invocation");
}

static void remote_pending_close_and_wakes() {
    GlobalContext global;
    auto first = global.make_user_context();
    auto provider = first->make_global_resource<lock_fixture::Provider>();
    first->set_remote_lock_provider(provider);
    auto& p = first->get(provider);
    p.pending = true;
    auto& key = first->make<String>("original key");
    auto& future = first->try_remote_lock(key, 37, 41);
    key = "changed key";
    check(!future.poll() && p.calls.back().key == "original key",
          "operation snapshots key before suspending");
    auto ticket = future.wake_signal().ticket();
    p.pending = false;
    p.wake->notify();
    check(future.wake_signal().ticket() == ticket + 1 && future.wait(),
          "native child wake reaches parent");
    p.pending = true;
    auto& cancelled = first->unlock_remote("original key");
    check(!cancelled.poll(), "remote unlock can suspend");
    cancelled.cancel();
    check(p.cancelled == 1 && p.owners.contains("original key"),
          "cancelled unlock does not release lease");
    auto& abandoned = first->try_remote_lock("close", 0, 0);
    check(!abandoned.poll(), "remote acquire suspends before Context close");
    first.reset();
    auto next = global.make_user_context();
    auto rebound = next->rebind_global(provider);
    check(next->get(rebound).cancelled == 2 && next->get(rebound).wake == nullptr,
          "User destruction joins remote descendants while Global provider remains alive");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        local_ownership_and_lifetime();
        expiry_and_timeout();
        budgets_and_thread_boundary();
        remote_optional_owner_and_replacement();
        remote_pending_close_and_wakes();
        std::pmr::set_default_resource(previous);
        std::cout << "Context locking contracts passed\n";
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
}
