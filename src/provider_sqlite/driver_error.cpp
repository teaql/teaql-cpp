#include <teaql/provider_sqlite/driver_error.hpp>
#include <sqlite3.h>

namespace teaql {
namespace {
template <class T>
void number(String& out, T value) {
    char buffer[32];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc{}) {
        throw std::length_error("SQLite diagnostic integer conversion failed");
    }
    out.append(buffer, result.ptr);
}

std::string_view type_name(SqliteValueType type) {
    constexpr std::string_view names[]{"Null", "Integer", "Real", "Text", "Blob"};
    auto index = static_cast<std::size_t>(type);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown SQLite value type");
    }
    return names[index];
}

void next_tuple_field(DebugWriter& writer) {
    writer.raw(writer.options().pretty ? ",\n" : ", ");
}
}

SqliteNativeError SqliteNativeError::from_extended_code(int code) noexcept {
    // SQLite primary codes 2..26 match the source category order; EMPTY (16)
    // has no dedicated source category, and SQL ERROR (1) maps to Unknown.
    constexpr Code categories[]{Code::Unknown,
                                Code::Unknown,
                                Code::InternalMalfunction,
                                Code::PermissionDenied,
                                Code::OperationAborted,
                                Code::DatabaseBusy,
                                Code::DatabaseLocked,
                                Code::OutOfMemory,
                                Code::ReadOnly,
                                Code::OperationInterrupted,
                                Code::SystemIoFailure,
                                Code::DatabaseCorrupt,
                                Code::NotFound,
                                Code::DiskFull,
                                Code::CannotOpen,
                                Code::FileLockingProtocolFailed,
                                Code::Unknown,
                                Code::SchemaChanged,
                                Code::TooBig,
                                Code::ConstraintViolation,
                                Code::TypeMismatch,
                                Code::ApiMisuse,
                                Code::NoLargeFileSupport,
                                Code::AuthorizationForStatementDenied,
                                Code::Unknown,
                                Code::ParameterOutOfRange,
                                Code::NotADatabase};
    auto primary = static_cast<unsigned>(code) & 0xff;
    return {primary < std::size(categories) ? categories[primary] : Code::Unknown, code};
}

String SqliteNativeError::to_string(Memory r) const {
    String text("Error code ", r);
    number(text, extended_code);
    text += ": ";
    text += sqlite3_errstr(extended_code);
    return text;
}

SqliteDriverError
SqliteDriverError::native_failure(Memory r, int code, std::optional<std::string_view> message) {
    return native_failure(r, SqliteNativeError::from_extended_code(code), message);
}

SqliteDriverError SqliteDriverError::native_failure(Memory r,
                                                    SqliteNativeError error,
                                                    std::optional<std::string_view> message) {
    NativeFailure payload{error, {}};
    if (message) {
        payload.message.emplace(*message, r);
    }
    return {r, std::move(payload)};
}

SqliteDriverError SqliteDriverError::sql_input(
    Memory r, int code, std::string_view message, std::string_view sql, int offset) {
    return sql_input(r, SqliteNativeError::from_extended_code(code), message, sql, offset);
}

SqliteDriverError SqliteDriverError::sql_input(
    Memory r, SqliteNativeError error, std::string_view message, std::string_view sql, int offset) {
    return {r, SqlInputError{error, String(message, r), String(sql, r), offset}};
}

SqliteDriverError
SqliteDriverError::invalid_parameter_count(Memory r, std::size_t given, std::size_t expected) {
    return {r, ParameterCount{given, expected}};
}

SqliteDriverError SqliteDriverError::multiple_statement(Memory r) {
    return {r, MultipleStatement{}};
}

SqliteDriverError SqliteDriverError::execute_returned_results(Memory r) {
    return {r, ExecuteReturnedResults{}};
}

SqliteDriverError SqliteDriverError::invalid_column_index(Memory r, std::size_t index) {
    return {r, InvalidColumnIndex{index}};
}

SqliteDriverError SqliteDriverError::invalid_column_type(Memory r,
                                                         std::size_t index,
                                                         std::string_view name,
                                                         SqliteValueType type) {
    type_name(type);
    return {r, InvalidColumnType{index, String(name, r), type}};
}

