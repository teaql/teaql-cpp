#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/memory/scoped.hpp>
#include <teaql/provider_sqlite/native_diagnostics.hpp>
#include <teaql/provider_sqlite/simple_error.hpp>
#include <teaql/provider_sqlite/boxed_error.hpp>

namespace teaql {
// The native result code and its source category are separate observables.
struct SqliteNativeError {
    enum class Code {
        InternalMalfunction,
        PermissionDenied,
        OperationAborted,
        DatabaseBusy,
        DatabaseLocked,
        OutOfMemory,
        ReadOnly,
        OperationInterrupted,
        SystemIoFailure,
        DatabaseCorrupt,
        NotFound,
        DiskFull,
        CannotOpen,
        FileLockingProtocolFailed,
        SchemaChanged,
        TooBig,
        ConstraintViolation,
        TypeMismatch,
        ApiMisuse,
        NoLargeFileSupport,
        AuthorizationForStatementDenied,
        ParameterOutOfRange,
        NotADatabase,
        Unknown
    };
    Code code;
    int extended_code;
    static SqliteNativeError from_extended_code(int code) noexcept;
    String to_string(Memory) const;

    const std::exception* source() const noexcept {
        return nullptr;
    }

    friend bool operator==(const SqliteNativeError&, const SqliteNativeError&) = default;
};

// Structured driver errors retain each enabled source tag and concrete payload.
// Explicit destination copies preserve scoped ownership without shared lifetimes.
class SqliteDriverError {
public:
    using allocator_type = Allocator;

    struct NativeFailure {
        SqliteNativeError error;
        std::optional<String> message;
    };

    struct SqlInputError {
        SqliteNativeError error;
        String message;
        String sql;
        int offset;
    };

    struct ParameterCount {
        std::size_t given;
        std::size_t expected;
    };

    struct MultipleStatement {};

    struct ExecuteReturnedResults {};

    struct InvalidColumnIndex {
        std::size_t index;
    };

    struct InvalidColumnType {
        std::size_t index;
        String name;
        SqliteValueType type;
    };

    struct Utf8Error {
        std::size_t index;
        SqliteUtf8Error error;
    };

    struct NulError {
        SqliteNulError error;
    };

    struct InvalidPath {
        SqlitePath path;
    };

    struct BoxedFailure {
        enum class Kind { FromSqlConversionFailure, ToSqlConversionFailure, UserFunctionError };
        Kind kind;
        std::size_t index;
        SqliteValueType type;
        SqliteBoxedError error;
    };

    using Payload = std::variant<NativeFailure,
                                 SqlInputError,
                                 ParameterCount,
                                 MultipleStatement,
                                 ExecuteReturnedResults,
                                 InvalidColumnIndex,
                                 InvalidColumnType,
                                 Utf8Error,
                                 NulError,
                                 InvalidPath,
                                 detail::SqliteSimpleError,
                                 BoxedFailure>;
    enum class Kind {
        SqliteFailure,
        SqlInputError,
        InvalidParameterCount,
        MultipleStatement,
        ExecuteReturnedResults,
        InvalidColumnIndex,
        InvalidColumnType,
        Utf8Error,
        NulError,
        InvalidPath,
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
        InvalidDatabaseIndex,
        FromSqlConversionFailure,
        ToSqlConversionFailure,
        UserFunctionError
    };

