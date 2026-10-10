#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <charconv>
#include <mutex>

namespace teaql::detail {
namespace {
String next_execution_id(const UserContext& context) {
    // A process-local scalar namespace, as in Rust. It owns no Context data.
    // A mutex avoids requiring a 64-bit atomic runtime on 32-bit MIPS.
    static std::mutex lock;
    static std::uint64_t sequence = 1;
    std::uint64_t value;
    {
        std::lock_guard guard(lock);
        value = sequence++;
    }
    String result(context.trace_id(), context.memory_resource());
    result += "-mutation-";
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), value).ptr;
    result.append(buffer, end);
    return result;
}
}

Result<MutationPlan, RuntimeError> graph_policy_plan(const UserContext& context,
                                                     const GraphMutationPlan& graph) {
    auto r = context.memory_resource();
    if (!graph.planned_root) {
        return Result<MutationPlan, RuntimeError>::err(
            r, RuntimeError::graph(r, "graph mutation plan has no planned root"));
    }
    MutationPlan plan(std::allocator_arg, Allocator(r));
    const auto& root = *graph.planned_root;
    plan.root_entity_type = root.entity;
    plan.request_key = root.entity;
    plan.request_key += ".saveGraph";
    plan.audit_reason = clone_in(r, root.comment);
    for (const auto& item : graph.items) {
        if (item.kind == GraphMutationKind::Reference) {
            continue;
        }
        auto kind = item.kind == GraphMutationKind::Create   ? MutationOperationKind::Create
                    : item.kind == GraphMutationKind::Delete ? MutationOperationKind::Delete
                                                             : MutationOperationKind::Update;
        const auto& values = item.values.fields;
        auto id = values.find(std::string_view("id"));
        Value missing_id(r, std::int64_t(0));
        EntityKey key(std::allocator_arg,
                      Allocator(r),
                      item.entity,
                      id == values.end() ? missing_id : id->second);
        plan.operations.emplace_back(kind, key);
        auto& operation = plan.operations.back();
        auto version = values.find(std::string_view("version"));
        if (version != values.end()) {
            operation.original_version = version->second.try_i64();
        }
        if (kind == MutationOperationKind::Create) {
            operation.changed_values = clone_record(r, values);
        } else if (kind == MutationOperationKind::Update) {
            for (const auto& field : item.update_fields) {
                auto found = values.find(field);
                if (found != values.end()) {
                    operation.changed_values.insert_or_assign(String(field, r),
                                                              Value(r, found->second));
                }
            }
        }
    }
    plan.execution_id = next_execution_id(context);
    return Result<MutationPlan, RuntimeError>::ok(r, std::move(plan));
}

MutationPlan ledger_policy_plan(const UserContext& context,
                                const GraphNode& node,
                                const EntityRuntimeState& root) {
    auto r = context.memory_resource();
    auto changes = root.current_change_set();
    auto deleted = root.deleted_keys();
    auto fresh = root.new_keys();
    EntityRuntimeState::Keys keys(r);
    for (const auto& [key, values] : changes.changes()) {
        keys.emplace(key);
    }
    for (const auto& key : deleted) {
        keys.emplace(key);
    }
    MutationPlan plan(std::allocator_arg, Allocator(r));
    plan.request_key = node.entity;
    plan.request_key += ".saveGraph";
    plan.root_entity_type = node.entity;
    auto comment = root.get_comment();
    plan.audit_reason = clone_in(r, comment ? comment : node.comment);
    for (const auto& key : keys) {
        if (fresh.contains(key) && deleted.contains(key)) {
            continue;
        }
        auto kind = deleted.contains(key) ? MutationOperationKind::Delete
                    : fresh.contains(key) ? MutationOperationKind::Create
                                          : MutationOperationKind::Update;
        plan.operations.emplace_back(kind, key);
        auto& operation = plan.operations.back();
        operation.original_version = root.get_original_version(key);
        auto found = changes.changes().find(key);
        if (found != changes.changes().end()) {
            operation.changed_values = clone_record(r, found->second.fields);
        }
    }
    plan.execution_id = next_execution_id(context);
    return plan;
}
}
