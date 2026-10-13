#include <teaql/core/dynamic_fields.hpp>
#include <teaql/core/detail/text.hpp>
#include <algorithm>

namespace teaql {
namespace {
bool blank(std::string_view text) {
    detail::trim(text);
    return text.empty();
}

bool valid_code(std::string_view code) {
    return !blank(code) && !code.starts_with('#') && !code.starts_with('_');
}

Arena& arena_for(Memory r) {
    auto* arena = dynamic_cast<Arena*>(r);
    if (!arena) {
        throw std::invalid_argument("Dynamic field geometry requires an explicit Context arena");
    }
    return *arena;
}

template <class T>
ArenaBorrow<T> retain(Memory r, const T& source) {
    auto& target = arena_for(r);
    auto& origin = arena_for(source.memory_resource());
    if (origin.can_lend_to(target)) {
        return ArenaBorrow<T>(std::allocator_arg, Allocator(r), source);
    }
    return ArenaBorrow<T>(std::allocator_arg, Allocator(r), target.make<T>(source));
}

template <class T>
Result<T, DynamicFieldError> error(Memory r, std::string_view code, std::string_view field) {
    return Result<T, DynamicFieldError>::err(r, DynamicFieldError(r, code, field));
}

Result<std::monostate, DynamicFieldError> success(Memory r) {
    return Result<std::monostate, DynamicFieldError>::ok(r, {});
}

Result<std::monostate, DynamicFieldError>
insert_fields(Memory r,
              DynamicFieldSelection::Types& destination,
              std::span<const DynamicFieldSelection::Field> fields) {
    for (const auto& [code, type] : fields) {
        if (!valid_code(code)) {
            return error<std::monostate>(r, "DYNAMIC_FIELD_INVALID_CODE", code);
        }
        if (!destination.emplace(String(code, r), type).second) {
            return error<std::monostate>(r, "DYNAMIC_FIELD_DUPLICATE_CODE", code);
        }
    }
    return success(r);
}
}

DynamicFieldSelection DynamicFieldSelection::all(Memory r) {
    return DynamicFieldSelection(r, true);
}

Result<DynamicFieldSelection, DynamicFieldError>
DynamicFieldSelection::fields(Memory r, std::span<const Field> fields) {
    DynamicFieldSelection selected(r, false);
    auto validation = insert_fields(r, selected.fields_, fields);
    if (!validation) {
        return Result<DynamicFieldSelection, DynamicFieldError>::err(r,
                                                                     std::move(validation).error());
    }
    return Result<DynamicFieldSelection, DynamicFieldError>::ok(r, std::move(selected));
}

DynamicFieldSelection::DynamicFieldSelection(std::allocator_arg_t,
                                             Allocator a,
                                             const DynamicFieldSelection& other)
    : all_(other.all_), fields_(other.fields_, a.resource()) {}

bool DynamicFieldSelection::contains(std::string_view code) const {
    return all_ || fields_.contains(code);
}

Result<std::monostate, DynamicFieldError>
DynamicFieldSelection::validate(const DynamicFieldDefinitions& definitions) const {
    auto r = memory_resource();
    for (const auto& [code, type] : fields_) {
        auto declared = definitions.data_type(code);
        if (!declared) {
            return Result<std::monostate, DynamicFieldError>::err(r, std::move(declared).error());
        }
        if (declared.value() != type) {
            return error<std::monostate>(r, "DYNAMIC_FIELD_TYPE_MISMATCH", code);
        }
    }
    return success(r);
}

Result<DynamicFieldDefinitions, DynamicFieldError> DynamicFieldDefinitions::from_fields(
    Memory r, std::string_view owner, std::string_view revision, std::span<const Field> fields) {
    if (blank(owner) || blank(revision)) {
        return error<DynamicFieldDefinitions>(r, "DYNAMIC_FIELD_INVALID_DEFINITIONS", owner);
    }
    DynamicFieldDefinitions definitions(r, owner, revision);
    auto validation = insert_fields(r, definitions.fields_, fields);
    if (!validation) {
        return Result<DynamicFieldDefinitions, DynamicFieldError>::err(
            r, std::move(validation).error());
    }
    return Result<DynamicFieldDefinitions, DynamicFieldError>::ok(r, std::move(definitions));
}

DynamicFieldDefinitions::DynamicFieldDefinitions(std::allocator_arg_t,
                                                 Allocator a,
                                                 const DynamicFieldDefinitions& other)
    : owner_type_(other.owner_type_, a.resource()), revision_(other.revision_, a.resource()),
      fields_(other.fields_, a.resource()), profile_(other.profile_, a.resource()),
      executor_(other.executor_) {}

Result<DataType, DynamicFieldError>
DynamicFieldDefinitions::data_type(std::string_view code) const {
    auto found = fields_.find(code);
    if (found == fields_.end()) {
        return error<DataType>(memory_resource(), "DYNAMIC_FIELD_NOT_FOUND", code);
    }
    return Result<DataType, DynamicFieldError>::ok(memory_resource(), found->second);
}

Result<std::monostate, DynamicFieldError>
DynamicFieldDefinitions::validate_value(std::string_view code, const Value& value) const {
    auto r = memory_resource();
    auto declared = data_type(code);
    if (!declared) {
        return Result<std::monostate, DynamicFieldError>::err(r, std::move(declared).error());
    }
    const auto type = declared.value();
    const bool matches = std::visit(
        [&](const auto& actual) {
            using T = std::decay_t<decltype(actual)>;
            if constexpr (std::same_as<T, std::monostate>) {
                return true;
            } else if constexpr (std::same_as<T, TypedNull>) {
                return actual.type == type;
            } else if constexpr (std::same_as<T, bool>) {
                return type == DataType::Bool;
            } else if constexpr (std::same_as<T, std::int64_t>) {
                return type == DataType::I64;
            } else if constexpr (std::same_as<T, std::uint64_t>) {
                return type == DataType::U64;
            } else if constexpr (std::same_as<T, double>) {
                return type == DataType::F64;
            } else if constexpr (std::same_as<T, Decimal>) {
                return type == DataType::Decimal;
            } else if constexpr (std::same_as<T, String>) {
                return type == DataType::Text || type == DataType::LargeText;
            } else if constexpr (std::same_as<T, Json>) {
                return type == DataType::Json;
            } else if constexpr (std::same_as<T, Date>) {
                return type == DataType::Date;
            } else if constexpr (std::same_as<T, Timestamp>) {
                return type == DataType::Timestamp;
            } else {
                return false;
            }
        },
        value.data);
    return matches ? success(r) : error<std::monostate>(r, "DYNAMIC_FIELD_TYPE_MISMATCH", code);
}

DynamicFieldDefinitions DynamicFieldDefinitions::with_storage_binding(
    Memory r, std::string_view profile, std::uint64_t executor) const {
    auto copy = detail::clone_in(r, *this);
    copy.profile_ = profile;
    copy.executor_ = executor;
    return copy;
}

bool DynamicFieldDefinitions::storage_binding_matches(std::string_view profile,
                                                      std::uint64_t executor) const {
    return executor_ && *executor_ == executor && profile_ == profile;
}

ArenaBorrow<DynamicFieldDefinitions> DynamicFieldDefinitions::with_storage_binding_shared(
    Memory r, std::string_view profile, std::uint64_t executor) const {
    if (storage_binding_matches(profile, executor)) {
        return retain(r, *this);
    }
    auto& stored =
        arena_for(r).make<DynamicFieldDefinitions>(with_storage_binding(r, profile, executor));
    return ArenaBorrow<DynamicFieldDefinitions>(std::allocator_arg, Allocator(r), stored);
}

bool DynamicFieldDefinitions::same_schema(const DynamicFieldDefinitions& other) const {
    return owner_type_ == other.owner_type_ && revision_ == other.revision_ &&
           fields_ == other.fields_;
}

bool DynamicFieldDefinitions::operator==(const DynamicFieldDefinitions& other) const {
    return same_schema(other) && profile_ == other.profile_ && executor_ == other.executor_;
}

DynamicFieldMergeShapes::DynamicFieldMergeShapes(std::allocator_arg_t, Allocator a)
    : arena_(&arena_for(a.resource())), unions_(a.resource()) {}

DynamicFieldMergeShapes::Borrow DynamicFieldMergeShapes::publish(const Codes& codes) {
    return Borrow(std::allocator_arg, Allocator(arena_), arena_->make<Shape>(codes));
}

DynamicFieldMergeShapes::Borrow DynamicFieldMergeShapes::unite(const Borrow& left,
                                                               const Borrow& right) {
    const auto& a = left.get().codes;
    const auto& b = right.get().codes;
    const auto subset = [](const Codes& small, const Codes& large) {
        return std::all_of(small.begin(), small.end(), [&](const auto& code) {
            return large.contains(code);
        });
    };
    if (subset(b, a)) {
        return retain(arena_, left.get());
    }
    if (subset(a, b)) {
        return retain(arena_, right.get());
    }
    const auto key = std::pair(left.identity(), right.identity());
    auto found = unions_.find(key);
    if (found != unions_.end()) {
        return found->second;
    }
    Codes merged(a, arena_);
    merged.insert(b.begin(), b.end());
    auto result = publish(merged);
    unions_.emplace(key, result);
    return result;
}

DynamicFieldValues::DynamicFieldValues(Memory r,
                                       const DynamicFieldDefinitions& definitions,
                                       DynamicFieldMergeShapes::Borrow selected,
                                       const Values& values)
    : definitions_(retain(r, definitions)), selected_(retain(r, selected.get())),
      values_(clone_record(r, values)) {}

DynamicFieldValues::DynamicFieldValues(std::allocator_arg_t,
                                       Allocator a,
                                       const DynamicFieldValues& other)
    : DynamicFieldValues(a.resource(), other.definitions(), other.selected_, other.values_) {}

Result<DynamicFieldValues, DynamicFieldError> DynamicFieldValues::from_values(
    Memory r, const DynamicFieldDefinitions& definitions, const Values& values) {
    Codes selected(r);
    for (const auto& [code, value] : values) {
        auto checked = definitions.validate_value(code, value);
        if (!checked) {
            return Result<DynamicFieldValues, DynamicFieldError>::err(r,
                                                                      std::move(checked).error());
        }
        selected.emplace(code);
    }
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
    return Result<DynamicFieldValues, DynamicFieldError>::ok(
        r, DynamicFieldValues(r, definitions, shapes.publish(selected), values));
}

Result<Vector<DynamicFieldValues>, DynamicFieldError> DynamicFieldValues::from_batch(
    Memory r, const DynamicFieldDefinitions& definitions, std::span<const Values> rows) {
    using Output = Result<Vector<DynamicFieldValues>, DynamicFieldError>;
    Vector<DynamicFieldValues> result(r);
    // Ordered geometry is collision-free. Equal sets share a Context object,
    // independent of row insertion order and payload values.
    std::pmr::map<Codes, DynamicFieldMergeShapes::Borrow> shapes(r);
    DynamicFieldMergeShapes pool(std::allocator_arg, Allocator(r));
    for (const auto& values : rows) {
        Codes selected(r);
        for (const auto& [code, value] : values) {
            auto checked = definitions.validate_value(code, value);
            if (!checked) {
                return Output::err(r, std::move(checked).error());
            }
            selected.emplace(code);
        }
        auto found = shapes.find(selected);
        auto geometry = found == shapes.end() ? pool.publish(selected) : found->second;
        if (found == shapes.end()) {
            shapes.emplace(std::move(selected), geometry);
        }
        result.push_back(DynamicFieldValues(r, definitions, geometry, values));
    }
    return Output::ok(r, std::move(result));
}

Result<std::monostate, DynamicFieldError> DynamicFieldValues::assign(std::string_view code,
                                                                     const Value& value) {
    auto r = memory_resource();
    auto checked = definitions().validate_value(code, value);
    if (!checked) {
        return Result<std::monostate, DynamicFieldError>::err(r, std::move(checked).error());
    }
    auto geometry = selected_;
    if (!selected_codes().contains(code)) {
        Codes codes(selected_codes(), r);
        codes.emplace(code);
        DynamicFieldMergeShapes pool(std::allocator_arg, Allocator(r));
        geometry = pool.publish(codes);
    }
    values_.insert_or_assign(String(code, r), Value(r, value));
    selected_ = geometry;
    return success(r);
}

Result<std::monostate, DynamicFieldError> DynamicFieldValues::erase(std::string_view code) {
    auto r = memory_resource();
    auto checked = definitions().data_type(code);
    if (!checked) {
        return Result<std::monostate, DynamicFieldError>::err(r, std::move(checked).error());
    }
    auto geometry = selected_;
    if (selected_codes().contains(code)) {
        Codes codes(selected_codes(), r);
        codes.erase(codes.find(code));
        DynamicFieldMergeShapes pool(std::allocator_arg, Allocator(r));
        geometry = pool.publish(codes);
    }
    auto found = values_.find(code);
    if (found != values_.end()) {
        values_.erase(found);
    }
    selected_ = geometry;
    return success(r);
}

Result<DynamicFieldValue, DynamicFieldError>
DynamicFieldValues::field(std::string_view code) const {
    auto checked = definitions().data_type(code);
    if (!checked) {
        return Result<DynamicFieldValue, DynamicFieldError>::err(memory_resource(),
                                                                 std::move(checked).error());
    }
    // Use stable definition bytes, never the caller's temporary name buffer.
    return Result<DynamicFieldValue, DynamicFieldError>::ok(
        memory_resource(), DynamicFieldValue(*this, definitions().fields().find(code)->first));
}

Result<std::monostate, DynamicFieldError>
DynamicFieldValues::validate_merge(const DynamicFieldValues& other) const {
    if (definitions() != other.definitions()) {
        return error<std::monostate>(
            memory_resource(), "DYNAMIC_FIELD_INCOMPATIBLE_VIEW", definitions().owner_type());
    }
    return success(memory_resource());
}

Result<std::monostate, DynamicFieldError>
DynamicFieldValues::merge_loaded(const DynamicFieldValues& other, DynamicFieldMergeShapes& shapes) {
    auto checked = validate_merge(other);
    if (!checked) {
        return checked;
    }
    auto geometry = retain(memory_resource(), shapes.unite(selected_, other.selected_).get());
    for (const auto& [code, value] : other.values_) {
        values_.insert_or_assign(String(code, memory_resource()), Value(memory_resource(), value));
    }
    selected_ = geometry;
    return success(memory_resource());
}

DynamicFieldValue::DynamicFieldValue(const DynamicFieldValues& owner, std::string_view code)
    : owner_(std::allocator_arg, Allocator(owner.memory_resource()), owner), code_(code) {}

DataType DynamicFieldValue::data_type() const {
    return owner_.get().definitions().data_type(code_).value();
}

DynamicFieldState DynamicFieldValue::state() const {
    const auto& values = owner_.get().values();
    auto found = values.find(code_);
    return found == values.end()     ? DynamicFieldState::NotLoaded
           : found->second.is_null() ? DynamicFieldState::Null
                                     : DynamicFieldState::Value;
}

const Value* DynamicFieldValue::value() const {
    const auto& values = owner_.get().values();
    auto found = values.find(code_);
    return found == values.end() || found->second.is_null() ? nullptr : &found->second;
}
}
