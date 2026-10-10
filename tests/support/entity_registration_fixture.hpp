#pragma once
#include <teaql/core/entity.hpp>

using namespace teaql;

struct DefaultEntity : EntityDefaults<DefaultEntity> {
    static constexpr std::string_view ENTITY_NAME = "Default";

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }
};

struct CustomEntity : EntityDefaults<CustomEntity> {
    static constexpr std::string_view ENTITY_NAME = "Custom";

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static void register_into(EntityDescriptorStore& store) {
        store.register_descriptor(
            EntityDescriptor(std::allocator_arg, Allocator(store.memory_resource()), "OverrideA"));
        store.register_descriptor(
            EntityDescriptor(std::allocator_arg, Allocator(store.memory_resource()), "OverrideB"));
    }
};

struct SilentEntity : EntityDefaults<SilentEntity> {
    static constexpr std::string_view ENTITY_NAME = "Silent";

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static void register_into(EntityDescriptorStore&) {}
};

struct Store final : EntityDescriptorStore {
    using allocator_type = Allocator;
    Vector<String> names;
    std::uint64_t attempted = 0;
    std::uint64_t limit;

    Store(Memory r, std::uint64_t limit) : names(r), limit(limit) {}

    Store(std::allocator_arg_t, Allocator a, std::uint64_t limit) : Store(a.resource(), limit) {}

    Store(std::allocator_arg_t, Allocator a, const Store& other)
        : names(a.resource()), attempted(other.attempted), limit(other.limit) {
        for (const auto& name : other.names) {
            names.emplace_back(name);
        }
    }

    Memory memory_resource() const override {
        return names.get_allocator().resource();
    }

    void register_descriptor(EntityDescriptor descriptor) override {
        ++attempted;
        if (names.size() >= limit) {
            throw std::runtime_error("fixture store rejects registration");
        }
        names.emplace_back(descriptor.name);
    }
};
