#include <teaql/runtime.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <iostream>

static SystemTime time(const Json& input) {
    return SystemTime::from_unix_seconds(
        number<std::int64_t>(field(input, "seconds")),
        detail::model_integer<std::uint32_t>(field(input, "nanos"), "nanos"));
}

static std::optional<String> optional_text(Memory owner, const Json& input) {
    if (std::holds_alternative<String>(input.data)) {
        return String(text(input), owner);
    }
    return {};
}

static SqlLogEntry entry(Memory owner, const Json& input) {
    SqlLogEntry result(
        std::allocator_arg,
        Allocator(owner),
        SqlLogOperation(detail::model_integer<unsigned>(field(input, "operation"), "operation")));
    result.log_context.database_kind.emplace("sqlite", owner);
    result.log_context.parameter_policies = {SqlParameterLogPolicy::Plain,
                                             SqlParameterLogPolicy::Masked,
                                             SqlParameterLogPolicy::Credential};
    result.log_context.generated_sql = true;
    result.log_context.masked_parameters = {false, true, true};
    result.log_context.mode.emplace("safe", owner);
    result.log_context.omission_reason = optional_text(owner, field(input, "optional"));
    result.log_context.execution_outcome =
        SqlExecutionOutcome(detail::model_integer<unsigned>(field(input, "outcome"), "outcome"));
    result.comment = optional_text(owner, field(input, "optional"));
    result.purpose.emplace("purpose🧊", owner);
    result.audit_reason.emplace("audit reason", owner);
    result.trace_path.emplace_back(TraceKind::Operation, "Order", "query", 7);
    result.trace_path.emplace_back(TraceKind::Sql, "sqlite", "statement🧊");
    result.sql = "SELECT ?";
    for (const auto& item : detail::model_array(field(input, "params"), "params")) {
        result.params.emplace_back(value(owner, item));
    }
    result.debug_sql = "SELECT 'raw'";
    result.pretty_sql = "SELECT\n  ?";
    result.started_at = time(field(input, "start"));
    result.ended_at = time(field(input, "end"));
    result.elapsed = LogDuration(
        number<std::uint64_t>(field(input, "duration_seconds")),
        detail::model_integer<std::uint32_t>(field(input, "duration_nanos"), "duration_nanos"));
    result.result_count = 17;
    result.result_type.emplace("Order", owner);
    result.affected_rows = UINT64_MAX;
    result.result_summary = "summary🧊";
    return result;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto output = detail::model_json(owner);
    auto put = [&](std::string_view name, auto&& value) {
        detail::model_put(output, name, std::forward<decltype(value)>(value));
    };
    auto kind = text(field(input, "kind"));
    if (kind == "options") {
        auto operation = SqlLogOperation(
            detail::model_integer<unsigned>(field(input, "operation"), "operation"));
        const SqlLogOptions modes[]{SqlLogOptions::disabled(),
                                    SqlLogOptions::select_only(),
                                    SqlLogOptions::mutation_only(),
                                    SqlLogOptions::all()};
        auto options = modes[detail::model_integer<unsigned>(field(input, "mode"), "mode")];
        put("operation", formatted_debug(owner, operation));
        put("select", is_select(operation));
        put("mutation", is_mutation(operation));
        put("options", formatted_debug(owner, options));
        put("default", formatted_debug(owner, SqlLogOptions{}));
        put("enabled", options.enabled_for(operation));
    } else if (kind == "level") {
        auto fallback =
            LogLevel(detail::model_integer<unsigned>(field(input, "fallback"), "fallback"));
        auto level = parse_log_level(text(field(input, "text")), fallback);
        put("debug", formatted_debug(owner, level));
        put("clone_equal", level == LogLevel(level));
    } else {
        auto sql = entry(owner, input);
        if (debug_string(owner, sql) != detail::debug_sql_log(owner, sql)) {
            throw std::runtime_error(
                "SQL fingerprint diagnostics differ from the complete model Debug");
        }
        auto changed_sql = detail::clone_in(owner, sql);
        changed_sql.result_summary += '!';
        InfoLogEntry info(std::allocator_arg,
                          Allocator(owner),
                          std::string_view("info🧊\0message", sizeof("info🧊\0message") - 1));
        LogPayload payload = std::get<bool>(field(input, "info").data)
                                 ? LogPayload(detail::clone_in(owner, info))
                                 : LogPayload(detail::clone_in(owner, sql));
        UnifiedLogEntry unified(
            std::allocator_arg, Allocator(owner), time(field(input, "start")), payload);
        unified.user_identifier = optional_text(owner, field(input, "optional"));
        unified.trace_chain = detail::clone_in(owner, sql.trace_path);
        auto other = detail::clone_in(owner, unified);
        other.trace_chain.clear();
        auto payload_clone = std::visit(
            [&](const auto& value) -> LogPayload {
                return detail::clone_in(owner, value);
            },
            payload);
        put("sql", formatted_debug(owner, sql));
        put("sql_clone", formatted_debug(owner, detail::clone_in(owner, sql)));
        put("sql_clone_equal", sql == detail::clone_in(owner, sql));
        put("sql_changed_equal", sql == changed_sql);
        put("parameter_count", std::uint64_t(sql.parameter_count()));
        put("info", formatted_debug(owner, info));
        put("payload", formatted_debug(owner, payload));
        put("payload_clone_equal", payload == payload_clone);
        put("unified", formatted_debug(owner, unified));
        put("unified_clone", formatted_debug(owner, detail::clone_in(owner, unified)));
        put("unified_clone_equal", unified == detail::clone_in(owner, unified));
        put("unified_changed_equal", unified == other);
    }
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid runtime log model fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
