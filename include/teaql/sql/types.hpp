#pragma once
#include <teaql/data_service/sql_log.hpp>
#include <teaql/core/result.hpp>
#include <teaql/memory/function_ref.hpp>

namespace teaql {
// Discriminators are retained; this edge implementation supplies SQLite only.
enum class DatabaseKind { PostgreSql, Sqlite, MySql };
using SqlRenderResult = Result<String, std::string_view>;
SqlRenderResult render_sql_value(Memory, const Value&, DatabaseKind);
SqlRenderResult render_sql_with(
    Memory, std::string_view, DatabaseKind, std::size_t, FunctionRef<SqlRenderResult(std::size_t)>);

struct CompiledQuery {
    using allocator_type = Allocator;
    SqlLogContext log_context;
    String sql;
    Vector<Value> params;
    std::optional<String> comment;

    CompiledQuery(std::allocator_arg_t, Allocator a)
        : log_context(a.resource()), sql(a.resource()), params(a.resource()) {}

    CompiledQuery(std::allocator_arg_t, Allocator a, const CompiledQuery& other)
        : log_context(std::allocator_arg, a, other.log_context), sql(other.sql, a.resource()),
          params(other.params, a.resource()),
          comment(detail::clone_in(a.resource(), other.comment)) {}

    CompiledQuery(std::allocator_arg_t, Allocator a, CompiledQuery&& other)
        : CompiledQuery(detail::move_to(a.resource(), std::move(other))) {}

    CompiledQuery(const CompiledQuery&) = delete;
    CompiledQuery(CompiledQuery&&) noexcept = default;

    CompiledQuery& operator=(CompiledQuery&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return sql.get_allocator().resource();
    }

    String sql_with_comment(Memory) const;
    // Diagnostic interpolation only. Runtime must project privacy policies first;
    // provider execution retains the parameterized SQL and independent bindings.
    String debug_sql(Memory, DatabaseKind) const;
    bool operator==(const CompiledQuery&) const = default;
};

struct SqlCompileError {
    enum class Code {
        UnknownEntity,
        UnknownField,
        EmptyInList,
        MissingIdProperty,
        MissingVersionProperty,
        EmptyMutation,
        InvalidRecoverVersion,
        UnsupportedSchemaType,
        InvalidSchemaShape,
        SchemaIdentifierTooLong,
        InvalidFunctionArguments,
        InvalidSubQueryOperator
    };
    using allocator_type = Allocator;
    Code code;
    String name, reason, identifier;
    std::int64_t version = 0;
    DataType data_type = DataType::Text;
    std::size_t actual_bytes = 0, max_bytes = 0;

    SqlCompileError(std::allocator_arg_t, Allocator a, Code code, std::string_view name = {})
        : code(code), name(name, a.resource()), reason(a.resource()), identifier(a.resource()) {}

    SqlCompileError(std::allocator_arg_t, Allocator a, const SqlCompileError& other)
        : code(other.code), name(other.name, a.resource()), reason(other.reason, a.resource()),
          identifier(other.identifier, a.resource()), version(other.version),
          data_type(other.data_type), actual_bytes(other.actual_bytes), max_bytes(other.max_bytes) {
    }

    SqlCompileError(std::allocator_arg_t, Allocator a, SqlCompileError&& other)
        : SqlCompileError(detail::move_to(a.resource(), std::move(other))) {}

    SqlCompileError(const SqlCompileError&) = delete;
    SqlCompileError(SqlCompileError&&) noexcept = default;

    SqlCompileError& operator=(SqlCompileError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    String to_string(Memory) const;
    bool operator==(const SqlCompileError&) const = default;
};
}
