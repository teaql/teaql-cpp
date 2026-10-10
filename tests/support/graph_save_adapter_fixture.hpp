#pragma once
#include "runtime_mutation_fixture.hpp"

namespace adapter_fixture {
using namespace teaql;

struct Model : EntityDefaults<Model> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    MutationValues values;
    std::optional<FieldSet> dirty;
    std::optional<EntitySnapshot> original;
    std::optional<String> comment;
    bool removed = false;
    mutation_fixture::State* state;

    Model(std::allocator_arg_t, Allocator a, mutation_fixture::State& state)
        : values(a.resource()), state(&state) {}

    Model(std::allocator_arg_t, Allocator a, const Model& source)
        : values(a.resource(), source.values), dirty(detail::clone_in(a.resource(), source.dirty)),
          original(detail::clone_in(a.resource(), source.original)),
          comment(detail::clone_in(a.resource(), source.comment)), removed(source.removed),
          state(source.state) {}

    Model(const Model&) = delete;
    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<Model, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("save adapter attempted typed hydration");
    }

    std::optional<FieldSet> dirty_fields() const {
        state->note("dirty");
        return detail::clone_in(memory_resource(), dirty);
    }

    std::optional<EntitySnapshot> original_values() const {
        state->note("original");
        return detail::clone_in(memory_resource(), original);
    }

    bool is_marked_as_delete() const {
        state->note("deleted");
        return removed;
    }

    std::optional<String> get_comment() const {
        state->note("comment");
        return detail::clone_in(memory_resource(), comment);
    }

    bool is_new() const {
        throw std::logic_error("save adapter called is_new");
    }

    bool is_field_loaded(std::string_view) const {
        throw std::logic_error("save adapter called is_field_loaded");
    }

    MutationValues into_values() && {
        state->note("values");
        return std::move(values);
    }
};

static_assert(Entity<Model>);
}
