#pragma once
#include <teaql/runtime/logging.hpp>
#include <teaql/runtime/event.hpp>

namespace teaql {
class GlobalContext;

class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual String format_sql_log(Memory, std::span<const TraceNode>, const SqlLogEntry&) const = 0;
    virtual String format_audit_log(Memory, const RawAuditEvent&) const = 0;

    virtual String format_sensitive_audit_log(Memory r, const RawAuditEvent& event) const {
        return format_audit_log(r, event);
    }
};

class HumanReaderFormatter final : public LogFormatter {
public:
    using allocator_type = Allocator;
    HumanReaderFormatter() = default;

    HumanReaderFormatter(std::allocator_arg_t, Allocator) {}

    String format_sql_log(Memory, std::span<const TraceNode>, const SqlLogEntry&) const override;
    String format_audit_log(Memory, const RawAuditEvent&) const override;
    String format_sensitive_audit_log(Memory, const RawAuditEvent&) const override;
};

class DebugReaderFormatter final : public LogFormatter {
public:
    using allocator_type = Allocator;
    DebugReaderFormatter() = default;

    DebugReaderFormatter(std::allocator_arg_t, Allocator) {}

    String format_sql_log(Memory, std::span<const TraceNode>, const SqlLogEntry&) const override;
    String format_audit_log(Memory, const RawAuditEvent&) const override;
    String format_sensitive_audit_log(Memory, const RawAuditEvent&) const override;
};

class LogFormatterFactory {
public:
    using allocator_type = Allocator;
    LogFormatterFactory(std::allocator_arg_t, Allocator, GlobalContext&);
    LogFormatterFactory(const LogFormatterFactory&) = delete;
    const LogFormatter& get_formatter(); // Borrow, valid until the owning Global closes.

    Memory memory_resource() const noexcept {
        return resource_;
    }

private:
    Memory resource_;
    GlobalContext& global_;
    const LogFormatter* formatter_ = nullptr;
};
enum class LogLevel { Silent, Summary, Full, FullWithPayload };
LogLevel parse_log_level(std::string_view, LogLevel default_level);

struct LogConfig {
    using allocator_type = Allocator;
    LogLevel audit_level = LogLevel::Full, sql_level = LogLevel::Summary,
             tool_level = LogLevel::Full;
    std::optional<Vector<String>> audit_entities, sql_tables, tool_focus;

    LogConfig(std::allocator_arg_t, Allocator a) : resource_(a.resource()) {}

    LogConfig(std::allocator_arg_t, Allocator a, const LogConfig& other)
        : audit_level(other.audit_level), sql_level(other.sql_level), tool_level(other.tool_level),
          audit_entities(detail::clone_in(a.resource(), other.audit_entities)),
          sql_tables(detail::clone_in(a.resource(), other.sql_tables)),
          tool_focus(detail::clone_in(a.resource(), other.tool_focus)), resource_(a.resource()) {}

    LogConfig(std::allocator_arg_t, Allocator a, LogConfig&& other)
        : LogConfig(detail::move_to(a.resource(), std::move(other))) {}

    LogConfig(const LogConfig&) = delete;
    LogConfig(LogConfig&&) noexcept = default;

    LogConfig& operator=(LogConfig&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static LogConfig load(Memory);
    bool should_log_audit(std::string_view) const;

    bool should_log_sensitive_audit(std::string_view entity) const {
        return audit_level == LogLevel::FullWithPayload && should_log_audit(entity);
    }

    bool should_log_sql(std::string_view) const;
    bool should_log_tool(std::string_view) const;

    Memory memory_resource() const noexcept {
        return resource_;
    }

private:
    Memory resource_;
};

namespace detail {
String debug_audit_event(Memory, const RawAuditEvent&);
String truncate_sensitive_sql_log(Memory, std::string_view, std::size_t);
String truncate_sensitive_audit_log(Memory, std::string_view, std::size_t);
}
}
