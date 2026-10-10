#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/core/meta.hpp>

namespace teaql {
namespace detail {
template <std::integral T>
void debug_optional_integer(DebugWriter& writer, const std::optional<T>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            out.integer(*value);
        });
    } else {
        writer.raw("None");
    }
}
}

// Raw model diagnostics, separate from safe SQL/audit projection.
inline void write_debug(DebugWriter& writer, const PropertyDescriptor& value) {
    writer.structure("PropertyDescriptor", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            out.text(value.name);
        });
        fields.field("data_type", [&](auto& out) {
            write_debug(out, value.data_type);
        });
        fields.field("nullable", [&](auto& out) {
            out.boolean(value.nullable);
        });
        fields.field("column_name", [&](auto& out) {
            out.text(value.column_name);
        });
        fields.field("is_id", [&](auto& out) {
            out.boolean(value.is_id);
        });
        fields.field("is_version", [&](auto& out) {
            out.boolean(value.is_version);
        });
        fields.field("max_length", [&](auto& out) {
            detail::debug_optional_integer(out, value.max_length);
        });
        fields.field("numeric_precision", [&](auto& out) {
            detail::debug_optional_integer(out, value.numeric_precision);
        });
        fields.field("numeric_scale", [&](auto& out) {
            detail::debug_optional_integer(out, value.numeric_scale);
        });
    });
}

inline void write_debug(DebugWriter& writer, const RelationDescriptor& value) {
    writer.structure("RelationDescriptor", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            out.text(value.name);
        });
        fields.field("target_entity", [&](auto& out) {
            out.text(value.target_entity);
        });
        fields.field("local_key", [&](auto& out) {
            out.text(value.local_key);
        });
        fields.field("foreign_key", [&](auto& out) {
            out.text(value.foreign_key);
        });
        fields.field("many", [&](auto& out) {
            out.boolean(value.many);
        });
        fields.field("attach", [&](auto& out) {
            out.boolean(value.attach);
        });
        fields.field("delete_missing", [&](auto& out) {
            out.boolean(value.delete_missing);
        });
    });
}

inline void write_debug(DebugWriter& writer, const EntityDescriptor& value) {
    writer.structure("EntityDescriptor", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            out.text(value.name);
        });
        fields.field("table_name", [&](auto& out) {
            out.text(value.table_name);
        });
        fields.field("data_service", [&](auto& out) {
            if (value.data_service) {
                out.tuple("Some", [&](auto& inner) {
                    inner.text(*value.data_service);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("properties", [&](auto& out) {
            out.sequence(value.properties, DefaultDebugElement{});
        });
        fields.field("relations", [&](auto& out) {
            out.sequence(value.relations, DefaultDebugElement{});
        });
        fields.field("audit_mask_fields", [&](auto& out) {
            out.sequence(value.audit_mask_fields, DefaultDebugElement{});
        });
        fields.field("audit_mask_fields_declared", [&](auto& out) {
            out.boolean(value.audit_mask_fields_declared);
        });
        fields.field("audit_value_max_len", [&](auto& out) {
            detail::debug_optional_integer(out, value.audit_value_max_len);
        });
    });
}
}
