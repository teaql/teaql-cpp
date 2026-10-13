#pragma once
#include <teaql/core/meta.hpp>
#include <teaql/core/mutation.hpp>
#include <teaql/core/result.hpp>
#include <teaql/core/row.hpp>
#include <teaql/core/detail/text.hpp>
#include <set>
#include <typeinfo>
#include <exception>

namespace teaql {
using FieldSet = std::pmr::set<String, std::less<>>;

// A presentation path guard; entity ledgers and identity registries are untouched.
class EntityJsonTraversal {
public:
    using allocator_type = Allocator;

    EntityJsonTraversal(std::allocator_arg_t, Allocator a) : path_(a.resource()) {}

    EntityJsonTraversal(const EntityJsonTraversal&) = delete;
    EntityJsonTraversal& operator=(const EntityJsonTraversal&) = delete;

    bool enter(std::string_view entity, std::uintptr_t address) {
        const bool expand =
            path_.size() < 128 && std::ranges::none_of(path_, [&](const auto& entry) {
                return entry.first == entity && entry.second == address;
            });
        path_.emplace_back(String(entity, memory_resource()), address);
        return expand;
    }

    void leave() {
        if (!path_.empty()) {
            path_.pop_back();
        }
    }

    Memory memory_resource() const noexcept {
        return path_.get_allocator().resource();
    }

private:
    Vector<std::pair<String, std::uintptr_t>> path_;
};

// Type-erased context borrow: equivalent to &dyn Any, never owns or copies context.
class AnyRef {
public:
    template <class T>
        requires(!std::same_as<std::remove_cvref_t<T>, AnyRef>)
    explicit AnyRef(T& value) : value_(&value), type_(&typeid(T)) {}
    template <class T>
        requires(!std::same_as<std::remove_cvref_t<T>, AnyRef> && !std::is_lvalue_reference_v<T>)
    AnyRef(T&&) = delete;

    template <class T>
    const T* get() const {
        return *type_ == typeid(T) ? static_cast<const T*>(value_) : nullptr;
    }

private:
    const void* value_;
    const std::type_info* type_;
};

struct EntityError {
    using allocator_type = Allocator;
    String entity, message;

    EntityError(std::allocator_arg_t,
                Allocator a,
                std::string_view entity,
                std::string_view message)
        : entity(entity, a.resource()), message(message, a.resource()) {}

    EntityError(std::allocator_arg_t, Allocator a, const EntityError& other)
        : EntityError(std::allocator_arg, a, other.entity, other.message) {}

    EntityError(const EntityError&) = delete;
    EntityError(EntityError&&) noexcept = default;

    Memory memory_resource() const {
        return entity.get_allocator().resource();
    }

    String to_string(Memory r) const {
        String text(entity, r);
        text += ": ";
        text += message;
        return text;
    }

    bool operator==(const EntityError& other) const {
        return entity == other.entity && message == other.message;
    }

    const std::exception* source() const noexcept {
        return nullptr;
    }
};

class EntityDescriptorStore {
public:
    virtual ~EntityDescriptorStore() = default;
    virtual Memory memory_resource() const = 0;
    virtual void register_descriptor(EntityDescriptor) = 0;
};
template <class T>
concept TeaqlEntity = requires(Memory r) {
    { T::ENTITY_NAME } -> std::convertible_to<std::string_view>;
    { T::entity_descriptor(r) } -> std::same_as<EntityDescriptor>;
};

template <TeaqlEntity... T>
void register_entities(EntityDescriptorStore& store) {
    auto register_one = [&]<class Model>() {
        if constexpr (requires { Model::register_into(store); }) {
            Model::register_into(store);
        } else {
            store.register_descriptor(Model::entity_descriptor(store.memory_resource()));
        }
    };
    (register_one.template operator()<T>(), ...);
}
template <class T>
class Audited;

// CRTP supplies Rust trait defaults, while allowing handwritten models to override
// projection/dirty tracking. These hooks do not silently invent mutation intent.
template <class Derived>
class EntityDefaults {
public:
    std::optional<DataType> dynamic_property_type(std::string_view key) const {
        const auto& model = static_cast<const Derived&>(*this);
        auto state = model.loaded_state_snapshot();
        const auto* definitions = state ? state->get().dynamic_property_definitions() : nullptr;
        return definitions ? definitions->data_type(key) : std::nullopt;
    }

    static constexpr bool supports_dynamic_property_load() {
        return false;
    }

    const Value* dynamic_property(std::string_view) const {
        return nullptr;
    }

    bool has_dynamic_property(std::string_view) const {
        return false;
    }

    const DynamicFieldValues* dynamic_field_values() const {
        return nullptr;
    }

    bool has_pending_dynamic_mutations() const {
        return false;
    }

    static constexpr bool supports_dynamic_field_load() {
        return false;
    }

