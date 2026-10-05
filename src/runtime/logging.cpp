#include <teaql/runtime/logging.hpp>
#include <teaql/core/detail/debug.hpp>
#include <charconv>

namespace teaql::detail {
namespace {
template <class T>
String number(Memory r, T n) {
    char b[32];
    auto end = std::to_chars(b, b + sizeof(b), n).ptr;
    return String(b, end, r);
}

String optional_text(Memory r, const std::optional<String>& value) {
    if (!value) {
        return String("None", r);
    }
    String out("Some(", r);
    out += debug_text(r, *value);
    out += ')';
    return out;
}

template <class T>
String optional_number(Memory r, std::optional<T> n) {
    if (!n) {
        return String("None", r);
    }
    String out("Some(", r);
    out += number(r, *n);
    out += ')';
    return out;
}

String system_time(Memory r, SqlLogEntry::SystemTime time) {
    auto ticks =
        std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
    auto seconds = ticks / 1000000000, nanos = ticks % 1000000000;
    if (nanos < 0) {
        --seconds;
        nanos += 1000000000;
    }
    String out("SystemTime { tv_sec: ", r);
    out += number(r, seconds);
    out += ", tv_nsec: ";
    out += number(r, nanos);
    out += " }";
    return out;
}

String duration(Memory r, LogDuration duration) {
    if (duration.nanos >= 1000000000) {
        throw std::invalid_argument("Invalid log duration nanoseconds");
    }
    auto fraction = [&](std::uint64_t integral,
                        std::uint32_t fractional,
                        unsigned width,
                        std::string_view unit) {
        String out = number(r, integral);
        if (fractional) {
            auto digits = number(r, fractional);
            out += '.';
            out.append(width - digits.size(), '0');
            out += digits;
            while (out.back() == '0') {
                out.pop_back();
            }
        }
        out += unit;
        return out;
    };
    if (duration.seconds) {
        return fraction(duration.seconds, duration.nanos, 9, "s");
    }
    if (duration.nanos >= 1000000) {
        return fraction(duration.nanos / 1000000, duration.nanos % 1000000, 6, "ms");
    }
    if (duration.nanos >= 1000) {
        return fraction(duration.nanos / 1000, duration.nanos % 1000, 3, "µs");
    }
    auto out = number(r, duration.nanos);
    out += "ns";
    return out;
}
}

String debug_sql_log(Memory r, const SqlLogEntry& e) {
    const auto& c = e.log_context;
    String out(
        "SqlLogEntry { log_context: SqlLogContext { projection_state: SqlProjectionState(<private>), intent_redactions: ",
        r);
    out += c.intent_redactions.debug(r);
    out += ", execution_outcome: ";
    constexpr std::string_view outcomes[]{"Success", "Failure", "Cancelled"};
    if (c.execution_outcome) {
        out += "Some(";
        out += outcomes[unsigned(*c.execution_outcome)];
        out += ')';
    } else {
        out += "None";
    }
    out += ", database_kind: ";
    out += optional_text(r, c.database_kind);
    out += ", parameter_policies: [";
    constexpr std::string_view policies[]{"Unknown", "Plain", "Masked", "Credential"};
    for (std::size_t i = 0; i < c.parameter_policies.size(); ++i) {
        if (i) {
            out += ", ";
        }
        out += policies[unsigned(c.parameter_policies[i])];
    }
    out += "], generated_sql: ";
    out += c.generated_sql ? "true" : "false";
    out += ", masked_parameters: [";
    for (std::size_t i = 0; i < c.masked_parameters.size(); ++i) {
        if (i) {
            out += ", ";
        }
        out += c.masked_parameters[i] ? "true" : "false";
    }
    out += "], mode: ";
    out += optional_text(r, c.mode);
    out += ", omission_reason: ";
    out += optional_text(r, c.omission_reason);
    out += " }, operation: ";
    constexpr std::string_view operations[]{"Select", "Insert", "Update", "Delete", "Recover"};
    out += operations[unsigned(e.operation)];
    out += ", comment: ";
    out += optional_text(r, e.comment);
    out += ", purpose: ";
    out += optional_text(r, e.purpose);
    out += ", audit_reason: ";
    out += optional_text(r, e.audit_reason);
    out += ", trace_path: [";
    constexpr std::string_view kinds[]{"Operation",
                                       "Request",
                                       "Relation",
                                       "Entity",
                                       "Provider",
                                       "Sql",
                                       "Comment",
                                       "Purpose",
                                       "AuditReason"};
    for (std::size_t i = 0; i < e.trace_path.size(); ++i) {
        const auto& n = e.trace_path[i];
        if (i) {
            out += ", ";
        }
        out += "TraceNode { kind: ";
        out += kinds[unsigned(n.kind)];
        out += ", entity_type: ";
        out += debug_text(r, n.entity_type);
        out += ", entity_id: ";
        out += optional_number(r, n.entity_id);
        out += ", comment: ";
        out += debug_text(r, n.comment);
        out += " }";
    }
    out += "], sql: ";
    out += debug_text(r, e.sql);
    out += ", params: [";
    for (std::size_t i = 0; i < e.params.size(); ++i) {
        if (i) {
            out += ", ";
        }
        out += debug_value(r, e.params[i]);
    }
    out += "], debug_sql: ";
    out += debug_text(r, e.debug_sql);
    out += ", pretty_sql: ";
    out += debug_text(r, e.pretty_sql);
    out += ", started_at: ";
    out += system_time(r, e.started_at);
    out += ", ended_at: ";
    out += system_time(r, e.ended_at);
    out += ", elapsed: ";
    out += duration(r, e.elapsed);
    out += ", result_count: ";
    out += optional_number(r, e.result_count);
    out += ", result_type: ";
    out += optional_text(r, e.result_type);
    out += ", affected_rows: ";
    out += optional_number(r, e.affected_rows);
    out += ", result_summary: ";
    out += debug_text(r, e.result_summary);
    out += " }";
    return out;
}
}
