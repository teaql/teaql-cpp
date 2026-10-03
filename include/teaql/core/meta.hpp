#pragma once
#include <teaql/core/value.hpp>
#include <stdexcept>

namespace teaql {
String default_table_name(Memory, std::string_view);
struct PropertyDescriptor {
    String name;
    DataType data_type;
    String column_name;
    bool nullable = true, is_id = false, is_version = false;
    PropertyDescriptor(Memory r, std::string_view name, DataType type, std::string_view column)
        : name(name, r), data_type(type), column_name(column.empty() ? name : column, r) {}
    PropertyDescriptor& not_null() { nullable = false; return *this; }
    PropertyDescriptor& id() { is_id = true; return *this; }
    PropertyDescriptor& version() { is_version = true; return *this; }
};
struct EntityDescriptor {
    using allocator_type = Allocator;
    String name, table_name;
    Vector<PropertyDescriptor> properties;
    Vector<String> audit_mask_fields;
    bool audit_mask_fields_declared = false;
    EntityDescriptor(std::allocator_arg_t, Allocator a, std::string_view name)
        : name(name, a.resource()), table_name(default_table_name(a.resource(), name)),
          properties(a.resource()), audit_mask_fields(a.resource()) {}
    EntityDescriptor(const EntityDescriptor&) = delete;
    EntityDescriptor& operator=(const EntityDescriptor&) = delete;
    EntityDescriptor& table(std::string_view v) { table_name = v; return *this; }
    PropertyDescriptor& property(std::string_view field, DataType type, std::string_view column = {}) {
        properties.emplace_back(memory_resource(), field, type, column); return properties.back();
    }
    EntityDescriptor& mask_fields(std::initializer_list<std::string_view> names) {
        audit_mask_fields.clear(); for (auto name : names) audit_mask_fields.emplace_back(name);
        audit_mask_fields_declared = true; return *this;
    }
    Memory memory_resource() const { return name.get_allocator().resource(); }
    const PropertyDescriptor& property_by_name(std::string_view) const;
    const PropertyDescriptor& id_property() const;
    const PropertyDescriptor& version_property() const;
};
}