    std::optional<Json> borrowed_json(Memory, EntityJsonTraversal&) const {
        return std::nullopt;
    }

    Record into_json_values() && {
        return std::move(derived()).into_values().fields;
    }

    Result<std::monostate, DynamicFieldError> update_dynamic_field(std::string_view code,
                                                                   const Value&) {
        auto r = derived().memory_resource();
        return Result<std::monostate, DynamicFieldError>::err(
            r, DynamicFieldError(r, "DYNAMIC_FIELD_CARRIER_MISSING", code));
    }

    Result<std::monostate, DynamicFieldError> delete_dynamic_field(std::string_view code) {
        auto r = derived().memory_resource();
        return Result<std::monostate, DynamicFieldError>::err(
            r, DynamicFieldError(r, "DYNAMIC_FIELD_CARRIER_MISSING", code));
    }

    Result<std::monostate, EntityError> install_loaded_dynamic_fields(const DynamicFieldValues&,
                                                                      LoadedSnapshot::Borrow) {
        auto r = derived().memory_resource();
        return Result<std::monostate, EntityError>::err(
            r,
            EntityError(std::allocator_arg,
                        Allocator(r),
                        Derived::ENTITY_NAME,
                        "entity lacks the runtime-owned indexed dynamic-field carrier"));
    }

    static void register_into(EntityDescriptorStore& store) {
        store.register_descriptor(Derived::entity_descriptor(store.memory_resource()));
    }

    static Result<Derived, EntityError>
    from_compact_row_with_context(Memory r, CompactRow row, AnyRef context) {
        auto result = Derived::from_compact_row(r, std::move(row));
        if (result) {
            result.value().on_loaded(context);
        }
        return result;
    }

    bool is_field_loaded(std::string_view) const {
        return true;
    }

    std::optional<LoadedSnapshot::Borrow> loaded_state_snapshot() const {
        return {};
    }

    void set_checker_loaded_fields(FieldSet) {}

    void set_checker_dirty_fields(FieldSet, const MutationValues&) {}

    std::optional<FieldSet> dirty_fields() const {
        return {};
    }

    bool is_marked_as_delete() const {
        return false;
    }

    bool is_new() const {
        return false;
    }

    void mark_as_new() {}

    std::optional<String> get_comment() const {
        return {};
    }

    void set_comment(std::string_view) {}

    std::optional<EntitySnapshot> original_values() const {
        return {};
    }

    void on_loaded(AnyRef) {}

    Audited<Derived> audit_as(std::string_view comment) && {
        return Audited<Derived>(std::allocator_arg,
                                Allocator(derived().memory_resource()),
                                std::move(derived()),
                                comment);
    }

    Json into_json(Memory destination) && {
        EntityJsonTraversal traversal(std::allocator_arg, Allocator(destination));
        if (auto json = std::as_const(derived()).borrowed_json(destination, traversal)) {
            return std::move(*json);
        }
        Record extensions(destination);
        if (const auto* fields = derived().dynamic_field_values()) {
            for (const auto& [code, value] : fields->values()) {
                String name("#", destination);
                name += code;
                extensions.emplace(std::move(name), Value(destination, value));
            }
        }
        auto values = std::move(derived()).into_json_values();
        for (std::string_view name :
             {"_comment", "_dirty_fields", "_original_values", "_is_new", "_is_deleted"}) {
            auto found = values.find(name);
            if (found != values.end()) {
                values.erase(found);
            }
        }
        auto json = Value(values.get_allocator().resource(), values).to_json_value(destination);
        auto& object = std::get<Json::Object>(json.data);
        for (const auto& [name, value] : extensions) {
            object.insert_or_assign(String(name, destination), value.to_json_value(destination));
        }
        return json;
    }

private:
    Derived& derived() {
        return static_cast<Derived&>(*this);
    }
};
template <class T>
concept Entity = TeaqlEntity<T> && requires(T& value,
                                            const T& c,
                                            Memory r,
                                            CompactRow row,
                                            FieldSet fields,
                                            const MutationValues& changes,
                                            AnyRef context) {
    { T::from_compact_row(r, std::move(row)) } -> std::same_as<Result<T, EntityError>>;
    {
        T::from_compact_row_with_context(r, std::move(row), context)
    } -> std::same_as<Result<T, EntityError>>;
    { std::move(value).into_values() } -> std::same_as<MutationValues>;
    { c.is_field_loaded(std::string_view{}) } -> std::same_as<bool>;
    value.set_checker_loaded_fields(std::move(fields));
    value.set_checker_dirty_fields(std::move(fields), changes);
    { c.dirty_fields() } -> std::same_as<std::optional<FieldSet>>;
    { c.is_marked_as_delete() } -> std::same_as<bool>;
    { c.is_new() } -> std::same_as<bool>;
    value.mark_as_new();
    { c.get_comment() } -> std::same_as<std::optional<String>>;
    value.set_comment(std::string_view{});
    { c.original_values() } -> std::same_as<std::optional<EntitySnapshot>>;
    value.on_loaded(context);
    { c.memory_resource() } -> std::same_as<Memory>;
};

template <class T>
class Audited {
public:
    using allocator_type = Allocator;

