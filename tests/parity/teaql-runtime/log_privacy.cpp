// Original log-privacy projections and the isolated file-log child fixture.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_manager.hpp>
#include <teaql/runtime/detail/audit_privacy.hpp>
#include <cstdlib>
#include <iostream>

using namespace teaql;
static constexpr const char* acknowledgement = "I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK";
static constexpr const char* acknowledgement_env = "TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS";

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void exact_ack_only(GlobalContext& global) {
    // The source accepts() helper is private; the actual manager uses the same exact gate.
    for (const char* value : {static_cast<const char*>(nullptr),
                              "",
                              "true",
                              "1",
                              " I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK"}) {
        if (value) {
            check(setenv(acknowledgement_env, value, 1) == 0, "set fixture acknowledgement");
        } else {
            check(unsetenv(acknowledgement_env) == 0, "clear fixture acknowledgement");
        }
        check(!global.log_manager().plaintext_enabled(),
              "all original inexact acknowledgements reject");
    }
    check(setenv(acknowledgement_env, acknowledgement, 1) == 0, "set exact acknowledgement");
    check(global.log_manager().plaintext_enabled(),
          "original exact acknowledgement permits diagnostics");
    check(unsetenv(acknowledgement_env) == 0,
          "leave ordinary projection tests without plaintext opt-in");
}

static RawAuditEvent
audit_projection_preserves_original_and_never_exposes_credentials(Memory owner) {
    Record values(owner);
    values.emplace(String("name", owner), Value(owner, "PRIVATE-CUSTOMER"));
    values.emplace(String("access_token", owner), Value(owner, "TOKEN-CANARY"));
    values.emplace(String("id", owner), Value(owner, std::int64_t(1)));
    auto event = RawAuditEvent::created(owner, "Customer", values);
    auto safe = detail::audit_event_projection(owner, event, false);
    check(detail::debug_audit_event(owner, safe).find("PRIVATE-CUSTOMER") == String::npos,
          "original safe audit projection excludes private business value");
    auto debug = detail::audit_event_projection(owner, event, true);
    auto output = detail::debug_audit_event(owner, debug);
    check(output.find("PRIVATE-CUSTOMER") != String::npos &&
              output.find("TOKEN-CANARY") == String::npos,
          "original plaintext projection never exposes credentials");
    check(detail::debug_audit_event(owner, event).find("TOKEN-CANARY") != String::npos,
          "original business event remains unchanged");
    check(event.values == values, "complete original execution values are unmodified");
    return safe;
}

static void audit_trace_scrubs_target_id_for_all_mutation_kinds(Memory owner) {
    Value id(owner, std::int64_t(1001));
    Record values(owner);
    values.emplace(String("id", owner), id);
    Vector<RawAuditEvent> events(owner);
    events.emplace_back(RawAuditEvent::created(owner, "Order", values));
    events.emplace_back(RawAuditEvent::updated(owner, "Order", values));
    events.emplace_back(RawAuditEvent::deleted(owner, "Order", id, 1));
    events.emplace_back(RawAuditEvent::recovered(owner, "Order", id, 1));
    for (auto& event : events) {
        event.trace_chain.emplace_back(TraceKind::AuditReason, "Order", "change order 1001", 1001);
        auto safe = event.build_safe_event(owner, {}, {});
        check(safe.trace_chain.size() == 1 &&
                  safe.trace_chain[0].comment == "change order [REDACTED]",
              "original safe-event target ID scrub for every mutation kind");
        auto standard = detail::audit_event_projection(owner, event, false);
        check(standard.trace_chain.size() == 1 &&
                  standard.trace_chain[0].comment == "change order [REDACTED]" &&
                  event.trace_chain[0].comment == "change order 1001",
              "original ordinary projection target ID scrub preserves source trace");
        auto found = event.values.find("id");
        check(found != event.values.end() && found->second == id,
              "original exact I64 target value is unchanged");
    }
}

