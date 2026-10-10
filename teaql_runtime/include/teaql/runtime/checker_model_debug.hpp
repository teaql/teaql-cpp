#pragma once
#include <teaql/runtime/checker.hpp>
#include <teaql/runtime/check_result_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, CheckObjectStatus value) {
    switch (value) {
    case CheckObjectStatus::Create:
        writer.raw("Create");
        return;
    case CheckObjectStatus::Update:
        writer.raw("Update");
        return;
    case CheckObjectStatus::Unknown:
        writer.raw("Unknown");
        return;
    }
    throw std::invalid_argument("Unknown check object status");
}

inline void write_debug(DebugWriter& writer, CheckRule value) {
    writer.raw(debug_name(value));
}

inline void write_debug(DebugWriter& writer, JsonFieldNamingProfile value) {
    switch (value) {
    case JsonFieldNamingProfile::CamelCase:
        writer.raw("CamelCase");
        return;
    case JsonFieldNamingProfile::SnakeCase:
        writer.raw("SnakeCase");
        return;
    case JsonFieldNamingProfile::PascalCase:
        writer.raw("PascalCase");
        return;
    }
    throw std::invalid_argument("Unknown JSON field naming profile");
}

inline void write_debug(DebugWriter& writer, const WireLocationSegment& value) {
    if (value.kind() == WireLocationSegment::Kind::Property) {
        writer.structure("Property", [&](auto& fields) {
            fields.field("name", [&](auto& out) {
                out.text(std::get<String>(value.data));
            });
        });
    } else {
        writer.structure("Index", [&](auto& fields) {
            fields.field("index", [&](auto& out) {
                out.integer(std::get<std::size_t>(value.data));
            });
        });
    }
}

inline void write_debug(DebugWriter& writer, const WireCheckResult& value) {
    auto optional = [](auto& out, const auto& item) {
        if (item) {
            out.tuple("Some", [&](auto& inner) {
                write_debug(inner, *item);
            });
        } else {
            out.raw("None");
        }
    };
    writer.structure("WireCheckResult", [&](auto& fields) {
        fields.field("rule_id", [&](auto& out) {
            out.text(value.rule_id);
        });
        fields.field("entity_type", [&](auto& out) {
            optional(out, value.entity_type);
        });
        fields.field("location", [&](auto& out) {
            out.sequence(value.location, [](auto& inner, const auto& segment) {
                write_debug(inner, segment);
            });
        });
        fields.field("instance_path", [&](auto& out) {
            out.text(value.instance_path);
        });
        fields.field("source_instance_path", [&](auto& out) {
            optional(out, value.source_instance_path);
        });
        fields.field("input_value", [&](auto& out) {
            optional(out, value.input_value);
        });
        fields.field("system_value", [&](auto& out) {
            optional(out, value.system_value);
        });
        fields.field("message", [&](auto& out) {
            optional(out, value.message);
        });
    });
}
}
