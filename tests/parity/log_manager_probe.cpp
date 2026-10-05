#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_manager.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_value.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
using namespace teaql;

static void environment(const Json& values) {
    for (const auto& [key, v] : std::get<Json::Object>(values.data)) {
        if (auto* s = std::get_if<String>(&v.data)) {
            setenv(key.c_str(), s->c_str(), 1);
        } else {
            unsetenv(key.c_str());
        }
    }
}

static String business_value(Memory r, const Json& step) {
    String out("private-school-name", r);
    if (auto* size = optional(step, "large")) {
        out.clear();
        for (std::uint64_t i = 0; i < number<std::uint64_t>(*size); ++i) {
            out += "密";
        }
    }
    return out;
}

static SqlLogEntry sql_entry(Memory r, const Json& step) {
    SqlLogEntry e(std::allocator_arg, Allocator(r), SqlLogOperation::Update);
    e.sql = "UPDATE orders SET name = ?, password = ? WHERE id = ?";
    e.params.emplace_back(Value(r, business_value(r, step)));
    e.params.emplace_back(Value(r, "secret-token"));
    e.params.emplace_back(Value(r, std::uint64_t(7)));
    e.log_context.database_kind.emplace("sqlite", r);
    e.log_context.generated_sql = true;
    e.log_context.parameter_policies = {SqlParameterLogPolicy::Masked,
                                        SqlParameterLogPolicy::Credential,
                                        SqlParameterLogPolicy::Plain};
    e.log_context.execution_outcome = SqlExecutionOutcome::Success;
    e.comment.emplace("change private-school-name secret-token 7", r);
    e.trace_path.emplace_back(
        TraceKind::Comment, "Order", "change private-school-name secret-token 7", 7);
    e.log_context.intent_redactions.capture_target_id(e.params[2]);
    e.debug_sql = "untrusted private-school-name secret-token";
    e.pretty_sql = e.debug_sql;
    e.elapsed = {0, 1500};
    e.affected_rows = 1;
    e.result_summary = "1 rows affected";
    if (auto* invalid = optional(step, "invalid"); invalid && std::get<bool>(invalid->data)) {
        e.sql = "SELECT ?4";
    }
    if (auto* safe = optional(step, "safe"); safe && std::get<bool>(safe->data)) {
        return detail::sql_entry_projection(r, e, false);
    }
    return e;
}

int main() {
    GlobalContext global;
    std::string line;
    std::getline(std::cin, line);
    auto input = Json::parse(global.memory_resource(), line);
    if (!input) {
        throw std::runtime_error("invalid input");
    }
    environment(field(*input, "env"));
    for (const auto& step : std::get<Json::Array>(field(*input, "steps").data)) {
        if (auto* delta = optional(step, "env")) {
            environment(*delta);
        }
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto op = text(field(step, "op"));
        auto& manager = global.log_manager();
        if (op == "config") {
            (void)manager.config();
            continue;
        }
        if (op == "warning") {
            (void)manager.plaintext_enabled();
            continue;
        }
        if (op == "mkdir") {
            std::filesystem::create_directory(text(field(step, "path")));
            continue;
        }
        if (op == "field" || op == "credential" || op == "empty_field") {
            Vector<String> masks(r);
            masks.emplace_back("name");
            (void)build_safe_audit_field(
                r,
                op == "credential" ? "password" : "name",
                op == "empty_field" ? std::nullopt
                                    : std::optional<std::string_view>("private-school-name"),
                masks,
                {});
            continue;
        }
        if (op == "sql" || op == "sensitive_sql") {
            auto e = sql_entry(r, step);
            auto before = detail::debug_sql_log(r, e);
            if (op == "sql") {
                manager.write_sql_log(r, e.trace_path, e);
            } else {
                manager.write_sensitive_sql_log(r, e.trace_path, e);
            }
            if (detail::debug_sql_log(r, e) != before) {
                throw std::runtime_error("manager mutated SQL source");
            }
            continue;
        }
        Record values(r);
        values.emplace("id", Value(r, std::uint64_t(7)));
        values.emplace("name", Value(r, business_value(r, step)));
        values.emplace("password", Value(r, "secret-token"));
        auto e = RawAuditEvent::created(r, "Order", values);
        e.trace_chain.emplace_back(
            TraceKind::AuditReason, "Order", "change private-school-name secret-token 7", 7);
        auto before = detail::debug_audit_event(r, e);
        if (op == "event_projection") {
            (void)e.build_safe_event(r, {}, {});
        } else {
            manager.write_audit_log(r, e);
        }
        if (detail::debug_audit_event(r, e) != before) {
            throw std::runtime_error("manager mutated event source");
        }
    }
}
