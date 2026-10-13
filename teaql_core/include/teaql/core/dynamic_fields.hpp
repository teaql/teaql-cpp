#pragma once
#include <teaql/core/result.hpp>
#include <teaql/core/value.hpp>
#include <teaql/memory/object_borrow.hpp>
#include <set>
#include <span>

namespace teaql {
class DebugWriter;

struct DynamicFieldError {
    using allocator_type = Allocator;
    String code;
    String field;

    DynamicFieldError(Memory r, std::string_view code, std::string_view field)
        : code(code, r), field(field, r) {}

    DynamicFieldError(std::allocator_arg_t, Allocator a, const DynamicFieldError& other)
        : DynamicFieldError(a.resource(), other.code, other.field) {}

    DynamicFieldError(std::allocator_arg_t, Allocator a, DynamicFieldError&& other)
        : DynamicFieldError(a.resource(), other.code, other.field) {}

    DynamicFieldError(DynamicFieldError&&) noexcept = default;

    Memory memory_resource() const {
        return field.get_allocator().resource();
    }

    bool operator==(const DynamicFieldError&) const = default;

    String to_string(Memory r) const {
        String result(code, r);
        result += ": ";
        result += field;
        return result;
    }
};

class DynamicFieldDefinitions;

class DynamicFieldSelection {
public:
    using allocator_type = Allocator;
    using Field = std::pair<std::string_view, DataType>;
    using Types = std::pmr::map<String, DataType, std::less<>>;
    static DynamicFieldSelection all(Memory);
    static Result<DynamicFieldSelection, DynamicFieldError> fields(Memory, std::span<const Field>);
    DynamicFieldSelection(std::allocator_arg_t, Allocator, const DynamicFieldSelection&);

    DynamicFieldSelection(std::allocator_arg_t, Allocator a, DynamicFieldSelection&& other)
        : DynamicFieldSelection(detail::move_to(a.resource(), std::move(other))) {}

    DynamicFieldSelection(DynamicFieldSelection&&) noexcept = default;
    bool contains(std::string_view) const;

    bool selects_all() const {
        return all_;
    }

    const Types& selected_fields() const {
        return fields_;
    }

    bool operator==(const DynamicFieldSelection& other) const {
        return all_ == other.all_ && fields_ == other.fields_;
    }

    Result<std::monostate, DynamicFieldError> validate(const DynamicFieldDefinitions&) const;

    Memory memory_resource() const {
        return fields_.get_allocator().resource();
    }

private:
    DynamicFieldSelection(Memory r, bool all) : all_(all), fields_(r) {}

    bool all_;
    Types fields_;
};

// Immutable owner/revision metadata. Storage authority is provider-owned and
// deliberately separate from SQL column names and generated fixed indexes.
class DynamicFieldDefinitions {
public:
    using allocator_type = Allocator;
    using Field = DynamicFieldSelection::Field;
    using Types = DynamicFieldSelection::Types;
    static Result<DynamicFieldDefinitions, DynamicFieldError>
    from_fields(Memory, std::string_view owner, std::string_view revision, std::span<const Field>);
    DynamicFieldDefinitions(std::allocator_arg_t, Allocator, const DynamicFieldDefinitions&);

    DynamicFieldDefinitions(std::allocator_arg_t, Allocator a, DynamicFieldDefinitions&& other)
        : DynamicFieldDefinitions(detail::move_to(a.resource(), std::move(other))) {}

    DynamicFieldDefinitions(DynamicFieldDefinitions&&) noexcept = default;

    std::string_view owner_type() const {
        return owner_type_;
    }

    std::string_view revision() const {
        return revision_;
    }

    const Types& fields() const {
        return fields_;
    }

    Result<DataType, DynamicFieldError> data_type(std::string_view) const;
    Result<std::monostate, DynamicFieldError> validate_value(std::string_view, const Value&) const;
    DynamicFieldDefinitions
    with_storage_binding(Memory, std::string_view profile, std::uint64_t executor) const;
    bool storage_binding_matches(std::string_view profile, std::uint64_t executor) const;
    ArenaBorrow<DynamicFieldDefinitions>
    with_storage_binding_shared(Memory, std::string_view profile, std::uint64_t executor) const;
    bool same_schema(const DynamicFieldDefinitions&) const;
    bool operator==(const DynamicFieldDefinitions&) const;

    Memory memory_resource() const {
        return owner_type_.get_allocator().resource();
    }

private:
    friend void write_debug(DebugWriter&, const DynamicFieldDefinitions&);

    DynamicFieldDefinitions(Memory r, std::string_view owner, std::string_view revision)
        : owner_type_(owner, r), revision_(revision, r), fields_(r), profile_(r) {}

    String owner_type_, revision_;
    Types fields_;
    String profile_;
    std::optional<std::uint64_t> executor_;
};

enum class DynamicFieldState { Value, Null, NotLoaded };

// An empty optional is Delete; a present Value, including NULL, is Set.
class DynamicFieldMutation {
public:
    using allocator_type = Allocator;

    static DynamicFieldMutation set(Memory r, const Value& value) {
        return DynamicFieldMutation(r, &value);
    }

    static DynamicFieldMutation erase(Memory r) {
        return DynamicFieldMutation(r, nullptr);
    }

    DynamicFieldMutation(std::allocator_arg_t, Allocator a, const DynamicFieldMutation& other)
        : DynamicFieldMutation(a.resource(), other.value_ ? &*other.value_ : nullptr) {}

