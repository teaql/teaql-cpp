#include <teaql/runtime/log_formatter.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/text.hpp>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>

namespace teaql {
namespace {
template <class T>
String number(Memory r, T n) {
    char b[32];
    auto end = std::to_chars(b, b + sizeof(b), n).ptr;
    return String(b, end, r);
}

std::string_view event_kind(RawAuditEventKind kind) {
    constexpr std::string_view names[]{"Created",
                                       "Updated",
                                       "Deleted",
                                       "Recovered",
                                       "SchemaCreated",
                                       "SchemaVerified",
                                       "FieldAdded",
                                       "DataSeeded"};
    return names[unsigned(kind)];
}

std::string_view trace_kind(TraceKind kind) {
    constexpr std::string_view names[]{"Operation",
                                       "Request",
                                       "Relation",
                                       "Entity",
                                       "Provider",
                                       "Sql",
                                       "Comment",
                                       "Purpose",
                                       "AuditReason"};
    return names[unsigned(kind)];
}

template <class T, class F>
String optional_debug(Memory r, const std::optional<T>& value, F&& render) {
    if (!value)
        return String("None", r);
    String out("Some(", r);
    out += render(*value);
    out += ')';
    return out;
}

String optional_text(Memory r, const std::optional<String>& value) {
    return optional_debug(r, value, [&](const auto& text) {
        return detail::debug_text(r, text);
    });
}

template <class T>
String optional_number(Memory r, std::optional<T> value) {
    return optional_debug(r, value, [&](auto n) {
        return number(r, n);
    });
}

String strings_debug(Memory r, const Vector<String>& values) {
    String out("[", r);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i)
            out += ", ";
        out += detail::debug_text(r, values[i]);
    }
    out += ']';
    return out;
}

String record_debug(Memory r, const Record& values) {
    String out("{", r);
    bool first = true;
    for (const auto& [key, value] : values) {
        if (!first)
            out += ", ";
        first = false;
        out += detail::debug_text(r, key);
        out += ": ";
        out += detail::debug_value(r, value);
    }
    out += '}';
    return out;
}

String trace_debug(Memory r, std::span<const TraceNode> trace) {
    String out("[", r);
    for (std::size_t i = 0; i < trace.size(); ++i) {
        const auto& n = trace[i];
        if (i)
            out += ", ";
        out += "TraceNode { kind: ";
        out += trace_kind(n.kind);
        out += ", entity_type: ";
        out += detail::debug_text(r, n.entity_type);
        out += ", entity_id: ";
        out += optional_number(r, n.entity_id);
        out += ", comment: ";
        out += detail::debug_text(r, n.comment);
        out += " }";
    }
    out += ']';
    return out;
}

String governance_debug(Memory r, const MutationGovernanceSnapshot& g) {
    String out("MutationGovernanceSnapshot { execution_id: ", r);
    out += detail::debug_text(r, g.execution_id);
    out += ", request_key: ";
    out += detail::debug_text(r, g.request_key);
    out += ", source: ";
    out += g.source == MutationPolicySource::Customer ? "Customer" : "GeneratedDefault";
    out += ", policy: ";
    out += optional_debug(r, g.policy, [&](const auto& p) {
        String s("MutationPolicyIdentity { id: ", r);
        s += detail::debug_text(r, p.id);
        s += ", version: ";
        s += detail::debug_text(r, p.version);
        s += ", fingerprint: ";
        s += detail::debug_text(r, p.fingerprint);
        s += " }";
        return s;
    });
    out += ", approval_status: ";
    constexpr std::string_view approvals[]{"NotApplicable", "Missing", "Approved"};
    out += approvals[unsigned(g.approval_status)];
    out += ", warning_codes: ";
    out += strings_debug(r, g.warning_codes);
    out += ", operations: [";
    constexpr std::string_view kinds[]{"Create", "Update", "Delete", "Recover"};
    for (std::size_t i = 0; i < g.operations.size(); ++i) {
        auto& op = g.operations[i];
        if (i)
            out += ", ";
        out += "MutationOperationSummary { kind: ";
        out += kinds[unsigned(op.kind)];
        out += ", entity: ";
        out += op.entity.debug(r);
        out += ", changed_fields: ";
        out += strings_debug(r, op.changed_fields);
        out += " }";
    }
    out += "] }";
    return out;
}

