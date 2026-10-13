#pragma once
#include <teaql/core/value.hpp>
#include <teaql/core/result.hpp>
#include <span>

namespace teaql {
// Readonly result schema: a declared property is not necessarily selected/present.
class DynamicPropertyDefinitions {
public:
    using allocator_type = Allocator;
    using Field = std::pair<std::string_view, DataType>;
    using Types = std::pmr::map<String, DataType, std::less<>>;

    static Result<DynamicPropertyDefinitions, String> from_fields(Memory, std::span<const Field>);

    DynamicPropertyDefinitions(std::allocator_arg_t, Allocator, const DynamicPropertyDefinitions&);

    DynamicPropertyDefinitions(std::allocator_arg_t,
                               Allocator a,
                               DynamicPropertyDefinitions&& value)
        : DynamicPropertyDefinitions(detail::move_to(a.resource(), std::move(value))) {}

    DynamicPropertyDefinitions(const DynamicPropertyDefinitions&) = delete;
    DynamicPropertyDefinitions(DynamicPropertyDefinitions&&) noexcept = default;
    DynamicPropertyDefinitions& operator=(const DynamicPropertyDefinitions&) = delete;
    DynamicPropertyDefinitions& operator=(DynamicPropertyDefinitions&&) = delete;

    const Types& types() const noexcept {
        return types_;
    }

    std::optional<DataType> data_type(std::string_view) const;
    Result<std::monostate, String> validate_value(std::string_view, const Value&) const;

    bool operator==(const DynamicPropertyDefinitions& other) const {
        return types_ == other.types_;
    }

    Memory memory_resource() const noexcept {
        return types_.get_allocator().resource();
    }

    std::size_t fingerprint() const noexcept {
        return static_cast<std::size_t>(fingerprint_);
    }

    std::uint64_t schema_hash() const noexcept {
        return fingerprint_;
    }

    // Match source subset reuse before allocation. The caller retains ownership
    // of a returned existing schema; merged definitions are copied to destination.
    static Result<const DynamicPropertyDefinitions*, String>
    reusable_merge(Memory, const DynamicPropertyDefinitions&, const DynamicPropertyDefinitions&);
    static Result<DynamicPropertyDefinitions, String>
    merge_copy(Memory, const DynamicPropertyDefinitions&, const DynamicPropertyDefinitions&);

private:
    explicit DynamicPropertyDefinitions(Memory r) : types_(r) {}

    void update_fingerprint() noexcept;
    Types types_;
    std::uint64_t fingerprint_ = 0;
};
}

namespace std {
template <>
struct hash<teaql::DynamicPropertyDefinitions> {
    std::size_t operator()(const teaql::DynamicPropertyDefinitions& definitions) const noexcept {
        return definitions.fingerprint();
    }
};
}