    static SqliteDriverError native_failure(Memory, int, std::optional<std::string_view>);
    static SqliteDriverError
        native_failure(Memory, SqliteNativeError, std::optional<std::string_view>);
    static SqliteDriverError sql_input(Memory, int, std::string_view, std::string_view, int);
    static SqliteDriverError
    sql_input(Memory, SqliteNativeError, std::string_view, std::string_view, int);
    static SqliteDriverError invalid_parameter_count(Memory, std::size_t, std::size_t);
    static SqliteDriverError multiple_statement(Memory);
    static SqliteDriverError execute_returned_results(Memory);
    static SqliteDriverError invalid_column_index(Memory, std::size_t);
    static SqliteDriverError
        invalid_column_type(Memory, std::size_t, std::string_view, SqliteValueType);
    static SqliteDriverError utf8_error(Memory, std::size_t, SqliteUtf8Error);
    static SqliteDriverError nul_error(Memory, SqliteNulError);
    static SqliteDriverError from_utf8_error(Memory, SqliteUtf8Error);
    static SqliteDriverError from_nul_error(Memory, SqliteNulError);
    static SqliteDriverError invalid_path(Memory, SqlitePath);
    static SqliteDriverError sqlite_single_threaded_mode(Memory);
    static SqliteDriverError integral_value_out_of_range(Memory, std::size_t, std::int64_t);
    static SqliteDriverError invalid_parameter_name(Memory, std::string_view);
    static SqliteDriverError query_returned_no_rows(Memory);
    static SqliteDriverError query_returned_more_than_one_row(Memory);
    static SqliteDriverError invalid_column_name(Memory, std::string_view);
    static SqliteDriverError statement_changed_rows(Memory, std::size_t);
    static SqliteDriverError invalid_function_parameter_type(Memory, std::size_t, SqliteValueType);
    static SqliteDriverError invalid_query(Memory);
    static SqliteDriverError unwinding_panic(Memory);
    static SqliteDriverError get_aux_wrong_type(Memory);
    static SqliteDriverError invalid_database_index(Memory, std::size_t);
    static SqliteDriverError
        from_sql_conversion_failure(Memory, std::size_t, SqliteValueType, SqliteBoxedError);
    static SqliteDriverError to_sql_conversion_failure(Memory, SqliteBoxedError);
    static SqliteDriverError user_function_error(Memory, SqliteBoxedError);
    SqliteDriverError(std::allocator_arg_t, Allocator, const SqliteDriverError&);

    SqliteDriverError(std::allocator_arg_t, Allocator a, SqliteDriverError&& other)
        : SqliteDriverError(detail::move_to(a.resource(), std::move(other))) {}

    SqliteDriverError(const SqliteDriverError&) = delete;
    SqliteDriverError(SqliteDriverError&&) noexcept = default;

    SqliteDriverError& operator=(SqliteDriverError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

    Kind kind() const noexcept {
        if (auto* scalar = std::get_if<detail::SqliteSimpleError>(&data_)) {
            return static_cast<Kind>(static_cast<std::size_t>(Kind::SqliteSingleThreadedMode) +
                                     static_cast<std::size_t>(scalar->kind()));
        }
        if (auto* boxed = std::get_if<BoxedFailure>(&data_)) {
            return static_cast<Kind>(static_cast<std::size_t>(Kind::FromSqlConversionFailure) +
                                     static_cast<std::size_t>(boxed->kind));
        }
        return static_cast<Kind>(data_.index());
    }

    const Payload& payload() const noexcept {
        return data_;
    }

    const ParameterCount& parameter_count() const {
        return std::get<ParameterCount>(data_);
    }

    const SqlInputError& sql_input_error() const {
        return std::get<SqlInputError>(data_);
    }

    const detail::SqliteSimpleError& simple_error() const {
        return std::get<detail::SqliteSimpleError>(data_);
    }

    // Source accessors expose codes only for SqliteFailure, not SqlInputError.
    const SqliteNativeError* sqlite_error() const noexcept;
    std::optional<SqliteNativeError::Code> sqlite_error_code() const noexcept;
    std::optional<int> sqlite_extended_error_code() const noexcept;
    SqliteErrorCauseView source() const noexcept;
    String to_string(Memory) const;
    friend bool operator==(const SqliteDriverError&, const SqliteDriverError&);

private:
    SqliteDriverError(Memory r, Payload data) : resource_(r), data_(std::move(data)) {}

    Memory resource_;
    Payload data_;
};

void write_debug(DebugWriter&, SqliteNativeError::Code);
void write_debug(DebugWriter&, const SqliteNativeError&);
void write_debug(DebugWriter&, SqliteValueType);
void write_debug(DebugWriter&, const SqliteDriverError&);
}
