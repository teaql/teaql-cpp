// Remaining pinned Context-root contracts; no production remote provider.
#include "../../support/locking_fixture.hpp"
#include <teaql/runtime/schema.hpp>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void user_context_reports_missing_schema_provider() {
    GlobalContext global;
    auto context = global.make_user_context();
    const auto& result = context->ensure_schema().wait();
    check(!result && result.error().kind() == RuntimeError::Kind::Schema &&
              result.error().message() == "missing schema provider",
          "original exact missing-schema variant and message");
}

static void user_context_stores_and_exposes_user_identifier() {
    GlobalContext global;
    auto context = global.make_user_context();
    auto owner = context->memory_resource();
    // C++ uses the native thread ID instead of Rust's process-local ThreadId.
    const char* os_user = std::getenv("USER");
    if (!os_user) {
        os_user = std::getenv("USERNAME");
    }
    String expected(os_user ? os_user : "main", owner);
    auto append = [&](auto number) {
        char buffer[32];
        auto end = std::to_chars(buffer, buffer + sizeof(buffer), number).ptr;
        expected.append(buffer, end);
    };
    expected += "@pid-";
#if defined(_WIN32)
    append(::_getpid());
#else
    append(::getpid());
#endif
    expected += ".tid-";
#if defined(__linux__)
    append(::syscall(SYS_gettid));
#else
    append(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
    check(context->user_identifier() == expected,
          "original independently constructed default identity");
    context->set_user_identifier("user-123");
    check(context->user_identifier() == "user-123", "original explicit identity setter");
    auto second = global.make_user_context();
    second->with_user_identifier("user-456");
    check(second->user_identifier() == "user-456", "original identity builder");
    auto third = global.make_user_context();
    third->set_user_identifier_option("user-789");
    check(third->user_identifier() == "user-789", "original present optional identity");
    third->set_user_identifier_option(std::nullopt);
    check(!third->user_identifier(), "original absent optional identity");
    auto fourth = global.make_user_context();
    fourth->with_user_identifier_option("user-abc");
    check(fourth->user_identifier() == "user-abc", "original optional identity builder");
}

static void local_lock_enforces_ownership_timeout_and_lease_expiry() {
    GlobalContext global;
    auto first = global.make_user_context();
    auto second = global.make_user_context();
    String key("local-lock-", first->memory_resource());
    key += first->user_identifier().value();
    check(first->try_local_lock(key, 0, 50), "original initial 50ms lease");
    check(!second->try_local_lock(key, 0, 50), "original zero-timeout rejection");
    second->unlock_local(key);
    check(!second->try_local_lock(key, 0, 50), "original wrong owner cannot release lease");
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    check(second->try_local_lock(key, 0, 50), "original actual 60ms wait expires 50ms lease");
    second->unlock_local(key);
    check(first->try_local_lock(key, 0, 50), "original old owner can reacquire after unlock");
    first->unlock_local(key);
}

static void remote_lock_delegates_and_preserves_context_ownership() {
    GlobalContext global;
    auto first = global.make_user_context();
    auto second = global.make_user_context();
    auto provider = first->make_global_resource<lock_fixture::Provider>();
    first->set_remote_lock_provider(provider);
    second->set_remote_lock_provider(second->rebind_global(provider));
    String key("remote-lock-", first->memory_resource());
    key += first->user_identifier().value();
    check(first->try_remote_lock(key, 0, 1000).wait(), "original initial provider acquisition");
    check(!second->try_remote_lock(key, 0, 1000).wait(), "original occupied provider key");
    check(!second->unlock_remote(key).wait(), "original wrong-owner provider unlock fails");
    check(!second->try_remote_lock(key, 0, 1000).wait(), "original provider ownership retained");
    check(first->unlock_remote(key).wait(), "original owner releases provider lease");
    check(second->try_remote_lock(key, 0, 1000).wait(),
          "original second owner acquires provider lease");
    check(second->unlock_remote(key).wait(), "original second owner releases provider lease");
    auto optional = global.make_user_context();
    check(optional->try_remote_lock("optional", 0, 0).wait(), "original absent provider succeeds");
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
        user_context_reports_missing_schema_provider();
        user_context_stores_and_exposes_user_identifier();
        local_lock_enforces_ownership_timeout_and_lease_expiry();
        remote_lock_delegates_and_preserves_context_ownership();
        std::cout << "Four original Context-tail contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
