#pragma once
#include <teaql/core/result.hpp>
#include <teaql/core/dynamic_properties.hpp>
#include <teaql/core/dynamic_fields.hpp>
#include <teaql/memory/object_borrow.hpp>
#include <map>
#include <set>
#include <span>
#include <string_view>
#include <tuple>

namespace teaql {
// Immutable model geometry. Indexes come from generated metadata, never lookup order.
// The owning Context bounds all strings and borrowed views. Cross-domain retention
// uses the allocator-aware copy constructor; no shared owner extends the source.
class FieldLayout {
public:
    using allocator_type = Allocator;
    using Index = std::pair<std::string_view, std::size_t>;
    using Mapping = std::tuple<std::string_view, std::string_view, std::string_view>;

    static Result<FieldLayout, String>
    from_generated(Memory,
                   std::string_view entity,
                   std::string_view revision,
                   std::span<const Index> indexes,
                   std::span<const Mapping> mappings,
                   std::span<const std::string_view> relations,
                   std::span<const std::string_view> expected_members);

    FieldLayout(std::allocator_arg_t, Allocator, const FieldLayout&);

    FieldLayout(std::allocator_arg_t, Allocator a, FieldLayout&& other)
        : FieldLayout(detail::move_to(a.resource(), std::move(other))) {}

    FieldLayout(const FieldLayout&) = delete;
    FieldLayout(FieldLayout&&) noexcept = default;
    FieldLayout& operator=(const FieldLayout&) = delete;
    FieldLayout& operator=(FieldLayout&&) = delete;

    std::string_view entity() const noexcept {
        return entity_;
    }

    // Source shared_entity_name shares immutable bytes; C++ borrows those bytes
    // inside the explicit owner lifetime rather than publishing an Arc owner.
    std::string_view shared_entity_name() const noexcept {
        return entity_;
    }

    std::string_view revision() const noexcept {
        return revision_;
    }

    std::optional<std::size_t> index(std::string_view) const;

    std::size_t field_count() const noexcept {
        return names_.size();
    }

    bool is_relation(std::string_view) const;

    Memory memory_resource() const noexcept {
        return entity_.get_allocator().resource();
    }

private:
    friend void write_debug(DebugWriter&, const FieldLayout&);
    FieldLayout(Memory,
                std::string_view entity,
                std::string_view revision,
                std::span<const Index>,
                std::span<const Mapping>,
                std::span<const std::string_view> relations,
                std::span<const std::string_view> expected_members);

    String entity_, revision_;
    Vector<String> names_;
    std::pmr::map<String, std::size_t, std::less<>> aliases_;
    std::pmr::set<String, std::less<>> relations_;
};
}

namespace teaql {
class SnapshotPool;

// Value-free availability. Managed publication makes the snapshot immutable;
// wrappers borrow it through a checked Context/object identity.
class LoadedSnapshot {
public:
    using allocator_type = Allocator;
    using Overflow = std::pmr::set<std::size_t>;
    using SelectedNames = std::pmr::set<String, std::less<>>;
    using Borrow = ArenaBorrow<LoadedSnapshot>;

    static LoadedSnapshot fully_loaded(Memory, const FieldLayout&);
    static LoadedSnapshot projection(Memory, const FieldLayout&, std::span<const std::string_view>);
    LoadedSnapshot(std::allocator_arg_t, Allocator, const LoadedSnapshot&);

    LoadedSnapshot(std::allocator_arg_t, Allocator a, LoadedSnapshot&& source)
        : LoadedSnapshot(detail::move_to(a.resource(), std::move(source))) {}

    LoadedSnapshot(const LoadedSnapshot&) = delete;
    LoadedSnapshot(LoadedSnapshot&&) noexcept = default;
    LoadedSnapshot& operator=(const LoadedSnapshot&) = delete;
    LoadedSnapshot& operator=(LoadedSnapshot&&) = delete;

