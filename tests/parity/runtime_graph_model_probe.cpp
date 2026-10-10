#define main teaql_original_graph_probe_main
#include "runtime_graph_probe.cpp"
#undef main
#include <teaql/runtime/graph_debug.hpp>
#include "debug_modes.hpp"

static Json graph_models(UserContext& context, const Json& input) {
    auto resource = context.memory_resource();
    auto original = node(resource, field(input, "node"));
    auto copied = detail::clone_in(resource, original);
    auto changed = detail::clone_in(resource, copied);
    changed.set_comment("changed field");
    EntityValues values(std::allocator_arg,
                        Allocator(resource),
                        record(resource, field(field(input, "node"), "values")));
    auto mutation = values.into_mutation_values(resource);
    EntityValues roundtrip(std::allocator_arg, Allocator(resource), mutation);
    const TraceScopeToken* token = nullptr;
    const ScopedCommentNode* scope = nullptr;
    std::uint64_t index = 0;
    for (const auto& comment : std::get<Json::Array>(field(input, "trace").data)) {
        TraceNode track(std::allocator_arg,
                        Allocator(resource),
                        TraceKind::Entity,
                        "Parent",
                        text(comment),
                        index);
        token = &context.make<TraceScopeToken>(token, track, index);
        scope = &context.make<ScopedCommentNode>(scope, track);
        ++index;
    }
    auto kind = GraphMutationKind(integer(field(input, "kind")));
    Vector<String> update_fields(resource);
    update_fields.emplace_back("label");
    update_fields.emplace_back("name🧊");
    GraphMutationPlanItem item(std::allocator_arg,
                               Allocator(resource),
                               original.entity,
                               kind,
                               mutation,
                               update_fields,
                               UINT64_MAX,
                               token,
                               original.original_values);
    GraphMutationBatch batch(
        std::allocator_arg, Allocator(resource), item.entity, kind, update_fields);
    batch.items.emplace_back(item);
    GraphMutationPlan plan(std::allocator_arg, Allocator(resource));
    plan.planned_root = detail::clone_in(resource, original);
    plan.items.emplace_back(item);
    plan.batches.emplace_back(batch);
    plan.next_item_index = UINT64_MAX;
    if (std::get<bool>(field(input, "visited").data)) {
        plan.visited_nodes.emplace(String("Parent🧊", resource), String("u:1", resource));
    }
    auto different = detail::clone_in(resource, plan);
    different.next_item_index = 0;
    auto output = detail::model_json(resource);
    auto put = [&](std::string_view name, auto&& value) {
        detail::model_put(output, name, std::forward<decltype(value)>(value));
    };
    put("values", formatted_debug(resource, values));
    put("values_roundtrip_equal", values == roundtrip);
    put("node", formatted_debug(resource, original));
    put("node_clone", formatted_debug(resource, copied));
    put("node_clone_equal", original == copied);
    put("node_changed_equal", original == changed);
    put("operation", formatted_debug(resource, original.operation));
    put("kind", formatted_debug(resource, kind));
    Json::Array for_update(resource);
    for_update.emplace_back(formatted_debug(resource, graph_mutation_kind_for_update(false)));
    for_update.emplace_back(formatted_debug(resource, graph_mutation_kind_for_update(true)));
    put("kind_for_update", for_update);
    put("boundary",
        formatted_debug(resource, GraphTransactionBoundary(integer(field(input, "boundary")))));
    put("token", token ? formatted_debug(resource, *token) : Json(resource));
    put("scope", scope ? formatted_debug(resource, *scope) : Json(resource));
    put("item", formatted_debug(resource, item));
    put("item_clone_equal", item == detail::clone_in(resource, item));
    put("batch", formatted_debug(resource, batch));
    put("batch_clone_equal", batch == detail::clone_in(resource, batch));
    put("plan", formatted_debug(resource, plan));
    put("plan_clone", formatted_debug(resource, detail::clone_in(resource, plan)));
    put("plan_clone_equal", plan == detail::clone_in(resource, plan));
    put("plan_changed_equal", plan == different);
    GraphMutationPlan empty(std::allocator_arg, Allocator(resource));
    EntityValues empty_values(std::allocator_arg, Allocator(resource));
    put("default_plan", formatted_debug(resource, empty));
    put("default_values", formatted_debug(resource, empty_values));
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto context = global.make_user_context();
        auto input = Json::parse(context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid graph model fixture");
        }
        std::cout << graph_models(*context, *input).dump(context->memory_resource()) << '\n';
    }
    return 0;
}