    template <class U>
        requires std::same_as<std::remove_cvref_t<U>, T>
    Audited(std::allocator_arg_t, Allocator a, U&& entity, std::string_view comment)
        : comment_(validate(a.resource(), comment)),
          inner_(std::make_obj_using_allocator<T>(a, std::forward<U>(entity))) {
        static_assert(Entity<T>);
    }

    Audited(std::allocator_arg_t, Allocator a, const Audited& other)
        requires detail::domain_copyable<T>
        : Audited(std::allocator_arg, a, other.inner_, other.comment_) {}

    Audited(std::allocator_arg_t, Allocator a, Audited&& other)
        : Audited(detail::move_to(a.resource(), std::move(other))) {}

    Audited(const Audited&) = delete;
    Audited(Audited&&) noexcept = default;

    const T& entity() const {
        return inner_;
    }

    T& entity_mut() {
        return inner_;
    }

    std::string_view get_comment() const {
        return comment_;
    }

    Memory memory_resource() const {
        return comment_.get_allocator().resource();
    }

    T into_entity() && {
        inner_.set_comment(comment_);
        return std::move(inner_);
    }

private:
    static String validate(Memory r, std::string_view text) {
        auto trimmed = text;
        detail::trim_start(trimmed);
        if (trimmed.empty()) {
            throw std::invalid_argument("audit comment must not be empty");
        }
        return String(text, r);
    }

    String comment_;
    T inner_;
};

struct BaseEntityData {
    using allocator_type = Allocator;
    std::uint64_t id = 0;
    std::int64_t version = 0;
    Record dynamic_values; // C++ cannot name both a member and accessor 'dynamic'.

    BaseEntityData(std::allocator_arg_t, Allocator a) : dynamic_values(a.resource()) {}

    BaseEntityData(std::allocator_arg_t, Allocator a, const BaseEntityData& other)
        : id(other.id), version(other.version),
          dynamic_values(clone_record(a.resource(), other.dynamic_values)) {}

    BaseEntityData(std::allocator_arg_t, Allocator a, BaseEntityData&& other)
        : BaseEntityData(detail::move_to(a.resource(), std::move(other))) {}

    BaseEntityData(const BaseEntityData&) = delete;
    BaseEntityData(BaseEntityData&&) noexcept = default;

    BaseEntityData& operator=(BaseEntityData&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return dynamic_values.get_allocator().resource();
    }

    BaseEntityData& with_id(std::uint64_t value) {
        id = value;
        return *this;
    }

    BaseEntityData& with_version(std::int64_t value) {
        version = value;
        return *this;
    }

    template <class T>
    BaseEntityData& with_dynamic(std::string_view key, T&& v) {
        put_dynamic(key, std::forward<T>(v));
        return *this;
    }

    const Value* dynamic(std::string_view key) const {
        auto it = dynamic_values.find(key);
        return it == dynamic_values.end() ? nullptr : &it->second;
    }

    std::optional<std::int64_t> dynamic_i64(std::string_view key) const {
        auto v = dynamic(key);
        return v ? v->try_i64() : std::nullopt;
    }

    std::optional<std::uint64_t> dynamic_u64(std::string_view key) const {
        auto v = dynamic(key);
        return v ? v->try_u64() : std::nullopt;
    }

    std::optional<Decimal> dynamic_decimal(std::string_view key) const {
        auto v = dynamic(key);
        return v ? v->try_decimal() : std::nullopt;
    }

    std::optional<double> dynamic_f64(std::string_view key) const {
        auto v = dynamic(key);
        return v ? v->try_f64() : std::nullopt;
    }

    std::optional<std::string_view> dynamic_text(std::string_view key) const {
        auto v = dynamic(key);
        return v ? v->try_text() : std::nullopt;
    }

    std::optional<bool> dynamic_bool(std::string_view key) const {
        auto v = dynamic(key);
        return v ? v->try_bool() : std::nullopt;
    }

    template <class T>
    std::optional<Value> put_dynamic(std::string_view key, T&& v) {
        Value replacement(memory_resource(), std::forward<T>(v));
        auto it = dynamic_values.find(key);
        if (it == dynamic_values.end()) {
            dynamic_values.emplace(String(key, memory_resource()), std::move(replacement));
            return {};
        }
        auto previous = std::move(it->second);
        it->second = std::move(replacement);
        return previous;
    }

    std::optional<Value> remove_dynamic(std::string_view key) {
        auto it = dynamic_values.find(key);
        if (it == dynamic_values.end()) {
            return {};
        }
        auto previous = std::move(it->second);
        dynamic_values.erase(it);
        return previous;
    }

