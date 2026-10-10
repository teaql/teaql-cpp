#include <teaql/runtime/context.hpp>
#include <teaql/provider_sqlite/id.hpp>
#include <array>
#include <algorithm>
#include <iostream>
#include <thread>
using namespace teaql;

static void check(bool b, const char* message) {
    if (!b) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejected(F f, const char* message) {
    bool caught = false;
    try {
        f();
    } catch (const std::logic_error&) {
        caught = true;
    }
    check(caught, message);
}

#include "../../tests/support/internal_id_fixture.hpp"

struct FailedGenerator : InternalIdGenerator {
    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        return Result<std::uint64_t, RuntimeError>::err(
            r, RuntimeError::id_generation(r, "unavailable"));
    }
};

static void context_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& entity = user->make<Model>();
    auto r = user->memory_resource();
    check(!user->generate_id("Order").value(), "no installed generator yields None");
    check(user->initialize_generated_bootstrap_entity(entity, "Order", 42).error().message() ==
                  "missing internal ID generator" &&
              !entity.fresh,
          "bootstrap requires installed generator; fallback cannot reserve fixed IDs");
    check(user->next_id("Order").value() == 1001, "fallback starts at 1001");
    auto sibling = global.make_user_context();
    check(sibling->next_id("Other").value() == 1002,
          "fallback state is shared across UserContexts and entities");
    auto& failed = user->make_resource<FailedGenerator>();
    user->set_internal_id_generator(*user, failed);
    check(user->next_id("Order").error().message() == "unavailable",
          "configured generator errors do not fall back");
    check(!user->initialize_generated_bootstrap_entity(entity, "Order", 42) && !entity.fresh,
          "default unsupported floor cannot mark entity as new");
    auto& local = user->make<AtomicCounterIdGenerator>(100);
    rejected(
        [&] {
            sibling->set_internal_id_generator(*user, local);
        },
        "sibling generator borrowing is rejected");
    rejected(
        [&] {
            user->set_internal_id_generator(global, local);
        },
        "origin must match actual arena");
    user->with_internal_id_generator(*user, local);
    check(user->initialize_generated_bootstrap_entity(entity, "Order", 900) && entity.fresh &&
              entity.id == 17,
          "bootstrap reserves then marks new without assigning fixed ID");
    check(user->next_id("Other").value() == 901, "floor applies across entity types");
    auto& shared = global.make<AtomicCounterIdGenerator>();
    user->set_internal_id_generator(global, shared);
    sibling->set_internal_id_generator(global, shared);
    check(user->next_id("Order").value() == 1001 && sibling->next_id("Order").value() == 1002,
          "Global generator explicitly shares its sequence");
    user->close();
    rejected(
        [&] {
            user->next_id("Order");
        },
        "closed Context refuses generation");
    check(sibling->next_id("Order").value() == 1003,
          "closing a borrowing Context does not destroy Global generator");
    GlobalContext isolated;
    auto other = isolated.make_user_context();
    check(other->next_id("Order").value() == 1001,
          "independent applications do not share a hidden singleton");
    (void)r;
}

static void checked_generator_expiry() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto generator = user->make_global<AtomicCounterIdGenerator>(10);
    user->with_internal_id_generator(generator);
    check(user->next_id("Order").value() == 11, "Global ID generator installs through User proxy");
    auto other = global.make_user_context();
    other->set_internal_id_generator(other->rebind_global(generator));
    user->destroy(generator);
    bool expired = false;
    try {
        (void)other->next_id("Order");
    } catch (const ReferenceError& error) {
        expired = error.kind() == ReferenceError::Kind::Dead;
    }
    check(expired, "early-destroyed ID generator is rejected before virtual dispatch");
    auto local = other->make_ref<AtomicCounterIdGenerator>(20);
    other->set_internal_id_generator(local);
    other->destroy(local);
    expired = false;
    try {
        (void)other->generate_id("Order");
    } catch (const ReferenceError& error) {
        expired = error.kind() == ReferenceError::Kind::Dead;
    }
    check(expired, "User ID generator uses the same checked lifetime");
}

