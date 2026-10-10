#pragma once
#include <teaql/core/value.hpp>
#include <teaql/memory/scoped.hpp>
#include <span>
#include <algorithm>
#include <stdexcept>
#include <ranges>

namespace teaql {
// Immutable, scope-owned column layout. Only rows in the same allocation domain
// borrow it; construction in another domain copies the layout.
class ColumnLayout {
public:
    using allocator_type = Allocator;

    ColumnLayout(std::allocator_arg_t, Allocator a, std::initializer_list<std::string_view> names)
        : names_(a.resource()) {
        for (auto name : names) {
            names_.emplace_back(name);
        }
    }

    ColumnLayout(std::allocator_arg_t, Allocator a, const ColumnLayout& other)
        : names_(a.resource()) {
        for (const auto& name : other.names_) {
            names_.emplace_back(name);
        }
    }

    ColumnLayout(std::allocator_arg_t, Allocator a, Vector<String> names)
        : names_(std::move(names), a.resource()) {}

    ColumnLayout(const ColumnLayout&) = delete;
    ColumnLayout& operator=(const ColumnLayout&) = delete;

    std::span<const String> names() const noexcept {
        return names_;
    }

    Memory memory_resource() const noexcept {
        return names_.get_allocator().resource();
    }

private:
    Vector<String> names_;
};

class CompactRow {
public:
    using allocator_type = Allocator;

    CompactRow(std::allocator_arg_t, Allocator);
    CompactRow(std::allocator_arg_t, Allocator, const Record&);
    CompactRow(std::allocator_arg_t, Allocator, Record&&);

    CompactRow(std::allocator_arg_t,
               Allocator a,
               const ColumnLayout& columns,
               std::span<const Value> values);

    CompactRow(std::allocator_arg_t,
               Allocator a,
               const ColumnLayout& columns,
               Vector<Value>&& values);

    CompactRow(std::allocator_arg_t, Allocator, const CompactRow&);

    CompactRow(const CompactRow&) = delete;

    CompactRow(CompactRow&& other) noexcept
        : shared_(std::exchange(other.shared_, nullptr)), values_(std::move(other.values_)) {
        other.values_.clear();
    }

    CompactRow(std::allocator_arg_t, Allocator a, CompactRow&& other)
        : CompactRow(detail::move_to(a.resource(), std::move(other))) {}

    CompactRow& operator=(CompactRow&& other);

    Memory memory_resource() const noexcept {
        return values_.get_allocator().resource();
    }

    const ColumnLayout* shared_columns() const;

    std::span<const String> keys() const noexcept {
        return shared_ ? shared_->names() : std::span<const String>{};
    }

    std::span<const Value> values() const noexcept {
        return values_;
    }

    auto iter() const {
        return std::views::iota(std::size_t{0}, values_.size()) |
               std::views::transform([this](std::size_t index) {
                   return std::pair<const String&, const Value&>(keys()[index], values_[index]);
               });
    }

    const Value* get(std::string_view name) const noexcept;
    Value* get_mut(std::string_view name) noexcept;

    bool contains_key(std::string_view name) const noexcept {
        return get(name) != nullptr;
    }

    std::size_t len() const noexcept {
        return values_.size();
    }

    bool is_empty() const noexcept {
        return values_.empty();
    }

    std::optional<Value> insert(std::string_view name, const Value& value);
    std::optional<Value> remove(std::string_view name);
    void extend(const CompactRow& other);
    Record copy_map(Memory target) const;

    Record into_map(Memory target) const& {
        return copy_map(target);
    }

    Record into_map(Memory target) &&;

    bool operator==(const CompactRow& other) const {
        return std::ranges::equal(keys(), other.keys()) && values_ == other.values_;
    }

private:
    mutable const ColumnLayout* shared_ = nullptr;
    Vector<Value> values_;
};

Json compact_row_to_json_value(Memory destination, const CompactRow&);
}
