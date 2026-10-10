#include <teaql/data_service/sql_log.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct NoncopyableState {
    using allocator_type = Allocator;
    String text;
    int& destroyed;

    NoncopyableState(std::allocator_arg_t, Allocator a, int& counter, std::size_t size = 8192)
        : text(size, 's', a.resource()), destroyed(counter) {}

    NoncopyableState(const NoncopyableState&) = delete;

    ~NoncopyableState() noexcept {
        ++destroyed;
    }
};

static void noncopyable_borrow_and_failure() {
    int destroyed = 0;
    GlobalContext global;
    auto user = global.make_user_context();
    auto first = SqlProjectionState::make<NoncopyableState>(user->memory_resource(), destroyed);
    auto borrowed = first;
    SqlProjectionState allocated(std::allocator_arg, Allocator(user->memory_resource()), first);
    check(first == borrowed && borrowed == allocated,
          "same-domain wrapper copies do not require payload copies");
    check(borrowed.get<NoncopyableState>() == first.get<NoncopyableState>(),
          "same-domain copies borrow the existing payload");
    check(borrowed.get<int>() == nullptr, "typed lookup rejects the wrong type");
    auto owner = global.memory_resource();
    auto& arena = detail::context_arena(owner);
    auto before = arena.stats();
    auto destination = SqlProjectionState::make<int>(owner, 17);
    auto preserved = destination;
    try {
        destination = std::move(first);
        throw std::runtime_error("noncopyable cross-domain assignment was accepted");
    } catch (const std::invalid_argument&) {
    }
    check(destination == preserved && *destination.get<int>() == 17,
          "failed retention preserves the destination");
    check(first == borrowed && first.get<NoncopyableState>()->text.size() == 8192,
          "failed retention does not consume or modify the source");
    check(arena.stats().issued_objects == before.issued_objects + 1,
          "failed payload copy does not publish another object");
    first = SqlProjectionState(user->memory_resource());
    check(destroyed == 0 && borrowed.get<NoncopyableState>()->text.size() == 8192,
          "dropping one wrapper does not destroy the Context-owned payload");
    user.reset();
    check(destroyed == 1, "User destruction cleans the payload exactly once");
    // All three User borrows are invalid now; no lookup is performed on them.
}

static void retained_identity_and_allocator() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto first = SqlProjectionState::make<String>(user->memory_resource(), 8192, 'r');
    auto same_value = SqlProjectionState::make<String>(user->memory_resource(), 8192, 'r');
    auto& retained = global.make<SqlProjectionState>(first);
    check(first == retained && !(first == same_value),
          "retention preserves origin identity, independent allocations remain distinct");
    check(first.get<String>() != retained.get<String>() &&
              first.get<String>()->data() != retained.get<String>()->data(),
          "cross-domain retention deep-copies the payload and its character storage");
    check(retained.get<String>()->get_allocator().resource() == global.memory_resource(),
          "retained strings use the destination allocator");
    auto source_identity_copy = retained;
    user.reset();
    check(retained.get<String>()->size() == 8192 && retained == source_identity_copy,
          "Global payload remains valid after actual User deletion");
    auto next_user = global.make_user_context();
    auto next = SqlProjectionState::make<String>(next_user->memory_resource(), 8192, 'r');
    auto& second_retained = global.make<SqlProjectionState>(next);
    check(!(retained == next) && second_retained == next,
          "new Context allocation cannot reuse a retained origin identity");
    SqlProjectionState empty(global.memory_resource());
    check(empty == SqlProjectionState(next_user->memory_resource()) && !(empty == retained),
          "empty states compare equal across domains and differ from allocated states");
}

static void arena_address_reuse() {
    alignas(Arena) std::byte storage[sizeof(Arena)];
    GlobalContext destination;
    SqlProjectionState retained(destination.memory_resource());
    for (int pass = 0; pass < 2; ++pass) {
        auto* arena = std::construct_at(
            reinterpret_cast<Arena*>(storage), 1024, std::pmr::new_delete_resource());
        auto original = SqlProjectionState::make<int>(arena, 7);
        if (pass == 0) {
            retained = SqlProjectionState(
                std::allocator_arg, Allocator(destination.memory_resource()), original);
        } else {
            check(!(retained == original),
                  "arena address and slot reuse cannot alias a previous logical identity");
        }
        std::destroy_at(arena);
    }
    check(*retained.get<int>() == 7, "retained scalar outlives both source arenas");
}

static void exhausted_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto source = SqlProjectionState::make<String>(user->memory_resource(), 8192, 'q');
    MemoryLimits limits;
    limits.max_objects = 1;
    Arena destination(1024, std::pmr::new_delete_resource(), nullptr, limits);
    auto previous = SqlProjectionState::make<int>(&destination, 23);
    auto target = previous;
    try {
        target = std::move(source);
        throw std::runtime_error("retention exceeded the destination allocation quota");
    } catch (const MemoryLimitError& error) {
        check(error.kind() == MemoryLimitError::Kind::Objects,
              "retention honors the destination object limit");
    }
    check(target == previous && *target.get<int>() == 23,
          "quota failure preserves the destination value and identity");
    check(source.get<String>()->size() == 8192 && source.get<String>()->front() == 'q',
          "quota failure does not consume or alter the source payload");
    check(destination.stats().live_objects == 1 && destination.stats().issued_objects == 1,
          "quota failure publishes no additional live allocation");
}

static void shutdown_diagnostic_identity() {
    struct Shutdown {
        Arena& arena;
        int destroyed = 0;
        bool verified = false;

        void run() noexcept {
            try {
                auto first = SqlProjectionState::make<NoncopyableState>(&arena, destroyed, 64);
                auto second = SqlProjectionState::make<NoncopyableState>(&arena, destroyed, 64);
                auto same = first;
                check(first == same && !(first == second),
                      "private shutdown payloads retain distinct logical identities");
                check(first.get<NoncopyableState>()->text.size() == 64 && destroyed == 0,
                      "shutdown payloads live until callback completion");
                try {
                    arena.make_identified<int>(42);
                    throw std::runtime_error("closing arena published a checked object");
                } catch (const std::logic_error&) {
                }
                verified = true;
            } catch (...) {
                verified = false;
            }
        }
    };

    MemoryLimits limits;
    limits.max_objects = 0;
    Arena arena(1024, std::pmr::new_delete_resource(), nullptr, limits);
    Shutdown shutdown{arena};
    Arena::Quiescence callback;
    callback.attach(arena, &shutdown, [](void* state) noexcept {
        static_cast<Shutdown*>(state)->run();
    });
    arena.close();
    check(shutdown.verified && shutdown.destroyed == 2,
          "private shutdown diagnostics clean up once despite exhausted object quota");
    check(arena.stats().issued_objects == 0 && arena.stats().live_blocks == 0,
          "shutdown tokens consume no public slots and all storage is reclaimed");
}

int main() {
    noncopyable_borrow_and_failure();
    retained_identity_and_allocator();
    arena_address_reuse();
    exhausted_retention();
    shutdown_diagnostic_identity();
    std::cout << "SQL projection ownership and identity contracts passed\n";
}