    Record to_values_map(Memory destination) const;
    static Result<BaseEntityData, EntityError> from_values_map(Memory, const Record&);

    bool operator==(const BaseEntityData& other) const {
        return id == other.id && version == other.version && dynamic_values == other.dynamic_values;
    }
};

template <class Derived>
class BaseEntityDefaults : public EntityDefaults<Derived> {
public:
    std::uint64_t id() const {
        return base_data().id;
    }

    void set_id(std::uint64_t value) {
        mutable_data().id = value;
    }

    std::int64_t version_value() const {
        return base_data().version;
    }

    void set_version(std::int64_t value) {
        mutable_data().version = value;
    }

    const Value* dynamic(std::string_view key) const {
        return base_data().dynamic(key);
    }

    auto dynamic_i64(std::string_view key) const {
        return base_data().dynamic_i64(key);
    }

    auto dynamic_u64(std::string_view key) const {
        return base_data().dynamic_u64(key);
    }

    auto dynamic_decimal(std::string_view key) const {
        return base_data().dynamic_decimal(key);
    }

    auto dynamic_f64(std::string_view key) const {
        return base_data().dynamic_f64(key);
    }

    auto dynamic_text(std::string_view key) const {
        return base_data().dynamic_text(key);
    }

    auto dynamic_bool(std::string_view key) const {
        return base_data().dynamic_bool(key);
    }

    template <class T>
    auto put_dynamic(std::string_view key, T&& value) {
        return mutable_data().put_dynamic(key, std::forward<T>(value));
    }

private:
    const BaseEntityData& base_data() const {
        return static_cast<const Derived&>(*this).base();
    }

    BaseEntityData& mutable_data() {
        return static_cast<Derived&>(*this).base_mut();
    }
};
template <class T>
concept BaseEntity = Entity<T> && requires(T& entity, const T& c) {
    { c.base() } -> std::same_as<const BaseEntityData&>;
    { entity.base_mut() } -> std::same_as<BaseEntityData&>;
    { c.id() } -> std::same_as<std::uint64_t>;
    { c.version_value() } -> std::same_as<std::int64_t>;
    entity.set_id(std::uint64_t{});
    entity.set_version(std::int64_t{});
};
template <class T>
concept IdentifiableEntity = Entity<T> && requires(const T& entity) {
    { entity.id_value() } -> std::same_as<Value>;
};
template <class T>
concept VersionedEntity = Entity<T> && requires(const T& entity) {
    { entity.version() } -> std::same_as<std::int64_t>;
};
template <class T>
concept TeaqlBoxedRelations =
    requires(EntityDescriptor& d, Memory r, const CompactRow& row, T&& relations, Record& values) {
        T::extend_descriptor(d);
        { T::extract_from_values(r, row) } -> std::same_as<Result<T, EntityError>>;
        std::move(relations).inject_into_values(values);
    };

// Rust's Box<T> delegation, with an explicit Context allocation domain.
template <TeaqlBoxedRelations T>
class BoxedRelations {
public:
    using allocator_type = Allocator;

    template <class U>
        requires std::same_as<std::remove_cvref_t<U>, T>
    BoxedRelations(std::allocator_arg_t, Allocator a, U&& value)
        : inner_(detail::make_box<T>(a.resource(), std::forward<U>(value))) {}

    BoxedRelations(std::allocator_arg_t, Allocator a, const BoxedRelations& other)
        requires detail::domain_copyable<T>
        : BoxedRelations(std::allocator_arg, a, *other.inner_) {}

    BoxedRelations(std::allocator_arg_t, Allocator a, BoxedRelations&& other)
        : BoxedRelations(detail::move_to(a.resource(), std::move(other))) {}

    BoxedRelations(const BoxedRelations&) = delete;
    BoxedRelations(BoxedRelations&&) noexcept = default;

    T& get() {
        return *inner_;
    }

    const T& get() const {
        return *inner_;
    }

    Memory memory_resource() const {
        return inner_.get_deleter().resource;
    }

    static void extend_descriptor(EntityDescriptor& descriptor) {
        T::extend_descriptor(descriptor);
    }

    static Result<BoxedRelations, EntityError> extract_from_values(Memory r,
                                                                   const CompactRow& row) {
        using Outcome = Result<BoxedRelations, EntityError>;
        auto result = T::extract_from_values(r, row);
        if (!result) {
            return Outcome::err(r, std::move(result).error());
        }
        return Outcome::ok(
            r, BoxedRelations(std::allocator_arg, Allocator(r), std::move(result).value()));
    }

    void inject_into_values(Record& values) && {
        std::move(*inner_).inject_into_values(values);
    }

private:
    detail::ScopedBox<T> inner_;
};
}
