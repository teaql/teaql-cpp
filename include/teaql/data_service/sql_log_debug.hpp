#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/data_service/sql_log.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, SqlParameterLogPolicy value) {
    constexpr std::string_view names[]{"Unknown", "Plain", "Masked", "Credential"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown SQL parameter policy diagnostic variant");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, SqlExecutionOutcome value) {
    constexpr std::string_view names[]{"Success", "Failure", "Cancelled"};
    auto index = static_cast<std::size_t>(value);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown SQL outcome diagnostic variant");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, const SqlProjectionState&) {
    writer.raw(SqlProjectionState::debug());
}

inline void write_debug(DebugWriter& writer, const SqlIntentRedactions& value) {
    writer.structure("SqlIntentRedactions", [&](auto& fields) {
        fields.field("count", [&](auto& out) {
            out.integer(value.values_.size());
        });
    });
}

namespace detail {
template <class T>
void write_sql_log_optional(DebugWriter& writer, const std::optional<T>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_debug(out, *value);
        });
    } else {
        writer.raw("None");
    }
}
}

inline void write_debug(DebugWriter& writer, const SqlLogContext& value) {
    writer.structure("SqlLogContext", [&](auto& fields) {
        fields.field("projection_state", [&](auto& out) {
            write_debug(out, value.projection_state);
        });
        fields.field("intent_redactions", [&](auto& out) {
            write_debug(out, value.intent_redactions);
        });
        fields.field("execution_outcome", [&](auto& out) {
            detail::write_sql_log_optional(out, value.execution_outcome);
        });
        fields.field("database_kind", [&](auto& out) {
            detail::write_sql_log_optional(out, value.database_kind);
        });
        fields.field("parameter_policies", [&](auto& out) {
            out.sequence(value.parameter_policies, DefaultDebugElement{});
        });
        fields.field("generated_sql", [&](auto& out) {
            out.boolean(value.generated_sql);
        });
        fields.field("masked_parameters", [&](auto& out) {
            out.sequence(value.masked_parameters, [](auto& inner, bool masked) {
                inner.boolean(masked);
            });
        });
        fields.field("mode", [&](auto& out) {
            detail::write_sql_log_optional(out, value.mode);
        });
        fields.field("omission_reason", [&](auto& out) {
            detail::write_sql_log_optional(out, value.omission_reason);
        });
    });
}
}
