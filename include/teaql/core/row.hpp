#pragma once
#include <teaql/core/value.hpp>
#include <span>
#include <algorithm>
#include <stdexcept>

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

    CompactRow(std::allocator_arg_t, Allocator a) : columns_(a.resource()), values_(a.resource()) {}

    CompactRow(std::allocator_arg_t, Allocator a, const Record& record)
        : CompactRow(std::allocator_arg, a) {
        for (const auto& [key, value] : record) {
            columns_.emplace_back(key);
            values_.emplace_back(value);
        }
    }

    CompactRow(std::allocator_arg_t,
               Allocator a,
               const ColumnLayout& columns,
               std::span<const Value> values)
        : CompactRow(std::allocator_arg, a) {
        if (columns.names().size() != values.size()) {
            throw std::invalid_argument("CompactRow column/value count mismatch");
        }
        if (columns.memory_resource() == a.resource()) {
            shared_ = &columns;
        } else {
            for (const auto& name : columns.names()) {
                columns_.emplace_back(name);
            }
        }
        for (const auto& value : values) {
            values_.emplace_back(value);
        }
    }

    CompactRow(std::allocator_arg_t,
               Allocator a,
               const ColumnLayout& columns,
               Vector<Value>&& values)
        : CompactRow(std::allocator_arg, a) {
        if (columns.names().size() != values.size()) {
            throw std::invalid_argument("CompactRow column/value count mismatch");
        }
        if (columns.memory_resource() == a.resource()) {
            shared_ = &columns;
        } else {
            for (const auto& name : columns.names()) {
                columns_.emplace_back(name);
            }
        }
        // Same-domain decoding transfers the values without duplicating nested
        // strings/JSON in a monotonic arena. Other domains still deep-copy.
        values_ = Vector<Value>(std::move(values), a.resource());
    }

    CompactRow(std::allocator_arg_t, Allocator a, const CompactRow& other)
        : CompactRow(std::allocator_arg, a) {
        for (const auto& key : other.keys()) {
            columns_.emplace_back(key);
        }
        for (const auto& value : other.values_) {
            values_.emplace_back(value);
        }
    }

    CompactRow(const CompactRow&) = delete;

    CompactRow(CompactRow&& other) noexcept
        : shared_(std::exchange(other.shared_, nullptr)), columns_(std::move(other.columns_)),
          values_(std::move(other.values_)) {
        other.columns_.clear();
        other.values_.clear();
    }

    CompactRow(std::allocator_arg_t, Allocator a, CompactRow&& other)
        : CompactRow(std::allocator_arg, a) {
        *this = std::move(other);
    }

    CompactRow& operator=(CompactRow&& other);

    Memory memory_resource() const noexcept {
        return columns_.get_allocator().resource();
    }

    const ColumnLayout* shared_columns() const noexcept {
        return shared_;
    }

    std::span<const String> keys() const noexcept {
        return shared_ ? shared_->names() : std::span<const String>(columns_);
    }

    std::span<const Value> values() const noexcept {
        return values_;
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
    Record into_map(Memory target) const;

    bool operator==(const CompactRow& other) const {
        return std::ranges::equal(keys(), other.keys()) && values_ == other.values_;
    }

private:
    const ColumnLayout* shared_ = nullptr;
    Vector<String> columns_;
    Vector<Value> values_;
    void detach_columns();
};

Json compact_row_to_json_value(Memory destination, const CompactRow&);
}
