#pragma once
#include <teaql/provider_sqlite/error.hpp>
#include <teaql/core/row.hpp>

struct sqlite3_stmt;

namespace teaql::detail {
enum class SqliteDecodeKind { Infer, Bool, Decimal, Json, Date, Timestamp, Text };
SqliteDecodeKind sqlite_decode_kind(std::optional<std::string_view> declaration);

// Context-owned immutable layout; rows may borrow it only in the same domain.
class SqliteRowLayout {
public:
    using allocator_type = Allocator;

    SqliteRowLayout(std::allocator_arg_t, Allocator a)
        : columns_(std::allocator_arg, a, Vector<String>(a.resource())), kinds_(a.resource()) {}

    SqliteRowLayout(std::allocator_arg_t, Allocator, sqlite3_stmt&);
    SqliteRowLayout(std::allocator_arg_t, Allocator, const SqliteRowLayout&);
    SqliteRowLayout(const SqliteRowLayout&) = delete;

    const ColumnLayout& columns() const noexcept {
        return columns_;
    }

    std::span<const SqliteDecodeKind> kinds() const noexcept {
        return kinds_;
    }

    Memory memory_resource() const noexcept {
        return columns_.memory_resource();
    }

private:
    ColumnLayout columns_;
    Vector<SqliteDecodeKind> kinds_;
};

Result<Value, MutationExecutorError> decode_sqlite_text(Memory, std::string_view, SqliteDecodeKind);
std::optional<String> sqlite_utf8_error(Memory, std::string_view);
Result<CompactRow, MutationExecutorError>
decode_sqlite_row(Memory, sqlite3_stmt&, const SqliteRowLayout&);
}
