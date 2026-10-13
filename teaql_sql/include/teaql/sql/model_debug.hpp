#pragma once
#include <teaql/data_service/sql_log_debug.hpp>
#include <teaql/sql/bindings.hpp>
#include <teaql/sql/error_debug.hpp>
#include <teaql/sql/dialect.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const SchemaIndexSpec& value) {
    writer.structure("SchemaIndexSpec", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            out.text(value.name);
        });
        fields.field("table", [&](auto& out) {
            out.text(value.table);
        });
        fields.field("columns", [&](auto& out) {
            out.sequence(value.columns, DefaultDebugElement{});
        });
        fields.field("unique", [&](auto& out) {
            out.boolean(value.unique);
        });
    });
}

inline void write_debug(DebugWriter& writer, DatabaseKind value) {
    constexpr std::string_view names[]{"PostgreSql", "Sqlite", "MySql"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown database diagnostic variant");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, const SqlBindings& value) {
    writer.structure("SqlBindings", [&](auto& fields) {
        fields.field("values", [&](auto& out) {
            out.sequence(value.values_, DefaultDebugElement{});
        });
        fields.field("policies", [&](auto& out) {
            out.sequence(value.policies_, DefaultDebugElement{});
        });
        fields.field("originals", [&](auto& out) {
            out.sequence(value.originals_, [](auto& element, const auto& original) {
                element.raw("(");
                write_debug(element, original.first);
                element.raw(", ");
                write_debug(element, original.second);
                element.raw(")");
            });
        });
        fields.field("scope", [&](auto& out) {
            detail::write_sql_log_optional(out, value.scope_);
        });
        fields.field("untrusted_sql", [&](auto& out) {
            out.boolean(value.untrusted_sql_);
        });
    });
}

inline void write_debug(DebugWriter& writer, const CompiledQuery& value) {
    writer.structure("CompiledQuery", [&](auto& fields) {
        fields.field("log_context", [&](auto& out) {
            write_debug(out, value.log_context);
        });
        fields.field("sql", [&](auto& out) {
            out.text(value.sql);
        });
        fields.field("params", [&](auto& out) {
            out.sequence(value.params, DefaultDebugElement{});
        });
        fields.field("comment", [&](auto& out) {
            detail::write_sql_log_optional(out, value.comment);
        });
    });
}
}