SqliteDriverError
SqliteDriverError::utf8_error(Memory r, std::size_t index, SqliteUtf8Error error) {
    return {r, Utf8Error{index, error}};
}

SqliteDriverError SqliteDriverError::nul_error(Memory r, SqliteNulError error) {
    return {r, NulError{detail::move_in(r, std::move(error))}};
}

SqliteDriverError SqliteDriverError::from_utf8_error(Memory owner, SqliteUtf8Error error) {
    return utf8_error(owner, SIZE_MAX, error);
}

SqliteDriverError SqliteDriverError::from_nul_error(Memory owner, SqliteNulError error) {
    return nul_error(owner, std::move(error));
}

SqliteDriverError SqliteDriverError::invalid_path(Memory r, SqlitePath path) {
    return {r, InvalidPath{detail::move_in(r, std::move(path))}};
}

SqliteDriverError SqliteDriverError::from_sql_conversion_failure(Memory owner,
                                                                 std::size_t index,
                                                                 SqliteValueType type,
                                                                 SqliteBoxedError error) {
    type_name(type);
    return {owner,
            BoxedFailure{BoxedFailure::Kind::FromSqlConversionFailure,
                         index,
                         type,
                         detail::move_in(owner, std::move(error))}};
}

SqliteDriverError SqliteDriverError::to_sql_conversion_failure(Memory owner,
                                                               SqliteBoxedError error) {
    return {owner,
            BoxedFailure{BoxedFailure::Kind::ToSqlConversionFailure,
                         0,
                         SqliteValueType::Null,
                         detail::move_in(owner, std::move(error))}};
}

SqliteDriverError SqliteDriverError::user_function_error(Memory owner, SqliteBoxedError error) {
    return {owner,
            BoxedFailure{BoxedFailure::Kind::UserFunctionError,
                         0,
                         SqliteValueType::Null,
                         detail::move_in(owner, std::move(error))}};
}

SqliteDriverError SqliteDriverError::sqlite_single_threaded_mode(Memory r) {
    return {r,
            detail::SqliteSimpleError::unit(
                r, detail::SqliteSimpleError::Kind::SqliteSingleThreadedMode)};
}

SqliteDriverError
SqliteDriverError::integral_value_out_of_range(Memory r, std::size_t index, std::int64_t value) {
    return {r, detail::SqliteSimpleError::integral_range(r, index, value)};
}

SqliteDriverError SqliteDriverError::invalid_parameter_name(Memory r, std::string_view name) {
    return {r,
            detail::SqliteSimpleError::named(
                r, detail::SqliteSimpleError::Kind::InvalidParameterName, name)};
}

SqliteDriverError SqliteDriverError::query_returned_no_rows(Memory r) {
    return {
        r,
        detail::SqliteSimpleError::unit(r, detail::SqliteSimpleError::Kind::QueryReturnedNoRows)};
}

SqliteDriverError SqliteDriverError::query_returned_more_than_one_row(Memory r) {
    return {r,
            detail::SqliteSimpleError::unit(
                r, detail::SqliteSimpleError::Kind::QueryReturnedMoreThanOneRow)};
}

SqliteDriverError SqliteDriverError::invalid_column_name(Memory r, std::string_view name) {
    return {r,
            detail::SqliteSimpleError::named(
                r, detail::SqliteSimpleError::Kind::InvalidColumnName, name)};
}

SqliteDriverError SqliteDriverError::statement_changed_rows(Memory r, std::size_t rows) {
    return {r,
            detail::SqliteSimpleError::indexed(
                r, detail::SqliteSimpleError::Kind::StatementChangedRows, rows)};
}

SqliteDriverError SqliteDriverError::invalid_function_parameter_type(Memory r,
                                                                     std::size_t index,
                                                                     SqliteValueType type) {
    return {r, detail::SqliteSimpleError::function_type(r, index, type)};
}

SqliteDriverError SqliteDriverError::invalid_query(Memory r) {
    return {r, detail::SqliteSimpleError::unit(r, detail::SqliteSimpleError::Kind::InvalidQuery)};
}

SqliteDriverError SqliteDriverError::unwinding_panic(Memory r) {
    return {r, detail::SqliteSimpleError::unit(r, detail::SqliteSimpleError::Kind::UnwindingPanic)};
}

