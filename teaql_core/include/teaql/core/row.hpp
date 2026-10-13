#pragma once
#include <teaql/core/value.hpp>
#include <teaql/core/load_state.hpp>
#include <teaql/memory/scoped.hpp>
#include <span>
#include <algorithm>
#include <stdexcept>
#include <ranges>

namespace teaql {
class DebugWriter;

namespace eval {
class LoadState;
}
template <class T>
class SmartList;

namespace detail {
struct LoadedRelationResults;
}

// Immutable, scope-owned column layout. Only rows in the same allocation domain
// borrow it; construction in another domain copies the layout.
class ColumnLayout {
public:
    using allocator_type = Allocator;

    ColumnLayout(std::allocator_arg_t, Allocator a, std::initializer_list<std::string_view> names)
        : names_(a.resource()), relations_(a.resource()), other_types_(a.resource()) {
        for (auto name : names) {
            names_.emplace_back(name);
        }
    }

    ColumnLayout(std::allocator_arg_t, Allocator, const ColumnLayout&);

    ColumnLayout(std::allocator_arg_t,
                 Allocator a,
                 Vector<String> names,
                 std::span<const String> relations = {},
                 const DynamicPropertyDefinitions* definitions = nullptr);

    std::span<const String> relation_names() const noexcept {
        return relations_;
    }

    ColumnLayout(const ColumnLayout&) = delete;
    ColumnLayout& operator=(const ColumnLayout&) = delete;

    std::span<const String> names() const noexcept {
        return names_;
    }

    Memory memory_resource() const noexcept {
        return names_.get_allocator().resource();
    }

    eval::LoadState load_state(SnapshotPool&, const FieldLayout&) const;

    const ColumnLayout& with_dynamic_property_definitions(const DynamicPropertyDefinitions&) const;
    bool operator==(const ColumnLayout&) const;

    const DynamicPropertyDefinitions* dynamic_property_definitions() const {
        return dynamic_properties_ ? &dynamic_properties_->get() : nullptr;
    }

private:
    friend void write_debug(DebugWriter&, const ColumnLayout&);
    Vector<String> names_;
    Vector<String> relations_;
    std::optional<ArenaBorrow<DynamicPropertyDefinitions>> dynamic_properties_;
    mutable std::optional<LoadedSnapshot::Borrow> primary_;
    mutable std::pmr::map<std::uint64_t, LoadedSnapshot::Borrow> other_types_;
};

// The existing C++ name remains an API-compatible spelling of the source layout.
using CompactRowLayout = ColumnLayout;

// One explicit hydration pass; keys retain arena-owned shape identities only.
class RelationShapeCache {
public:
    using allocator_type = Allocator;

    RelationShapeCache(std::allocator_arg_t, Allocator a)
        : relation_(a.resource()), shapes_(a.resource()) {}

    Memory memory_resource() const noexcept {
        return relation_.get_allocator().resource();
    }

private:
    friend class CompactRow;
    String relation_;
    std::pmr::map<const ColumnLayout*, const ColumnLayout*> shapes_;
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

    CompactRow(CompactRow&&) noexcept;
    ~CompactRow();

    CompactRow(std::allocator_arg_t, Allocator, CompactRow&&);

    CompactRow& operator=(CompactRow&& other);

    Memory memory_resource() const noexcept {
        return values_.get_allocator().resource();
    }

    const ColumnLayout* shared_columns() const;

    const ColumnLayout* shared_layout() const {
        return shared_columns();
    }

    eval::LoadState indexed_load_state(SnapshotPool&, const FieldLayout&) const;
    void mark_relation_loaded(std::string_view, RelationShapeCache&);
    Result<std::monostate, String> validate_dynamic_properties() const;
    static void attach_dynamic_property_definitions(std::span<CompactRow>,
                                                    const DynamicPropertyDefinitions&);
    const SmartList<CompactRow>* loaded_relation(std::string_view) const;
    void set_loaded_relation(std::string_view, const SmartList<CompactRow>&);
    std::optional<SmartList<CompactRow>> take_loaded_relation(std::string_view);
    bool has_loaded_relations() const;
    void clear_loaded_relations();
    void set_loaded_dynamic_fields(const DynamicFieldValues&);
    Result<std::monostate, DynamicFieldError> merge_loaded_dynamic_fields(const DynamicFieldValues&,
                                                                          DynamicFieldMergeShapes&);
    const DynamicFieldValues* loaded_dynamic_fields() const;
    std::optional<DynamicFieldValues> take_loaded_dynamic_fields();

    bool is_loaded_relation(std::string_view name) const {
        const auto names = shared_columns()->relation_names();
        return std::ranges::find(names, name) != names.end();
    }

    // Temporary batch interning shares actual projections, never row values.
    static void share_layouts(std::span<CompactRow>);

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
    Result<std::monostate, DynamicFieldError> try_extend(const CompactRow&,
                                                         DynamicFieldMergeShapes&);
    Record copy_map(Memory target) const;

    Record into_map(Memory target) const& {
        return copy_map(target);
    }

    Record into_map(Memory target) &&;

    bool operator==(const CompactRow&) const;

private:
    friend void write_debug(DebugWriter&, const CompactRow&);
    mutable const ColumnLayout* shared_ = nullptr;
    Vector<Value> values_;
    detail::ScopedBox<detail::LoadedRelationResults> loaded_relations_;
    detail::LoadedRelationResults& read_metadata();
};

Json compact_row_to_json_value(Memory destination, const CompactRow&);
}
