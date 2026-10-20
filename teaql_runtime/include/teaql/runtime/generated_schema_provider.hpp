#pragma once
#include <teaql/data_service/executor.hpp>
#include <teaql/runtime/resources.hpp>
#include <optional>

namespace teaql {
// The generated name dispatch is stateless. Returned descriptors are fresh
// managed objects in the provider's explicit allocation domain, independently
// of mutable Context metadata and its runtime schema-initialization provider.
class GeneratedLocalSchemaProvider : public SchemaProvider {
public:
    using allocator_type = Allocator;
    using Lookup = std::optional<EntityDescriptor> (*)(Memory, std::string_view);

    GeneratedLocalSchemaProvider(std::allocator_arg_t, Allocator allocator, Lookup lookup)
        : resource_(allocator.resource()), lookup_(lookup) {
        if (!lookup_) {
            throw std::invalid_argument("Generated schema lookup is required");
        }
    }

    GeneratedLocalSchemaProvider(std::allocator_arg_t,
                                 Allocator allocator,
                                 const GeneratedLocalSchemaProvider& other)
        : GeneratedLocalSchemaProvider(std::allocator_arg, allocator, other.lookup_) {}

    GeneratedLocalSchemaProvider(const GeneratedLocalSchemaProvider&) = delete;

    Memory memory_resource() const noexcept {
        return resource_;
    }

    const EntityDescriptor* get_entity(std::string_view name) const override {
        auto descriptor = lookup_(resource_, name);
        if (!descriptor) {
            return nullptr;
        }
        return &detail::resource_arena(resource_).make<EntityDescriptor>(std::move(*descriptor));
    }

private:
    Memory resource_;
    Lookup lookup_;
};
}
