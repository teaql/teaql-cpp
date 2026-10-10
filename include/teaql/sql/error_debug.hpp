#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/sql/types.hpp>
#include <teaql/sql/error.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, const SqlCompileError& error) {
    using Code = SqlCompileError::Code;
    if (static_cast<std::size_t>(error.code) >= 12) {
        throw std::invalid_argument("Unknown SQL compile error diagnostic variant");
    }
    if (error.code == Code::EmptyInList) {
        writer.raw("EmptyInList");
        return;
    }
    if (error.code == Code::InvalidSchemaShape) {
        writer.structure("InvalidSchemaShape", [&](auto& fields) {
            fields.field("property", [&](auto& out) {
                out.text(error.name);
            });
            fields.field("reason", [&](auto& out) {
                out.text(error.reason);
            });
        });
        return;
    }
    if (error.code == Code::SchemaIdentifierTooLong) {
        writer.structure("SchemaIdentifierTooLong", [&](auto& fields) {
            fields.field("object", [&](auto& out) {
                out.text(error.name);
            });
            fields.field("identifier", [&](auto& out) {
                out.text(error.identifier);
            });
            fields.field("actual_bytes", [&](auto& out) {
                out.integer(error.actual_bytes);
            });
            fields.field("max_bytes", [&](auto& out) {
                out.integer(error.max_bytes);
            });
        });
        return;
    }
    constexpr std::string_view names[]{"UnknownEntity",
                                       "UnknownField",
                                       "EmptyInList",
                                       "MissingIdProperty",
                                       "MissingVersionProperty",
                                       "EmptyMutation",
                                       "InvalidRecoverVersion",
                                       "UnsupportedSchemaType",
                                       "InvalidSchemaShape",
                                       "SchemaIdentifierTooLong",
                                       "InvalidFunctionArguments",
                                       "InvalidSubQueryOperator"};
    writer.tuple(names[static_cast<std::size_t>(error.code)], [&](auto& out) {
        if (error.code == Code::InvalidRecoverVersion) {
            out.integer(error.version);
        } else if (error.code == Code::UnsupportedSchemaType) {
            write_debug(out, error.data_type);
        } else {
            out.text(error.name);
        }
    });
}

template <class E, class Element>
void write_debug(DebugWriter& writer, const SqlExecutorError<E>& error, Element&& element) {
    using Kind = typename SqlExecutorError<E>::Kind;
    constexpr std::string_view names[]{"Compile", "Transport", "PersistedRecord"};
    writer.tuple(names[static_cast<std::size_t>(error.kind())], [&](auto& out) {
        if (error.kind() == Kind::Compile) {
            write_debug(out, error.compile_error());
        } else if (error.kind() == Kind::Transport) {
            std::invoke(std::forward<Element>(element), out, error.transport_error());
        } else {
            out.text(error.persisted_record_error());
        }
    });
}

template <class E>
    requires requires(DebugWriter& writer, const E& value) { write_debug(writer, value); }
void write_debug(DebugWriter& writer, const SqlExecutorError<E>& error) {
    write_debug(writer, error, [](auto& out, const E& value) {
        using teaql::write_debug;
        write_debug(out, value);
    });
}
}
