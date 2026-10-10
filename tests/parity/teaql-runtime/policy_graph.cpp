// Original policy-review and graph-trace/batching contracts with explicit owners.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/graph.hpp>
#include <teaql/runtime/mutation_policy.hpp>
#include <iostream>

using namespace teaql;
using SinkResult = Result<std::monostate, RuntimeError>;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static MutationPolicyIdentity identity(Memory owner, std::string_view fingerprint = "sha256:test") {
    return MutationPolicyIdentity(
        std::allocator_arg, Allocator(owner), "order-policy", "1.0.0", fingerprint);
}

static MutationPlan plan(Memory owner) {
    MutationPlan result(std::allocator_arg, Allocator(owner));
    result.execution_id = "exec-1";
    result.request_key = "Order.saveGraph";
    result.root_entity_type = "Order";
    result.audit_reason.emplace("submit order", owner);
    EntityKey key(std::allocator_arg, Allocator(owner), "Order", Value(owner, std::uint64_t(7)));
    result.operations.emplace_back(MutationOperationKind::Create, key);
    result.operations.back().changed_values.emplace(String("total_amount", owner),
                                                    Value(owner, std::int64_t(25000)));
    return result;
}

class SourcePolicy final : public MutationPolicy {
public:
    using allocator_type = Allocator;
    MutationPolicyIdentity policy;
    bool deny;

    SourcePolicy(std::allocator_arg_t, Allocator a, bool deny)
        : policy(::identity(a.resource())), deny(deny) {}

    MutationPolicyIdentity identity(Memory owner) const override {
        return detail::clone_in(owner, policy);
    }

    MutationDecision
    review(Memory owner, const UserContext&, const MutationPlan& reviewed) const override {
        check(reviewed == plan(owner), "the policy receives the complete original mutation plan");
        if (!deny) {
            return MutationDecision::allow(owner);
        }
        Vector<String> fields(owner);
        fields.emplace_back("total_amount");
        return MutationDecision::deny(owner, "ORDER-DENIED", "order mutation denied", fields);
    }
};

static void install_policy(UserContext& user, bool deny) {
    auto policy = user.make_resource_ref<SourcePolicy>(deny);
    auto registry = user.make_ref<FunctionMutationPolicyRegistry>(
        [&user, policy](Memory owner,
                        std::string_view request) -> std::optional<MutationPolicyBorrow> {
            check(request == "Order.saveGraph", "original registry request key");
            return MutationPolicyBorrow(std::allocator_arg,
                                        Allocator(owner),
                                        user.get(policy),
                                        detail::resource_arena(user.memory_resource()),
                                        detail::reference_slot(policy.id()));
        });
    user.set_mutation_policy_registry(registry);
}

static void generated_default_allows_with_warning_and_deduplicates_log_occurrence() {
    GlobalContext global;
    Ref<Vector<MutationGovernanceEvent>> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        retained = user->make_global<Vector<MutationGovernanceEvent>>();
        auto sink = user->make_ref<FunctionMutationGovernanceSink>(
            [&user,
             retained](Memory owner, const UserContext&, const MutationGovernanceEvent& event) {
                user->get(retained).emplace_back(event);
                return SinkResult::ok(owner, {});
            });
        user->set_mutation_governance_sink(sink);
        auto first = user->review_mutation_plan(plan(owner));
        auto second = user->review_mutation_plan(plan(owner));
        check(first && second && first.value().source == MutationPolicySource::GeneratedDefault &&
                  first.value().warning_codes ==
                      Vector<String>({String(missing_mutation_policy_warning, owner)}, owner) &&
                  second.value().warning_codes == first.value().warning_codes,
              "all original generated-default provenance and complete warning-code assertions");
        const auto& events = user->get(retained);
        check(events.size() == 2 && events[0].first_occurrence && !events[1].first_occurrence,
              "both original warning occurrences are delivered with exact deduplication flags");
    }
    check(global.live_user_bytes() == 0, "actual User deletion returns all review storage");
    auto next = global.make_user_context();
    const auto& events = next->get(next->rebind_global(retained));
    check(
        events.size() == 2 && events[0].snapshot.operations.size() == 1 &&
            events[0].snapshot.operations[0].changed_fields ==
                Vector<String>({String("total_amount", next->memory_resource())},
                               next->memory_resource()),
        "explicit Global event copies preserve original nested operation summaries after User deletion");
}

static void warning_sink_failure_is_fail_open_for_the_business_save() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto sink = user->make_ref<FunctionMutationGovernanceSink>(
        [](Memory owner, const UserContext&, const MutationGovernanceEvent&) {
            return SinkResult::err(owner, RuntimeError::event(owner, "warning sink unavailable"));
        });
    user->set_mutation_governance_sink(sink);
    auto snapshot = user->review_mutation_plan(plan(owner));
    check(
        snapshot && snapshot.value().source == MutationPolicySource::GeneratedDefault &&
            snapshot.value().warning_codes ==
                Vector<String>({String(missing_mutation_policy_warning, owner)}, owner),
        "original warning Result failure cannot reject the allowed review or discard its warning");
}

static void customer_approval(bool approved) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    install_policy(*user, false);
    auto provider = user->make_ref<FunctionMutationPolicyApprovalProvider>(
        [approved](Memory owner, const MutationPolicyIdentity& requested) {
            check(requested == identity(owner), "original full approval lookup identity");
            return std::optional<MutationPolicyApproval>(
                std::in_place,
                std::allocator_arg,
                Allocator(owner),
                identity(owner, approved ? "sha256:test" : "sha256:other"),
                "security-review",
                SystemTime{});
        });
    user->set_mutation_policy_approval_provider(provider);
    auto snapshot = user->review_mutation_plan(plan(owner));
    check(snapshot.is_ok(), "original customer allow policy succeeds");
    if (approved) {
        check(snapshot.value().approval_status == MutationPolicyApprovalStatus::Approved &&
                  snapshot.value().warning_codes.empty(),
              "original exact approval has no warning");
    } else {
        check(snapshot.value().source == MutationPolicySource::Customer &&
                  snapshot.value().approval_status == MutationPolicyApprovalStatus::Missing &&
                  snapshot.value().warning_codes ==
                      Vector<String>({String(missing_mutation_policy_approval_warning, owner)},
                                     owner),
              "original fingerprint mismatch is missing approval with its complete warning code");
    }
}