String trace_text(Memory r, std::span<const TraceNode> trace) {
    String out(r);
    for (std::size_t i = 0; i < trace.size(); ++i) {
        if (i)
            out += " -> ";
        out += number(r, i);
        out += ':';
        out += trace_kind(trace[i].kind);
        out += ':';
        out += trace[i].entity_type;
        out += '=';
        out += trace[i].comment;
    }
    return out;
}

String debug_trace_text(Memory r, std::span<const TraceNode> trace) {
    if (trace.empty())
        return String("(Trace: None)", r);
    String out("(Trace: ", r);
    out += trace_text(r, trace);
    out += ')';
    return out;
}

String entity_id(Memory r, const RawAuditEvent& e) {
    const Value* id = nullptr;
    if (e.new_values) {
        auto it = e.new_values->find("id");
        if (it != e.new_values->end())
            id = &it->second;
    }
    if (!id) {
        auto it = e.values.find("id");
        if (it != e.values.end())
            id = &it->second;
    }
    return id ? detail::debug_value(r, *id) : String("Unknown", r);
}

String timestamp(Memory r) {
    auto t = Timestamp::now().to_datetime();
    String out("[", r);
    out += t.date.to_string(r);
    out += ' ';
    auto part = [&](unsigned n, unsigned width) {
        auto text = number(r, n);
        if (text.size() < width)
            out.append(width - text.size(), '0');
        out += text;
    };
    part(t.hour, 2);
    out += ':';
    part(t.minute, 2);
    out += ':';
    part(t.second, 2);
    out += '.';
    part(t.millisecond, 3);
    out += ']';
    return out;
}

String human_audit(Memory r, const RawAuditEvent& e, bool values) {
    auto out = timestamp(r);
    out += "-[AUDIT]-Entity [";
    out += e.entity;
    out += ':';
    out += entity_id(r, e);
    out += "] ";
    out += event_kind(e.kind);
    if (!e.trace_chain.empty()) {
        out += " (Trace: ";
        out += trace_text(r, e.trace_chain);
        out += ')';
    }
    bool first = true;
    for (const auto& change : e.changes) {
        if (change.field.starts_with('_'))
            continue;
        if (first) {
            out += values ? " {" : " fields=[";
            first = false;
        } else
            out += ", ";
        out += change.field;
        if (values) {
            out += ": ";
            out += change.new_value ? detail::debug_value(r, *change.new_value) : String("null", r);
        }
    }
    if (!first)
        out += values ? '}' : ']';
    return out;
}

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

char ascii_lower(char c) {
    return c >= 'A' && c <= 'Z' ? char(c - 'A' + 'a') : c;
}

bool equal_ascii(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return ascii_lower(x) == ascii_lower(y);
           });
}

bool allowed(const std::optional<Vector<String>>& names, std::string_view name) {
    return !names || std::any_of(names->begin(), names->end(), [&](const auto& n) {
        return equal_ascii(n, name);
    });
}
}

String HumanReaderFormatter::format_sql_log(Memory r,
                                            std::span<const TraceNode> trace,
                                            const SqlLogEntry& entry) const {
    auto out = timestamp(r);
    out += "-[";
    double micros = std::round(
        (double(entry.elapsed.seconds) + double(entry.elapsed.nanos) / 1000000000.0) * 1000000.0);
    auto count = micros >= double(std::numeric_limits<std::uint64_t>::max())
                     ? std::numeric_limits<std::uint64_t>::max()
                     : std::uint64_t(micros);
    auto text = number(r, count);
    if (text.size() < 5)
        out.append(5 - text.size(), ' ');
    out += text;
    out += "µs]-[DEBUG]-SqlLogEntry";
    if (!trace.empty()) {
        out += " - [";
        out += trace_text(r, trace);
        out += ']';
    }
    out += " - [";
    out += entry.result_summary;
    out += "] outcome=";
    out += entry.log_context.execution_outcome ? as_str(*entry.log_context.execution_outcome)
                                               : "unknown";
    out += " comment=";
    out += optional_text(r, entry.comment);
    out += " purpose=";
    out += optional_text(r, entry.purpose);
    out += " auditReason=";
    out += optional_text(r, entry.audit_reason);
    out += "\n          SQL: ";
    out += entry.debug_sql.empty()
               ? std::string_view("[SQL omitted: missing safe projection; NOT REPLAYABLE]")
               : std::string_view(entry.debug_sql);
    return out;
}

