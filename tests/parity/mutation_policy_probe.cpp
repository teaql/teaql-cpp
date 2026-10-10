#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>
using Outcome = Result<std::monostate, RuntimeError>;

static Json snapshot(Memory r, const MutationGovernanceSnapshot& s) {
    Json::Object result(r);
    result.emplace(
        String("source", r),
        Json(r, s.source == MutationPolicySource::Customer ? "Customer" : "GeneratedDefault"));
    result.emplace(String("approval", r),
                   Json(r,
                        s.approval_status == MutationPolicyApprovalStatus::Approved ? "Approved"
                        : s.approval_status == MutationPolicyApprovalStatus::Missing
                            ? "Missing"
                            : "NotApplicable"));
    Json::Array warnings(r), operations(r);
    for (const auto& warning : s.warning_codes) {
        warnings.emplace_back(warning);
    }
    for (const auto& operation : s.operations) {
        Json::Object op(r);
        Json::Array fields(r);
        for (const auto& name : operation.changed_fields) {
            fields.emplace_back(name);
        }
        auto kind = operation.kind == MutationOperationKind::Create   ? "Create"
                    : operation.kind == MutationOperationKind::Update ? "Update"
                    : operation.kind == MutationOperationKind::Delete ? "Delete"
                                                                      : "Recover";
        op.emplace(String("kind", r), Json(r, kind));
        op.emplace(String("entity", r), Json(r, operation.entity.entity));
        op.emplace(String("fields", r), Json(r, std::move(fields)));
        operations.emplace_back(std::move(op));
    }
    result.emplace(String("warnings", r), Json(r, std::move(warnings)));
    result.emplace(String("operations", r), Json(r, std::move(operations)));
    return Json(r, std::move(result));
}

struct State {
    using allocator_type = Allocator;
    Json input;
    Json::Array notes, events;

    State(std::allocator_arg_t, Allocator a, const Json& j)
        : input(a.resource(), j), notes(a.resource()), events(a.resource()) {}

    Memory memory_resource() const {
        return input.memory_resource();
    }

    void note(std::string_view text) {
        notes.emplace_back(text);
    }
};

struct Policy final : MutationPolicy {
    State& s;

    explicit Policy(State& s) : s(s) {}

    MutationPolicyIdentity identity(Memory r) const override {
        s.note("identity");
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), " policy ", "5.0.5", "fingerprint");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan&) const override {
        s.note("review");
        auto kind = text(field(s.input, "policy"));
        if (kind == "throw") {
            throw std::runtime_error("policy exception");
        }
        auto decision = MutationDecision::allow(r);
        if (kind == "deny") {
            decision.verdict = MutationVerdict::Deny;
            for (auto [name, target] :
                 {std::pair{"code", &decision.code}, std::pair{"message", &decision.message}}) {
                auto& v = field(s.input, name);
                if (!std::holds_alternative<std::monostate>(v.data)) {
                    target->emplace(text(v), r);
                }
            }
            decision.field_paths.emplace_back("unrendered.path");
        }
        return decision;
    }
};

static Json evaluate(Memory output, const Json& input) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& state = user->make<State>(input);
    auto& policy = user->make_resource<Policy>(state);
    auto registry = user->make_ref<FunctionMutationPolicyRegistry>(
        [&state, &policy, r](Memory target,
                             std::string_view) -> std::optional<MutationPolicyBorrow> {
            state.note("resolve");
            auto kind = text(field(state.input, "policy"));
            if (kind == "registry_throw") {
                throw std::runtime_error("registry exception");
            }
            if (kind == "none") {
                return {};
            }
            return MutationPolicyBorrow(
                std::allocator_arg, Allocator(target), policy, detail::resource_arena(r));
        });
    user->set_mutation_policy_registry(registry);
    auto provider = user->make_ref<FunctionMutationPolicyApprovalProvider>(
        [&state](Memory r,
                 const MutationPolicyIdentity& identity) -> std::optional<MutationPolicyApproval> {
            state.note("approval");
            auto mode = text(field(state.input, "approval"));
            if (mode == "throw") {
                throw std::runtime_error("approval exception");
            }
            if (mode == "none") {
                return {};
            }
            MutationPolicyApproval approval(std::allocator_arg,
                                            Allocator(r),
                                            identity,
                                            "reviewer",
                                            SystemTime::from_unix_seconds(0));
            if (mode == "id") {
                approval.policy.id += 'x';
            } else if (mode == "version") {
                approval.policy.version += 'x';
            } else if (mode == "fingerprint") {
                approval.policy.fingerprint += 'x';
            } else if (mode == "trim") {
                approval.policy.id = "policy";
            } else if (mode == "blank") {
                approval.approved_by = "\xe2\x80\x83";
            }
            return approval;
        });
    user->set_mutation_policy_approval_provider(provider);
    auto sink = user->make_ref<FunctionMutationGovernanceSink>(
        [&state](Memory r, const UserContext&, const MutationGovernanceEvent& event) {
            state.note("sink");
            Json::Object e(r);
            e.emplace(String("first", r), Json(r, event.first_occurrence));
            e.emplace(String("warning", r), Json(r, event.warning_code));
            e.emplace(String("snapshot", r), snapshot(r, event.snapshot));
            state.events.emplace_back(std::move(e));
            auto mode = text(field(state.input, "sink"));
            if (mode == "throw") {
                throw std::runtime_error("sink exception");
            }
            if (mode == "error") {
                return Outcome::err(r, RuntimeError::event(r, "delivery failed"));
            }
            return Outcome::ok(r, {});
        });
    user->set_mutation_governance_sink(sink);
    auto& plan = user->make<MutationPlan>();
    plan.execution_id = "execution";
    plan.request_key = text(field(input, "request"));
    plan.root_entity_type = "Order";
    for (auto kind : {MutationOperationKind::Create,
                      MutationOperationKind::Update,
                      MutationOperationKind::Delete,
                      MutationOperationKind::Recover}) {
        EntityKey key(std::allocator_arg,
                      Allocator(r),
                      "Order",
                      Value(r, std::uint64_t(plan.operations.size())));
        plan.operations.emplace_back(kind, key);
        auto& op = plan.operations.back();
        op.original_version = 9;
        op.changed_values.emplace(String("z", r), Value(r, "private"));
        op.changed_values.emplace(String("a", r), Value(r));
    }
    Json::Array outcomes(r);
    auto repeats = std::get<std::uint64_t>(field(input, "repeat").data);
    for (unsigned i = 0; i < repeats; ++i) {
        try {
            auto result = user->review_mutation_plan(plan);
            if (result) {
                outcomes.emplace_back(snapshot(r, result.value()));
            } else {
                Json::Object error(r);
                error.emplace(String("error", r), Json(r, result.error().message()));
                outcomes.emplace_back(std::move(error));
            }
        } catch (const std::runtime_error&) {
            Json::Object error(r);
            error.emplace(String("panic", r), Json(r, true));
            outcomes.emplace_back(std::move(error));
        }
    }
    Json::Object result(r);
    result.emplace(String("outcomes", r), Json(r, std::move(outcomes)));
    result.emplace(String("notes", r), Json(r, std::move(state.notes)));
    result.emplace(String("events", r), Json(r, std::move(state.events)));
    return Json(output, Json(r, std::move(result)));
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena output(4096, std::pmr::new_delete_resource());
        auto input = Json::parse(&output, line);
        std::cout << evaluate(&output, *input).dump(&output) << '\n';
    }
}