SqliteDriverError SqliteDriverError::get_aux_wrong_type(Memory r) {
    return {r,
            detail::SqliteSimpleError::unit(r, detail::SqliteSimpleError::Kind::GetAuxWrongType)};
}

SqliteDriverError SqliteDriverError::invalid_database_index(Memory r, std::size_t index) {
    return {r,
            detail::SqliteSimpleError::indexed(
                r, detail::SqliteSimpleError::Kind::InvalidDatabaseIndex, index)};
}

SqliteDriverError::SqliteDriverError(std::allocator_arg_t,
                                     Allocator a,
                                     const SqliteDriverError& other)
    : resource_(a.resource()),
      data_(std::visit(
          [&](const auto& payload) -> Payload {
              using T = std::decay_t<decltype(payload)>;
              if constexpr (std::same_as<T, NativeFailure>) {
                  return NativeFailure{payload.error, detail::clone_in(resource_, payload.message)};
              } else if constexpr (std::same_as<T, SqlInputError>) {
                  return SqlInputError{payload.error,
                                       String(payload.message, resource_),
                                       String(payload.sql, resource_),
                                       payload.offset};
              } else if constexpr (std::same_as<T, InvalidColumnType>) {
                  return InvalidColumnType{
                      payload.index, String(payload.name, resource_), payload.type};
              } else if constexpr (std::same_as<T, NulError>) {
                  return NulError{detail::clone_in(resource_, payload.error)};
              } else if constexpr (std::same_as<T, InvalidPath>) {
                  return InvalidPath{detail::clone_in(resource_, payload.path)};
              } else if constexpr (std::same_as<T, detail::SqliteSimpleError>) {
                  return detail::clone_in(resource_, payload);
              } else if constexpr (std::same_as<T, BoxedFailure>) {
                  return BoxedFailure{payload.kind,
                                      payload.index,
                                      payload.type,
                                      detail::clone_in(resource_, payload.error)};
              } else {
                  return payload;
              }
          },
          other.data_)) {}

const SqliteNativeError* SqliteDriverError::sqlite_error() const noexcept {
    if (auto* payload = std::get_if<NativeFailure>(&data_)) {
        return &payload->error;
    }
    return nullptr;
}

std::optional<SqliteNativeError::Code> SqliteDriverError::sqlite_error_code() const noexcept {
    if (auto* error = sqlite_error()) {
        return error->code;
    }
    return {};
}

std::optional<int> SqliteDriverError::sqlite_extended_error_code() const noexcept {
    if (auto* error = sqlite_error()) {
        return error->extended_code;
    }
    return {};
}

SqliteErrorCauseView SqliteDriverError::source() const noexcept {
    if (auto* failure = std::get_if<NativeFailure>(&data_)) {
        return SqliteErrorCauseView::from(failure->error);
    }
    if (auto* input = std::get_if<SqlInputError>(&data_)) {
        return SqliteErrorCauseView::from(input->error);
    }
    if (auto* invalid = std::get_if<Utf8Error>(&data_)) {
        return SqliteErrorCauseView::from(invalid->error);
    }
    if (auto* invalid = std::get_if<NulError>(&data_)) {
        return SqliteErrorCauseView::from(invalid->error);
    }
    if (auto* boxed = std::get_if<BoxedFailure>(&data_)) {
        return boxed->error.view();
    }
    return {};
}

