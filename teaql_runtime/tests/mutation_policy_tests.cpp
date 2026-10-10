#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <iostream>

using namespace teaql;
using Outcome = Result<std::monostate, RuntimeError>;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Policy final : MutationPolicy {
    using allocator_type = Allocator;
    MutationPolicyIdentity policy;
    MutationDecision decision;
    mutable int identities = 0, reviews = 0;

    Policy(std::allocator_arg_t, Allocator a)
        : policy(std::allocator_arg, a, " policy ", "5.0.5", "fingerprint"),
          decision(std::allocator_arg, a) {}

    Memory memory_resource() const {
        return policy.memory_resource();
    }

    MutationPolicyIdentity identity(Memory r) const override {
        ++identities;
        return detail::clone_in(r, policy);
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan&) const override {
        check(identities == reviews + 1, "identity must precede review");
        ++reviews;
        return detail::clone_in(r, decision);
    }
};

template <class F>
static void expired(F&& action) {
    try {
        action();
    } catch (const ReferenceError&) {
        return;
    }
    throw std::runtime_error("expired policy resource was accepted");
}

static void review_and_scopes() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& events = global.make<Vector<MutationGovernanceEvent>>();
    MutationGovernanceSnapshot* retained = nullptr;
    {
        auto r = user->memory_resource();
        int mode = 0, approvals = 0;
        auto sink = user->make_ref<FunctionMutationGovernanceSink>(
            [&events, &mode](Memory r, const UserContext&, const MutationGovernanceEvent& event) {
                events.emplace_back(event);
                if (mode == 1) {
                    return Outcome::err(r, RuntimeError::event(r, "delivery failed"));
                }
                if (mode == 2) {
                    throw std::runtime_error("sink exception");
                }
                return Outcome::ok(r, {});
            });
        user->set_mutation_governance_sink(sink);
        auto& plan = user->make<MutationPlan>();
        plan.execution_id = "execution";
        plan.request_key = "Order.saveGraph";
        plan.root_entity_type = "Order";
        EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, std::int64_t(42)));
        plan.operations.emplace_back(MutationOperationKind::Recover, key);
        plan.operations.back().changed_values.emplace(String("z", r), Value(r, "secret"));
        plan.operations.back().changed_values.emplace(String("a", r), Value(r, std::int64_t(3)));
        mode = 1;
        auto first = user->review_mutation_plan(plan);
        check(first && first.value().source == MutationPolicySource::GeneratedDefault,
              "missing policy allows with generated-default provenance");
        check(user->review_mutation_plan(plan).is_ok(), "sink Result errors are fail-open");
        check(events.size() == 2 && events[0].first_occurrence && !events[1].first_occurrence,
              "every warning is delivered; failed delivery does not refund deduplication");
        check(events[0].snapshot.operations[0].changed_fields[0] == "a" &&
                  events[0].snapshot.operations[0].changed_fields[1] == "z",
              "summaries sort names");

        auto policy_ref = user->make_global<Policy>();
        auto& policy = user->get(policy_ref);
        auto* owner = &detail::resource_arena(policy.memory_resource());
        auto registry = user->make_ref<FunctionMutationPolicyRegistry>(
            [&policy, owner](Memory r, std::string_view) -> std::optional<MutationPolicyBorrow> {
                return MutationPolicyBorrow(std::allocator_arg, Allocator(r), policy, *owner);
            });
        user->set_mutation_policy_registry(registry);
        auto provider = user->make_ref<FunctionMutationPolicyApprovalProvider>(
            [&approvals](Memory r, const MutationPolicyIdentity& identity) {
                ++approvals;
                return std::optional<MutationPolicyApproval>(std::in_place,
                                                             std::allocator_arg,
                                                             Allocator(r),
                                                             identity,
                                                             "reviewer",
                                                             SystemTime::from_unix_seconds(1));
            });
        user->set_mutation_policy_approval_provider(provider);
        mode = 0;
        auto approved = user->review_mutation_plan(plan);
        check(approved &&
                  approved.value().approval_status == MutationPolicyApprovalStatus::Approved &&
                  approved.value().warning_codes.empty() && approvals == 1,
              "full identity approval");
        retained = &global.make<MutationGovernanceSnapshot>(approved.value());
        check(retained->policy->id.get_allocator().resource() == global.memory_resource(),
              "retention explicitly deep-copies identity into Global");

        policy.decision = MutationDecision::deny(policy.memory_resource(), " DENIED ", "", {});
        auto denied = user->review_mutation_plan(plan);
        check(!denied && denied.error().kind() == RuntimeError::Kind::Policy &&
                  denied.error().message() == "[MUTATION POLICY DENIED]  DENIED : " &&
                  approvals == 1,
              "denial preserves strings and stops before approval and warning delivery");
        policy.decision.code.reset();
        policy.decision.message.reset();
        denied = user->review_mutation_plan(plan);
        check(denied.error().message() ==
                  "[MUTATION POLICY DENIED] MUTATION-POLICY-DENIED: mutation rejected",
              "absent denial fields receive source defaults");
        policy.decision = MutationDecision::allow(policy.memory_resource());
        user->destroy(provider);
        expired([&] {
            (void)user->review_mutation_plan(plan);
        });
        user->set_mutation_policy_approval_provider(
            user->make_ref<NoMutationPolicyApprovalProvider>());
        mode = 2;
        try {
            (void)user->review_mutation_plan(plan);
            throw std::logic_error("sink exception swallowed");
        } catch (const std::runtime_error& error) {
            check(std::string_view(error.what()) == "sink exception", "sink exceptions propagate");
        }
        mode = 0;
        auto missing = user->review_mutation_plan(plan);
        check(missing && missing.value().approval_status == MutationPolicyApprovalStatus::Missing &&
                  !events.back().first_occurrence,
              "dedup records before a throwing sink");
        user->destroy(policy_ref);
        expired([&] {
            (void)user->review_mutation_plan(plan);
        });
    }
    user.reset();
    check(retained->policy->id == " policy " && retained->operations[0].changed_fields[1] == "z",
          "nested retained snapshots survive actual User destruction");
    auto next = global.make_user_context();
    auto& next_plan = next->make<MutationPlan>();
    next_plan.request_key = "Order.saveGraph";
    auto next_sink = next->make_ref<FunctionMutationGovernanceSink>(
        [&events](Memory r, const UserContext&, const MutationGovernanceEvent& event) {
            events.emplace_back(event);
            return Outcome::ok(r, {});
        });
    next->set_mutation_governance_sink(next_sink);
    check(next->review_mutation_plan(next_plan).is_ok() && events.back().first_occurrence,
          "warning dedup belongs to each User");
}

