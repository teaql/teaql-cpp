#include <teaql/runtime.hpp>
#include <teaql/runtime/mutation_policy_debug.hpp>
#include <iostream>
#include <unordered_set>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto source = user->make_ref<MutationPolicyIdentity>(" policy ", " version ", " fingerprint ");
    auto& policy = user->get(source);
    {
        std::pmr::unordered_set<MutationPolicyIdentity> identities(owner);
        identities.emplace(detail::clone_in(owner, policy));
        identities.emplace(detail::clone_in(owner, policy));
        auto different = detail::clone_in(owner, policy);
        different.version += '!';
        identities.emplace(std::move(different));
        check(identities.size() == 2 && policy.id == " policy ",
              "Hash/equality preserve every identity field and untrimmed constructor data");
    }
    auto& plan = user->make<MutationPlan>();
    plan.execution_id.assign(512, 'e');
    plan.request_key = "Order.saveGraph";
    plan.root_entity_type = "Order";
    plan.audit_reason.emplace(512, 'r', owner);
    EntityKey key(std::allocator_arg, Allocator(owner), "Order", Value(owner, std::uint64_t(7)));
    auto& operation = plan.operations.emplace_back(MutationOperationKind::Update, key);
    operation.original_version = -3;
    operation.changed_values.emplace(String("private payload", owner),
                                     Value(owner, String(512, 'p', owner)));
    auto& snapshot = user->make<MutationGovernanceSnapshot>(plan.execution_id,
                                                            plan.request_key,
                                                            MutationPolicySource::Customer,
                                                            MutationPolicyApprovalStatus::Approved);
    snapshot.policy.emplace(std::allocator_arg, Allocator(owner), policy);
    snapshot.warning_codes.emplace_back("warning");
    auto& summary = snapshot.operations.emplace_back(operation.kind, operation.entity);
    summary.changed_fields.emplace_back("private payload");
    auto& event = user->make<MutationGovernanceEvent>(snapshot, "warning", true);
    auto retained_plan = user->make_global<MutationPlan>(plan);
    auto retained_event = user->make_global<MutationGovernanceEvent>(event);
    auto retained_policy = user->make_global<MutationPolicyIdentity>(policy);
    user.reset();
    check(global.live_user_bytes() == 0, "Original policy User arena is fully reclaimed");
    auto next = global.make_user_context();
    const auto& kept_plan = next->get(next->rebind_global(retained_plan));
    const auto& kept_event = next->get(next->rebind_global(retained_event));
    const auto& kept_policy = next->get(next->rebind_global(retained_policy));
    check(kept_plan.operations[0].changed_values.begin()->second.memory_resource() ==
                  global.memory_resource() &&
              kept_event.snapshot.operations[0].changed_fields[0].get_allocator().resource() ==
                  global.memory_resource() &&
              kept_event.snapshot.policy->memory_resource() == global.memory_resource(),
          "All nested policy payloads belong to the explicit destination domain");
    {
        auto debug = debug_string(next->memory_resource(), kept_plan, DebugOptions{.pretty = true});
        check(debug.find("private payload") != debug.npos &&
                  debug.get_allocator().resource() == next->memory_resource(),
              "Raw diagnostics preserve actual values and use the caller's allocation domain");
        check(std::hash<MutationPolicyIdentity>{}(kept_policy) ==
                  std::hash<MutationPolicyIdentity>{}(*kept_event.snapshot.policy),
              "Equal retained identities retain equal native hash behavior");
    }
    bool rejected = false;
    try {
        (void)next->get(source);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Expired source policy access rejects before hashing or formatting");
    std::cout << "Policy model hash and ownership contracts pass\n";
}
