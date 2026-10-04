#pragma once
#include <teaql/provider_sqlite/error.hpp>
#include <span>

struct sqlite3_stmt;
namespace teaql::detail {
// SQLite storage values after TEAQL conversion, before connection acquisition
// and statement preparation. Conversion must finish before touching SQLite.
struct SqliteBoundValue {
    using allocator_type = Allocator;
    using Storage = std::variant<std::monostate, std::int64_t, double, String>;
    Storage data;
    SqliteBoundValue(std::allocator_arg_t, Allocator a) : resource_(a.resource()) {}
    SqliteBoundValue(std::allocator_arg_t, Allocator a, const SqliteBoundValue& other)
        : resource_(a.resource()) {
        std::visit([&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, String>) data.emplace<String>(value, resource_);
            else data = value;
        }, other.data);
    }
    SqliteBoundValue(std::allocator_arg_t, Allocator a, SqliteBoundValue&& other)
        : SqliteBoundValue(move_to(a.resource(), std::move(other))) {}
    SqliteBoundValue(const SqliteBoundValue&) = delete;
    SqliteBoundValue(SqliteBoundValue&&) noexcept = default;
    SqliteBoundValue& operator=(SqliteBoundValue&& other) { return assign_scoped(*this, std::move(other)); }
    Memory memory_resource() const noexcept { return resource_; }
private:
    Memory resource_;
};
using SqliteBindings = Vector<SqliteBoundValue>;
Result<SqliteBoundValue, MutationExecutorError> bind_sqlite_value(Memory, const Value&);
Result<SqliteBindings, MutationExecutorError> bind_values(Memory, std::span<const Value>);
// The caller owns the statement and must reset/clear it before reuse. SQLite
// copies bound text, so no Context pointer survives in its native allocations.
Result<std::monostate, MutationExecutorError> bind_statement(Memory, sqlite3_stmt&, std::span<const SqliteBoundValue>);
}
