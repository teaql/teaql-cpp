#pragma once
#include <teaql/core/value.hpp>
#include <teaql/memory/scoped.hpp>
#include <compare>

namespace teaql {
class DebugWriter;

class EntityKey {
public:
    using allocator_type = Allocator;
    String entity;
    Value id;
    EntityKey(std::allocator_arg_t, Allocator, std::string_view, const Value&);
    EntityKey(std::allocator_arg_t, Allocator, const EntityKey&);

    EntityKey(std::allocator_arg_t, Allocator a, EntityKey&& other)
        : EntityKey(detail::move_to(a.resource(), std::move(other))) {}

    EntityKey(const EntityKey&) = delete;
    EntityKey(EntityKey&&) noexcept = default;

    EntityKey& operator=(EntityKey&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entity.get_allocator().resource();
    }

    bool operator==(const EntityKey& other) const {
        return (*this <=> other) == 0;
    }

    std::strong_ordering operator<=>(const EntityKey&) const;
    String debug(Memory) const;

private:
    friend void write_debug(DebugWriter&, const EntityKey&);
    enum class Kind { Null, Bool, I64, U64, F64, Decimal, Text, Date, Timestamp, Other };
    Kind kind_;
    Value identity_;
};
}
