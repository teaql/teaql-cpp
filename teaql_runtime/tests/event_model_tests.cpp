#include <teaql/runtime.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <cstdlib>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

int main() {
    unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& values = user->make<Record>();
    values.emplace(String("id", owner), Value(owner, std::uint64_t(1001)));
    values.emplace(String("name", owner), Value(owner, "display🧊"));
    values.emplace(String("password", owner), Value(owner, "credential-secret"));
    auto source = user->make_ref<RawAuditEvent>(RawAuditEvent::created(owner, "Order", values));
    auto& event = user->get(source);
    event.trace_chain.emplace_back(TraceKind::AuditReason, "Order", "credential-secret 1001", 1001);
    event.bootstrap_audit.emplace(
        std::allocator_arg, Allocator(owner), "actor", "category", "reason", -3, UINT64_MAX);
    event.mutation_governance.emplace(std::allocator_arg,
                                      Allocator(owner),
                                      "execution",
                                      "Order.saveGraph",
                                      MutationPolicySource::Customer,
                                      MutationPolicyApprovalStatus::Approved);
    event.mutation_governance->policy.emplace(
        std::allocator_arg, Allocator(owner), "policy", "v1", "fingerprint");
    auto& safe = user->make<SafeAuditEvent>(event.build_safe_event(owner, {}, {}));
    auto retained_raw = user->make_global<RawAuditEvent>(event);
    auto retained_safe = user->make_global<SafeAuditEvent>(safe);
    {
        auto raw_text = debug_string(owner, event);
        auto safe_text = debug_string(owner, safe);
        check(raw_text.find("credential-secret") != raw_text.npos,
              "Raw source-derived diagnostics retain actual event data");
        check(
            safe_text.find("credential-secret") == safe_text.npos &&
                safe_text.find("[REDACTED]") != safe_text.npos,
            "Safe diagnostics format the actual privacy projection without reintroducing credentials");
    }
    auto kept_raw = retained_raw;
    auto kept_safe = retained_safe;
    user.reset();
    check(global.live_user_bytes() == 0, "Source event User blocks are reclaimed");
    auto next = global.make_user_context();
    const auto& raw = next->get(next->rebind_global(kept_raw));
    const auto& projected = next->get(next->rebind_global(kept_safe));
    check(raw.values.get_allocator().resource() == global.memory_resource() &&
              raw.bootstrap_audit->memory_resource() == global.memory_resource() &&
              raw.mutation_governance->policy->memory_resource() == global.memory_resource() &&
              projected.fields[0].memory_resource() == global.memory_resource(),
          "Raw and safe nested data are independently owned by the selected destination");
    {
        auto text = debug_string(next->memory_resource(), projected, DebugOptions{.pretty = true});
        check(text.find("credential-secret") == text.npos &&
                  text.get_allocator().resource() == next->memory_resource(),
              "Retained safe events stay safe and format in the caller's User");
    }
    bool rejected = false;
    try {
        (void)next->get(source);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Expired raw event reference rejects before diagnostic access");
    std::cout << "Raw/safe event model ownership and privacy contracts pass\n";
}