static void counters_and_threads() {
    GlobalContext owner;
    auto& counter = owner.make<AtomicCounterIdGenerator>(0);
    std::array<std::array<std::uint64_t, 128>, 4> values{};
    std::array<std::thread, 4> threads;
    for (std::size_t t = 0; t < threads.size(); ++t) {
        threads[t] = std::thread([&, t] {
            GlobalContext output;
            auto r = output.memory_resource();
            for (auto& value : values[t]) {
                value = counter.generate_id(r, "Order").value();
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    std::array<std::uint64_t, 512> all{};
    std::size_t i = 0;
    for (auto& batch : values) {
        for (auto value : batch) {
            all[i++] = value;
        }
    }
    std::sort(all.begin(), all.end());
    for (i = 0; i < all.size(); ++i) {
        check(all[i] == i + 1,
              "atomic generator supports concurrent calls with caller-owned result arenas");
    }
    auto r = owner.memory_resource();
    check(bool(counter.ensure_floor(r, "x", UINT64_MAX)), "full u64 floor is accepted");
#ifndef NDEBUG
    bool overflow = false;
    try {
        counter.generate_id(r, "x");
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    check(overflow, "debug checked addition mirrors Rust panic");
#else
    check(counter.generate_id(r, "x").value() == 0, "release addition wraps like Rust release");
#endif
    check(counter.generate_id(r, "x").value() == 1,
          "atomic state wraps before checked return addition");
}

static void snowflake_transitions() {
    GlobalContext global;
    auto r = global.memory_resource();
    constexpr std::uint64_t epoch = 1288834974657;
    detail::SnowflakeState state;
    std::uint64_t tick = epoch, waited = 0;
    bool fail_wait = false;
    auto now = [&] {
        return detail::IdMillis::ok(r, tick);
    };
    auto wait = [&](std::uint64_t last) {
        waited = last;
        if (fail_wait) {
            return detail::IdMillis::err(r, RuntimeError::id_generation(r, "clock error"));
        }
        return detail::IdMillis::ok(r, last + 1);
    };
    auto next = [&] {
        return detail::advance_snowflake(r, state, 31, 17, now, wait);
    };
    check(next().value() == ((17ULL << 17) | (31ULL << 12)),
          "epoch has zero relative timestamp and sequence");
    check((next().value() & 4095) == 1, "same millisecond advances sequence");
    state.sequence = 4095;
    check((next().value() >> 22) == 1 && waited == epoch && state.sequence == 0,
          "sequence exhaustion waits for next millisecond");
    tick = epoch - 2;
    check((next().value() >> 22) == 2 && state.sequence == 0,
          "clock rollback waits beyond last timestamp then resets sequence");
    auto before = state;
    fail_wait = true;
    check(!next() && state.last_timestamp == before.last_timestamp &&
              state.sequence == before.sequence,
          "rollback wait errors preserve state");
    tick = state.last_timestamp;
    state.sequence = 4095;
    check(!next() && state.sequence == 0 && state.last_timestamp == tick,
          "overflow wait failure retains source sequence reset");
    fail_wait = false;
    state = {};
    tick = epoch - 1;
    check(next().error().message() == "system clock is before snowflake epoch" &&
              state.last_timestamp == tick,
          "pre-epoch error records observed timestamp first");
    rejected(
        [&] {
            global.make<SnowflakeIdGenerator>(32, 0);
        },
        "worker range checked");
    rejected(
        [&] {
            global.make<SnowflakeIdGenerator>(0, 32);
        },
        "datacenter range checked");
    auto& live = global.make<SnowflakeIdGenerator>(5, 7);
    check(bool(live.ensure_floor(r, "any", UINT64_MAX)), "snowflake floor is no-op");
    auto previous = live.generate_id(r, "a").value();
    for (unsigned i = 0; i < 5000; ++i) {
        auto value = live.generate_id(r, "b").value();
        check(value > previous && ((value >> 12) & 31) == 5 && ((value >> 17) & 31) == 7,
              "live snowflake IDs increase with preserved worker/datacenter fields");
        previous = value;
    }
}

static void sqlite_adapter() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>();
    auto& executor = global.make<SqliteMutationExecutor>(connection);
    auto& ids = global.make<SqliteIdSpaceGenerator>(executor);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->set_internal_id_generator(global, ids);
    auto& entity = user->make<Model>();
    check(user->initialize_generated_bootstrap_entity(entity, "OrderItem", 100) && entity.fresh,
          "SQLite runtime floor reserves before marking");
    check(user->next_id("order_item").value() == 101,
          "SQLite runtime generator preserves canonical ID space");
    entity.fresh = false;
    auto low = ids.ensure_floor(r, "OrderItem", UINT64_MAX);
    auto high =
        static_cast<const InternalIdGenerator&>(ids).ensure_floor(r, "OrderItem", UINT64_MAX);
    check(!low && !high && high.error().kind() == RuntimeError::Kind::IdGeneration &&
              high.error().message() == low.error().to_string(r),
          "SQLite runtime error wraps exact underlying executor display");
    check(!user->initialize_generated_bootstrap_entity(entity, "OrderItem", UINT64_MAX) &&
              !entity.fresh,
          "failed SQLite floor leaves entity unchanged");
}

struct ClosingGenerator final : InternalIdGenerator {
    UserContext& context;

    explicit ClosingGenerator(UserContext& context) : context(context) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r, std::string_view) const override {
        context.close();
        return Result<std::uint64_t, RuntimeError>::ok(r, 1);
    }

protected:
    Result<std::monostate, RuntimeError>
    do_ensure_floor(Memory r, std::string_view, std::uint64_t) const override {
        context.close();
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

int main(int argc, char** argv) {
    if (argc == 2) {
        std::set_terminate([] {
            std::_Exit(0);
        });
        try {
            GlobalContext global;
            auto user = global.make_user_context();
            auto& generator = user->make_resource<ClosingGenerator>(*user);
            user->set_internal_id_generator(*user, generator);
            if (std::string_view(argv[1]) == "reentrant-close") {
                (void)user->next_id("Order");
            } else {
                auto& model = user->make<Model>();
                (void)user->initialize_generated_bootstrap_entity(model, "Order", 1);
            }
        } catch (...) {
            std::_Exit(2);
        }
        return 1;
    }
    try {
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        context_contracts();
        checked_generator_expiry();
        counters_and_threads();
        snowflake_transitions();
        sqlite_adapter();
        std::pmr::set_default_resource(old);
        std::cout << "Internal ID contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
