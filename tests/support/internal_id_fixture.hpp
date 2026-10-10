#pragma once
#include <teaql/runtime/context.hpp>
using namespace teaql;

struct Model : EntityDefaults<Model> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    Memory resource;
    bool fresh = false;
    std::uint64_t id = 17;

    Model(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, const Model& other)
        : resource(a.resource()), fresh(other.fresh), id(other.id) {}

    Model(Model&&) = default;

    Memory memory_resource() const {
        return resource;
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<Model, EntityError> from_compact_row(Memory r, CompactRow) {
        return Result<Model, EntityError>::ok(r, Model(std::allocator_arg, Allocator(r)));
    }

    MutationValues into_values() && {
        return MutationValues(resource);
    }

    void mark_as_new() {
        fresh = true;
    }

    bool is_new() const {
        return fresh;
    }
};