String SqliteDriverError::to_string(Memory r) const {
    return std::visit(
        [&](const auto& payload) -> String {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::same_as<T, NativeFailure>) {
                return payload.message ? String(*payload.message, r) : payload.error.to_string(r);
            } else if constexpr (std::same_as<T, SqlInputError>) {
                String text(payload.message, r);
                text += " in ";
                text += payload.sql;
                text += " at offset ";
                number(text, payload.offset);
                return text;
            } else if constexpr (std::same_as<T, ParameterCount>) {
                String text("Wrong number of parameters passed to query. Got ", r);
                number(text, payload.given);
                text += ", needed ";
                number(text, payload.expected);
                return text;
            } else if constexpr (std::same_as<T, MultipleStatement>) {
                return String("Multiple statements provided", r);
            } else if constexpr (std::same_as<T, ExecuteReturnedResults>) {
                return String("Execute returned results - did you mean to call query?", r);
            } else if constexpr (std::same_as<T, InvalidColumnIndex>) {
                String text("Invalid column index: ", r);
                number(text, payload.index);
                return text;
            } else if constexpr (std::same_as<T, InvalidColumnType>) {
                String text("Invalid column type ", r);
                text += type_name(payload.type);
                text += " at index: ";
                number(text, payload.index);
                text += ", name: ";
                text += payload.name;
                return text;
            } else if constexpr (std::same_as<T, Utf8Error>) {
                auto text = payload.error.to_string(r);
                if (payload.index != SIZE_MAX) {
                    text += " at index ";
                    number(text, payload.index);
                }
                return text;
            } else if constexpr (std::same_as<T, NulError>) {
                return payload.error.to_string(r);
            } else if constexpr (std::same_as<T, InvalidPath>) {
                String text("Invalid path: ", r);
                text += payload.path.to_string_lossy(r);
                return text;
            } else if constexpr (std::same_as<T, detail::SqliteSimpleError>) {
                return payload.to_string(r);
            } else {
                if (payload.kind != BoxedFailure::Kind::FromSqlConversionFailure ||
                    payload.index == SIZE_MAX) {
                    return payload.error.to_string(r);
                }
                String text("Conversion error from type ", r);
                text += type_name(payload.type);
                text += " at index: ";
                number(text, payload.index);
                text += ", ";
                text += payload.error.to_string(r);
                return text;
            }
        },
        data_);
}

bool operator==(const SqliteDriverError& left, const SqliteDriverError& right) {
    if (left.kind() != right.kind()) {
        return false;
    }
    return std::visit(
        [&](const auto& a) {
            using T = std::decay_t<decltype(a)>;
            const auto& b = std::get<T>(right.data_);
            if constexpr (std::same_as<T, SqliteDriverError::NativeFailure>) {
                return a.error == b.error && a.message == b.message;
            } else if constexpr (std::same_as<T, SqliteDriverError::SqlInputError>) {
                return a.error == b.error && a.message == b.message && a.sql == b.sql &&
                       a.offset == b.offset;
            } else if constexpr (std::same_as<T, SqliteDriverError::ParameterCount>) {
                return a.given == b.given && a.expected == b.expected;
            } else if constexpr (std::same_as<T, SqliteDriverError::MultipleStatement>) {
                // The pinned driver omits MultipleStatement from its PartialEq match.
                return false;
            } else if constexpr (std::same_as<T, SqliteDriverError::ExecuteReturnedResults>) {
                return true;
            } else if constexpr (std::same_as<T, SqliteDriverError::InvalidColumnIndex>) {
                return a.index == b.index;
            } else if constexpr (std::same_as<T, SqliteDriverError::InvalidColumnType>) {
                return a.index == b.index && a.name == b.name && a.type == b.type;
            } else if constexpr (std::same_as<T, SqliteDriverError::Utf8Error>) {
                return a.index == b.index && a.error == b.error;
            } else if constexpr (std::same_as<T, SqliteDriverError::NulError>) {
                return a.error == b.error;
            } else if constexpr (std::same_as<T, SqliteDriverError::InvalidPath>) {
                return a.path == b.path;
            } else if constexpr (std::same_as<T, detail::SqliteSimpleError>) {
                return a == b;
            } else {
                // The locked driver's boxed alternatives never compare equal.
                return false;
            }
        },
        left.data_);
}

void write_debug(DebugWriter& writer, SqliteNativeError::Code code) {
    constexpr std::string_view names[]{"InternalMalfunction",
                                       "PermissionDenied",
                                       "OperationAborted",
                                       "DatabaseBusy",
                                       "DatabaseLocked",
                                       "OutOfMemory",
                                       "ReadOnly",
                                       "OperationInterrupted",
                                       "SystemIoFailure",
                                       "DatabaseCorrupt",
                                       "NotFound",
                                       "DiskFull",
                                       "CannotOpen",
                                       "FileLockingProtocolFailed",
                                       "SchemaChanged",
                                       "TooBig",
                                       "ConstraintViolation",
                                       "TypeMismatch",
                                       "ApiMisuse",
                                       "NoLargeFileSupport",
                                       "AuthorizationForStatementDenied",
                                       "ParameterOutOfRange",
                                       "NotADatabase",
                                       "Unknown"};
    auto index = static_cast<std::size_t>(code);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown SQLite error category");
    }
    writer.raw(names[index]);
}

