#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/runtime/telemetry.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const RuntimeAttributeValue& attribute) {
    std::visit(
        [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            constexpr auto name = std::same_as<T, String>         ? "String"
                                  : std::same_as<T, std::int64_t> ? "Integer"
                                  : std::same_as<T, double>       ? "Float"
                                                                  : "Boolean";
            writer.tuple(name, [&](auto& out) {
                write_debug(out, value);
            });
        },
        attribute.data);
}

inline void write_debug(DebugWriter& writer, const RuntimeOperation& operation) {
    writer.structure("RuntimeOperation", [&](auto& fields) {
        fields.field("family", [&](auto& out) {
            out.text(operation.family);
        });
        fields.field("name", [&](auto& out) {
            out.text(operation.name);
        });
        fields.field("attributes", [&](auto& out) {
            out.sequence(
                operation.attributes,
                [](auto& inner, const auto& pair) {
                    inner.text(pair.first);
                    inner.raw(": ");
                    write_debug(inner, pair.second);
                },
                "{",
                "}");
        });
    });
}
}
