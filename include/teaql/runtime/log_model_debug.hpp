#pragma once
#include <teaql/runtime/log_formatter.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include <teaql/core/query_debug.hpp>
#include <teaql/core/system_time_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, SqlLogOperation value) {
    switch (value) {
    case SqlLogOperation::Select:
        writer.raw("Select");
        return;
    case SqlLogOperation::Insert:
        writer.raw("Insert");
        return;
    case SqlLogOperation::Update:
        writer.raw("Update");
        return;
    case SqlLogOperation::Delete:
        writer.raw("Delete");
        return;
    case SqlLogOperation::Recover:
        writer.raw("Recover");
        return;
    }
    throw std::invalid_argument("Unknown SQL log operation");
}

inline void write_debug(DebugWriter& writer, LogLevel value) {
    switch (value) {
    case LogLevel::Silent:
        writer.raw("Silent");
        return;
    case LogLevel::Summary:
        writer.raw("Summary");
        return;
    case LogLevel::Full:
        writer.raw("Full");
        return;
    case LogLevel::FullWithPayload:
        writer.raw("FullWithPayload");
        return;
    }
    throw std::invalid_argument("Unknown log level");
}

inline void write_debug(DebugWriter& writer, const SqlLogOptions& value) {
    writer.structure("SqlLogOptions", [&](auto& fields) {
        fields.field("select", [&](auto& out) {
            out.boolean(value.select);
        });
        fields.field("mutation", [&](auto& out) {
            out.boolean(value.mutation);
        });
    });
}

inline void write_debug(DebugWriter& writer, LogDuration value) {
    write_debug(writer, SystemDuration{value.seconds, value.nanos});
}

inline void write_debug(DebugWriter& writer, const InfoLogEntry& value) {
    writer.structure("InfoLogEntry", [&](auto& fields) {
        fields.field("message", [&](auto& out) {
            out.text(value.message);
        });
    });
}

inline void write_debug(DebugWriter& writer, const SqlLogEntry& value) {
    writer.structure("SqlLogEntry", [&](auto& fields) {
        fields.field("log_context", [&](auto& out) {
            write_debug(out, value.log_context);
        });
        fields.field("operation", [&](auto& out) {
            write_debug(out, value.operation);
        });
        fields.field("comment", [&](auto& out) {
            detail::write_sql_log_optional(out, value.comment);
        });
        fields.field("purpose", [&](auto& out) {
            detail::write_sql_log_optional(out, value.purpose);
        });
        fields.field("audit_reason", [&](auto& out) {
            detail::write_sql_log_optional(out, value.audit_reason);
        });
        fields.field("trace_path", [&](auto& out) {
            out.sequence(value.trace_path, DefaultDebugElement{});
        });
        fields.field("sql", [&](auto& out) {
            out.text(value.sql);
        });
        fields.field("params", [&](auto& out) {
            out.sequence(value.params, DefaultDebugElement{});
        });
        fields.field("debug_sql", [&](auto& out) {
            out.text(value.debug_sql);
        });
        fields.field("pretty_sql", [&](auto& out) {
            out.text(value.pretty_sql);
        });
        fields.field("started_at", [&](auto& out) {
            write_debug(out, value.started_at);
        });
        fields.field("ended_at", [&](auto& out) {
            write_debug(out, value.ended_at);
        });
        fields.field("elapsed", [&](auto& out) {
            write_debug(out, value.elapsed);
        });
        fields.field("result_count", [&](auto& out) {
            detail::write_sql_log_optional(out, value.result_count);
        });
        fields.field("result_type", [&](auto& out) {
            detail::write_sql_log_optional(out, value.result_type);
        });
        fields.field("affected_rows", [&](auto& out) {
            detail::write_sql_log_optional(out, value.affected_rows);
        });
        fields.field("result_summary", [&](auto& out) {
            out.text(value.result_summary);
        });
    });
}

inline void write_debug(DebugWriter& writer, const LogPayload& value) {
    std::visit(
        [&](const auto& payload) {
            using T = std::remove_cvref_t<decltype(payload)>;
            writer.tuple(std::same_as<T, SqlLogEntry> ? "Sql" : "Info", [&](auto& out) {
                write_debug(out, payload);
            });
        },
        value);
}

inline void write_debug(DebugWriter& writer, const UnifiedLogEntry& value) {
    writer.structure("UnifiedLogEntry", [&](auto& fields) {
        fields.field("timestamp", [&](auto& out) {
            write_debug(out, value.timestamp);
        });
        fields.field("user_identifier", [&](auto& out) {
            detail::write_sql_log_optional(out, value.user_identifier);
        });
        fields.field("trace_chain", [&](auto& out) {
            out.sequence(value.trace_chain, DefaultDebugElement{});
        });
        fields.field("payload", [&](auto& out) {
            write_debug(out, value.payload);
        });
    });
}
}
