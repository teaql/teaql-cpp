#include <teaql/provider_sqlite/simple_error.hpp>

namespace teaql::detail {
namespace {
template <class T>
void number(String& output, T value) {
    char buffer[32];
    auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (converted.ec != std::errc{}) {
        throw std::length_error("SQLite error number conversion failed");
    }
    output.append(buffer, converted.ptr);
}

std::string_view type_name(SqliteValueType value) {
    constexpr std::string_view names[]{"Null", "Integer", "Real", "Text", "Blob"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown SQLite value type");
    }
    return names[index];
}
}

SqliteSimpleError SqliteSimpleError::unit(Memory r, Kind kind) {
    switch (kind) {
    case Kind::SqliteSingleThreadedMode:
    case Kind::QueryReturnedNoRows:
    case Kind::QueryReturnedMoreThanOneRow:
    case Kind::InvalidQuery:
    case Kind::UnwindingPanic:
    case Kind::GetAuxWrongType:
        return {r, kind};
    default:
        throw std::invalid_argument("SQLite error variant requires a payload");
    }
}

SqliteSimpleError
SqliteSimpleError::integral_range(Memory r, std::size_t index, std::int64_t value) {
    SqliteSimpleError error(r, Kind::IntegralValueOutOfRange);
    error.index_ = index;
    error.value_ = value;
    return error;
}

SqliteSimpleError SqliteSimpleError::named(Memory r, Kind kind, std::string_view name) {
    if (kind != Kind::InvalidParameterName && kind != Kind::InvalidColumnName) {
        throw std::invalid_argument("SQLite error variant does not accept a name");
    }
    SqliteSimpleError error(r, kind);
    error.name_ = name;
    return error;
}

SqliteSimpleError SqliteSimpleError::indexed(Memory r, Kind kind, std::size_t index) {
    if (kind != Kind::StatementChangedRows && kind != Kind::InvalidDatabaseIndex) {
        throw std::invalid_argument("SQLite error variant does not accept a scalar index");
    }
    SqliteSimpleError error(r, kind);
    error.index_ = index;
    return error;
}

SqliteSimpleError
SqliteSimpleError::function_type(Memory r, std::size_t index, SqliteValueType type) {
    type_name(type);
    SqliteSimpleError error(r, Kind::InvalidFunctionParameterType);
    error.index_ = index;
    error.type_ = type;
    return error;
}

SqliteSimpleError::SqliteSimpleError(std::allocator_arg_t,
                                     Allocator a,
                                     const SqliteSimpleError& other)
    : kind_(other.kind_), name_(other.name_, a.resource()), index_(other.index_),
      value_(other.value_), type_(other.type_) {}

String SqliteSimpleError::to_string(Memory r) const {
    switch (kind_) {
    case Kind::SqliteSingleThreadedMode:
        return String("SQLite was compiled or configured for single-threaded use only", r);
    case Kind::QueryReturnedNoRows:
        return String("Query returned no rows", r);
    case Kind::QueryReturnedMoreThanOneRow:
        return String("Query returned more than one row", r);
    case Kind::InvalidQuery:
        return String("Query is not read-only", r);
    case Kind::UnwindingPanic:
        return String("unwinding panic", r);
    case Kind::GetAuxWrongType:
        return String("get_aux called with wrong type", r);
    default:
        break;
    }
    String text(r);
    switch (kind_) {
    case Kind::IntegralValueOutOfRange:
        text = "Integer ";
        number(text, value_);
        text += " out of range";
        if (index_ != SIZE_MAX) {
            text += " at index ";
            number(text, index_);
        }
        break;
    case Kind::InvalidParameterName:
        text = "Invalid parameter name: ";
        text += name_;
        break;
    case Kind::InvalidColumnName:
        text = "Invalid column name: ";
        text += name_;
        break;
    case Kind::StatementChangedRows:
        text = "Query changed ";
        number(text, index_);
        text += " rows";
        break;
    case Kind::InvalidFunctionParameterType:
        text = "Invalid function parameter type ";
        text += type_name(type_);
        text += " at index ";
        number(text, index_);
        break;
    case Kind::InvalidDatabaseIndex:
        text = "Invalid database index: ";
        number(text, index_);
        break;
    default:
        throw std::invalid_argument("Unknown SQLite simple error variant");
    }
    return text;
}

bool operator==(const SqliteSimpleError& left, const SqliteSimpleError& right) {
    if (left.kind_ != right.kind_) {
        return false;
    }
    switch (left.kind_) {
    case SqliteSimpleError::Kind::IntegralValueOutOfRange:
        return left.index_ == right.index_ && left.value_ == right.value_;
    case SqliteSimpleError::Kind::InvalidParameterName:
    case SqliteSimpleError::Kind::InvalidColumnName:
        return left.name_ == right.name_;
    case SqliteSimpleError::Kind::StatementChangedRows:
    case SqliteSimpleError::Kind::InvalidDatabaseIndex:
        return left.index_ == right.index_;
    case SqliteSimpleError::Kind::InvalidFunctionParameterType:
        return left.index_ == right.index_ && left.type_ == right.type_;
    default:
        return true;
    }
}

void write_debug(DebugWriter& writer, const SqliteSimpleError& error) {
    constexpr std::string_view names[]{"SqliteSingleThreadedMode",
                                       "IntegralValueOutOfRange",
                                       "InvalidParameterName",
                                       "QueryReturnedNoRows",
                                       "QueryReturnedMoreThanOneRow",
                                       "InvalidColumnName",
                                       "StatementChangedRows",
                                       "InvalidFunctionParameterType",
                                       "InvalidQuery",
                                       "UnwindingPanic",
                                       "GetAuxWrongType",
                                       "InvalidDatabaseIndex"};
    auto index = static_cast<std::size_t>(error.kind());
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown SQLite simple error diagnostic");
    }
    switch (error.kind()) {
    case SqliteSimpleError::Kind::IntegralValueOutOfRange:
        writer.tuple(names[index], [&](auto& out) {
            out.integer(error.index());
            out.raw(out.options().pretty ? ",\n" : ", ");
            out.integer(error.value());
        });
        break;
    case SqliteSimpleError::Kind::InvalidParameterName:
    case SqliteSimpleError::Kind::InvalidColumnName:
        writer.tuple(names[index], [&](auto& out) {
            out.text(error.name());
        });
        break;
    case SqliteSimpleError::Kind::StatementChangedRows:
    case SqliteSimpleError::Kind::InvalidDatabaseIndex:
        writer.tuple(names[index], [&](auto& out) {
            out.integer(error.index());
        });
        break;
    case SqliteSimpleError::Kind::InvalidFunctionParameterType:
        writer.tuple(names[index], [&](auto& out) {
            out.integer(error.index());
            out.raw(out.options().pretty ? ",\n" : ", ");
            write_debug(out, error.value_type());
        });
        break;
    default:
        writer.raw(names[index]);
        break;
    }
}
}
