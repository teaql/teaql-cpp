#pragma once
#include <teaql/core/result.hpp>
#include <exception>

namespace teaql {
class ContextError {
public:
    using allocator_type = Allocator;
    enum class Kind { MissingResource, MissingTypedResource, MissingEntityDataService };

    ContextError(std::allocator_arg_t, Allocator a, Kind kind, std::string_view name)
        : kind_(kind), name_(name, a.resource()) {}

    ContextError(std::allocator_arg_t, Allocator a, const ContextError& other)
        : ContextError(std::allocator_arg, a, other.kind_, other.name_) {}

    ContextError(std::allocator_arg_t, Allocator a, ContextError&& other)
        : ContextError(detail::move_to(a.resource(), std::move(other))) {}

    ContextError(const ContextError&) = delete;
    ContextError(ContextError&&) noexcept = default;

    ContextError& operator=(ContextError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Kind kind() const noexcept {
        return kind_;
    }

    std::string_view name() const noexcept {
        return name_;
    }

    Memory memory_resource() const noexcept {
        return name_.get_allocator().resource();
    }

    String to_string(Memory r) const {
        String text(kind_ == Kind::MissingResource ? "missing named resource: "
                    : kind_ == Kind::MissingTypedResource
                        ? "missing typed resource: "
                        : "missing entity data service for entity: ",
                    r);
        text += name_;
        return text;
    }

    const std::exception* source() const noexcept {
        return nullptr;
    }

private:
    Kind kind_;
    String name_;
};
}