static void unit_contracts() {
    GlobalContext global;
    exact_ack_only(global);
    Ref<RawAuditEvent> retained;
    {
        auto user = global.make_user_context();
        auto safe = audit_projection_preserves_original_and_never_exposes_credentials(
            user->memory_resource());
        retained = user->make_global<RawAuditEvent>(safe);
        audit_trace_scrubs_target_id_for_all_mutation_kinds(user->memory_resource());
    }
    check(global.live_user_bytes() == 0, "actual User deletion returns all projection storage");
    auto next = global.make_user_context();
    const auto& retained_event = next->get(next->rebind_global(retained));
    auto output = detail::debug_audit_event(next->memory_resource(), retained_event);
    check(output.find("PRIVATE-CUSTOMER") == String::npos &&
              output.find("TOKEN-CANARY") == String::npos,
          "explicit Global safe event remains private after source User deletion");
}

static void file_log_child() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& manager = global.log_manager();
    Record values(owner);
    values.emplace(String("name", owner), Value(owner, "PRIVATE-CUSTOMER"));
    values.emplace(String("password", owner), Value(owner, "TOKEN-CANARY"));
    auto event = RawAuditEvent::created(owner, "Customer", values);
    manager.write_audit_log(owner, event);
    SqlLogEntry entry(std::allocator_arg, Allocator(owner), SqlLogOperation::Update);
    entry.log_context.database_kind.emplace("sqlite", owner);
    entry.log_context.parameter_policies.push_back(SqlParameterLogPolicy::Masked);
    entry.comment.emplace("edit PRIVATE-CUSTOMER", owner);
    entry.purpose.emplace("debug fixture", owner);
    entry.sql = "UPDATE customer SET name = ?";
    entry.params.emplace_back("PRIVATE-CUSTOMER");
    entry.debug_sql = "UPDATE customer SET name = 'PRIVATE-CUSTOMER'";
    entry.started_at = std::chrono::system_clock::now();
    entry.ended_at = entry.started_at;
    entry.affected_rows = 1;
    entry.result_summary = "1 rows affected";
    manager.write_sql_log(owner, {}, entry);
    manager.write_sensitive_sql_log(owner, {}, entry);
    auto credential = detail::clone_in(owner, entry);
    credential.sql = "UPDATE customer SET password = ?";
    credential.params.clear();
    credential.params.emplace_back("TOKEN-CANARY");
    credential.comment.emplace("edit TOKEN-CANARY", owner);
    credential.debug_sql = "UPDATE customer SET password = 'TOKEN-CANARY'";
    manager.write_sql_log(owner, {}, credential);
    manager.write_sensitive_sql_log(owner, {}, credential);
    auto readback = detail::clone_in(owner, entry);
    Vector<Value> intent_values(owner);
    intent_values.emplace_back("RETAINED-WRITE-CANARY");
    readback.log_context.intent_redactions =
        SqlIntentRedactions::from_bindings(owner, entry.log_context, intent_values, entry.sql);
    readback.log_context.parameter_policies[0] = SqlParameterLogPolicy::Plain;
    readback.sql = "SELECT name FROM customer WHERE id = ?";
    readback.params.clear();
    readback.params.emplace_back(std::uint64_t(1));
    readback.comment.emplace("read authoritative snapshot RETAINED-WRITE-CANARY", owner);
    auto retained = detail::sql_entry_projection(owner, readback, true);
    check(retained.comment && retained.comment->find("RETAINED-WRITE-CANARY") != String::npos,
          "original retained plaintext record keeps its intent before ordinary sink downgrade");
    manager.write_sql_log(owner, retained.trace_path, retained);
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--child") {
            file_log_child();
        } else {
            check(argc == 1, "only the isolated child mode is supported");
            unit_contracts();
        }
        std::pmr::set_default_resource(previous);
        std::cout << "Original log-privacy fixture passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
