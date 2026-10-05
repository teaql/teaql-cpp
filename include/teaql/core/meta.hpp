#pragma once
#include <teaql/core/value.hpp>
#include <stdexcept>
#include <ranges>

namespace teaql {
String default_table_name(Memory, std::string_view);

struct PropertyDescriptor {
    using allocator_type = Allocator;
    String name;
    DataType data_type;
    String column_name;
    bool nullable = true, is_id = false, is_version = false;
    std::optional<std::uint32_t> max_length, numeric_precision, numeric_scale;

    PropertyDescriptor(Memory r, std::string_view name, DataType type, std::string_view column = {})
        : name(name, r), data_type(type), column_name(column.empty() ? name : column, r) {}

    PropertyDescriptor(std::allocator_arg_t,
                       Allocator a,
                       std::string_view name,
                       DataType type,
                       std::string_view column = {})
        : PropertyDescriptor(a.resource(), name, type, column) {}

    PropertyDescriptor(std::allocator_arg_t, Allocator a, const PropertyDescriptor& other)
        : PropertyDescriptor(a.resource(), other.name, other.data_type, other.column_name) {
        column_name = other.column_name; // Preserve an explicitly empty mapping when cloning.
        nullable = other.nullable;
        is_id = other.is_id;
        is_version = other.is_version;
        max_length = other.max_length;
        numeric_precision = other.numeric_precision;
        numeric_scale = other.numeric_scale;
    }

    PropertyDescriptor(const PropertyDescriptor&) = delete;
    PropertyDescriptor(PropertyDescriptor&&) noexcept = default;
    bool operator==(const PropertyDescriptor&) const = default;

    PropertyDescriptor& not_null() {
        nullable = false;
        return *this;
    }

    PropertyDescriptor& id() {
        is_id = true;
        return *this;
    }

    PropertyDescriptor& version() {
        is_version = true;
        return *this;
    }

    PropertyDescriptor& column(std::string_view value) {
        column_name = value;
        return *this;
    }

    PropertyDescriptor& with_max_length(std::uint32_t value) {
        max_length = value;
        return *this;
    }

    PropertyDescriptor& with_numeric_precision(std::uint32_t value) {
        numeric_precision = value;
        return *this;
    }

    PropertyDescriptor& with_numeric_scale(std::uint32_t value) {
        numeric_scale = value;
        return *this;
    }
};

struct RelationDescriptor {
    using allocator_type = Allocator;
    String name, target_entity, local_key, foreign_key;
    bool many = false, attach = true, delete_missing = true;

    RelationDescriptor(std::allocator_arg_t,
                       Allocator a,
                       std::string_view name,
                       std::string_view target)
        : name(name, a.resource()), target_entity(target, a.resource()),
          local_key("id", a.resource()), foreign_key("id", a.resource()) {}

    RelationDescriptor(std::allocator_arg_t, Allocator a, const RelationDescriptor& other)
        : RelationDescriptor(std::allocator_arg, a, other.name, other.target_entity) {
        local_key = other.local_key;
        foreign_key = other.foreign_key;
        many = other.many;
        attach = other.attach;
        delete_missing = other.delete_missing;
    }

    RelationDescriptor(const RelationDescriptor&) = delete;
    RelationDescriptor(RelationDescriptor&&) noexcept = default;
    bool operator==(const RelationDescriptor&) const = default;

    RelationDescriptor& local(std::string_view key) {
        local_key = key;
        return *this;
    }

    RelationDescriptor& foreign(std::string_view key) {
        foreign_key = key;
        return *this;
    }

    RelationDescriptor& to_many() {
        many = true;
        return *this;
    }

    RelationDescriptor& attached() {
        attach = true;
        return *this;
    }

    RelationDescriptor& detached() {
        attach = false;
        return *this;
    }

    RelationDescriptor& remove_missing() {
        delete_missing = true;
        return *this;
    }

    RelationDescriptor& keep_missing() {
        delete_missing = false;
        return *this;
    }
};

struct EntityDescriptor {
    using allocator_type = Allocator;
    String name, table_name;
    std::optional<String> data_service;
    Vector<PropertyDescriptor> properties;
    Vector<RelationDescriptor> relations;
    Vector<String> audit_mask_fields;
    bool audit_mask_fields_declared = false;
    std::optional<std::size_t> audit_value_max_len;

    EntityDescriptor(std::allocator_arg_t, Allocator a, std::string_view name)
        : name(name, a.resource()), table_name(default_table_name(a.resource(), name)),
          properties(a.resource()), relations(a.resource()), audit_mask_fields(a.resource()) {}

    EntityDescriptor(std::allocator_arg_t, Allocator a, const EntityDescriptor& other)
        : EntityDescriptor(std::allocator_arg, a, other.name) {
        table_name = other.table_name;
        if (other.data_service)
            data_service.emplace(*other.data_service, a.resource());
        for (const auto& property : other.properties)
            properties.emplace_back(property);
        for (const auto& relation : other.relations)
            relations.emplace_back(relation);
        for (const auto& field : other.audit_mask_fields)
            audit_mask_fields.emplace_back(field);
        audit_mask_fields_declared = other.audit_mask_fields_declared;
        audit_value_max_len = other.audit_value_max_len;
    }

    EntityDescriptor(const EntityDescriptor&) = delete;
    EntityDescriptor(EntityDescriptor&&) noexcept = default;
    EntityDescriptor& operator=(const EntityDescriptor&) = delete;
    bool operator==(const EntityDescriptor&) const = default;

    EntityDescriptor& table(std::string_view value) {
        table_name = value;
        return *this;
    }

    EntityDescriptor& service(std::string_view value) {
        String copy(value, memory_resource());
        data_service.emplace(std::move(copy));
        return *this;
    }

    PropertyDescriptor&
    property(std::string_view field, DataType type, std::string_view column = {}) {
        properties.emplace_back(field, type, column);
        return properties.back();
    }

    EntityDescriptor& property(const PropertyDescriptor& value) {
        properties.emplace_back(value);
        return *this;
    }

    RelationDescriptor& relation(std::string_view name, std::string_view target) {
        relations.emplace_back(name, target);
        return relations.back();
    }

    EntityDescriptor& relation(const RelationDescriptor& value) {
        relations.emplace_back(value);
        return *this;
    }

    EntityDescriptor& mask_fields(std::initializer_list<std::string_view> names) {
        Vector<String> copy(memory_resource());
        for (auto name : names)
            copy.emplace_back(name);
        audit_mask_fields = std::move(copy);
        audit_mask_fields_declared = true;
        return *this;
    }

    EntityDescriptor& audit_max_length(std::optional<std::size_t> value) {
        audit_value_max_len = value;
        return *this;
    }

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    const PropertyDescriptor* find_property(std::string_view) const noexcept;
    const RelationDescriptor* find_relation(std::string_view) const noexcept;

    const PropertyDescriptor* property_by_name(std::string_view field) const noexcept {
        return find_property(field);
    }

    const RelationDescriptor* relation_by_name(std::string_view name) const noexcept {
        return find_relation(name);
    }

    const PropertyDescriptor* id_property() const noexcept;
    const PropertyDescriptor* version_property() const noexcept;

    auto writable_properties() const {
        return properties | std::views::filter([](const auto& property) {
                   return !property.is_id;
               });
    }

    // Required lookups are explicit at consumers such as SQL compilation.
    const PropertyDescriptor& require_property(std::string_view) const;
    const RelationDescriptor& require_relation(std::string_view) const;
    const PropertyDescriptor& require_id_property() const;
    const PropertyDescriptor& require_version_property() const;
};
}
