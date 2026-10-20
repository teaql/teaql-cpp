#include <teaql/runtime.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <teaql/runtime/detail/audit_privacy.hpp>
#include <array>
#include <cstdlib>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void root_only(RawAuditEvent& event) {
    event.trace_chain.clear();
    event.trace_chain.emplace_back(TraceKind::AuditReason, "CustomerOrder", "submit order", 1);
}

static Record identified(Memory r, const Value& id) {
    Record values(r);
    values.emplace(String("id", r), Value(r, id));
    return values;
}

static void safe_target_source_contracts(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto id = Value(r, std::uint64_t{7123});
    auto values = identified(r, id);
    values.emplace(String("name", r), Value(r, "private name"));
    std::optional<Record> old(std::in_place, values, r);
    Vector<String> names(r);
    names.emplace_back("name");
    std::array events{
        RawAuditEvent::created(r, "OrderItem", values),
        RawAuditEvent::updated_with_old_values(r, "OrderItem", values, {}, values, names),
        RawAuditEvent::deleted_with_old_values(r, "OrderItem", id, 1, old),
        RawAuditEvent::recovered_with_old_values(r, "OrderItem", id, -1, old)};
    for (auto& raw : events) {
        root_only(raw);
        auto safe = raw.build_safe_event(r, names, 3);
        check(safe.entity == "OrderItem" && safe.entity_id == 7123,
              "all four mutation kinds retain their actual target");
        check(safe.trace_chain == raw.trace_chain,
              "target does not fabricate a responsibility node");
        if (raw.kind != RawAuditEventKind::Created) {
            for (const auto& field : safe.fields) {
                check(field.name != "id" || !field.value, "target identity is not an ID update");
            }
        }
        check(debug_string(r, safe).find("private name") == String::npos,
              "safe event does not disclose masked business values");
    }
    auto order = RawAuditEvent::deleted(r, "CustomerOrder", Value(r, std::uint64_t{1}), 1);
    auto payment = RawAuditEvent::deleted(r, "Payment", Value(r, std::uint64_t{1}), 1);
    root_only(order);
    root_only(payment);
    auto safe_order = order.build_safe_event(r, {}, {});
    auto safe_payment = payment.build_safe_event(r, {}, {});
    check(safe_order.entity_id == safe_payment.entity_id &&
              safe_order.entity != safe_payment.entity &&
              safe_order.trace_chain == safe_payment.trace_chain,
          "equal numeric IDs remain distinct targets by entity type");
    auto authoritative = identified(r, Value(r, std::int64_t{7123}));
    std::optional<Record> old_snapshot(std::in_place, identified(r, Value(r, std::uint64_t{999})));
    auto changed = identified(r, Value(r, std::uint64_t{888}));
    auto raw = RawAuditEvent::updated_with_old_values(
        r, "Payment", authoritative, old_snapshot, changed, {});
    root_only(raw);
    auto safe = raw.build_safe_event(r, {}, {});
    raw.values.insert_or_assign(String("id", r), Value(r, std::uint64_t{456}));
    check(
        safe.entity_id == 7123 && safe.fields.empty() && safe.trace_chain[0].entity_id == 1,
        "safe target is an owned snapshot of authoritative values, not lineage or old/new fields");
    auto schema = RawAuditEvent::schema_created(r, "Payment", "payment_data", 3);
    check(!schema.build_safe_event(r, {}, {}).entity_id, "schema event has no invented target");
    std::array valid_ids{Value(r, UINT64_MAX), Value(r, INT64_MAX)};
    for (const auto& identifier : valid_ids) {
        auto event = RawAuditEvent::created(r, "Payment", identified(r, identifier));
        check(event.build_safe_event(r, {}, {}).entity_id == identifier.try_u64(),
              "maximum integer IDs retain their full width");
    }
    auto missing = RawAuditEvent::updated(r, "Payment", Record(r));
    root_only(missing);
    check(!missing.build_safe_event(r, {}, {}).entity_id,
          "missing target does not fall back to lineage");
    std::array invalid_ids{Value(r, std::uint64_t{0}),
                           Value(r, std::int64_t{-1}),
                           Value(r, 7.5),
                           Value(r, "7123"),
                           Value(r),
                           Value(r, *Decimal::parse("7.5"))};
    for (const auto& identifier : invalid_ids) {
        auto event = RawAuditEvent::created(r, "Payment", identified(r, identifier));
        check(!event.build_safe_event(r, {}, {}).entity_id,
              "invalid or noninteger ID has no target");
    }
}

