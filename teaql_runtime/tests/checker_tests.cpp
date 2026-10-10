#include "../../tests/support/checker_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <cstdlib>
#include <iostream>
#include <thread>
using namespace teaql;
using namespace checker_fixture;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class E, class F>
static void rejected(F&& action, const char* message) {
    bool caught = false;
    try {
        action();
    } catch (const E&) {
        caught = true;
    }
    check(caught, message);
}

static void set(EntityValues& values, std::string_view name, Value value) {
    values.fields.insert_or_assign(String(name, values.memory_resource()), std::move(value));
}

static void checked_registration_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto checker = user->make_global<Plain>("fill");
    auto registry = user->make_global<InMemoryCheckerRegistry>();
    user->get(registry).with_checker(*user, checker);
    user->with_checker_registry(registry);
    check(user->has_checker("Item") && !user->has_checker("Missing"),
          "checked lookup preserves missing entity");
    auto& data = user->make<EntityValues>();
    auto result = user->check_and_fix_values("Item", data);
    check(bool(result) &&
              data.fields.at(String("name", user->memory_resource())).try_text() == "fixed",
          "User-only global registration executes the selected checker");
    auto& local = user->make<InMemoryCheckerRegistry>();
    {
        Plain temporary(std::allocator_arg, Allocator(user->memory_resource()), "none");
        rejected<std::invalid_argument>(
            [&] {
                local.register_checker(*user, temporary);
            },
            "temporary native checker cannot be registered");
    }
    auto scoped = user->make_ref<Plain>("none");
    rejected<std::invalid_argument>(
        [&] {
            user->get(registry).register_checker(*user, scoped);
        },
        "Global registry rejects User-owned checker retention");
    auto next = global.make_user_context();
    rejected<ReferenceError>(
        [&] {
            next->get(next->rebind_global(registry)).register_checker(*next, checker);
        },
        "another User must explicitly rebind a Global checker ID");
    // Stack carriers finish before their arena owner is destroyed.
    result = RuntimeHookResult::ok(user->memory_resource(), {});
    user->close();
    next->with_checker_registry(next->rebind_global(registry));
    auto& next_data = next->make<EntityValues>();
    check(bool(next->check_and_fix_values("Item", next_data)),
          "Global registration survives old User access expiry");
    auto rebound = next->rebind_global(checker);
    next->destroy(rebound);
    rejected<ReferenceError>(
        [&] {
            (void)next->has_checker("Item");
        },
        "lookup rejects early-destroyed checker");
    next->destroy(next->rebind_global(registry));
    rejected<ReferenceError>(
        [&] {
            (void)next->has_checker("Missing");
        },
        "registry is checked before virtual dispatch");
}

static void typed_copies_and_sparse_values() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto fix = user->make_global<Fix>("fill");
    auto adapter = user->make_ref<TypedEntityChecker<Item, Fix>>(*user, fix);
    auto retained = user->make_global<TypedEntityChecker<Item, Fix>>(user->get(adapter));
    auto module = user->make_global<RuntimeModule>();
    user->get(module)
        .descriptor(Item::entity_descriptor(user->get(module).memory_resource()))
        .checker(*user, retained);
    user->get(module).apply_to(*user);
    auto& values = user->make<EntityValues>();
    set(values, "id", Value(user->memory_resource(), std::uint64_t{7}));
    set(values, "score", Value(user->memory_resource(), std::int64_t{3}));
    set(values,
        "unmapped",
        Value(user->memory_resource(),
              "Long original value retained outside the typed model projection"));
    check(bool(user->check_and_fix_values("Item", values)) && values.fields.size() == 4,
          "fix merges changed name but does not add omitted version or remove unmapped data");
    auto copy = user->make_global<EntityValues>(values);
    Ref<RuntimeError> error;
    {
        auto& missing = user->make<EntityValues>();
        auto outcome = user->check_and_fix_values("Item", missing);
        check(!outcome && outcome.error().check_results().size() == 2,
              "typed required checks and metadata ID check preserve source ordering");
        error = user->make_global<RuntimeError>(outcome.error());
    }
    user.reset();
    auto next = global.make_user_context();
    next->get(next->rebind_global(module)).apply_to(*next);
    auto& kept = next->get(next->rebind_global(copy));
    check(kept.fields.size() == 4 &&
              kept.fields.find("unmapped")->second.try_text()->starts_with("Long original"),
          "nested mutation values are explicitly copied into Global");
    auto& diagnostic = next->get(next->rebind_global(error));
    check(diagnostic.check_results().size() == 2 && diagnostic.check_results()[0].message &&
              diagnostic.check_results()[0].location.model_path(next->memory_resource()) == "score",
          "translated errors survive actual source User destruction");
    auto& second = next->make<EntityValues>();
    set(second, "id", Value(next->memory_resource(), std::uint64_t{9}));
    set(second, "score", Value(next->memory_resource(), std::int64_t{0}));
    check(bool(next->check_and_fix_values("Item", second)),
          "copied typed adapter executes through copied module");
    check(
        next->get(next->rebind_global(fix)).calls.size() == 3,
        "registry/module copies retain the selected checker state without retaining User storage");
    next->destroy(next->rebind_global(fix));
    rejected<ReferenceError>(
        [&] {
            (void)next->check_and_fix_values("Item", second);
        },
        "typed callback is checked before materialization takes mutation values");
    check(second.fields.size() == 3, "dead typed callback leaves caller mutation boundary intact");
}

struct Throwing final : Checker {
    std::string_view entity() const override {
        return "Item";
    }

