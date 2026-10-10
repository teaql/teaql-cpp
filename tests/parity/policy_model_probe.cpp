#include <teaql/runtime.hpp>
#include <teaql/runtime/mutation_policy_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <iostream>
#include <unordered_set>

static MutationPolicyIdentity identity(Memory owner, const Json& input) {
    return MutationPolicyIdentity(std::allocator_arg,
                                  Allocator(owner),
                                  text(field(input, "id")),
                                  text(field(input, "version")),
                                  text(field(input, "fingerprint")));
}

static MutationOperation operation(Memory owner, const Json& input) {
    auto kind_name = text(field(input, "kind"));
    auto kind = kind_name == "Create"   ? MutationOperationKind::Create
                : kind_name == "Update" ? MutationOperationKind::Update
                : kind_name == "Delete" ? MutationOperationKind::Delete
                                        : MutationOperationKind::Recover;
    const auto& key_input = field(input, "key");
    EntityKey key(std::allocator_arg,
                  Allocator(owner),
                  text(field(key_input, "entity")),
                  value(owner, field(key_input, "id")));
    MutationOperation result(std::allocator_arg, Allocator(owner), kind, key);
    if (std::holds_alternative<String>(field(input, "version").data)) {
        result.original_version = number<std::int64_t>(field(input, "version"));
    }
    for (const auto& [name, item] : detail::model_object(field(input, "values"), "values")) {
        result.changed_values.emplace(String(name, owner), value(owner, item));
    }
    return result;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto output = detail::model_json(owner);
    auto put = [&](std::string_view name, auto&& value) {
        detail::model_put(output, name, std::forward<decltype(value)>(value));
    };
    auto type = text(field(input, "type"));
    try {
        if (type == "identity") {
            auto value = identity(owner, input);
            auto clone = detail::clone_in(owner, value);
            auto other = identity(owner, field(input, "other"));
            std::pmr::unordered_set<MutationPolicyIdentity> set(owner);
            set.emplace(detail::clone_in(owner, value));
            set.emplace(detail::clone_in(owner, value));
            set.emplace(detail::clone_in(owner, other));
            put("debug", formatted_debug(owner, value));
            put("clone", formatted_debug(owner, clone));
            put("clone_equal", value == clone);
            put("other_equal", value == other);
            auto order = value <=> other;
            put("order", std::string_view(order < 0 ? "Less" : order > 0 ? "Greater" : "Equal"));
            put("set_size", std::uint64_t(set.size()));
        } else if (type == "decision") {
            Vector<String> fields(owner);
            for (const auto& item : detail::model_array(field(input, "fields"), "fields")) {
                fields.emplace_back(text(item));
            }
            auto value =
                std::get<bool>(field(input, "deny").data)
                    ? MutationDecision::deny(
                          owner, text(field(input, "code")), text(field(input, "message")), fields)
                    : MutationDecision::allow(owner);
            auto clone = detail::clone_in(owner, value);
            auto other = detail::clone_in(owner, value);
            other.message.emplace("changed", owner);
            put("debug", formatted_debug(owner, value));
            put("clone", formatted_debug(owner, clone));
            put("clone_equal", value == clone);
            put("changed_equal", value == other);
            put("allowed", value.is_allowed());
        } else if (type == "approval") {
            auto policy = identity(owner, field(input, "policy"));
            auto time = SystemTime::from_unix_seconds(
                number<std::int64_t>(field(input, "seconds")),
                detail::model_integer<std::uint32_t>(field(input, "nanos"), "nanos"));
            MutationPolicyApproval value(
                std::allocator_arg, Allocator(owner), policy, text(field(input, "approver")), time);
            auto clone = detail::clone_in(owner, value);
            auto other = detail::clone_in(owner, value);
            other.approved_by += '!';
            put("debug", formatted_debug(owner, value));
            put("clone", formatted_debug(owner, clone));
            put("clone_equal", value == clone);
            put("changed_equal", value == other);
        } else if (type == "defaults") {
            EmptyMutationPolicyRegistry registry;
            NoMutationPolicyApprovalProvider approvals;
            auto policy = identity(owner, field(input, "policy"));
            put("empty_registry", !registry.resolve(owner, "Order.saveGraph"));
            put("no_approval", !approvals.find_approval(owner, policy));
            put("warning", missing_mutation_policy_warning);
            put("approval_warning", missing_mutation_policy_approval_warning);
        } else if (type == "models") {
            MutationPlan plan(std::allocator_arg, Allocator(owner));
            plan.execution_id = text(field(input, "execution"));
            plan.request_key = text(field(input, "request"));
            plan.root_entity_type = text(field(input, "root"));
            if (std::holds_alternative<String>(field(input, "reason").data)) {
                plan.audit_reason.emplace(text(field(input, "reason")), owner);
            }
            for (const auto& item : detail::model_array(field(input, "operations"), "operations")) {
                plan.operations.emplace_back(operation(owner, item));
            }
            auto approval = text(field(input, "approval"));
            MutationGovernanceSnapshot snapshot(
                std::allocator_arg,
                Allocator(owner),
                plan.execution_id,
                plan.request_key,
                std::get<bool>(field(input, "customer").data)
                    ? MutationPolicySource::Customer
                    : MutationPolicySource::GeneratedDefault,
                approval == "Approved"  ? MutationPolicyApprovalStatus::Approved
                : approval == "Missing" ? MutationPolicyApprovalStatus::Missing
                                        : MutationPolicyApprovalStatus::NotApplicable);
            if (std::holds_alternative<Json::Object>(field(input, "policy").data)) {
                snapshot.policy.emplace(identity(owner, field(input, "policy")));
            }
            for (const auto& item : detail::model_array(field(input, "warnings"), "warnings")) {
                snapshot.warning_codes.emplace_back(text(item));
            }
            for (const auto& op : plan.operations) {
                auto& summary = snapshot.operations.emplace_back(op.kind, op.entity);
                for (const auto& [name, item] : op.changed_values) {
                    summary.changed_fields.emplace_back(name);
                }
            }
            MutationGovernanceEvent event(std::allocator_arg,
                                          Allocator(owner),
                                          snapshot,
                                          text(field(input, "warning")),
                                          std::get<bool>(field(input, "first").data));
            auto plan_clone = detail::clone_in(owner, plan);
            auto plan_other = detail::clone_in(owner, plan);
            plan_other.execution_id += '!';
            auto snapshot_clone = detail::clone_in(owner, snapshot);
            auto snapshot_other = detail::clone_in(owner, snapshot);
            snapshot_other.warning_codes.emplace_back("changed");
            auto event_clone = detail::clone_in(owner, event);
            auto event_other = detail::clone_in(owner, event);
            event_other.first_occurrence = !event_other.first_occurrence;
            put("plan", formatted_debug(owner, plan));
            put("plan_clone", formatted_debug(owner, plan_clone));
            put("plan_clone_equal", plan == plan_clone);
            put("plan_changed_equal", plan == plan_other);
            put("snapshot", formatted_debug(owner, snapshot));
            put("snapshot_clone", formatted_debug(owner, snapshot_clone));
            put("snapshot_clone_equal", snapshot == snapshot_clone);
            put("snapshot_changed_equal", snapshot == snapshot_other);
            put("event", formatted_debug(owner, event));
            put("event_clone", formatted_debug(owner, event_clone));
            put("event_clone_equal", event == event_clone);
            put("event_changed_equal", event == event_other);
        } else {
            throw std::runtime_error("Unknown policy model input");
        }
    } catch (const std::invalid_argument& error) {
        put("panic", std::string_view(error.what()));
    }
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid policy model fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
