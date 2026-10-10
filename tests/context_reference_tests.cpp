#include <teaql/runtime/context.hpp>
#include <array>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <thread>
#include <vector>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static ReferenceError rejected(ReferenceError::Kind kind, F&& action) {
    try {
        action();
    } catch (const ReferenceError& error) {
        check(error.kind() == kind, "reference rejection has the expected reason");
        return error;
    }
    throw std::runtime_error("invalid reference was accepted");
}

template <class F>
static void limited(MemoryLimitError::Kind kind, F&& action) {
    try {
        action();
    } catch (const MemoryLimitError& error) {
        check(error.kind() == kind, "allocation failure identifies its budget");
        return;
    }
    throw std::runtime_error("allocation limit was bypassed");
}

class TrackingResource final : public std::pmr::memory_resource {
public:
    bool fail = false;
    std::optional<std::size_t> fail_attempt;
    std::size_t allocation_attempts = 0;
    std::map<void*, std::pair<std::size_t, std::size_t>> allocations;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto attempt = allocation_attempts++;
        if (fail || fail_attempt == attempt) {
            throw std::bad_alloc{};
        }
        auto* result = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        allocations.emplace(result, std::pair{bytes, alignment});
        return result;
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        auto entry = allocations.find(pointer);
        if (entry == allocations.end() || entry->second != std::pair{bytes, alignment}) {
            std::terminate();
        }
        allocations.erase(entry);
        std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

struct Owned {
    int id;
    std::vector<int>& destroyed;

    Owned(int id, std::vector<int>& destroyed) : id(id), destroyed(destroyed) {}

