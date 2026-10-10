#pragma once
#include <teaql/core/result.hpp>
#include <exception>

namespace teaql {
struct ContextEntityRef {
    using allocator_type = Allocator;
    String entity_type;
    std::uint64_t id;

    ContextEntityRef(std::allocator_arg_t, Allocator a, std::string_view entity, std::uint64_t id)
        : entity_type(entity, a.resource()), id(id) {}

    ContextEntityRef(std::allocator_arg_t, Allocator a, const ContextEntityRef& other)
        : ContextEntityRef(std::allocator_arg, a, other.entity_type, other.id) {}

    ContextEntityRef(std::allocator_arg_t, Allocator a, ContextEntityRef&& other)
        : ContextEntityRef(detail::move_to(a.resource(), std::move(other))) {}

    ContextEntityRef(const ContextEntityRef&) = delete;
    ContextEntityRef(ContextEntityRef&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return entity_type.get_allocator().resource();
    }

    bool operator==(const ContextEntityRef&) const = default;
};

struct ContextRootError {
    using allocator_type = Allocator;
    String expected_entity_type;
    std::optional<ContextEntityRef> actual_root;

    ContextRootError(std::allocator_arg_t,
                     Allocator a,
                     std::string_view expected,
                     const ContextEntityRef* actual)
        : expected_entity_type(expected, a.resource()) {
        if (actual) {
            actual_root.emplace(std::allocator_arg, a, *actual);
        }
    }

    ContextRootError(std::allocator_arg_t, Allocator a, const ContextRootError& other)
        : ContextRootError(std::allocator_arg,
                           a,
                           other.expected_entity_type,
                           other.actual_root ? &*other.actual_root : nullptr) {}

    ContextRootError(std::allocator_arg_t, Allocator a, ContextRootError&& other)
        : ContextRootError(detail::move_to(a.resource(), std::move(other))) {}

    ContextRootError(const ContextRootError&) = delete;
    ContextRootError(ContextRootError&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return expected_entity_type.get_allocator().resource();
    }

    String to_string(Memory) const;

    const std::exception* source() const noexcept {
        return nullptr;
    }

    bool operator==(const ContextRootError&) const = default;
};
}
