#pragma once
#include <teaql/provider_sqlite/native_diagnostics.hpp>

namespace teaql {
enum class SqliteValueType { Null, Integer, Real, Text, Blob };
void write_debug(DebugWriter&, SqliteValueType);
}

namespace teaql::detail {
// Compact storage for the locked driver's scalar/text/unit alternatives.
// Driver facade assembly must keep every individual source tag distinct.
class SqliteSimpleError {
public:
    using allocator_type = Allocator;
    enum class Kind {
        SqliteSingleThreadedMode,
        IntegralValueOutOfRange,
        InvalidParameterName,
        QueryReturnedNoRows,
        QueryReturnedMoreThanOneRow,
        InvalidColumnName,
        StatementChangedRows,
        InvalidFunctionParameterType,
        InvalidQuery,
        UnwindingPanic,
        GetAuxWrongType,
        InvalidDatabaseIndex
    };
    static SqliteSimpleError unit(Memory, Kind);
    static SqliteSimpleError integral_range(Memory, std::size_t, std::int64_t);
    static SqliteSimpleError named(Memory, Kind, std::string_view);
    static SqliteSimpleError indexed(Memory, Kind, std::size_t);
    static SqliteSimpleError function_type(Memory, std::size_t, SqliteValueType);
    SqliteSimpleError(std::allocator_arg_t, Allocator, const SqliteSimpleError&);

    SqliteSimpleError(std::allocator_arg_t, Allocator a, SqliteSimpleError&& other)
        : SqliteSimpleError(detail::move_to(a.resource(), std::move(other))) {}

    SqliteSimpleError(const SqliteSimpleError&) = delete;
    SqliteSimpleError(SqliteSimpleError&&) noexcept = default;

    SqliteSimpleError& operator=(SqliteSimpleError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Kind kind() const noexcept {
        return kind_;
    }

    std::size_t index() const noexcept {
        return index_;
    }

    std::int64_t value() const noexcept {
        return value_;
    }

    std::string_view name() const noexcept {
        return name_;
    }

    SqliteValueType value_type() const noexcept {
        return type_;
    }

    Memory memory_resource() const noexcept {
        return name_.get_allocator().resource();
    }

    String to_string(Memory) const;

    const std::exception* source() const noexcept {
        return nullptr;
    }

    friend bool operator==(const SqliteSimpleError&, const SqliteSimpleError&);

private:
    SqliteSimpleError(Memory r, Kind kind) : kind_(kind), name_(r) {}

    Kind kind_;
    String name_;
    std::size_t index_ = 0;
    std::int64_t value_ = 0;
    SqliteValueType type_ = SqliteValueType::Null;
};

void write_debug(DebugWriter&, const SqliteSimpleError&);
}
