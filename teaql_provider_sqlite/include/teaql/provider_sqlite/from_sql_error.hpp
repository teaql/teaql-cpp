#pragma once
#include <teaql/provider_sqlite/boxed_error.hpp>
#include <teaql/provider_sqlite/simple_error.hpp>

namespace teaql {
class SqliteDriverError;

// Distinct column-conversion errors and their source From<FromSqlError> mapping.
class SqliteFromSqlError {
public:
    using allocator_type = Allocator;

    struct InvalidType {};

    struct OutOfRange {
        std::int64_t value;
    };

    struct InvalidBlobSize {
        std::size_t expected_size;
        std::size_t blob_size;
    };

    using Payload =
        std::variant<InvalidType, OutOfRange, SqliteUtf8Error, InvalidBlobSize, SqliteBoxedError>;
    enum class Kind { InvalidType, OutOfRange, Utf8Error, InvalidBlobSize, Other };

    static SqliteFromSqlError invalid_type(Memory);
    static SqliteFromSqlError out_of_range(Memory, std::int64_t);
    static SqliteFromSqlError utf8_error(Memory, SqliteUtf8Error);
    static SqliteFromSqlError invalid_blob_size(Memory, std::size_t, std::size_t);
    static SqliteFromSqlError other(Memory, SqliteBoxedError);
    SqliteFromSqlError(std::allocator_arg_t, Allocator, const SqliteFromSqlError&);

    SqliteFromSqlError(std::allocator_arg_t, Allocator a, SqliteFromSqlError&& other)
        : SqliteFromSqlError(detail::move_to(a.resource(), std::move(other))) {}

    SqliteFromSqlError(const SqliteFromSqlError&) = delete;
    SqliteFromSqlError(SqliteFromSqlError&&) noexcept = default;

    SqliteFromSqlError& operator=(SqliteFromSqlError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return owner_;
    }

    Kind kind() const noexcept {
        return static_cast<Kind>(data_.index());
    }

    const Payload& payload() const noexcept {
        return data_;
    }

    String to_string(Memory) const;
    SqliteErrorCauseView source() const noexcept;
    SqliteDriverError into_driver_error(Memory) &&;
    friend bool operator==(const SqliteFromSqlError&, const SqliteFromSqlError&);

private:
    SqliteFromSqlError(Memory owner, Payload data) : owner_(owner), data_(std::move(data)) {}

    Memory owner_;
    Payload data_;
};

void write_debug(DebugWriter&, const SqliteFromSqlError&);
}
