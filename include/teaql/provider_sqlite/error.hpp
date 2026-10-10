#pragma once
#include <teaql/sql/types.hpp>
#include <teaql/provider_sqlite/driver_error.hpp>

namespace teaql {
// Provider failures stay distinct from SqlExecutorError's transport wrapper.
// All retained error text belongs to the explicitly supplied Context.
class MutationExecutorError {
public:
    using allocator_type = Allocator;
    enum class Kind { Sqlite, SqlCompile, UnsupportedValue, UnsupportedColumnType, Bind, Lock };
    static MutationExecutorError sqlite(Memory r, int extended_code, std::string_view message);
    static MutationExecutorError sqlite(Memory r, SqliteDriverError error);
    static MutationExecutorError sql_compile(Memory r, SqlCompileError error);
    static MutationExecutorError unsupported_value(Memory r, std::string_view kind);
    static MutationExecutorError unsupported_column_type(Memory r, std::string_view kind);
    static MutationExecutorError bind(Memory r, std::string_view message);
    static MutationExecutorError lock(Memory r, std::string_view message);
    MutationExecutorError(std::allocator_arg_t, Allocator, const MutationExecutorError&);

    MutationExecutorError(std::allocator_arg_t, Allocator a, MutationExecutorError&& other)
        : MutationExecutorError(detail::move_to(a.resource(), std::move(other))) {}

    MutationExecutorError(const MutationExecutorError&) = delete;
    MutationExecutorError(MutationExecutorError&&) noexcept = default;

    MutationExecutorError& operator=(MutationExecutorError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Kind kind() const noexcept {
        return kind_;
    }

    int sqlite_extended_code() const noexcept {
        return sqlite_code_;
    }

    std::string_view detail() const noexcept {
        return message_;
    }

    const SqlCompileError& compile_error() const {
        return compile_.value();
    }

    const SqliteDriverError& driver_error() const {
        return driver_.value();
    }

    Memory memory_resource() const noexcept {
        return message_.get_allocator().resource();
    }

    String to_string(Memory) const;

    const std::exception* source() const noexcept {
        return nullptr;
    }

private:
    MutationExecutorError(Memory r, Kind kind, std::string_view message)
        : kind_(kind), message_(message, r) {}

    Kind kind_;
    String message_;
    std::optional<SqlCompileError> compile_;
    std::optional<SqliteDriverError> driver_;
    int sqlite_code_ = 0;
};
}
