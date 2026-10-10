#pragma once
#include <teaql/memory/types.hpp>
#include <teaql/core/entity.hpp>
#include <stdexcept>
#include <utility>

namespace list_fixture {
// Observe ownership cleanup, excluding harmless destruction of moved-from shells.
struct DropItem {
    using allocator_type = teaql::Allocator;
    teaql::String payload;
    teaql::Vector<int>* events;
    int value;
    bool owns = true;

    DropItem(std::allocator_arg_t, teaql::Allocator owner, int value, teaql::Vector<int>& events)
        : payload(4096, 'x', owner.resource()), events(&events), value(value) {}

    DropItem(std::allocator_arg_t, teaql::Allocator owner, DropItem&& other)
        : payload(owner.resource()), events(other.events), value(other.value), owns(false) {
        if (owner.resource() != other.payload.get_allocator().resource()) {
            throw std::invalid_argument("DropItem requires its original allocation domain");
        }
        payload = std::move(other.payload);
        owns = std::exchange(other.owns, false);
    }

    DropItem(DropItem&& other) noexcept
        : payload(std::move(other.payload)), events(other.events), value(other.value),
          owns(std::exchange(other.owns, false)) {}

    DropItem(const DropItem&) = delete;
    DropItem& operator=(DropItem&&) = delete;

    ~DropItem() {
        if (owns) {
            events->push_back(-value);
        }
    }
};

struct DropEntity : DropItem, teaql::EntityDefaults<DropEntity> {
    using DropItem::DropItem;
    static constexpr std::string_view ENTITY_NAME = "DropEntity";
    int fail_at = 0;

    DropEntity(DropEntity&&) noexcept = default;

    DropEntity(std::allocator_arg_t, teaql::Allocator owner, DropEntity&& other)
        : DropItem(std::allocator_arg, owner, std::move(other)), fail_at(other.fail_at) {}

    teaql::Memory memory_resource() const {
        return payload.get_allocator().resource();
    }

    static teaql::EntityDescriptor entity_descriptor(teaql::Memory owner) {
        return teaql::EntityDescriptor(std::allocator_arg, teaql::Allocator(owner), ENTITY_NAME);
    }

    static teaql::Result<DropEntity, teaql::EntityError> from_compact_row(teaql::Memory,
                                                                          teaql::CompactRow) {
        throw std::logic_error("DropEntity fixture does not decode rows");
    }

    teaql::MutationValues into_values() && {
        events->push_back(value);
        if (value == fail_at) {
            throw std::runtime_error("entity conversion failed");
        }
        teaql::MutationValues result(memory_resource());
        result.fields.emplace("id", std::uint64_t(value));
        return result;
    }
};

static_assert(teaql::Entity<DropEntity>);
}