    void check_and_fix(const UserContext&,
                       EntityValues&,
                       const ObjectLocation& path,
                       CheckResults& results) const override {
        results.emplace_back(
            CheckResult::required(results.get_allocator().resource(),
                                  detail::clone_in(results.get_allocator().resource(), path)));
        throw std::runtime_error("callback failure");
    }
};

struct Closing final : Checker {
    UserContext& user;

    explicit Closing(UserContext& user) : user(user) {}

    std::string_view entity() const override {
        return "Item";
    }

    void check_and_fix(const UserContext&,
                       EntityValues&,
                       const ObjectLocation&,
                       CheckResults&) const override {
        user.close();
    }
};

struct ClosingName final : Checker {
    UserContext& user;
    bool close;

    explicit ClosingName(UserContext& user, bool close = true) : user(user), close(close) {}

    std::string_view entity() const override {
        if (close) {
            user.close();
        }
        throw std::runtime_error("registration returned after closing its destination");
    }

    void check_and_fix(const UserContext&,
                       EntityValues&,
                       const ObjectLocation&,
                       CheckResults&) const override {}
};

struct ClosingTyped final : TypedChecker<Item> {
    bool close;

    explicit ClosingTyped(bool close = true) : close(close) {}

    void check_and_fix_typed(const UserContext& context,
                             Item&,
                             CheckObjectStatus,
                             const ObjectLocation&,
                             CheckResults&) const override {
        if (close) {
            const_cast<UserContext&>(context).close();
        }
        throw std::runtime_error("typed callback returned after closing its context");
    }
};

static void direct_callback_exception_cleanup() {
    GlobalContext global;
    for (bool typed : {false, true}) {
        auto user = global.make_user_context();
        if (typed) {
            auto& checker = global.make_resource<ClosingTyped>(false);
            auto& adapter = global.make<TypedEntityChecker<Item, ClosingTyped>>(global, checker);
            auto& values = global.make<EntityValues>();
            auto& location = global.make<ObjectLocation>();
            auto& results = global.make<CheckResults>();
            rejected<std::runtime_error>(
                [&] {
                    adapter.check_and_fix(*user, values, location, results);
                },
                "direct typed callback propagates its original exception");
        } else {
            auto& checker = global.make_resource<ClosingName>(*user, false);
            auto& registry = user->make<InMemoryCheckerRegistry>();
            rejected<std::runtime_error>(
                [&] {
                    registry.register_checker(global, checker);
                },
                "checker name callback propagates its original exception");
            check(!registry.checker(user->memory_resource(), "Item"),
                  "failed registration leaves the checker map unchanged");
        }
        user.reset();
        check(global.live_user_bytes() == 0,
              "direct callback exceptions release guards before actual User reclamation");
    }
}

static void cleanup_and_metadata_expiry() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto registry = user->make_ref<InMemoryCheckerRegistry>();
    auto callback = user->make_resource_ref<Throwing>();
    user->get(registry).register_checker(*user, callback);
    user->set_checker_registry(registry);
    auto& values = user->make<EntityValues>();
    rejected<std::runtime_error>(
        [&] {
            (void)user->check_and_fix_values("Item", values);
        },
        "callback exception unwinds synchronous registration");
    auto empty = user->make_ref<InMemoryCheckerRegistry>();
    user->set_checker_registry(empty);
    auto metadata = user->make_ref<InMemoryMetadataStore>();
    user->get(metadata).register_entity(Item::entity_descriptor(user->memory_resource()));
    user->set_metadata(*user, user->get(metadata));
    user->destroy(metadata);
    rejected<ReferenceError>(
        [&] {
            (void)user->check_and_fix_values("Item", values);
        },
        "required-property checks reject early-destroyed metadata");
    user->close();
    check(user->allocation_stats().live_bytes == 0,
          "exception paths release synchronous close guard and all arena blocks");
    rejected<ReferenceError>(
        [&] {
            (void)user->has_checker("Item");
        },
        "closed Context rejects checker lookup");
}

int main(int argc, char** argv) {
    if (argc == 2 && (std::string_view(argv[1]) == "registration-close" ||
                      std::string_view(argv[1]) == "typed-direct-close")) {
        std::set_terminate([] {
            std::_Exit(0);
        });
        try {
            GlobalContext global;
            auto user = global.make_user_context();
            if (std::string_view(argv[1]) == "registration-close") {
                auto& checker = global.make_resource<ClosingName>(*user);
                auto& registry = user->make<InMemoryCheckerRegistry>();
                registry.register_checker(global, checker);
            } else {
                auto& checker = global.make_resource<ClosingTyped>();
                auto& adapter =
                    global.make<TypedEntityChecker<Item, ClosingTyped>>(global, checker);
                auto& values = global.make<EntityValues>();
                auto& location = global.make<ObjectLocation>();
                auto& results = global.make<CheckResults>();
                adapter.check_and_fix(*user, values, location, results);
            }
        } catch (...) {
            std::_Exit(2);
        }
        return 1;
    }
    if (argc == 2 && std::string_view(argv[1]) == "reentrant-close") {
        std::set_terminate([] {
            std::_Exit(0);
        });
        try {
            GlobalContext global;
            auto user = global.make_user_context();
            auto registry = user->make_ref<InMemoryCheckerRegistry>();
            auto callback = user->make_resource_ref<Closing>(*user);
            user->get(registry).register_checker(*user, callback);
            user->set_checker_registry(registry);
            auto& values = user->make<EntityValues>();
            (void)user->check_and_fix_values("Item", values);
        } catch (...) {
            std::_Exit(2);
        }
        return 1;
    }
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        checked_registration_and_retention();
        typed_copies_and_sparse_values();
        cleanup_and_metadata_expiry();
        direct_callback_exception_cleanup();
        std::pmr::set_default_resource(previous);
        std::cout << "Checker/fix Context ownership contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
