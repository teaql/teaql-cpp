# Runtime support: runtime_custom

Application examples; these functions are not generated SDK methods. Install capabilities only during trusted UserContext assembly. The default RequestPolicy shown here is permissive; the application must supply actual tenancy/authorization restrictions. Checker/Behavior defaults can be replaced after generated configuration and before schema initialization; reconfiguration installs generated defaults again. Use model-aware Checker/Behavior and sample guidance in the index.

```cpp
#include <cpp_inheritance_variants.hpp>
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
namespace cpp_inheritance_variants::assist_runtime {
class TrustedRequestPolicy final : public ::teaql::RequestPolicy {
public:
    using allocator_type = ::teaql::Allocator;
    TrustedRequestPolicy(std::allocator_arg_t, ::teaql::Allocator) {}
};
class AppAuditSink final : public ::teaql::SafeAuditEventSink {
public:
    using allocator_type = ::teaql::Allocator;
    mutable ::teaql::Vector<::teaql::SafeAuditEvent> events;
    AppAuditSink(std::allocator_arg_t, ::teaql::Allocator allocator) : events(allocator.resource()) {}
    ::teaql::RuntimeHookResult on_safe_event(::teaql::Memory memory, const ::teaql::UserContext&,
                                            const ::teaql::SafeAuditEvent& event) const override {
        events.emplace_back(event);
        return ::teaql::RuntimeHookResult::ok(memory, {});
    }
};
inline ::teaql::Ref<AppAuditSink> configuredRuntime(
    ::teaql::UserContext& user_context, std::string_view filename) {
    ::cpp_inheritance_variants::configureRuntime(user_context, filename);
    auto& policy = user_context.make<TrustedRequestPolicy>();
    user_context.set_request_policy(user_context, policy);
    auto& telemetry = user_context.make<::teaql::NoopRuntimeTelemetry>();
    user_context.set_runtime_telemetry(user_context, telemetry);
    auto sink = user_context.make_ref<AppAuditSink>();
    user_context.set_custom_event_sink(user_context, user_context.get(sink));
    auto tenant = user_context.make_ref<::teaql::String>("system");
    user_context.insert_named_resource("trusted_tenant", tenant);
    return sink;
}
inline void readiness(::teaql::UserContext& user_context) {
    if (!user_context.get_named_resource<::teaql::String>("trusted_tenant")) {
        throw std::logic_error("missing trusted tenant");
    }
    ::cpp_inheritance_variants::require_value(user_context.ensure_schema().wait());
}
inline bool governanceKey(std::string_view key) {
    constexpr std::array<std::string_view, 6> names{
        "tenant", "provider", "requestPolicy", "auditSink", "hardLimit", "continuousPage"};
    for (auto name : names) {
        if (key.size() != name.size()) {
            continue;
        }
        bool equal = true;
        for (std::size_t index = 0; index != name.size(); ++index) {
            auto lower = [](char value) {
                return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
            };
            equal = equal && lower(key[index]) == lower(name[index]);
        }
        if (equal) {
            return true;
        }
    }
    return false;
}
inline void rejectGovernanceOverride(const ::teaql::Json& input) {
    if (const auto* object = std::get_if<::teaql::Json::Object>(&input.data)) {
        for (const auto& [key, value] : *object) {
            if (governanceKey(key)) {
                throw std::invalid_argument("forbidden governance override: " + std::string(key));
            }
            rejectGovernanceOverride(value);
        }
    } else if (const auto* array = std::get_if<::teaql::Json::Array>(&input.data)) {
        for (const auto& value : *array) {
            rejectGovernanceOverride(value);
        }
    }
}
}
```

Module installation is passive. Call ensure_schema explicitly at startup, administration or an explicit readiness verification; never from liveness, each request or telemetry installation. Use generated metadata instead of duplicating DDL/entity lists. Propagate failures.

AppAuditSink stores safe events after commit; raw immutable row audit remains independent. Rollback discards application delivery. Delivery failure after commit does not mean rollback; do not retry as if nothing committed. This in-memory example is not a durable outbox. The sink and all nested event/evidence strings belong to their explicit Context; they are checked borrows at registration and expire at close. No shared owner extends lifetime.

SQL evidence includes operation, parameterized SQL, parameter count, native LogDuration (unsigned seconds/nanoseconds), row count, affected rows and safe summary. It excludes bindings and rendered SQL. Keep full native duration range instead of narrowing to signed microseconds. Select and mutation logging switches are independent and do not disable intent validation. TEAQL_SQL_LOG/TEAQL_AUDIT_LOG accept _silent, _summary, _full and _full_with_payload; TEAQL_SQL_LOG_TABLES/TEAQL_AUDIT_LOG_ENTITIES filter diagnostics, not authorization. Payload levels do not independently authorize sensitive plaintext. Preserve masked diagnostics and use only present runtime opt-ins/sinks for controlled operator troubleshooting.

Keep current native error categories: missing data, Checker validation, optimistic conflict, provider failure and committed audit-delivery failure are not successful empty results. Request intent and audit reasons remain required when diagnostics are off. Telemetry uses the application-owned RuntimeTelemetry SPI; no exporter is installed here. Telemetry must not export bindings/rendered SQL or control audit delivery/business outcomes. Concrete OpenTelemetry/cloud adapters are outside this edge kernel scope. Context-bound reference/document security SPIs remain independent; a token is not authorization, an absent child is not deletion and decoded IDs do not authorize persistence. The Rust document POC does not establish a generated C++ document adapter or a remote utility endpoint.

[Model, sample and extension index](../../ASSIST.md)
