#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/runtime/check_result.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const LocationSegment& segment) {
    writer.tuple(segment.kind() == LocationSegment::Kind::Member ? "Member" : "Index",
                 [&](auto& out) {
                     if (segment.kind() == LocationSegment::Kind::Member) {
                         out.text(std::get<String>(segment.data));
                     } else {
                         out.integer(std::get<std::size_t>(segment.data));
                     }
                 });
}

inline void write_debug(DebugWriter& writer, const ObjectLocation& location) {
    writer.structure("ObjectLocation", [&](auto& fields) {
        fields.field("segments", [&](auto& out) {
            out.sequence(location.segments(), [](auto& inner, const auto& segment) {
                write_debug(inner, segment);
            });
        });
    });
}

inline void write_debug(DebugWriter& writer, const CheckResult& result) {
    auto optional = [](auto& out, const auto& value) {
        if (value) {
            out.tuple("Some", [&](auto& inner) {
                write_debug(inner, *value);
            });
        } else {
            out.raw("None");
        }
    };
    writer.structure("CheckResult", [&](auto& fields) {
        fields.field("rule", [&](auto& out) {
            out.raw(debug_name(result.rule));
        });
        fields.field("location", [&](auto& out) {
            write_debug(out, result.location);
        });
        fields.field("input_value", [&](auto& out) {
            optional(out, result.input_value);
        });
        fields.field("system_value", [&](auto& out) {
            optional(out, result.system_value);
        });
        fields.field("message", [&](auto& out) {
            optional(out, result.message);
        });
        fields.field("entity_type", [&](auto& out) {
            optional(out, result.entity_type);
        });
        fields.field("source_instance_path", [&](auto& out) {
            optional(out, result.source_instance_path);
        });
    });
}
}
