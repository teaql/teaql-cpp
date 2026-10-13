#pragma once
#include <teaql/core/dynamic_fields.hpp>
#include <teaql/core/debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const DynamicFieldDefinitions& value) {
    writer.structure("DynamicFieldDefinitions", [&](auto& fields) {
        fields.field("owner_type", [&](auto& out) {
            out.text(value.owner_type());
        });
        fields.field("revision", [&](auto& out) {
            out.text(value.revision());
        });
        fields.field("fields", [&](auto& out) {
            out.sequence(
                value.fields(),
                [](auto& element, const auto& field) {
                    element.text(field.first);
                    element.raw(": ");
                    write_debug(element, field.second);
                },
                "{",
                "}");
        });
        fields.field("storage_binding", [&](auto& out) {
            if (value.executor_) {
                out.tuple("Some", [](auto& inner) {
                    inner.raw("DynamicFieldStorageBinding(<opaque>)");
                });
            } else {
                out.raw("None");
            }
        });
    });
}

inline void write_debug(DebugWriter& writer, const DynamicFieldValues& value) {
    writer.structure("DynamicFieldValues", [&](auto& fields) {
        fields.field("definitions", [&](auto& out) {
            write_debug(out, value.definitions());
        });
        fields.field("selected_codes", [&](auto& out) {
            out.sequence(
                value.selected_codes(),
                [](auto& element, const auto& code) {
                    element.text(code);
                },
                "{",
                "}");
        });
        fields.field("values", [&](auto& out) {
            out.sequence(
                value.values(),
                [](auto& element, const auto& field) {
                    element.text(field.first);
                    element.raw(": ");
                    write_debug(element, field.second);
                },
                "{",
                "}");
        });
    });
}
}