    DynamicFieldMutation(std::allocator_arg_t, Allocator a, DynamicFieldMutation&& other)
        : DynamicFieldMutation(detail::move_to(a.resource(), std::move(other))) {}

    DynamicFieldMutation(DynamicFieldMutation&&) noexcept = default;

    DynamicFieldMutation& operator=(DynamicFieldMutation&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    bool is_delete() const {
        return !value_;
    }

    const Value* value() const {
        return value_ ? &*value_ : nullptr;
    }

    Memory memory_resource() const {
        return owner_;
    }

    bool operator==(const DynamicFieldMutation& other) const {
        return value_ == other.value_;
    }

private:
    DynamicFieldMutation(Memory r, const Value* value) : owner_(r) {
        if (value) {
            value_.emplace(r, *value);
        }
    }

    Memory owner_;
    std::optional<Value> value_;
};

class DynamicFieldValues;

class DynamicFieldValue {
public:
    using allocator_type = Allocator;

    DynamicFieldValue(std::allocator_arg_t, Allocator a, const DynamicFieldValue& other)
        : owner_(std::allocator_arg, a, other.owner_), code_(other.code_) {}

    DynamicFieldValue(std::allocator_arg_t, Allocator a, DynamicFieldValue&& other)
        : DynamicFieldValue(std::allocator_arg, a, std::as_const(other)) {}

    DynamicFieldValue(DynamicFieldValue&&) noexcept = default;

    Memory memory_resource() const {
        return owner_.memory_resource();
    }

    DataType data_type() const;
    DynamicFieldState state() const;
    const Value* value() const;

    bool is_loaded() const {
        return state() != DynamicFieldState::NotLoaded;
    }

private:
    friend class DynamicFieldValues;
    DynamicFieldValue(const DynamicFieldValues&, std::string_view);
    ArenaBorrow<DynamicFieldValues> owner_;
    std::string_view code_;
};

// Value-free immutable geometry is borrowed inside its explicit Context. The
// operation-local union cache never retains row values or extends their lifetime.
class DynamicFieldMergeShapes {
public:
    using allocator_type = Allocator;
    using Codes = std::pmr::set<String, std::less<>>;

    struct Shape {
        using allocator_type = Allocator;
        Codes codes;

        Shape(std::allocator_arg_t, Allocator a, const Codes& source)
            : codes(source, a.resource()) {}

        Shape(std::allocator_arg_t, Allocator a, const Shape& source)
            : codes(source.codes, a.resource()) {}

        Memory memory_resource() const {
            return codes.get_allocator().resource();
        }
    };

    using Borrow = ArenaBorrow<Shape>;
    DynamicFieldMergeShapes(std::allocator_arg_t, Allocator);
    Borrow publish(const Codes&);
    Borrow unite(const Borrow&, const Borrow&);

    Memory memory_resource() const {
        return arena_;
    }

private:
    Arena* arena_;
    std::pmr::map<std::pair<std::uint64_t, std::uint64_t>, Borrow> unions_;
};

class DynamicFieldValues {
public:
    using allocator_type = Allocator;
    using Codes = DynamicFieldMergeShapes::Codes;
    using Values = Value::Object;
    static Result<DynamicFieldValues, DynamicFieldError>
    from_values(Memory, const DynamicFieldDefinitions&, const Values&);
    static Result<Vector<DynamicFieldValues>, DynamicFieldError>
    from_batch(Memory, const DynamicFieldDefinitions&, std::span<const Values>);
    DynamicFieldValues(std::allocator_arg_t, Allocator, const DynamicFieldValues&);

    DynamicFieldValues(std::allocator_arg_t, Allocator a, DynamicFieldValues&& other)
        : DynamicFieldValues(detail::move_to(a.resource(), std::move(other))) {}

    DynamicFieldValues(DynamicFieldValues&&) noexcept = default;

    DynamicFieldValues& operator=(DynamicFieldValues&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    bool operator==(const DynamicFieldValues& other) const {
        return definitions() == other.definitions() && selected_codes() == other.selected_codes() &&
               values_ == other.values_;
    }

    const DynamicFieldDefinitions& definitions() const {
        return definitions_.get();
    }

    const Codes& selected_codes() const {
        return selected_.get().codes;
    }

    std::uint64_t selection_identity() const {
        return selected_.identity();
    }

    const Values& values() const {
        return values_;
    }

    Values into_values() && {
        return std::move(values_);
    }

    Result<std::monostate, DynamicFieldError> assign(std::string_view, const Value&);
    Result<std::monostate, DynamicFieldError> erase(std::string_view);
    Result<DynamicFieldValue, DynamicFieldError> field(std::string_view) const;
    Result<std::monostate, DynamicFieldError> validate_merge(const DynamicFieldValues&) const;
    Result<std::monostate, DynamicFieldError> merge_loaded(const DynamicFieldValues&,
                                                           DynamicFieldMergeShapes&);

    Memory memory_resource() const {
        return values_.get_allocator().resource();
    }

private:
    DynamicFieldValues(Memory,
                       const DynamicFieldDefinitions&,
                       DynamicFieldMergeShapes::Borrow,
                       const Values&);
    ArenaBorrow<DynamicFieldDefinitions> definitions_;
    DynamicFieldMergeShapes::Borrow selected_;
    Values values_;
};
}
