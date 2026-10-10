#include <teaql/provider_sqlite/from_sql_error.hpp>
#include <teaql/provider_sqlite/driver_error.hpp>

namespace teaql {
namespace {
template <class T>
void number(String& text, T value) {
    char buffer[32];
    auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (converted.ec != std::errc{}) {
        throw std::length_error("SQLite conversion-error number formatting failed");
    }
    text.append(buffer, converted.ptr);
}
}

SqliteFromSqlError SqliteFromSqlError::invalid_type(Memory owner) {
    return {owner, InvalidType{}};
}

SqliteFromSqlError SqliteFromSqlError::out_of_range(Memory owner, std::int64_t value) {
    return {owner, OutOfRange{value}};
}

SqliteFromSqlError SqliteFromSqlError::utf8_error(Memory owner, SqliteUtf8Error error) {
    return {owner, error};
}

SqliteFromSqlError
SqliteFromSqlError::invalid_blob_size(Memory owner, std::size_t expected, std::size_t actual) {
    return {owner, InvalidBlobSize{expected, actual}};
}

SqliteFromSqlError SqliteFromSqlError::other(Memory owner, SqliteBoxedError error) {
    return {owner, detail::move_in(owner, std::move(error))};
}

SqliteFromSqlError::SqliteFromSqlError(std::allocator_arg_t,
                                       Allocator a,
                                       const SqliteFromSqlError& other)
    : owner_(a.resource()), data_(std::visit(
                                [&](const auto& value) -> Payload {
                                    using T = std::decay_t<decltype(value)>;
                                    if constexpr (std::same_as<T, SqliteBoxedError>) {
                                        return detail::clone_in(owner_, value);
                                    } else {
                                        return value;
                                    }
                                },
                                other.data_)) {}

String SqliteFromSqlError::to_string(Memory owner) const {
    return std::visit(
        [&](const auto& value) -> String {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<T, InvalidType>) {
                return String("Invalid type", owner);
            } else if constexpr (std::same_as<T, OutOfRange>) {
                String text("Value ", owner);
                number(text, value.value);
                text += " out of range";
                return text;
            } else if constexpr (std::same_as<T, InvalidBlobSize>) {
                String text("Cannot read ", owner);
                number(text, value.expected_size);
                text += " byte value out of ";
                number(text, value.blob_size);
                text += " byte blob";
                return text;
            } else {
                return value.to_string(owner);
            }
        },
        data_);
}

SqliteErrorCauseView SqliteFromSqlError::source() const noexcept {
    if (auto* utf8 = std::get_if<SqliteUtf8Error>(&data_)) {
        return SqliteErrorCauseView::from(*utf8);
    }
    if (auto* other = std::get_if<SqliteBoxedError>(&data_)) {
        return other->view();
    }
    return {};
}

SqliteDriverError SqliteFromSqlError::into_driver_error(Memory owner) && {
    if (auto* range = std::get_if<OutOfRange>(&data_)) {
        return SqliteDriverError::integral_value_out_of_range(owner, SIZE_MAX, range->value);
    }
    if (auto* other = std::get_if<SqliteBoxedError>(&data_)) {
        return SqliteDriverError::from_sql_conversion_failure(
            owner, SIZE_MAX, SqliteValueType::Null, detail::move_in(owner, std::move(*other)));
    }
    auto type = kind() == Kind::InvalidBlobSize ? SqliteValueType::Blob : SqliteValueType::Null;
    auto boxed =
        SqliteBoxedError::make<SqliteFromSqlError>(owner, detail::move_in(owner, std::move(*this)));
    return SqliteDriverError::from_sql_conversion_failure(owner, SIZE_MAX, type, std::move(boxed));
}

bool operator==(const SqliteFromSqlError& left, const SqliteFromSqlError& right) {
    if (left.kind() != right.kind()) {
        return false;
    }
    return std::visit(
        [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            const auto& other = std::get<T>(right.data_);
            if constexpr (std::same_as<T, SqliteFromSqlError::InvalidType>) {
                return true;
            } else if constexpr (std::same_as<T, SqliteFromSqlError::OutOfRange>) {
                return value.value == other.value;
            } else if constexpr (std::same_as<T, SqliteFromSqlError::InvalidBlobSize>) {
                return value.expected_size == other.expected_size &&
                       value.blob_size == other.blob_size;
            } else if constexpr (std::same_as<T, SqliteUtf8Error>) {
                return value == other;
            } else {
                return false;
            }
        },
        left.data_);
}

void write_debug(DebugWriter& writer, const SqliteFromSqlError& error) {
    std::visit(
        [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::same_as<T, SqliteFromSqlError::InvalidType>) {
                writer.raw("InvalidType");
            } else if constexpr (std::same_as<T, SqliteFromSqlError::OutOfRange>) {
                writer.tuple("OutOfRange", [&](auto& out) {
                    out.integer(value.value);
                });
            } else if constexpr (std::same_as<T, SqliteFromSqlError::InvalidBlobSize>) {
                writer.structure("InvalidBlobSize", [&](auto& fields) {
                    fields.field("expected_size", [&](auto& out) {
                        out.integer(value.expected_size);
                    });
                    fields.field("blob_size", [&](auto& out) {
                        out.integer(value.blob_size);
                    });
                });
            } else if constexpr (std::same_as<T, SqliteUtf8Error>) {
                writer.tuple("Utf8Error", [&](auto& out) {
                    write_debug(out, value);
                });
            } else {
                writer.tuple("Other", [&](auto& out) {
                    write_debug(out, value);
                });
            }
        },
        error.payload());
}
}
