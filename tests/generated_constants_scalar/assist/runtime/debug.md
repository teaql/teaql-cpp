# Runtime support: debug

Application examples; these functions are not generated SDK methods. Install capabilities only during trusted UserContext assembly. The default RequestPolicy shown here is permissive; the application must supply actual tenancy/authorization restrictions. Checker/Behavior defaults can be replaced after generated configuration and before schema initialization; reconfiguration installs generated defaults again. Use model-aware Checker/Behavior and sample guidance in the index.

```cpp
#include <cpp_constants_scalar.hpp>
#if !defined(TEAQL_USE_MODULES)
#include <teaql/runtime/event.hpp>
#include <teaql/runtime/logging.hpp>
#include <teaql/runtime/telemetry.hpp>
#endif
#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
namespace cpp_constants_scalar::assist_runtime {
inline std::string_view sqlOperationName(::teaql::SqlLogOperation operation) {
    switch (operation) {
    case ::teaql::SqlLogOperation::Select: return "select";
    case ::teaql::SqlLogOperation::Insert: return "insert";
    case ::teaql::SqlLogOperation::Update: return "update";
    case ::teaql::SqlLogOperation::Delete: return "delete";
    case ::teaql::SqlLogOperation::Recover: return "recover";
    }
    throw std::logic_error("unknown SQL operation");
}
struct SafeSqlEvidence {
    using allocator_type = ::teaql::Allocator;
    std::string_view operation;
    ::teaql::String parameterized_sql;
    std::size_t parameter_count;
    ::teaql::LogDuration elapsed;
    std::optional<std::size_t> result_count;
    std::optional<std::uint64_t> affected_rows;
    ::teaql::String result_summary;
    SafeSqlEvidence(std::allocator_arg_t, ::teaql::Allocator allocator, const ::teaql::SqlLogEntry& entry)
        : operation(sqlOperationName(entry.operation)), parameterized_sql(entry.sql, allocator.resource()),
          parameter_count(entry.parameter_count()), elapsed(entry.elapsed), result_count(entry.result_count),
          affected_rows(entry.affected_rows), result_summary(entry.result_summary, allocator.resource()) {}
    SafeSqlEvidence(std::allocator_arg_t, ::teaql::Allocator allocator, const SafeSqlEvidence& other)
        : operation(other.operation), parameterized_sql(other.parameterized_sql, allocator.resource()),
          parameter_count(other.parameter_count), elapsed(other.elapsed), result_count(other.result_count),
          affected_rows(other.affected_rows), result_summary(other.result_summary, allocator.resource()) {}
    SafeSqlEvidence(std::allocator_arg_t, ::teaql::Allocator allocator, SafeSqlEvidence&& other)
        : SafeSqlEvidence(std::allocator_arg, allocator, static_cast<const SafeSqlEvidence&>(other)) {}
    SafeSqlEvidence(const SafeSqlEvidence&) = delete;
    SafeSqlEvidence(SafeSqlEvidence&&) noexcept = default;
};
inline void enableAllSqlEvidence(::teaql::UserContext& user_context) {
    user_context.set_sql_log_options(::teaql::SqlLogOptions::all());
    user_context.clear_sql_logs();
}
inline void enableSelectSqlEvidence(::teaql::UserContext& user_context) {
    user_context.set_sql_log_options(::teaql::SqlLogOptions::select_only());
    user_context.clear_sql_logs();
}
inline void enableMutationSqlEvidence(::teaql::UserContext& user_context) {
    user_context.set_sql_log_options(::teaql::SqlLogOptions::mutation_only());
    user_context.clear_sql_logs();
}
inline void disableSelectSqlEvidence(::teaql::UserContext& user_context) {
    user_context.disable_select_sql_log();
    user_context.clear_sql_logs();
}
inline void disableMutationSqlEvidence(::teaql::UserContext& user_context) {
    user_context.disable_mutation_sql_log();
    user_context.clear_sql_logs();
}
inline void disableSqlEvidence(::teaql::UserContext& user_context) {
    user_context.disable_sql_log();
}
inline const ::teaql::Vector<SafeSqlEvidence>& sqlEvidence(::teaql::UserContext& user_context) {
    auto& result = user_context.make<::teaql::Vector<SafeSqlEvidence>>();
    for (const auto& entry : user_context.sql_logs()) {
        result.emplace_back(entry);
    }
    return result;
}
}
```

Module installation is passive. Call ensure_schema explicitly at startup, administration or an explicit readiness verification; never from liveness, each request or telemetry installation. Use generated metadata instead of duplicating DDL/entity lists. Propagate failures.

AppAuditSink stores safe events after commit; raw immutable row audit remains independent. Rollback discards application delivery. Delivery failure after commit does not mean rollback; do not retry as if nothing committed. This in-memory example is not a durable outbox. The sink and all nested event/evidence strings belong to their explicit Context; they are checked borrows at registration and expire at close. No shared owner extends lifetime.

SQL evidence includes operation, parameterized SQL, parameter count, native LogDuration (unsigned seconds/nanoseconds), row count, affected rows and safe summary. It excludes bindings and rendered SQL. Keep full native duration range instead of narrowing to signed microseconds. Select and mutation logging switches are independent and do not disable intent validation. TEAQL_SQL_LOG/TEAQL_AUDIT_LOG accept _silent, _summary, _full and _full_with_payload; TEAQL_SQL_LOG_TABLES/TEAQL_AUDIT_LOG_ENTITIES filter diagnostics, not authorization. Payload levels do not independently authorize sensitive plaintext. Preserve masked diagnostics and use only present runtime opt-ins/sinks for controlled operator troubleshooting.

Keep current native error categories: missing data, Checker validation, optimistic conflict, provider failure and committed audit-delivery failure are not successful empty results. Request intent and audit reasons remain required when diagnostics are off. Telemetry uses the application-owned RuntimeTelemetry SPI; no exporter is installed here. Telemetry must not export bindings/rendered SQL or control audit delivery/business outcomes. Concrete OpenTelemetry/cloud adapters are outside this edge kernel scope. Context-bound reference/document security SPIs remain independent; a token is not authorization, an absent child is not deletion and decoded IDs do not authorize persistence. The Rust document POC does not establish a generated C++ document adapter or a remote utility endpoint.

[Model, sample and extension index](../../ASSIST.md)
