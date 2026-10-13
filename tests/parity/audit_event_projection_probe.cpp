#include <teaql/runtime/context.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <teaql/runtime/detail/audit_privacy.hpp>
#include "fixture_value.hpp"
#include <cstdlib>
#include <iostream>

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    const bool allow = std::get<bool>(field(input, "allow").data);
    if (allow) {
        setenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS",
               "I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK",
               1);
    } else {
        unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    }
    Record fields(r);
    fields.emplace(String("note", r), Value(r, "note PRIVATE-OPERAND PRIVATE-TOKEN"));
    fields.emplace(String("#extension", r), Value(r, "PRIVATE-DYNAMIC"));
    fields.emplace(String("version", r), Value(r, std::int64_t(3)));
    const auto& identifier = field(input, "id");
    if (text(field(identifier, "kind")) != "absent") {
        fields.emplace(String("id", r), value(r, identifier));
    }
    auto& event = user_context.make<RawAuditEvent>(RawAuditEvent::created(r, "Item", fields));
    event.old_values.emplace(r);
    event.old_values->emplace(String("id", r), Value(r, std::uint64_t(999)));
    event.trace_chain.emplace_back(
        TraceKind::AuditReason,
        "Parent",
        "PRIVATE-OPERAND PRIVATE-TOKEN PRIVATE-DYNAMIC parent 123 old 999",
        123);
    if (std::get<bool>(field(input, "bootstrap").data)) {
        event.bootstrap_audit.emplace(std::allocator_arg,
                                      Allocator(r),
                                      "actor PRIVATE-TOKEN",
                                      "bootstrap",
                                      "reason PRIVATE-OPERAND PRIVATE-DYNAMIC",
                                      3,
                                      17);
    }
    if (std::get<bool>(field(input, "provenance").data)) {
        SqlLogContext context(std::allocator_arg, Allocator(r));
        context.parameter_policies = {SqlParameterLogPolicy::Masked,
                                      SqlParameterLogPolicy::Credential};
        Vector<Value> operands(r);
        operands.emplace_back("PRIVATE-OPERAND");
        operands.emplace_back("PRIVATE-TOKEN");
        auto source = SqlIntentRedactions::from_bindings(r, context, operands, "");
        event.with_diagnostic_redactions(&source);
    }
    auto safe = event.build_safe_event(r, {}, {});
    auto copy = detail::clone_in(r, safe);
    auto projected = detail::audit_event_projection(r, event, allow);
    Json::Object output(r);
    output.emplace("safe", Json(r, debug_string(r, safe)));
    output.emplace("safe_clone_equal", Json(r, safe == copy));
    output.emplace("projected", Json(r, debug_string(r, projected)));
    output.emplace("raw_has_provenance", Json(r, event.diagnostic_redactions() != nullptr));
    output.emplace("projected_has_provenance",
                   Json(r, projected.diagnostic_redactions() != nullptr));
    return Json(r, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("invalid fixture JSON");
        }
        std::cout << observe(*user_context, *input).dump(r) << '\n';
    }
}