static void bootstrap_identity_source_contract(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto values = identified(r, Value(r, std::uint64_t{7123}));
    values.emplace(String("password", r), Value(r, "BOOTSTRAP-PASSWORD-CANARY"));
    auto raw = RawAuditEvent::created(r, "Platform", values);
    raw.bootstrap_audit.emplace(std::allocator_arg,
                                Allocator(r),
                                "bootstrap BOOTSTRAP-PASSWORD-CANARY",
                                "runtime-bootstrap",
                                "seed 7123 BOOTSTRAP-PASSWORD-CANARY",
                                1,
                                42);
    auto safe = raw.build_safe_event(r, {}, {});
    check(bool(safe.bootstrap_audit), "safe event retains bootstrap attribution");
    const auto& identity = *safe.bootstrap_audit;
    check(identity.actor == "bootstrap [REDACTED]" && identity.category == "runtime-bootstrap" &&
              identity.reason == "seed [REDACTED] [REDACTED]" && identity.resulting_version == 1 &&
              identity.occurred_at_millis == 42,
          "safe bootstrap identity retains provenance while redacting credential and ID prose");
    check(raw.bootstrap_audit->reason == "seed 7123 BOOTSTRAP-PASSWORD-CANARY",
          "safe projection leaves original bootstrap attribution unchanged");
    check(!RawAuditEvent::created(r, "Platform", Record(r))
               .build_safe_event(r, {}, {})
               .bootstrap_audit,
          "missing bootstrap attribution remains absent");
}

int main() {
    unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    safe_target_source_contracts(*user);
    bootstrap_identity_source_contract(*user);
    std::cout << "SAFE_AUDIT_SOURCE_CONTRACTS=5\n";
    auto owner = user->memory_resource();
    auto& values = user->make<Record>();
    values.emplace(String("id", owner), Value(owner, std::uint64_t(1001)));
    values.emplace(String("name", owner), Value(owner, "display🧊"));
    values.emplace(String("password", owner), Value(owner, "credential-secret"));
    auto source = user->make_ref<RawAuditEvent>(RawAuditEvent::created(owner, "Order", values));
    auto& event = user->get(source);
    auto& binding_context = user->make<SqlLogContext>();
    binding_context.parameter_policies = {SqlParameterLogPolicy::Masked,
                                          SqlParameterLogPolicy::Credential};
    auto& operands = user->make<Vector<Value>>();
    operands.emplace_back("PRIVATE-SIBLING");
    operands.emplace_back("PRIVATE-TOKEN");
    auto& privacy = user->make<SqlIntentRedactions>(
        SqlIntentRedactions::from_bindings(owner, binding_context, operands, ""));
    event.with_diagnostic_redactions(&privacy);
    event.with_diagnostic_redactions(event.diagnostic_redactions());
    privacy.clear();
    event.trace_chain.emplace_back(TraceKind::AuditReason,
                                   "Order",
                                   "credential-secret 1001 PRIVATE-SIBLING PRIVATE-TOKEN",
                                   999);
    event.bootstrap_audit.emplace(std::allocator_arg,
                                  Allocator(owner),
                                  "actor PRIVATE-TOKEN",
                                  "category",
                                  "reason PRIVATE-SIBLING 1001",
                                  -3,
                                  UINT64_MAX);
    event.mutation_governance.emplace(std::allocator_arg,
                                      Allocator(owner),
                                      "execution",
                                      "Order.saveGraph",
                                      MutationPolicySource::Customer,
                                      MutationPolicyApprovalStatus::Approved);
    event.mutation_governance->policy.emplace(
        std::allocator_arg, Allocator(owner), "policy", "v1", "fingerprint");
    auto& safe = user->make<SafeAuditEvent>(event.build_safe_event(owner, {}, {}));
    check(
        safe.entity_id == 1001 && safe.trace_chain[0].entity_id == 999 && safe.bootstrap_audit &&
            safe.bootstrap_audit->actor == "actor [REDACTED]" &&
            safe.bootstrap_audit->reason == "reason [REDACTED] [REDACTED]",
        "safe target identity stays separate from lineage and bootstrap uses the same privacy source");
    {
        auto projected = detail::audit_event_projection(owner, event, false);
        check(!projected.diagnostic_redactions() && event.diagnostic_redactions() &&
                  projected.trace_chain[0].comment.find("PRIVATE-") == String::npos,
              "log projection consumes detached provenance without mutating raw event");
    }
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
              raw.diagnostic_redactions()->memory_resource() == global.memory_resource() &&
              projected.bootstrap_audit->memory_resource() == global.memory_resource() &&
              projected.entity_id == 1001 &&
              projected.fields[0].memory_resource() == global.memory_resource(),
          "Raw and safe nested data are independently owned by the selected destination");
    {
        auto text = debug_string(next->memory_resource(), projected, DebugOptions{.pretty = true});
        check(text.find("credential-secret") == text.npos &&
                  text.get_allocator().resource() == next->memory_resource(),
              "Retained safe events stay safe and format in the caller's User");
    }
    {
        auto safe_again = raw.build_safe_event(next->memory_resource(), {}, {});
        check(safe_again.trace_chain[0].comment.find("PRIVATE-") == String::npos &&
                  safe_again.bootstrap_audit->actor.find("PRIVATE-TOKEN") == String::npos,
              "retained raw provenance remains effective after source Context deletion");
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
