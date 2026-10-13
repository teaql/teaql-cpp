#pragma once
#include <teaql/runtime/entity_key.hpp>
#include <teaql/core/dynamic_fields.hpp>

namespace teaql::detail {
struct DynamicReadbacks {
    using allocator_type = Allocator;
    std::pmr::map<EntityKey, DynamicFieldValues> rows;

    DynamicReadbacks(std::allocator_arg_t, Allocator a) : rows(a.resource()) {}

    DynamicReadbacks(std::allocator_arg_t, Allocator a, const DynamicReadbacks& other)
        : rows(other.rows, a.resource()) {}
};

struct LedgerExecutionOutcome {
    using allocator_type = Allocator;
    std::pmr::map<EntityKey, Value> generated_ids;
    ScopedBox<DynamicReadbacks> dynamic_readbacks;

    LedgerExecutionOutcome(std::allocator_arg_t, Allocator a) : generated_ids(a.resource()) {}

    LedgerExecutionOutcome(std::allocator_arg_t, Allocator a, const LedgerExecutionOutcome& other)
        : generated_ids(other.generated_ids, a.resource()) {
        if (other.dynamic_readbacks) {
            dynamic_readbacks = make_box<DynamicReadbacks>(a.resource(), *other.dynamic_readbacks);
        }
    }

    LedgerExecutionOutcome(std::allocator_arg_t, Allocator a, LedgerExecutionOutcome&& other)
        : LedgerExecutionOutcome(move_to(a.resource(), std::move(other))) {}

    LedgerExecutionOutcome(LedgerExecutionOutcome&&) noexcept = default;

    LedgerExecutionOutcome& operator=(LedgerExecutionOutcome&& other) {
        return assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return generated_ids.get_allocator().resource();
    }

    auto find(const EntityKey& key) const {
        return generated_ids.find(key);
    }

    auto begin() const {
        return generated_ids.begin();
    }

    auto end() const {
        return generated_ids.end();
    }

    bool empty() const {
        return generated_ids.empty();
    }

    std::size_t size() const {
        return generated_ids.size();
    }

    const Value& at(const EntityKey& key) const {
        return generated_ids.at(key);
    }
};
}
