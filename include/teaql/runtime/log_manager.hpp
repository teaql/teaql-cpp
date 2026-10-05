#pragma once
#include <teaql/runtime/log_formatter.hpp>

namespace teaql {
// Application-owned policy/endpoint state. Each call uses its explicit Context
// scratch arena; no event, source bindings, or per-request text is retained here.
class LogManager {
public:
    using allocator_type = Allocator;
    LogManager(std::allocator_arg_t, Allocator, GlobalContext&);
    LogManager(const LogManager&) = delete;
    const LogConfig& config();
    bool plaintext_enabled();
    void write_sql_log(Memory, std::span<const TraceNode>, const SqlLogEntry&);
    void write_sensitive_sql_log(Memory, std::span<const TraceNode>, const SqlLogEntry&);
    void write_audit_log(Memory, const RawAuditEvent&);

    Memory memory_resource() const noexcept {
        return resource_;
    }

private:
    Memory resource_;
    GlobalContext& global_;
    std::optional<LogConfig> config_;
    bool endpoint_loaded_ = false, sql_endpoint_loaded_ = false, audit_endpoint_loaded_ = false;
    std::optional<String> endpoint_, sql_endpoint_, audit_endpoint_;
    enum class Once { Fresh, Done, Poisoned };
    Once header_ = Once::Fresh, warning_ = Once::Fresh;
    std::string_view endpoint();
    const std::optional<String>& debug_endpoint(bool sql);
    void write_header(std::string_view);
    void write_ordinary(std::string_view);
    void require_scope(Memory) const;
};
}
