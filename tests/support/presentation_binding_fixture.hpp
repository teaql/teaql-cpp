#pragma once
#include <teaql/core/web.hpp>

namespace teaql::test {
// Overrides expose whether a binding honors the public BaseEntity customization.
struct BindingEntity : BaseEntityDefaults<BindingEntity> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Binding";
    BaseEntityData data;
    Vector<String> calls;
    unsigned mode;

    BindingEntity(std::allocator_arg_t, Allocator a, unsigned mode)
        : data(std::allocator_arg, a), calls(a.resource()), mode(mode) {}

    static EntityDescriptor entity_descriptor(Memory owner) {
        return EntityDescriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
    }

    static Result<BindingEntity, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("Unused binding materializer");
    }

    MutationValues into_values() && {
        return MutationValues(memory_resource(), data.to_values_map(memory_resource()));
    }

    const BaseEntityData& base() const {
        return data;
    }

    BaseEntityData& base_mut() {
        return data;
    }

    Memory memory_resource() const {
        return data.memory_resource();
    }

    template <class T>
    std::optional<Value> put_dynamic(std::string_view key, T&& value) {
        calls.emplace_back(key);
        if (mode == 2) {
            return {};
        }
        return data.put_dynamic(mode == 1 ? "redirectedStyle" : key, std::forward<T>(value));
    }
};

static_assert(BaseEntity<BindingEntity>);
}
