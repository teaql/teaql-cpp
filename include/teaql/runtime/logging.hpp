#pragma once
#include <teaql/data_service/types.hpp>
#include <teaql/runtime/resources.hpp>
#include <limits>

namespace teaql {
enum class SqlLogOperation { Select, Insert, Update, Delete, Recover };

constexpr bool is_select(SqlLogOperation operation) {
    return operation == SqlLogOperation::Select;
}

constexpr bool is_mutation(SqlLogOperation operation) {
    return !is_select(operation);
}

struct SqlLogOptions {
    bool select = true, mutation = true;

    static constexpr SqlLogOptions disabled() {
        return {false, false};
    }

    static constexpr SqlLogOptions select_only() {
        return {true, false};
    }

    static constexpr SqlLogOptions mutation_only() {
        return {false, true};
    }

    static constexpr SqlLogOptions all() {
        return {};
    }

    constexpr bool enabled_for(SqlLogOperation operation) const {
        return is_select(operation) ? select : mutation;
    }

    bool operator==(const SqlLogOptions&) const = default;
};

// Preserve Rust Duration's unsigned seconds and subsecond precision without
// overflowing a single signed nanosecond counter.
struct LogDuration {
    std::uint64_t seconds = 0;
    std::uint32_t nanos = 0;

    LogDuration(std::uint64_t seconds = 0, std::uint32_t nanos = 0)
        : seconds(seconds), nanos(nanos % 1000000000) {
        auto carry = nanos / 1000000000;
        if (seconds > std::numeric_limits<std::uint64_t>::max() - carry) {
            throw std::overflow_error("overflow in log duration");
        }
        this->seconds += carry;
    }

    bool operator==(const LogDuration&) const = default;
};

struct SqlLogEntry {
    using allocator_type = Allocator;
    using SystemTime = ExecutionMetadata::SystemTime;
    SqlLogContext log_context;
    SqlLogOperation operation;
    std::optional<String> comment, purpose, audit_reason;
    TraceChain trace_path;
    String sql;
    Vector<Value> params;
    String debug_sql, pretty_sql;
    SystemTime started_at{}, ended_at{};
    LogDuration elapsed;
    std::optional<std::size_t> result_count;
    std::optional<String> result_type;
    std::optional<std::uint64_t> affected_rows;
    String result_summary;

    SqlLogEntry(std::allocator_arg_t,
                Allocator a,
                SqlLogOperation operation = SqlLogOperation::Select)
        : log_context(a.resource()), operation(operation), trace_path(a.resource()),
          sql(a.resource()), params(a.resource()), debug_sql(a.resource()),
          pretty_sql(a.resource()), result_summary(a.resource()) {}

    SqlLogEntry(std::allocator_arg_t, Allocator a, const SqlLogEntry& other)
        : log_context(std::allocator_arg, a, other.log_context), operation(other.operation),
          comment(detail::clone_in(a.resource(), other.comment)),
          purpose(detail::clone_in(a.resource(), other.purpose)),
          audit_reason(detail::clone_in(a.resource(), other.audit_reason)),
          trace_path(other.trace_path, a.resource()), sql(other.sql, a.resource()),
          params(other.params, a.resource()), debug_sql(other.debug_sql, a.resource()),
          pretty_sql(other.pretty_sql, a.resource()), started_at(other.started_at),
          ended_at(other.ended_at), elapsed(other.elapsed), result_count(other.result_count),
          result_type(detail::clone_in(a.resource(), other.result_type)),
          affected_rows(other.affected_rows), result_summary(other.result_summary, a.resource()) {}

    SqlLogEntry(std::allocator_arg_t, Allocator a, SqlLogEntry&& other)
        : SqlLogEntry(detail::move_to(a.resource(), std::move(other))) {}

    SqlLogEntry(const SqlLogEntry&) = delete;
    SqlLogEntry(SqlLogEntry&&) noexcept = default;

    SqlLogEntry& operator=(SqlLogEntry&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return sql.get_allocator().resource();
    }

    std::size_t parameter_count() const noexcept {
        return params.size();
    }