String HumanReaderFormatter::format_audit_log(Memory r, const RawAuditEvent& e) const {
    return human_audit(r, e, false);
}

String HumanReaderFormatter::format_sensitive_audit_log(Memory r, const RawAuditEvent& e) const {
    return human_audit(r, e, true);
}

String DebugReaderFormatter::format_sql_log(Memory r,
                                            std::span<const TraceNode> trace,
                                            const SqlLogEntry& e) const {
    String out("[SQL_LOG] ", r);
    out += debug_trace_text(r, trace);
    out += " - Event: ";
    out += detail::debug_sql_log(r, e);
    return out;
}

String DebugReaderFormatter::format_audit_log(Memory r, const RawAuditEvent& e) const {
    String out("[AUDIT_LOG] ", r);
    out += debug_trace_text(r, e.trace_chain);
    out += " - entity=";
    out += e.entity;
    out += " id=";
    out += entity_id(r, e);
    out += " kind=";
    out += event_kind(e.kind);
    out += " fields=[";
    bool first = true;
    for (const auto& change : e.changes) {
        if (change.field.starts_with('_'))
            continue;
        if (!first)
            out += ',';
        first = false;
        out += change.field;
    }
    out += ']';
    return out;
}

String DebugReaderFormatter::format_sensitive_audit_log(Memory r, const RawAuditEvent& e) const {
    String out("[AUDIT_LOG_RAW] ", r);
    out += debug_trace_text(r, e.trace_chain);
    out += " - Event: ";
    out += detail::debug_audit_event(r, e);
    return out;
}

LogFormatterFactory& GlobalContext::log_formatter_factory() {
    if (!log_formatter_factory_)
        log_formatter_factory_ = &make<LogFormatterFactory>(*this);
    return *log_formatter_factory_;
}

LogFormatterFactory::LogFormatterFactory(std::allocator_arg_t, Allocator a, GlobalContext& global)
    : resource_(a.resource()), global_(global) {
    if (resource_ != global.memory_resource())
        throw std::invalid_argument("LogFormatterFactory must belong to its GlobalContext");
}

const LogFormatter& LogFormatterFactory::get_formatter() {
    if (!formatter_) {
        auto format = env(resource_, "TEAQL_LOG_FORMAT");
        if (format == "json" || format == "debug")
            formatter_ = &global_.make<DebugReaderFormatter>();
        else
            formatter_ = &global_.make<HumanReaderFormatter>();
    }
    return *formatter_;
}

LogLevel parse_log_level(std::string_view value, LogLevel default_level) {
    if (value == "_silent")
        return LogLevel::Silent;
    if (value == "_summary")
        return LogLevel::Summary;
    if (value == "_full")
        return LogLevel::Full;
    if (value == "_full_with_payload")
        return LogLevel::FullWithPayload;
    return default_level;
}

LogConfig LogConfig::load(Memory r) {
    LogConfig c(std::allocator_arg, Allocator(r));
    c.audit_level = parse_log_level(env(r, "TEAQL_AUDIT_LOG").value_or(""), LogLevel::Full);
    c.sql_level = parse_log_level(env(r, "TEAQL_SQL_LOG").value_or(""), LogLevel::Summary);
    c.tool_level = parse_log_level(env(r, "TEAQL_TOOL_LOG").value_or(""), LogLevel::Full);
    auto list = [&](const char* name) -> std::optional<Vector<String>> {
        auto value = env(r, name);
        if (!value)
            return {};
        Vector<String> out(r);
        for (;;) {
            auto end = value->find(',');
            auto part = value->substr(0, end);
            detail::trim(part);
            out.emplace_back(part);
            if (end == value->npos)
                break;
            value->remove_prefix(end + 1);
        }
        return out;
    };
    c.audit_entities = list("TEAQL_AUDIT_LOG_ENTITIES");
    c.sql_tables = list("TEAQL_SQL_LOG_TABLES");
    c.tool_focus = list("TEAQL_TOOL_LOG_FOCUS");
    return c;
}