static void denied_customer_policy_fails_before_execution() {
    GlobalContext global;
    auto user = global.make_user_context();
    install_policy(*user, true);
    unsigned persisted = 0;
    auto result = user->review_mutation_plan(plan(user->memory_resource()));
    if (result.is_ok()) {
        ++persisted;
    }
    check(!result && result.error().kind() == RuntimeError::Kind::Policy &&
              result.error().message().find("ORDER-DENIED") != String::npos && persisted == 0,
          "original typed policy denial gates the persistence counter before execution");
}

static void test_hierarchical_trace_chain_recovery(Memory owner) {
    TraceNode root(
        std::allocator_arg, Allocator(owner), TraceKind::Entity, "User", "Create User", 1);
    TraceNode child(
        std::allocator_arg, Allocator(owner), TraceKind::Entity, "Profile", "Create Profile");
    TraceNode empty(std::allocator_arg, Allocator(owner), TraceKind::Entity, "AuditLog", "");
    ScopedCommentNode root_scope(std::allocator_arg, Allocator(owner), nullptr, root);
    ScopedCommentNode child_scope(std::allocator_arg, Allocator(owner), &root_scope, child);
    ScopedCommentNode empty_scope(std::allocator_arg, Allocator(owner), &child_scope, empty);
    auto chain = empty_scope.to_trace_chain(owner);
    check(
        chain.size() == 2 && chain[0] == root && chain[1] == child,
        "original synchronous trace recovery omits only empty comments and preserves complete nodes");
    auto& arena = detail::resource_arena(owner);
    auto& root_token = arena.make<TraceScopeToken>(nullptr, root, 0);
    auto& child_token = arena.make<TraceScopeToken>(&root_token, child, 1);
    auto& empty_token = arena.make<TraceScopeToken>(&child_token, empty, 2);
    chain = empty_token.recover_trace_chain(owner);
    check(chain.size() == 2 && chain[0] == root && chain[1] == child,
          "original persistent token recovery has the same complete ordered trace");
}

static GraphMutationPlan test_graph_mutation_plan_batching_keys_and_counts(Memory owner) {
    GraphMutationPlan graph(std::allocator_arg, Allocator(owner));
    MutationValues values(owner);
    graph.push("User", GraphMutationKind::Create, values, {});
    graph.push("User", GraphMutationKind::Create, values, {});
    Vector<String> fields(owner);
    fields.emplace_back("name");
    graph.push("User", GraphMutationKind::Update, values, fields);
    graph.push("User", GraphMutationKind::Update, values, fields);
    fields[0] = "email";
    graph.push("User", GraphMutationKind::Update, values, fields);
    graph.push("Profile", GraphMutationKind::Create, values, {});
    check(graph.len() == 6, "all six original plan items");
    graph.rebuild_batches();
    check(graph.batch_count() == 4 && graph.batches[0].entity == "User" &&
              graph.batches[0].kind == GraphMutationKind::Create &&
              graph.batches[1].update_fields == Vector<String>({String("name", owner)}, owner) &&
              graph.batches[2].update_fields == Vector<String>({String("email", owner)}, owner) &&
              graph.batches[3].entity == "Profile",
          "all original batch keys and order");
    check(graph.batches[0].items.size() == 2 && graph.batches[1].items.size() == 2 &&
              graph.batches[2].items.size() == 1 && graph.batches[3].items.size() == 1,
          "complete original batch membership counts");
    auto counts = graph.grouped_counts(owner);
    auto count_is = [&](std::string_view entity, GraphMutationKind kind, std::size_t expected) {
        auto found = counts.find({String(entity, owner), kind});
        return found != counts.end() && found->second == expected;
    };
    check(counts.size() == 3 && count_is("User", GraphMutationKind::Create, 2) &&
              count_is("User", GraphMutationKind::Update, 3) &&
              count_is("Profile", GraphMutationKind::Create, 1),
          "all original grouped counts");
    return graph;
}

static void graph_contracts() {
    GlobalContext global;
    Ref<GraphMutationPlan> retained;
    {
        auto user = global.make_user_context();
        test_hierarchical_trace_chain_recovery(user->memory_resource());
        auto graph = test_graph_mutation_plan_batching_keys_and_counts(user->memory_resource());
        retained = user->make_global<GraphMutationPlan>(graph);
    }
    check(global.live_user_bytes() == 0, "actual User deletion returns all trace/plan storage");
    auto next = global.make_user_context();
    const auto& graph = next->get(next->rebind_global(retained));
    check(graph.len() == 6 && graph.batch_count() == 4 &&
              graph.batches[1].update_fields.front() == "name" &&
              graph.batches[1].update_fields.front().get_allocator().resource() ==
                  graph.memory_resource(),
          "explicit Global plan copy retains batches and nested field strings after User deletion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        generated_default_allows_with_warning_and_deduplicates_log_occurrence();
        warning_sink_failure_is_fail_open_for_the_business_save();
        customer_approval(false);
        customer_approval(true);
        denied_customer_policy_fails_before_execution();
        graph_contracts();
        std::pmr::set_default_resource(previous);
        std::cout << "Original seven mutation-policy/graph contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