    const FieldLayout& layout() const {
        return layout_.get();
    }

    std::uint64_t bits() const noexcept {
        return bits_;
    }

    const Overflow* overflow() const;

    const SelectedNames* selected_names() const;

    const DynamicPropertyDefinitions* dynamic_property_definitions() const {
        return dynamic_properties_ ? &dynamic_properties_->get() : nullptr;
    }

    bool is_loaded(std::string_view) const;
    bool operator==(const LoadedSnapshot&) const;
    std::size_t fingerprint() const;

    Memory memory_resource() const noexcept {
        return owner_;
    }

    Borrow into_shared(SnapshotPool&) &&;
    static Result<Borrow, String> with_loaded(SnapshotPool&, const Borrow&, std::string_view, bool);
    static Borrow with_dynamic_property_definitions(SnapshotPool&,
                                                    const Borrow&,
                                                    const DynamicPropertyDefinitions&);
    // Internal result-schema replacement, including explicit metadata clearing.
    static Borrow
    with_property_schema(SnapshotPool&, const Borrow&, const DynamicPropertyDefinitions*);
    static Result<Borrow, String>
    with_dynamic_fields(SnapshotPool&, const Borrow&, const DynamicFieldValues&);

private:
    friend class SnapshotPool;

    struct OverflowNode {
        using allocator_type = Allocator;
        Overflow fields;

        OverflowNode(std::allocator_arg_t, Allocator a, Overflow value)
            : fields(std::move(value), a.resource()) {}

        OverflowNode(std::allocator_arg_t, Allocator a, const OverflowNode& source)
            : fields(source.fields, a.resource()) {}

        Memory memory_resource() const noexcept {
            return fields.get_allocator().resource();
        }
    };

    struct NamesNode {
        using allocator_type = Allocator;
        SelectedNames fields;

        NamesNode(std::allocator_arg_t, Allocator a, SelectedNames value)
            : fields(std::move(value), a.resource()) {}

        NamesNode(std::allocator_arg_t, Allocator a, const NamesNode& source)
            : fields(source.fields, a.resource()) {}

        Memory memory_resource() const noexcept {
            return fields.get_allocator().resource();
        }
    };

    LoadedSnapshot(Memory, const FieldLayout&);
    void mark(std::string_view, bool);
    Memory owner_;
    ArenaBorrow<FieldLayout> layout_;
    std::uint64_t bits_ = 0;
    std::optional<Overflow> overflow_;
    std::optional<SelectedNames> selected_names_;
    std::optional<ArenaBorrow<OverflowNode>> shared_overflow_;
    std::optional<ArenaBorrow<NamesNode>> shared_names_;
    std::optional<ArenaBorrow<DynamicPropertyDefinitions>> dynamic_properties_;
};

// Explicit result-Context interner. Global type metadata never retains User
// snapshots. Admission is bounded per layout; saturation does not change values.
class SnapshotPool {
public:
    using allocator_type = Allocator;
    static constexpr std::size_t max_shared_state_hints = 512;
    SnapshotPool(std::allocator_arg_t, Allocator);
    SnapshotPool(const SnapshotPool&) = delete;
    SnapshotPool(SnapshotPool&&) = delete;
    SnapshotPool& operator=(const SnapshotPool&) = delete;
    SnapshotPool& operator=(SnapshotPool&&) = delete;

    Memory memory_resource() const noexcept {
        return arena_;
    }

    LoadedSnapshot::Borrow publish(LoadedSnapshot);
    LoadedSnapshot::Borrow copy(const LoadedSnapshot&);
    std::size_t hint_count(const FieldLayout&) const;

private:
    using Bucket = Vector<LoadedSnapshot::Borrow>;
    using Hints = std::pmr::map<std::size_t, Bucket>;
    using LayoutHints = std::pmr::map<std::uint64_t, Hints>;
    Arena* arena_;
    LayoutHints hints_;
};
}
