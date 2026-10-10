#pragma once
#include <teaql/runtime/relation.hpp>
#include <teaql/core/query_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const RelationLoadPlan& value) {
    writer.structure("RelationLoadPlan", [&](auto& fields) {
        fields.field("parent_entity", [&](auto& out) {
            out.text(value.parent_entity);
        });
        fields.field("relation_name", [&](auto& out) {
            out.text(value.relation_name);
        });
        fields.field("path", [&](auto& out) {
            out.text(value.path);
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
        fields.field("query", [&](auto& out) {
            if (value.query) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.query);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("children", [&](auto& out) {
            out.sequence(value.children, [](auto& inner, const auto& child) {
                write_debug(inner, child);
            });
        });
    });
}
}
