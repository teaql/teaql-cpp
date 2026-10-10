#include "../../tests/support/entity_registration_fixture.hpp"
#include <teaql/runtime/context.hpp>
#include <iostream>

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Descriptor-only C++ models retain the Rust trait's implicit default.
struct BareEntity {
    static constexpr std::string_view ENTITY_NAME = "Bare";

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }
};

static void dispatch_and_retention() {
    GlobalContext global;
    Store* retained = nullptr;
    {
        auto user = global.make_user_context();
        auto& store = user->make<Store>(64);
        register_entities<DefaultEntity, CustomEntity, SilentEntity, DefaultEntity>(store);
        check(store.names.size() == 4 && store.names[0] == "Default" &&
                  store.names[1] == "OverrideA" && store.names[2] == "OverrideB" &&
                  store.names[3] == "Default" && store.attempted == 4,
              "registration invokes overridden hooks and preserves duplicate order");
        DefaultEntity::register_into(store);
        check(store.names.back() == "Default" && store.attempted == 5,
              "inherited default registers directly without recursively redispatching");
        register_entities<BareEntity>(store);
        check(store.names.back() == "Bare", "descriptor-only models use the implicit default");
        register_entities<>(store);
        check(store.attempted == 6, "empty C++ pack extension is a no-op");
        retained = &global.make<Store>(store);
    }
    check(retained->names.size() == 6 && retained->names[1] == "OverrideA" &&
              retained->names.back() == "Bare",
          "explicit destination copies survive actual source User destruction");
    for (const auto& name : retained->names) {
        check(name.get_allocator().resource() == global.memory_resource(),
              "registered names propagate the destination allocator");
    }
    auto user = global.make_user_context();
    auto& limited = user->make<Store>(2);
    bool rejected = false;
    try {
        register_entities<DefaultEntity, CustomEntity, SilentEntity, DefaultEntity>(limited);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected && limited.attempted == 3 && limited.names.size() == 2 &&
              limited.names[0] == "Default" && limited.names[1] == "OverrideA",
          "store failure preserves earlier registrations and stops subsequent hooks");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        dispatch_and_retention();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