bool LogConfig::should_log_audit(std::string_view entity) const {
    return audit_level != LogLevel::Silent && allowed(audit_entities, entity);
}

bool LogConfig::should_log_tool(std::string_view module) const {
    return tool_level != LogLevel::Silent && allowed(tool_focus, module);
}

bool LogConfig::should_log_sql(std::string_view sql) const {
    if (sql_level == LogLevel::Silent)
        return false;
    if (!sql_tables)
        return true;
    return std::any_of(sql_tables->begin(), sql_tables->end(), [&](const auto& t) {
        return t.empty() ||
               std::search(sql.begin(), sql.end(), t.begin(), t.end(), [](char a, char b) {
                   return ascii_lower(a) == ascii_lower(b);
               }) != sql.end();
    });
}

namespace detail {
String debug_audit_event(Memory r, const RawAuditEvent& e) {
    String out("RawAuditEvent { kind: ", r);
    out += event_kind(e.kind);
    out += ", entity: ";
    out += debug_text(r, e.entity);
    out += ", values: ";
    out += record_debug(r, e.values);
    out += ", updated_fields: ";
    out += strings_debug(r, e.updated_fields);
    out += ", old_values: ";
    out += optional_debug(r, e.old_values, [&](const auto& record) {
        return record_debug(r, record);
    });
    out += ", new_values: ";
    out += optional_debug(r, e.new_values, [&](const auto& record) {
        return record_debug(r, record);
    });
    out += ", changes: [";
    for (std::size_t i = 0; i < e.changes.size(); ++i) {
        const auto& c = e.changes[i];
        if (i)
            out += ", ";
        out += "EntityPropertyChange { field: ";
        out += debug_text(r, c.field);
        out += ", old_value: ";
        out += optional_debug(r, c.old_value, [&](const auto& value) {
            return debug_value(r, value);
        });
        out += ", new_value: ";
        out += optional_debug(r, c.new_value, [&](const auto& value) {
            return debug_value(r, value);
        });
        out += " }";
    }
    out += "], trace_chain: ";
    out += trace_debug(r, e.trace_chain);
    out += ", bootstrap_audit: ";
    out += optional_debug(r, e.bootstrap_audit, [&](const auto& b) {
        String s("BootstrapAuditIdentity { actor: ", r);
        s += debug_text(r, b.actor);
        s += ", category: ";
        s += debug_text(r, b.category);
        s += ", reason: ";
        s += debug_text(r, b.reason);
        s += ", resulting_version: ";
        s += optional_number(r, b.resulting_version);
        s += ", occurred_at_millis: ";
        s += number(r, b.occurred_at_millis);
        s += " }";
        return s;
    });
    out += ", mutation_governance: ";
    out += optional_debug(r, e.mutation_governance, [&](const auto& g) {
        return governance_debug(r, g);
    });
    out += " }";
    return out;
}

namespace {
String truncate_log(Memory r, std::string_view content, std::size_t maximum, bool sql) {
    if (content.size() <= maximum)
        return String(content, r);
    auto end = maximum;
    while (end && (static_cast<unsigned char>(content[end]) & 0xc0) == 0x80)
        --end;
    String out(content.substr(0, end), r);
    out += sql ? "\n[TRUNCATED; NOT EXECUTABLE: diagnostic SQL exceeded "
               : "\n[TRUNCATED: sensitive audit event exceeded ";
    out += number(r, maximum);
    out += " bytes]";
    return out;
}
}

String truncate_sensitive_sql_log(Memory r, std::string_view content, std::size_t maximum) {
    return truncate_log(r, content, maximum, true);
}

String truncate_sensitive_audit_log(Memory r, std::string_view content, std::size_t maximum) {
    return truncate_log(r, content, maximum, false);
}
}
}