    ~Owned() noexcept {
        destroyed.push_back(id);
    }
};

static void identities_and_expiry() {
    TrackingResource upstream;
    {
        GlobalContext global(&upstream);
        auto user = global.make_user_context();
        auto other = global.make_user_context();
        auto value = user->make_ref<int>(42);
        auto copied = value;
        check(user->get(copied) == 42 && copied == value, "copied IDs resolve one object");
        auto wrong = Ref<long>(value.id());
        rejected(ReferenceError::Kind::Type, [&] {
            (void)user->get(wrong);
        });
        rejected(ReferenceError::Kind::Invalid, [&] {
            (void)user->get(Ref<int>{});
        });
        rejected(ReferenceError::Kind::ForeignContext, [&] {
            (void)other->get(value);
        });
        rejected(ReferenceError::Kind::Invalid, [&] {
            (void)user->get(Ref<int>(value.id() + 100));
        });
        GlobalContext foreign;
        auto foreign_user = foreign.make_user_context();
        auto foreign_value = foreign_user->make_ref<int>(9);
        rejected(ReferenceError::Kind::ForeignRuntime, [&] {
            (void)user->get(foreign_value);
        });
        auto close = std::source_location::current();
        user->close(close);
        user->close();
        auto access = std::source_location::current();
        auto error = rejected(ReferenceError::Kind::Closed, [&] {
            (void)user->get(value, access);
        });
        check(error.close_location().line() == close.line() &&
                  error.access_location().line() == access.line(),
              "errors retain the original close and current access location");
        check(user->allocation_stats().bitmap_bytes == 0 &&
                  user->allocation_stats().live_bytes == 0,
              "User bitmap and all backing blocks are reclaimed");
        user.reset();
        error = rejected(ReferenceError::Kind::Closed, [&] {
            (void)other->get(copied);
        });
        check(error.close_location().line() == close.line(),
              "expiry diagnostic remains valid after User destruction");
        auto fresh = other->make_ref<int>(8);
        check(detail::reference_slot(fresh.id()) == detail::reference_slot(value.id()) &&
                  fresh != value && other->get(fresh) == 8,
              "equal local slots in different Contexts cannot revive old IDs");
        bool wrong_thread = false;
        std::thread worker([&] {
            try {
                (void)other->get(fresh);
            } catch (const std::logic_error&) {
                wrong_thread = true;
            }
        });
        worker.join();
        check(wrong_thread && other->get(fresh) == 8, "foreign-thread access is rejected");
    }
    check(upstream.allocations.empty(), "Global destruction returns control metadata too");

    std::optional<GlobalContext> runtime;
    runtime.emplace();
    Ref<int> expired;
    {
        auto user = runtime->make_user_context();
        expired = user->make_ref<int>(1);
    }
    runtime.reset();
    runtime.emplace();
    auto user = runtime->make_user_context();
    rejected(ReferenceError::Kind::ForeignRuntime, [&] {
        (void)user->get(expired);
    });
}

static void global_delegation_and_destruction() {
    std::vector<int> destroyed;
    destroyed.reserve(8);
    GlobalContext global;
    auto first = global.make_user_context();
    auto second = global.make_user_context();
    auto local = first->make_resource_ref<Owned>(1, destroyed);
    auto shared = first->make_global_resource<Owned>(2, destroyed);
    auto bootstrap = global.make_ref<int>(19);
    rejected(ReferenceError::Kind::Invalid, [&] {
        (void)first->get(bootstrap);
    });
    check(first->get(first->rebind_global(bootstrap)) == 19,
          "bootstrap Global references acquire access through the current User");
    auto unissued_context =
        bootstrap.id() | (std::uint64_t(context_identity_limit) << detail::context_shift);
    rejected(ReferenceError::Kind::Invalid, [&] {
        (void)first->rebind_global(Ref<int>(unissued_context));
    });
    auto text = first->make_ref<String>(4096, 'x');
    auto rows = first->make_ref<Vector<String>>();
    first->get(rows).emplace_back(2048, 'r');
    auto kept = first->make_global<Vector<String>>(first->get(rows));
    check(first->get(text).get_allocator().resource() == first->memory_resource(),
          "checked construction propagates User allocator");
    check(first->get(kept).front().get_allocator().resource() == global.memory_resource(),
          "Global delegation performs nested allocator-aware copying");
    first->destroy(local);
    check(destroyed == std::vector<int>{1}, "early destruction runs exactly once");
    rejected(ReferenceError::Kind::Dead, [&] {
        (void)first->get(local);
    });
    rejected(ReferenceError::Kind::Dead, [&] {
        first->destroy(local);
    });
    auto newer = first->make_resource_ref<Owned>(3, destroyed);
    check(newer.id() != local.id(), "destroyed object slots are not reused");
    first->close();
    check(destroyed == std::vector<int>({1, 3}), "close skips already destroyed payloads");
    rejected(ReferenceError::Kind::Closed, [&] {
        (void)first->get(shared);
    });
    first.reset();
    auto rebound = second->rebind_global(shared);
    auto copied = second->rebind_global(kept);
    check(second->get(rebound).id == 2 && second->get(copied).front().size() == 2048,
          "Global objects survive while old User access expires");
    rejected(ReferenceError::Kind::NotGlobal, [&] {
        (void)second->rebind_global(newer);
    });
    second->destroy(rebound);
    check(destroyed == std::vector<int>({1, 3, 2}), "Global resource cleanup uses its owner");
}

static void registered_resource_borrows() {
    std::vector<int> destroyed;
    destroyed.reserve(4);
    GlobalContext global;
    auto user = global.make_user_context();
    auto local = user->make_resource_ref<Owned>(1, destroyed);
    user->insert_resource(local);
    user->insert_named_resource("local", local);
    check(user->get_resource<Owned>()->id == 1 && user->get_named_resource<Owned>("local")->id == 1,
          "checked resource registration delegates its owner without exposing Global");
    auto* raw = &user->get(local);
    auto* member = &raw->id;
    ResourceBorrow<Owned> borrow(std::allocator_arg,
                                 Allocator(user->memory_resource()),
                                 user->get(local),
                                 detail::resource_arena(user->memory_resource()));
    user->destroy(local);
    rejected(ReferenceError::Kind::Dead, [&] {
        (void)detail::resource_arena(user->memory_resource()).slot_for_address(raw);
    });
    rejected(ReferenceError::Kind::Dead, [&] {
        ResourceBorrow<Owned> recreated(std::allocator_arg,
                                        Allocator(user->memory_resource()),
                                        *raw,
                                        detail::resource_arena(user->memory_resource()));
    });
    rejected(ReferenceError::Kind::Dead, [&] {
        ResourceBorrow<int> recreated(std::allocator_arg,
                                      Allocator(user->memory_resource()),
                                      *member,
                                      detail::resource_arena(user->memory_resource()));
    });
    rejected(ReferenceError::Kind::Dead, [&] {
        user->insert_resource(*user, *raw);
    });
    rejected(ReferenceError::Kind::Dead, [&] {
        (void)borrow.get();
    });
    rejected(ReferenceError::Kind::Dead, [&] {
        (void)user->get_resource<Owned>();
    });
    rejected(ReferenceError::Kind::Dead, [&] {
        (void)user->get_named_resource<Owned>("local");
    });
    auto shared = user->make_global_resource<Owned>(2, destroyed);
    ResourceBorrow<Owned> global_borrow(std::allocator_arg,
                                        Allocator(user->memory_resource()),
                                        user->get(shared),
                                        detail::resource_arena(global.memory_resource()));
    user.reset();
    rejected(ReferenceError::Kind::Closed, [&] {
        (void)global_borrow.get();
    });
    rejected(ReferenceError::Kind::Closed, [&] {
        (void)global.make<ResourceBorrow<Owned>>(global_borrow);
    });
    auto next = global.make_user_context();
    check(next->get(next->rebind_global(shared)).id == 2,
          "expired access does not destroy its Global resource");
}

struct FailingParent {
    FailingParent(UserContext& user, Ref<int>& child) {
        child = user.make_ref<int>(73);
        throw std::runtime_error("parent construction fails after publishing a child");
    }