    bool operator==(const SqlLogEntry&) const = default;
};

struct InfoLogEntry {
    using allocator_type = Allocator;
    String message;

    InfoLogEntry(std::allocator_arg_t, Allocator a, std::string_view message)
        : message(message, a.resource()) {}

    InfoLogEntry(std::allocator_arg_t, Allocator a, const InfoLogEntry& other)
        : InfoLogEntry(std::allocator_arg, a, other.message) {}

    InfoLogEntry(std::allocator_arg_t, Allocator a, InfoLogEntry&& other)
        : InfoLogEntry(detail::move_to(a.resource(), std::move(other))) {}

    InfoLogEntry(const InfoLogEntry&) = delete;
    InfoLogEntry(InfoLogEntry&&) noexcept = default;

    InfoLogEntry& operator=(InfoLogEntry&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return message.get_allocator().resource();
    }

    bool operator==(const InfoLogEntry&) const = default;
};

using LogPayload = std::variant<SqlLogEntry, InfoLogEntry>;

struct UnifiedLogEntry {
    using allocator_type = Allocator;
    SqlLogEntry::SystemTime timestamp;
    std::optional<String> user_identifier;
    TraceChain trace_chain;
    LogPayload payload;

    UnifiedLogEntry(std::allocator_arg_t,
                    Allocator a,
                    SqlLogEntry::SystemTime timestamp,
                    const LogPayload& payload)
        : timestamp(timestamp), trace_chain(a.resource()),
          payload(std::visit(
              [&](const auto& p) -> LogPayload {
                  return detail::clone_in(a.resource(), p);
              },
              payload)) {}

    UnifiedLogEntry(std::allocator_arg_t, Allocator a, const UnifiedLogEntry& other)
        : timestamp(other.timestamp),
          user_identifier(detail::clone_in(a.resource(), other.user_identifier)),
          trace_chain(other.trace_chain, a.resource()),
          payload(std::visit(
              [&](const auto& p) -> LogPayload {
                  return detail::clone_in(a.resource(), p);
              },
              other.payload)) {}

    UnifiedLogEntry(std::allocator_arg_t, Allocator a, UnifiedLogEntry&& other)
        : UnifiedLogEntry(detail::move_to(a.resource(), std::move(other))) {}

    UnifiedLogEntry(const UnifiedLogEntry&) = delete;
    UnifiedLogEntry(UnifiedLogEntry&&) noexcept = default;

    UnifiedLogEntry& operator=(UnifiedLogEntry&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return trace_chain.get_allocator().resource();
    }

    bool operator==(const UnifiedLogEntry&) const = default;
};

// Context confinement replaces Arc ownership without losing shared mutations.
// Same-domain clones borrow one registered vector; cross-domain copies own a
// new vector and recursively copy entries into the destination Context.
struct UnifiedLogBuffer {
    using allocator_type = Allocator;
    Vector<UnifiedLogEntry>& entries;

    UnifiedLogBuffer(std::allocator_arg_t, Allocator a)
        : entries(detail::resource_arena(a.resource()).make<Vector<UnifiedLogEntry>>()) {}

    UnifiedLogBuffer(std::allocator_arg_t, Allocator a, const UnifiedLogBuffer& other)
        : entries(a.resource() == other.memory_resource()
                      ? other.entries
                      : detail::resource_arena(a.resource())
                            .make<Vector<UnifiedLogEntry>>(other.entries)) {}

    UnifiedLogBuffer(std::allocator_arg_t, Allocator a, UnifiedLogBuffer&& other)
        : UnifiedLogBuffer(detail::move_to(a.resource(), std::move(other))) {}

    UnifiedLogBuffer(const UnifiedLogBuffer&) = delete;
    UnifiedLogBuffer(UnifiedLogBuffer&&) noexcept = default;

    UnifiedLogBuffer& operator=(UnifiedLogBuffer&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entries.get_allocator().resource();
    }
};

namespace detail {
String debug_sql_log(Memory, const SqlLogEntry&);
SqlLogEntry sql_entry_projection(Memory, const SqlLogEntry&, bool allow_plaintext);
}
}
