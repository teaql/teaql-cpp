#include <teaql/runtime/log_manager.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/detail/audit_privacy.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/text.hpp>
#include "log_header.hpp"
#include <cstdio>
#include <cstdlib>
#include <memory>
#if defined(__linux__)
#include <unistd.h>
#endif
namespace teaql {
namespace {
std::optional<std::string_view> env(Memory r, const char* name) {
    auto* value = std::getenv(name);
    if (!value)
        return {};
    try {
        (void)detail::debug_text(r, value);
    } catch (const std::invalid_argument&) {
        return {};
    }
    return std::string_view(value);
}

// Match Rust's OsStr::to_string_lossy for Linux executable file names.
String lossy_utf8(Memory r, std::string_view bytes) {
    String out(r);
    for (std::size_t i = 0; i < bytes.size();) {
        auto start = i;
        auto b = static_cast<unsigned char>(bytes[i]);
        if (b < 0x80) {
            out += bytes[i++];
            continue;
        }
        unsigned n = b >= 0xc2 && b <= 0xdf   ? 2
                     : b >= 0xe0 && b <= 0xef ? 3
                     : b >= 0xf0 && b <= 0xf4 ? 4
                                              : 0;
        if (!n) {
            out += "\xef\xbf\xbd";
            ++i;
            continue;
        }
        unsigned valid = 1;
        while (valid < n && i + valid < bytes.size()) {
            auto c = static_cast<unsigned char>(bytes[i + valid]);
            bool ok = (c & 0xc0) == 0x80;
            if (valid == 1)
                ok = ok && !(b == 0xe0 && c < 0xa0) && !(b == 0xed && c >= 0xa0) &&
                     !(b == 0xf0 && c < 0x90) && !(b == 0xf4 && c >= 0x90);
            if (!ok)
                break;
            ++valid;
        }
        if (valid == n) {
            out.append(bytes.substr(start, n));
            i += n;
        } else {
            out += "\xef\xbf\xbd";
            i += valid;
        }
    }
    return out;
}

String executable_name(Memory r) {
#if defined(__linux__)
    Vector<char> path(256, r);
    for (;;) {
        auto count = ::readlink("/proc/self/exe", path.data(), path.size());
        if (count < 0)
            break;
        if (std::size_t(count) < path.size()) {
            std::string_view full(path.data(), std::size_t(count));
            auto slash = full.rfind('/');
            auto name = slash == full.npos ? full : full.substr(slash + 1);
            if (!name.empty())
                return lossy_utf8(r, name);
            break;
        }
        path.resize(path.size() * 2);
    }
#endif
    return String("teaql", r);
}

void stream_line(std::FILE* file, std::string_view text) {
    if (std::fwrite(text.data(), 1, text.size(), file) != text.size() ||
        std::fputc('\n', file) == EOF || std::fflush(file) != 0)
        throw std::runtime_error("Failed to write runtime log stream");
}

void output(std::string_view endpoint, std::string_view text) {
    if (endpoint == "stdout") {
        stream_line(stdout, text);
        return;
    }
    // Endpoints are cached PMR strings, so data() is NUL terminated. Embedded
    // NUL cannot enter an environment variable. File errors are source fail-open.
    auto* file = std::fopen(endpoint.data(), "ab");
    if (!file)
        return;
    const std::unique_ptr<std::FILE, decltype(&std::fclose)> close(file, &std::fclose);
    if (std::fwrite(text.data(), 1, text.size(), file) == text.size())
        (void)std::fputc('\n', file);
}
}

LogManager::LogManager(std::allocator_arg_t, Allocator a, GlobalContext& global)
    : resource_(a.resource()), global_(global) {
    if (resource_ != global.memory_resource())
        throw std::invalid_argument("LogManager must belong to its GlobalContext");
}

LogManager& GlobalContext::log_manager() {
    if (!log_manager_)
        log_manager_ = &make<LogManager>(*this);
    return *log_manager_;
}

const LogConfig& LogManager::config() {
    if (!config_)
        config_.emplace(LogConfig::load(resource_));
    return *config_;
}

void LogManager::require_scope(Memory r) const {
    if (&detail::application_context(r) != &global_)
        throw std::invalid_argument("Log output Context belongs to another application");
}

bool LogManager::plaintext_enabled() {
    auto* value = std::getenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    bool enabled =
        value && std::string_view(value) == "I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK";
    if (!enabled)
        return false;
    if (warning_ == Once::Poisoned)
        throw std::runtime_error("Sensitive logging warning initialization was poisoned");
    if (warning_ == Once::Fresh) {
        warning_ = Once::Poisoned;
        stream_line(
            stderr,
            "TeaQL WARNING: sensitive plaintext logging enabled; application data may be written to disk. Authentication secrets remain redacted.");
        warning_ = Once::Done;
    }
    return true;
}

std::string_view LogManager::endpoint() {
    if (!endpoint_loaded_) {
        auto mode = env(resource_, "TEAQL_TRACE_MODE");
        if (mode == "off" &&
            env(resource_, "TEAQL_TRACE_OFF_ACK") ==
                "__i_agree_to_disable_runtime_trace_only_for_extreme_performance_testing")
            endpoint_.emplace("off", resource_);
        else if (auto value = env(resource_, "TEAQL_LOG_ENDPOINT"); value && !value->empty())
            endpoint_.emplace(*value, resource_);
        else if (auto domain = env(resource_, "TEAQL_DOMAIN"); domain && !domain->empty()) {
            endpoint_.emplace(*domain, resource_);
            *endpoint_ += ".log";
        } else {
            endpoint_.emplace(executable_name(resource_));
            *endpoint_ += ".log";
        }
        endpoint_loaded_ = true;
    }
    return *endpoint_;
}

const std::optional<String>& LogManager::debug_endpoint(bool sql) {
    auto& loaded = sql ? sql_endpoint_loaded_ : audit_endpoint_loaded_;
    auto& result = sql ? sql_endpoint_ : audit_endpoint_;
    if (!loaded) {
        auto value =
            env(resource_, sql ? "TEAQL_SQL_DEBUG_ENDPOINT" : "TEAQL_AUDIT_DEBUG_ENDPOINT");
        if (value) {
            auto trimmed = *value;
            detail::trim(trimmed);
            if (!trimmed.empty())
                result.emplace(*value, resource_);
        }
        loaded = true;
    }
    return result;
}

void LogManager::write_header(std::string_view endpoint) {
    if (endpoint == "off")
        return;
    if (header_ == Once::Poisoned)
        throw std::runtime_error("Log header initialization was poisoned");
    if (header_ == Once::Fresh) {
        header_ = Once::Poisoned;
        output(endpoint, detail::log_header);
        header_ = Once::Done;
    }
}

void LogManager::write_ordinary(std::string_view text) {
    auto path = endpoint();
    if (path == "off")
        return;
    write_header(path);
    output(path, text);
}

void LogManager::write_sql_log(Memory r,
                               std::span<const TraceNode> trace,
                               const SqlLogEntry& entry) {
    require_scope(r);
    auto source = detail::clone_in(r, entry);
    source.trace_path.clear();
    for (const auto& n : trace)
        source.trace_path.emplace_back(n);
    auto safe = detail::sql_entry_projection(r, source, false);
    if (!config().should_log_sql(safe.sql) || endpoint() == "off")
        return;
    auto text =
        global_.log_formatter_factory().get_formatter().format_sql_log(r, safe.trace_path, safe);
    write_ordinary(text);
}

void LogManager::write_sensitive_sql_log(Memory r,
                                         std::span<const TraceNode> trace,
                                         const SqlLogEntry& entry) {
    require_scope(r);
    if (!plaintext_enabled())
        return;
    auto source = detail::clone_in(r, entry);
    source.trace_path.clear();
    for (const auto& n : trace)
        source.trace_path.emplace_back(n);
    auto projected = detail::sql_entry_projection(r, source, true);
    if (!config().should_log_sql(projected.sql) || endpoint() == "off")
        return;
    auto& path = debug_endpoint(true);
    if (!path)
        return;
    auto text = global_.log_formatter_factory().get_formatter().format_sql_log(
        r, projected.trace_path, projected);
    output(*path, detail::truncate_sensitive_sql_log(r, text, 64 * 1024));
}

void LogManager::write_audit_log(Memory r, const RawAuditEvent& event) {
    require_scope(r);
    if (!config().should_log_audit(event.entity) || endpoint() == "off")
        return;
    auto safe = detail::audit_event_projection(r, event, false);
    auto text = global_.log_formatter_factory().get_formatter().format_audit_log(r, safe);
    write_ordinary(text);
    if (!config().should_log_sensitive_audit(event.entity) || !plaintext_enabled())
        return;
    auto& path = debug_endpoint(false);
    if (!path)
        return;
    auto projected = detail::audit_event_projection(r, event, true);
    auto raw =
        global_.log_formatter_factory().get_formatter().format_sensitive_audit_log(r, projected);
    output(*path, detail::truncate_sensitive_audit_log(r, raw, 64 * 1024));
}
}