static void plan_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->set_trace_id("trace");
    auto& graph = user->make<GraphMutationPlan>();
    check(!detail::graph_policy_plan(*user, graph), "planned root is mandatory");
    graph.planned_root.emplace(std::allocator_arg, Allocator(r), "Order");
    graph.planned_root->set_comment("");
    MutationValues values(r);
    values.fields.emplace(String("version", r), Value(r, std::int64_t(7)));
    values.fields.emplace(String("name", r), Value(r, "changed"));
    Vector<String> fields(r);
    fields.emplace_back("missing");
    fields.emplace_back("name");
    fields.emplace_back("name");
    graph.push("Order", GraphMutationKind::Reference, values, fields);
    graph.push("Order", GraphMutationKind::Create, values, fields);
    graph.push("Order", GraphMutationKind::Update, values, fields);
    graph.push("Order", GraphMutationKind::Delete, values, fields);
    auto plan = detail::graph_policy_plan(*user, graph);
    check(plan && plan.value().execution_id == "trace-mutation-1" && plan.value().audit_reason &&
              plan.value().audit_reason->empty() && plan.value().operations.size() == 3,
          "root, empty audit reason and skipped references");
    check(plan.value().operations[0].entity.id.try_i64() == 0 &&
              plan.value().operations[0].changed_values.size() == 2 &&
              plan.value().operations[1].changed_values.size() == 1 &&
              plan.value().operations[2].changed_values.empty() &&
              plan.value().operations[2].original_version == 7,
          "graph operation projections");
    auto& state = user->make<EntityRuntimeState>();
    EntityKey key(std::allocator_arg, Allocator(r), "Order", Value(r, std::int64_t(3)));
    EntityKey cancelled(std::allocator_arg, Allocator(r), "Order", Value(r, std::int64_t(4)));
    EntityKey untouched(std::allocator_arg, Allocator(r), "Order", Value(r, std::int64_t(5)));
    state.set(key, "name", Value(r, "deleted change"));
    state.mark_as_delete(key);
    check(state.current_change_set().is_empty(), "deletion clears prior pending changes");
    state.set(key, "name", Value(r, "change recorded after deletion"));
    state.set_original_version(key, 9);
    state.mark_as_new(cancelled);
    state.mark_as_delete(cancelled);
    state.mark_as_new(untouched);
    state.set_comment("");
    auto ledger = detail::ledger_policy_plan(*user, *graph.planned_root, state);
    check(ledger.execution_id == "trace-mutation-2" && ledger.operations.size() == 1 &&
              ledger.operations[0].kind == MutationOperationKind::Delete &&
              ledger.operations[0].original_version == 9 &&
              ledger.operations[0].changed_values.size() == 1 && ledger.audit_reason->empty(),
          "ledger excludes cancelled and untouched new keys but retains delete changes");
    GlobalContext second;
    auto other = second.make_user_context();
    other->set_trace_id("trace");
    auto& other_graph = other->make<GraphMutationPlan>(graph);
    auto other_plan = detail::graph_policy_plan(*other, other_graph);
    check(other_plan.value().execution_id == "trace-mutation-3",
          "execution namespace spans runtimes");
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "reentrant-close") {
        std::set_terminate([] {
            std::_Exit(0);
        });
        try {
            GlobalContext global;
            auto user = global.make_user_context();
            auto sink = user->make_ref<FunctionMutationGovernanceSink>(
                [&user](Memory r, const UserContext&, const MutationGovernanceEvent&) {
                    user->close();
                    return Outcome::ok(r, {});
                });
            user->set_mutation_governance_sink(sink);
            auto& plan = user->make<MutationPlan>();
            (void)user->review_mutation_plan(plan);
        } catch (...) {
            std::_Exit(2);
        }
        return 1;
    }
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        review_and_scopes();
        plan_contracts();
        std::pmr::set_default_resource(previous);
        std::cout << "Mutation policy Context contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
