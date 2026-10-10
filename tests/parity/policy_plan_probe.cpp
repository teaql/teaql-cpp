#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Record record(Memory r, const Json& input) {
    Record out(r);
    for (const auto& [name, v] : std::get<Json::Object>(input.data)) {
        out.emplace(String(name, r), value(r, v));
    }
    return out;
}

static Json output(Memory r, const MutationPlan& plan) {
    auto out = detail::model_json(r);
    detail::model_put(out, "execution", plan.execution_id);
    detail::model_put(out, "request", plan.request_key);
    detail::model_put(out, "root", plan.root_entity_type);
    detail::model_put(out, "reason", plan.audit_reason ? Json(r, *plan.audit_reason) : Json(r));
    Json::Array ops(r);
    for (const auto& operation : plan.operations) {
        auto op = detail::model_json(r);
        constexpr std::string_view names[]{"Create", "Update", "Delete", "Recover"};
        detail::model_put(op, "kind", names[std::size_t(operation.kind)]);
        detail::model_put(op, "key", operation.entity.debug(r));
        detail::model_put(op,
                          "version",
                          operation.original_version ? Json(r, *operation.original_version)
                                                     : Json(r));
        auto values = detail::model_json(r);
        for (const auto& [name, v] : operation.changed_values) {
            detail::model_put(values, name, detail::debug_value(r, v));
        }
        detail::model_put(op, "values", std::move(values));
        ops.emplace_back(std::move(op));
    }
    detail::model_put(out, "operations", std::move(ops));
    return out;
}

static Json evaluate(Memory target, const Json& input) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->set_trace_id(text(field(input, "trace")));
    auto& node = user->make<GraphNode>(text(field(input, "entity")));
    if (std::holds_alternative<String>(field(input, "comment").data)) {
        node.set_comment(text(field(input, "comment")));
    }
    if (text(field(input, "mode")) == "graph") {
        auto& graph = user->make<GraphMutationPlan>();
        if (std::get<bool>(field(input, "root").data)) {
            graph.planned_root.emplace(std::allocator_arg, Allocator(r), node);
        }
        for (const auto& item : std::get<Json::Array>(field(input, "items").data)) {
            MutationValues values(r);
            values.fields = record(r, field(item, "values"));
            Vector<String> fields(r);
            for (const auto& f : std::get<Json::Array>(field(item, "fields").data)) {
                fields.emplace_back(text(f));
            }
            std::optional<EntitySnapshot> old;
            if (std::holds_alternative<Json::Object>(field(item, "old").data)) {
                old.emplace(r);
                old->fields = record(r, field(item, "old"));
            }
            graph.push(text(field(item, "entity")),
                       GraphMutationKind(std::get<std::uint64_t>(field(item, "kind").data)),
                       values,
                       fields,
                       nullptr,
                       old);
        }
        auto plan = detail::graph_policy_plan(*user, graph);
        if (plan) {
            return Json(target, output(r, plan.value()));
        }
        auto out = detail::model_json(r);
        detail::model_put(out, "error", plan.error().to_string(r));
        return Json(target, out);
    }
    auto& state = user->make<EntityRuntimeState>();
    if (std::holds_alternative<Json::Object>(field(input, "loaded").data)) {
        auto loaded = record(r, field(input, "loaded"));
        state.set_original_compact_row(node.entity,
                                       CompactRow(std::allocator_arg, Allocator(r), loaded));
    }
    for (const auto& seed : std::get<Json::Array>(field(input, "seed").data)) {
        EntityKey key(std::allocator_arg,
                      Allocator(r),
                      text(field(seed, "entity")),
                      value(r, field(seed, "id")));
        if (std::get<bool>(field(seed, "new").data)) {
            state.mark_as_new(key);
        }
        if (std::get<bool>(field(seed, "delete").data)) {
            state.mark_as_delete(key);
        }
        auto& version = field(seed, "version");
        if (auto v = std::get_if<std::int64_t>(&version.data)) {
            state.set_original_version(key, *v);
        } else if (auto v = std::get_if<std::uint64_t>(&version.data)) {
            state.set_original_version(key, std::int64_t(*v));
        }
        for (const auto& [name, v] : record(r, field(seed, "values"))) {
            state.set(key, name, v);
        }
    }
    if (std::holds_alternative<String>(field(input, "ledger_comment").data)) {
        state.set_comment(text(field(input, "ledger_comment")));
    }
    auto plan = detail::ledger_policy_plan(*user, node, state);
    return Json(target, output(r, plan));
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        Arena output(4096, std::pmr::new_delete_resource());
        auto input = Json::parse(&output, line);
        std::cout << evaluate(&output, *input).dump(&output) << '\n';
    }
}
