#include <teaql/provider_sqlite/detail/bindings.hpp>
#include <sqlite3.h>
#include <algorithm>
#include <charconv>
#include <limits>

namespace teaql::detail {
namespace {
template <class T>
void append_number(String& out, T value) {
    char buffer[32];
    auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value);
    out.append(buffer, converted.ptr);
}
}

Result<SqliteBoundValue, MutationExecutorError> bind_sqlite_value(Memory r, const Value& input) {
    using Output = Result<SqliteBoundValue, MutationExecutorError>;
    SqliteBoundValue bound(std::allocator_arg, Allocator(r));
    return std::visit(
        [&](const auto& value) -> Output {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<T, TypedNull>) {
            } else if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, std::int64_t>)
                bound.data = std::int64_t(value);
            else if constexpr (std::is_same_v<T, std::uint64_t>) {
                if (value > std::uint64_t(INT64_MAX)) {
                    String message("u64 value ", r);
                    append_number(message, value);
                    message += " exceeds i64 range";
                    return Output::err(r, MutationExecutorError::bind(r, message));
                }
                bound.data = std::int64_t(value);
            } else if constexpr (std::is_same_v<T, double>)
                bound.data = value;
            else if constexpr (std::is_same_v<T, String>)
                bound.data.emplace<String>(value, r);
            else if constexpr (std::is_same_v<T, Decimal> || std::is_same_v<T, Date>)
                bound.data = value.to_string(r);
            else if constexpr (std::is_same_v<T, Json>)
                bound.data = value.dump(r);
            else if constexpr (std::is_same_v<T, Timestamp>)
                bound.data = value.as_millis();
            else if constexpr (std::is_same_v<T, Value::Object>)
                return Output::err(r, MutationExecutorError::unsupported_value(r, "object"));
            else if constexpr (std::is_same_v<T, Value::List>)
                return Output::err(r, MutationExecutorError::unsupported_value(r, "list"));
            return Output::ok(r, std::move(bound));
        },
        input.data);
}

Result<SqliteBindings, MutationExecutorError> bind_values(Memory r, std::span<const Value> values) {
    using Output = Result<SqliteBindings, MutationExecutorError>;
    SqliteBindings bindings(r);
    bindings.reserve(values.size());
    for (const auto& value : values) {
        auto bound = bind_sqlite_value(r, value);
        if (!bound)
            return Output::err(r, std::move(bound).error());
        bindings.push_back(std::move(bound).value());
    }
    return Output::ok(r, std::move(bindings));
}

Result<std::monostate, MutationExecutorError>
bind_statement(Memory r, sqlite3_stmt& statement, std::span<const SqliteBoundValue> values) {
    using Output = Result<std::monostate, MutationExecutorError>;
    const auto expected = std::size_t(sqlite3_bind_parameter_count(&statement));
    for (std::size_t i = 0; i < std::min(expected, values.size()); ++i) {
        const int index = int(i + 1);
        const int code = std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                    return sqlite3_bind_null(&statement, index);
                else if constexpr (std::is_same_v<T, std::int64_t>)
                    return sqlite3_bind_int64(&statement, index, value);
                else if constexpr (std::is_same_v<T, double>)
                    return sqlite3_bind_double(&statement, index, value);
                else
                    return sqlite3_bind_text64(&statement,
                                               index,
                                               value.data(),
                                               value.size(),
                                               SQLITE_TRANSIENT,
                                               SQLITE_UTF8);
            },
            values[i].data);
        if (code != SQLITE_OK)
            return Output::err(r,
                               MutationExecutorError::sqlite(
                                   r, code, sqlite3_errmsg(sqlite3_db_handle(&statement))));
    }
    if (values.size() != expected) {
        // rusqlite's iterator stops at the first excess parameter.
        String message("Wrong number of parameters passed to query. Got ", r);
        append_number(message, std::min(values.size(), expected + 1));
        message += ", needed ";
        append_number(message, expected);
        return Output::err(r, MutationExecutorError::sqlite(r, 0, message));
    }
    return Output::ok(r, {});
}
}
