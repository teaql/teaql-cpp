#include <teaql/runtime/context.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <teaql/runtime/detail/audit_privacy.hpp>
#include "fixture_value.hpp"
#include <cstdlib>
#include <iostream>

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    bool allow = std::get<bool>(field(input, "allow").data);
    if (allow) {
        setenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS",
               "I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK",
               1);
    } else {
        unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    }
    auto name = text(field(input, "field"));
    Record old_values(r), new_values(r), values(r);
    old_values.emplace("id", Value(r, std::uint64_t(7)));
    new_values.emplace("id", Value(r, std::uint64_t(7)));
    old_values.emplace("note", Value(r, "old PRIVATE-OLD PRIVATE-NEW PRIVATE-OPERAND"));
    new_values.emplace("note", Value(r, "new PRIVATE-OLD PRIVATE-NEW PRIVATE-OPERAND"));
    for (auto pair : {std::pair{&old_values, &field(input, "old")},
                      std::pair{&new_values, &field(input, "new")}}) {
        if (text(field(*pair.second, "kind")) != "absent") {
            pair.first->emplace(String(name, r), value(r, *pair.second));
        }
    }
    values.emplace("id", Value(r, std::uint64_t(7)));
    Vector<String> updated(r);
    updated.emplace_back(name);
    updated.emplace_back("note");
    std::optional<Record> old(std::in_place, old_values, r);
    auto& event = user_context.make<RawAuditEvent>(
        RawAuditEvent::updated_with_old_values(r, "Item", values, old, new_values, updated));
    event.trace_chain.emplace_back(
        TraceKind::AuditReason, "Item", "PRIVATE-OLD PRIVATE-NEW PRIVATE-OPERAND id 7", 7);
    event.bootstrap_audit.emplace(std::allocator_arg,
                                  Allocator(r),
                                  "PRIVATE-OLD actor",
                                  "bootstrap",
                                  "PRIVATE-NEW PRIVATE-OPERAND reason",
                                  4,
                                  17);
    if (std::get<bool>(field(input, "provenance").data)) {
        SqlLogContext log(std::allocator_arg, Allocator(r));
        log.parameter_policies = {SqlParameterLogPolicy::Masked};
        Vector<Value> operands(r);
        operands.emplace_back("PRIVATE-OPERAND");
        auto source = SqlIntentRedactions::from_bindings(r, log, operands, "");
        event.with_diagnostic_redactions(&source);
    }
    Vector<String> masks(r);
    if (std::get<bool>(field(input, "masked").data)) {
        masks.emplace_back(name);
    }
    std::optional<std::size_t> limit;
    if (!std::holds_alternative<std::monostate>(field(input, "limit").data)) {
        limit = std::get<std::uint64_t>(field(input, "limit").data);
    }
    auto safe = event.build_safe_event(r, masks, limit);
    auto projected = detail::audit_event_projection(r, event, allow);
    Json::Object output(r);
    output.emplace("safe", Json(r, debug_string(r, safe)));
    output.emplace("projected", Json(r, debug_string(r, projected)));
    output.emplace("safe_clone_equal", Json(r, safe == detail::clone_in(r, safe)));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("invalid dynamic audit input");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