    ~FailingParent() noexcept = default;
};

static void quotas_and_failed_construction() {
    ContextLimits limits;
    limits.max_user_contexts = 2;
    limits.user.max_objects = 3;
    GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    auto user = global.make_user_context();
    Ref<int> child;
    bool failed = false;
    try {
        (void)user->make_resource_ref<FailingParent>(*user, child);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    check(failed && user->get(child) == 73, "failed parent leaves a published child valid");
    rejected(ReferenceError::Kind::Dead, [&] {
        (void)user->get(Ref<int>(child.id() - 1));
    });
    auto last = user->make_ref<int>(99);
    check(user->allocation_stats().issued_objects == 3 &&
              user->allocation_stats().live_objects == 2,
          "failed reservations remain unavailable and count against the finite namespace");
    limited(MemoryLimitError::Kind::Objects, [&] {
        (void)user->make_ref<int>();
    });
    check(user->get(last) == 99, "existing objects survive count exhaustion");
    user->close();
    auto second = global.make_user_context();
    check(global.issued_contexts() == 2 && second->identity() != user->identity(),
          "closed Context identities are not reused");
    limited(MemoryLimitError::Kind::Contexts, [&] {
        (void)global.make_user_context();
    });
    auto next = second->make_ref<int>(12);
    check(second->get(next) == 12, "Context identity exhaustion preserves existing Users");

    MemoryLimits no_objects;
    no_objects.max_objects = 0;
    GlobalContext payload_global;
    UserContext payloads(payload_global, no_objects);
    auto text = payloads.save_string("payload storage has no independent object ID");
    auto bytes = payloads.save_bytes(std::as_bytes(std::span("abc", 3)));
    check(payloads.get(text).size() > 0 && payloads.get(bytes).size() == 3 &&
              payloads.allocation_stats().issued_objects == 0,
          "ordinary character and byte storage do not consume object IDs");
    limited(MemoryLimitError::Kind::Objects, [&] {
        (void)payloads.make_ref<int>();
    });
}

static void million_object_bitmap() {
    GlobalContext global;
    auto user = global.make_user_context();
    Ref<int> first;
    Ref<int> last;
    for (int index = 0; index < 1'000'000; ++index) {
        auto value = user->make_ref<int>(index);
        if (index == 0) {
            first = value;
        }
        last = value;
    }
    check(user->allocation_stats().bitmap_bytes == 125000 && user->get(first) == 0 &&
              user->get(last) == 999999,
          "one million exact object bits occupy 125000 bytes");
    limited(MemoryLimitError::Kind::Objects, [&] {
        (void)user->make_ref<int>();
    });
    user->close();
    check(user->allocation_stats().live_blocks == 0 && user->allocation_stats().bitmap_bytes == 0,
          "million-object metadata is User-owned and reclaimed on close");
}

static void object_publication_allocation_failures() {
    struct LargeResource {
        std::array<std::byte, 8192> payload{};
        int& destroyed;

        explicit LargeResource(int& destroyed) : destroyed(destroyed) {}

        ~LargeResource() noexcept {
            ++destroyed;
        }
    };

    bool failed_before_reservation = false;
    bool failed_after_reservation = false;
    bool completed = false;
    for (std::size_t offset = 0; offset < 8; ++offset) {
        TrackingResource upstream;
        int destroyed = 0;
        {
            GlobalContext global(&upstream, 128);
            auto user = global.make_user_context(128);
            Ref<int> first;
            for (int index = 0; index < 64; ++index) {
                auto value = user->make_ref<int>(index);
                if (index == 0) {
                    first = value;
                }
            }
            upstream.fail_attempt = upstream.allocation_attempts + offset;
            bool failed = false;
            try {
                auto value = user->make_resource_ref<LargeResource>(destroyed);
                check(user->get(value).payload.back() == std::byte{},
                      "successfully published resource is fully constructed");
                completed = true;
            } catch (const std::bad_alloc&) {
                failed = true;
            }
            upstream.fail_attempt.reset();
            check(user->get(first) == 0 && destroyed == 0,
                  "allocation failure preserves existing objects and never destroys unbuilt ones");
            auto issued = user->allocation_stats().issued_objects;
            if (failed) {
                check(user->allocation_stats().live_objects == 64,
                      "failed object publication leaves no live bit");
                failed_before_reservation |= issued == 64;
                failed_after_reservation |= issued == 65;
                if (issued == 65) {
                    auto dead = Ref<LargeResource>(detail::reference_id(
                        detail::reference_runtime(first.id()), user->identity(), false, 65));
                    rejected(ReferenceError::Kind::Dead, [&] {
                        (void)user->get(dead);
                    });
                }
            }
            auto retry = user->make_resource_ref<LargeResource>(destroyed);
            check(detail::reference_slot(retry.id()) == issued + 1 &&
                      user->get(retry).payload.front() == std::byte{},
                  "retry never reuses a consumed identity and publishes a complete object");
            user->destroy(retry);
            check(destroyed == 1 && user->get(first) == 0,
                  "early resource destruction leaves prior objects usable");
            user.reset();
            check(destroyed == (failed ? 1 : 2) && global.live_user_bytes() == 0,
                  "User deletion destroys each published resource once and returns all blocks");
        }
        check(upstream.allocations.empty(), "failed object publication leaks no backing blocks");
        if (completed) {
            break;
        }
    }
    check(failed_before_reservation && failed_after_reservation && completed,
          "fault injection covers bitmap growth and reserved-object storage allocation");
}

static void independent_tables_and_page_boundary_failure() {
    ContextLimits limits;
    limits.global.max_objects = 2;
    limits.user.max_objects = 65;
    GlobalContext global(std::pmr::new_delete_resource(), 128, limits);
    auto user = global.make_user_context(128);
    auto shared = user->make_global<int>(100);
    auto shared2 = user->make_global<int>(200);
    limited(MemoryLimitError::Kind::Objects, [&] {
        (void)user->make_global<int>();
    });

    Ref<int> first;
    for (int index = 0; index < 63; ++index) {
        auto value = user->make_ref<int>(index);
        if (index == 0) {
            first = value;
        }
    }
    Ref<int> child;
    bool failed = false;
    try {
        (void)user->make_resource_ref<FailingParent>(*user, child);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    check(failed && detail::reference_slot(child.id()) == 65 && user->get(child) == 73,
          "nested construction across a bitmap page preserves the published child");
    rejected(ReferenceError::Kind::Dead, [&] {
        (void)user->get(Ref<int>(child.id() - 1));
    });
    check(user->allocation_stats().issued_objects == 65 &&
              user->allocation_stats().live_objects == 64 &&
              user->allocation_stats().bitmap_bytes == 16 &&
              global.allocation_stats().issued_objects == 2 &&
              global.allocation_stats().bitmap_bytes == 8,
          "User and Global reservations and bitmaps grow independently");
    limited(MemoryLimitError::Kind::Objects, [&] {
        (void)user->make_ref<int>();
    });
    user->destroy(child);
    check(user->get(first) == 0 && user->get(shared) == 100 && user->get(shared2) == 200,
          "clearing a User bit leaves other User and Global objects live");
    user->destroy(shared);
    check(user->get(shared2) == 200 && global.allocation_stats().live_objects == 1,
          "clearing a Global bit leaves its adjacent Global object live");
    user->close();
    check(user->allocation_stats().bitmap_bytes == 0 && global.allocation_stats().bitmap_bytes == 8,
          "User close reclaims only the User bitmap");
    auto next = global.make_user_context();
    check(next->get(next->rebind_global(shared2)) == 200,
          "Global survivor can be rebound after the exhausted User closes");
    rejected(ReferenceError::Kind::Dead, [&] {
        (void)next->rebind_global(shared);
    });
}

struct ShutdownProbe {
    UserContext& user;
    Ref<int> reference;
    bool rejected = false;
    bool diagnostic = false;
    bool oversized_diagnostic = false;
    Arena::Quiescence registration;

    void stop() noexcept {
        try {
            (void)user.get(reference);
        } catch (const ReferenceError& error) {
            rejected = error.kind() == ReferenceError::Kind::Closed;
        }
        auto& message = user.make<String>(1024, 'd');
        diagnostic = message.size() == 1024;
        try {
            (void)user.make<String>(8192, 'x');
        } catch (const MemoryLimitError& error) {
            oversized_diagnostic = error.kind() == MemoryLimitError::Kind::Bytes;
        }
        user.close();
    }
};

static void budget_exhaustion_and_shutdown() {
    TrackingResource upstream;
    {
        GlobalContext global(&upstream);
        MemoryLimits limits;
        limits.max_bytes = 16384;
        limits.shutdown_reserve_bytes = 4096;
        limits.max_objects = 1;
        UserContext user(global, limits, 128);
        auto value = user.make_ref<int>(7);
        ShutdownProbe probe{user, value};
        detail::resource_arena(user.memory_resource()).check_thread();
        probe.registration.attach(
            detail::resource_arena(user.memory_resource()), &probe, [](void* raw) noexcept {
                static_cast<ShutdownProbe*>(raw)->stop();
            });
        limited(MemoryLimitError::Kind::Bytes, [&] {
            for (;;) {
                (void)user.memory_resource()->allocate(1024);
            }
        });
        check(user.allocation_stats().live_bytes <= limits.max_bytes && user.get(value) == 7,
              "whole-block accounting respects the hard byte cap");
        upstream.fail = true;
        user.close();
        check(probe.rejected && probe.diagnostic && probe.oversized_diagnostic &&
                  user.allocation_stats().live_bytes == 0,
              "full quotas and failed upstream still permit precharged shutdown diagnostics");
        check(upstream.allocations.size() == global.allocation_stats().live_blocks,
              "shutdown returns User metadata and reserve blocks before Global close");
        upstream.fail = false;
    }
    check(upstream.allocations.empty(), "all domains return matching upstream blocks");
}

struct ClosingAccessProbe {
    UserContext& user;
    Ref<int> object;
    Ref<int> global_object;
    StringRef text;
    BytesRef bytes;
    std::size_t rejections = 0;
    bool unexpected_error = false;
    Arena::Quiescence registration;

    template <class F>
    void expect_closed(F&& action) noexcept {
        try {
            action();
        } catch (const ReferenceError& error) {
            if (error.kind() == ReferenceError::Kind::Closed) {
                ++rejections;
            } else {
                unexpected_error = true;
            }
        } catch (...) {
            unexpected_error = true;
        }
    }

    void stop() noexcept {
        expect_closed([&] {
            (void)user.get(object);
        });
        expect_closed([&] {
            (void)user.get(global_object);
        });
        expect_closed([&] {
            (void)user.get(text);
        });
        expect_closed([&] {
            (void)user.get(bytes);
        });
        expect_closed([&] {
            (void)user.make_ref<int>(1);
        });
        expect_closed([&] {
            (void)user.make_resource_ref<int>(1);
        });
        expect_closed([&] {
            (void)user.make_global<int>(1);
        });
        expect_closed([&] {
            (void)user.make_global_resource<int>(1);
        });
        expect_closed([&] {
            (void)user.save_string("closing");
        });
        expect_closed([&] {
            (void)user.intern_string("closing");
        });
        expect_closed([&] {
            (void)user.save_global_string("closing");
        });
        expect_closed([&] {
            (void)user.intern_global_string("closing");
        });
        expect_closed([&] {
            (void)user.literal_string("closing");
        });
        expect_closed([&] {
            (void)user.save_bytes({});
        });
        expect_closed([&] {
            (void)user.save_global_bytes({});
        });
        expect_closed([&] {
            (void)user.rebind_global(global_object);
        });
        expect_closed([&] {
            user.destroy(object);
        });
        expect_closed([&] {
            user.insert_resource(global_object);
        });
    }
};

static void closing_rejects_business_access() {
    TrackingResource upstream;
    GlobalContext global(&upstream);
    auto user = global.make_user_context();
    auto object = user->make_ref<int>(7);
    auto global_object = user->make_global<int>(11);
    auto text = user->save_string("User-owned characters");
    std::array<std::byte, 1> input{std::byte{42}};
    auto bytes = user->save_bytes(input);
    ClosingAccessProbe probe{*user, object, global_object, text, bytes};
    probe.registration.attach(
        detail::resource_arena(user->memory_resource()), &probe, [](void* raw) noexcept {
            static_cast<ClosingAccessProbe*>(raw)->stop();
        });
    auto global_before = global.allocation_stats();
    auto attempts_before = upstream.allocation_attempts;
    upstream.fail = true;
    user->close();
    check(probe.rejections == 18 && !probe.unexpected_error,
          "Closing rejects business reads, allocation, rebinding, destruction and registration");
    check(upstream.allocation_attempts == attempts_before &&
              global.allocation_stats().issued_objects == global_before.issued_objects &&
              global.allocation_stats().live_bytes == global_before.live_bytes &&
              user->allocation_stats().live_bytes == 0,
          "rejected shutdown access cannot allocate or change surviving Global objects");
    user.reset();
    upstream.fail = false;
    auto next = global.make_user_context();
    check(next->get(next->rebind_global(global_object)) == 11,
          "rejecting Global access during User shutdown preserves the Global owner");
}

static void aggregate_budget_and_invalid_limits() {
    TrackingResource upstream;
    {
        ContextLimits limits;
        limits.max_live_user_bytes = 20000;
        GlobalContext global(&upstream, 4096, limits);
        auto first = global.make_user_context();
        auto second = global.make_user_context();
        auto occupied = global.live_user_bytes();
        limited(MemoryLimitError::Kind::Bytes, [&] {
            (void)global.make_user_context();
        });
        check(global.live_user_bytes() == occupied && global.issued_contexts() == 2,
              "failed reserve does not leak aggregate bytes or publish a Context identity");
        first.reset();
        auto replacement = global.make_user_context();
        check(replacement->identity() == 3 &&
                  global.live_user_bytes() <= limits.max_live_user_bytes,
              "actual User block release restores aggregate byte capacity without reusing IDs");
        second.reset();
        replacement.reset();
        check(global.live_user_bytes() == 0 &&
                  upstream.allocations.size() == global.allocation_stats().live_blocks,
              "aggregate budget owns no User storage after all Users are destroyed");
    }
    check(upstream.allocations.empty(), "aggregate resource is accounting, not a third owner");
    MemoryLimits malformed;
    malformed.max_objects = object_slot_limit + 1;
    bool rejected_limit = false;
    try {
        GlobalContext global;
        UserContext user(global, malformed);
    } catch (const std::invalid_argument&) {
        rejected_limit = true;
    }
    check(rejected_limit, "object limits must fit the packed ID");
    malformed = {};
    malformed.max_bytes = 1;
    rejected_limit = false;
    try {
        GlobalContext global;
        UserContext user(global, malformed);
    } catch (const std::invalid_argument&) {
        rejected_limit = true;
    }
    check(rejected_limit, "shutdown reserve must fit the complete byte budget");
    GlobalContext global;
    auto user = global.make_user_context();
    limited(MemoryLimitError::Kind::Bytes, [&] {
        (void)user->save_string(std::string_view("x", std::numeric_limits<std::size_t>::max()));
    });
    limited(MemoryLimitError::Kind::Bytes, [&] {
        // Exercise runtime rejection without a compile-time oversized-allocation diagnostic.
        volatile std::size_t requested = std::numeric_limits<std::size_t>::max();
        (void)user->memory_resource()->allocate(requested);
    });
}

static void context_initialization_failures() {
    bool failed_before_registration = false;
    bool failed_after_registration = false;
    bool completed = false;
    for (std::size_t offset = 0; offset < 16; ++offset) {
        TrackingResource upstream;
        {
            GlobalContext global(&upstream, 128);
            auto bootstrap = global.make_ref<int>(41);
            upstream.fail_attempt = upstream.allocation_attempts + offset;
            std::unique_ptr<UserContext> user;
            bool failed = false;
            try {
                user = global.make_user_context(128);
            } catch (const std::bad_alloc&) {
                failed = true;
            }
            upstream.fail_attempt.reset();
            if (!failed) {
                check(user->get(user->rebind_global(bootstrap)) == 41,
                      "successful initialization preserves bootstrap objects");
                user.reset();
                completed = true;
            } else {
                check(global.live_user_bytes() == 0,
                      "failed initialization returns every User backing block");
                auto issued = global.issued_contexts();
                failed_before_registration |= issued == 0;
                failed_after_registration |= issued != 0;
                auto replacement = global.make_user_context(128);
                check(replacement->identity() == issued + 1 &&
                          replacement->get(replacement->rebind_global(bootstrap)) == 41,
                      "retry preserves Global objects and never reuses failed Context identities");
                if (issued != 0) {
                    auto unpublished =
                        Ref<int>(detail::reference_id(detail::reference_runtime(bootstrap.id()),
                                                      issued,
                                                      true,
                                                      detail::reference_slot(bootstrap.id())));
                    rejected(ReferenceError::Kind::Invalid, [&] {
                        (void)replacement->rebind_global(unpublished);
                    });
                }
                replacement.reset();
            }
            check(global.live_user_bytes() == 0 &&
                      upstream.allocations.size() == global.allocation_stats().live_blocks,
                  "only bounded Global metadata remains after initialization recovery");
        }
        check(upstream.allocations.empty(), "initialization failure never leaks upstream blocks");
        if (completed) {
            break;
        }
    }
    check(completed && failed_before_registration && failed_after_registration,
          "fault injection covers both sides of Context identity registration");
}

static void string_storage_and_payload_views() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ContextLimits limits;
        limits.user.strings.max_entries = 2;
        limits.user.strings.max_bytes = 1024;
        GlobalContext global(std::pmr::new_delete_resource(), 128, limits);
        auto user = global.make_user_context(128);
        auto other = global.make_user_context();
        auto ordinary = user->save_string("same");
        auto ordinary2 = user->save_string("same");
        check(user->get(ordinary).data() != user->get(ordinary2).data(),
              "ordinary saving does not pay for deduplication");
        auto shared = user->intern_string("same");
        auto shared2 = user->intern_string("same");
        check(user->get(shared).data() == user->get(shared2).data(),
              "explicit interning shares exact content");
        auto binary = user->intern_string(std::string_view("a\0b", 3));
        check(user->get(binary).size() == 3 && user->get(binary)[2] == 'b',
              "interning preserves embedded zero bytes");
        auto fallback = user->intern_string("pool full: ordinary storage remains available");
        check(user->get(fallback).starts_with("pool full") &&
                  user->string_storage_stats().interned_entries == 2 &&
                  user->string_storage_stats().fallbacks == 1,
              "entry limits stop admission without evicting borrowed strings");
        for (int index = 0; index < 256; ++index) {
            (void)user->save_string(std::string_view("large payload", 13));
        }
        check(user->get(shared) == "same" && user->allocation_stats().issued_objects == 0,
              "string growth preserves addresses and consumes no object IDs");
        auto literal = user->literal_string("source literal");
        check(user->get(literal) == "source literal", "static literal borrowing is checked");
        auto copied = user->save_global_string(user->get(binary));
        auto global_shared = user->intern_global_string("global repeated metadata");
        auto global_shared2 = user->intern_global_string("global repeated metadata");
        check(user->get(global_shared).data() == user->get(global_shared2).data(),
              "Global and User interners are separate explicit owners");
        std::array<std::byte, 3> input{std::byte{1}, std::byte{2}, std::byte{3}};
        auto bytes = user->save_bytes(input);
        auto kept = user->save_global_bytes(input);
        input[0] = std::byte{9};
        check(user->get(bytes)[0] == std::byte{1}, "byte saving copies input into its domain");
        rejected(ReferenceError::Kind::ForeignContext, [&] {
            (void)other->get(shared);
        });
        user->close();
        user.reset();
        rejected(ReferenceError::Kind::Closed, [&] {
            (void)other->get(shared);
        });
        rejected(ReferenceError::Kind::Closed, [&] {
            (void)other->get(bytes);
        });
        check(other->get(other->rebind_global(copied)) == std::string_view("a\0b", 3) &&
                  other->get(other->rebind_global(kept))[2] == std::byte{3},
              "explicit Global payload copies survive without extending User access");
        check(other->get(other->rebind_global(literal)) == "source literal",
              "static characters remain borrowable through a new User access scope");
        rejected(ReferenceError::Kind::NotGlobal, [&] {
            (void)other->rebind_global(bytes);
        });
        MemoryLimits tiny_pool;
        tiny_pool.strings.max_bytes = 1;
        UserContext tiny(global, tiny_pool);
        auto view = tiny.intern_string("too large for this pool");
        check(tiny.get(view) == "too large for this pool" &&
                  tiny.string_storage_stats().pool_bytes <= 1 &&
                  tiny.string_storage_stats().interned_entries == 0,
              "pool byte exhaustion falls back inside the owner budget");
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
    std::pmr::set_default_resource(previous);
}

static void global_string_allocation_failures() {
    std::size_t failures = 0;
    bool completed = false;
    for (std::size_t offset = 0; offset < 12; ++offset) {
        TrackingResource upstream;
        {
            GlobalContext global(&upstream, 128);
            auto user = global.make_user_context(128);
            auto local = user->make_ref<int>(17);
            auto retained = user->make_global<int>(23);
            auto input = user->save_string(String(16384, 's', user->memory_resource()));
            auto issued_user = user->allocation_stats().issued_objects;
            auto issued_global = global.allocation_stats().issued_objects;
            upstream.fail_attempt = upstream.allocation_attempts + offset;
            try {
                auto shared = user->intern_global_string(user->get(input));
                check(user->get(shared) == user->get(input),
                      "successful interning preserves the complete input");
                completed = true;
            } catch (const std::bad_alloc&) {
                ++failures;
            }
            upstream.fail_attempt.reset();
            check(user->get(local) == 17 && user->get(retained) == 23,
                  "string allocation failure preserves published object references");
            auto shared = user->intern_global_string(user->get(input));
            auto again = user->intern_global_string(user->get(input));
            check(user->get(shared).data() == user->get(again).data() &&
                      user->allocation_stats().issued_objects == issued_user &&
                      global.allocation_stats().issued_objects == issued_global,
                  "retry deduplicates without consuming payload object identities");
            user.reset();
            check(global.live_user_bytes() == 0 &&
                      upstream.allocations.size() == global.allocation_stats().live_blocks,
                  "failed interning retains no User storage after actual destruction");
            auto next = global.make_user_context(128);
            auto rebound = next->rebind_global(shared);
            check(next->get(rebound).size() == 16384 && next->get(rebound).front() == 's' &&
                      next->get(rebound).back() == 's',
                  "Global interning deep-copies User input before reclamation");
        }
        check(upstream.allocations.empty(),
              "Global close reclaims every failed and successful pool allocation");
    }
    check(failures >= 3 && completed,
          "fault injection covers pool initialization, characters and index growth");
}

static_assert(sizeof(Ref<String>) == 8);
template <class T>
concept ExposesGlobal = requires(T& user) { user.global(); };
static_assert(!ExposesGlobal<UserContext>);

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "early-global-close") {
        std::set_terminate([] {
            std::_Exit(0);
        });
        GlobalContext global;
        auto user = global.make_user_context();
        detail::resource_arena(global.memory_resource()).close();
        return 1;
    }
    try {
        auto run = [](const char* name, auto action) {
            try {
                action();
            } catch (...) {
                std::cerr << name << ": ";
                throw;
            }
        };
        run("identities and expiry", identities_and_expiry);
        run("Global delegation and destruction", global_delegation_and_destruction);
        run("registered resource borrows", registered_resource_borrows);
        run("quotas and failed construction", quotas_and_failed_construction);
        run("million-object bitmap", million_object_bitmap);
        run("object publication allocation failures", object_publication_allocation_failures);
        run("independent tables and page boundary failure",
            independent_tables_and_page_boundary_failure);
        run("budget exhaustion and shutdown", budget_exhaustion_and_shutdown);
        run("Closing rejects business access", closing_rejects_business_access);
        run("aggregate budget and invalid limits", aggregate_budget_and_invalid_limits);
        run("Context initialization failures", context_initialization_failures);
        run("string storage and payload views", string_storage_and_payload_views);
        run("Global string allocation failures", global_string_allocation_failures);
        std::cout << "Checked Context memory contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