void write_debug(DebugWriter& writer, const SqliteNativeError& error) {
    writer.structure("Error", [&](auto& fields) {
        fields.field("code", [&](auto& out) {
            write_debug(out, error.code);
        });
        fields.field("extended_code", [&](auto& out) {
            out.integer(error.extended_code);
        });
    });
}

void write_debug(DebugWriter& writer, SqliteValueType type) {
    writer.raw(type_name(type));
}

void write_debug(DebugWriter& writer, const SqliteDriverError& error) {
    std::visit(
        [&](const auto& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::same_as<T, SqliteDriverError::NativeFailure>) {
                writer.tuple("SqliteFailure", [&](auto& out) {
                    write_debug(out, payload.error);
                    next_tuple_field(out);
                    if (payload.message) {
                        out.tuple("Some", [&](auto& inner) {
                            inner.text(*payload.message);
                        });
                    } else {
                        out.raw("None");
                    }
                });
            } else if constexpr (std::same_as<T, SqliteDriverError::SqlInputError>) {
                writer.structure("SqlInputError", [&](auto& fields) {
                    fields.field("error", [&](auto& out) {
                        write_debug(out, payload.error);
                    });
                    fields.field("msg", [&](auto& out) {
                        out.text(payload.message);
                    });
                    fields.field("sql", [&](auto& out) {
                        out.text(payload.sql);
                    });
                    fields.field("offset", [&](auto& out) {
                        out.integer(payload.offset);
                    });
                });
            } else if constexpr (std::same_as<T, SqliteDriverError::ParameterCount>) {
                writer.tuple("InvalidParameterCount", [&](auto& out) {
                    out.integer(payload.given);
                    next_tuple_field(out);
                    out.integer(payload.expected);
                });
            } else if constexpr (std::same_as<T, SqliteDriverError::MultipleStatement>) {
                writer.raw("MultipleStatement");
            } else if constexpr (std::same_as<T, SqliteDriverError::ExecuteReturnedResults>) {
                writer.raw("ExecuteReturnedResults");
            } else if constexpr (std::same_as<T, SqliteDriverError::InvalidColumnIndex>) {
                writer.tuple("InvalidColumnIndex", [&](auto& out) {
                    out.integer(payload.index);
                });
            } else if constexpr (std::same_as<T, SqliteDriverError::InvalidColumnType>) {
                writer.tuple("InvalidColumnType", [&](auto& out) {
                    out.integer(payload.index);
                    next_tuple_field(out);
                    out.text(payload.name);
                    next_tuple_field(out);
                    write_debug(out, payload.type);
                });
            } else if constexpr (std::same_as<T, SqliteDriverError::Utf8Error>) {
                writer.tuple("Utf8Error", [&](auto& out) {
                    out.integer(payload.index);
                    next_tuple_field(out);
                    write_debug(out, payload.error);
                });
            } else if constexpr (std::same_as<T, SqliteDriverError::NulError>) {
                writer.tuple("NulError", [&](auto& out) {
                    write_debug(out, payload.error);
                });
            } else if constexpr (std::same_as<T, SqliteDriverError::InvalidPath>) {
                writer.tuple("InvalidPath", [&](auto& out) {
                    write_debug(out, payload.path);
                });
            } else if constexpr (std::same_as<T, detail::SqliteSimpleError>) {
                detail::write_debug(writer, payload);
            } else {
                constexpr std::string_view names[]{
                    "FromSqlConversionFailure", "ToSqlConversionFailure", "UserFunctionError"};
                writer.tuple(names[static_cast<std::size_t>(payload.kind)], [&](auto& out) {
                    if (payload.kind ==
                        SqliteDriverError::BoxedFailure::Kind::FromSqlConversionFailure) {
                        out.integer(payload.index);
                        next_tuple_field(out);
                        write_debug(out, payload.type);
                        next_tuple_field(out);
                    }
                    write_debug(out, payload.error);
                });
            }
        },
        error.payload());
}
}
