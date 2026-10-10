#include <teaql/runtime/graph_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void require_graph(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void graph_model_lifetimes() {
    GlobalContext global;
    auto source = global.make_user_context();
    auto resource = source->memory_resource();
    auto& root = source->make<GraphNode>("Root with a long managed model name");
    root.value("id", std::uint64_t(7))
        .value("password", "RAW-MODEL-CANARY")
        .with_comment("source model intent");
    root.dirty_fields.emplace(resource);
    root.dirty_fields->emplace("a long managed dirty field");
    root.original_values.emplace(resource);
    root.original_values->fields.emplace(String("label", resource),
                                         Value(resource, "original managed payload"));
    auto& track = source->make<TraceNode>(TraceKind::Entity, "Root", "parent intent", UINT64_MAX);
    auto& parent = source->make<TraceScopeToken>(nullptr, track, UINT64_MAX);
    auto& token = source->make<TraceScopeToken>(&parent, track, 7);
    auto& plan = source->make<GraphMutationPlan>();
    plan.planned_root = detail::clone_in(resource, root);
    plan.visited_nodes.emplace(String("Root", resource), String("u:7", resource));
    plan.visited_nodes.emplace(String("Other", resource), String("u:9", resource));
    auto& mutation = source->make<MutationValues>(root.values.into_mutation_values(resource));
    auto& fields = source->make<Vector<String>>();
    fields.emplace_back("password");
    plan.push(
        root.entity, GraphMutationKind::Update, mutation, fields, &token, root.original_values);
    plan.rebuild_batches();
    auto retained_handle = source->make_global<GraphMutationPlan>(plan);
    auto output_handle = source->make_global<String>(debug_string(resource, root));
    source.reset();
    require_graph(global.live_user_bytes() == 0,
                  "graph source blocks are reclaimed after actual User destruction");
    auto reader = global.make_user_context();
    auto& retained = reader->get(reader->rebind_global(retained_handle));
    const auto& output = reader->get(reader->rebind_global(output_handle));
    auto target = global.memory_resource();
    auto& after = global.make<String>(debug_string(target, *retained.planned_root));
    require_graph(
        after == output && after.find("RAW-MODEL-CANARY") != String::npos,
        "raw structural diagnostics retain source values independently of safe publication");
    require_graph(retained.memory_resource() == target &&
                      retained.planned_root->memory_resource() == target &&
                      retained.items[0].scope_token->memory_resource() == target &&
                      retained.items[0].scope_token->parent()->memory_resource() == target,
                  "retained model/token parents own the Global allocation domain");
    auto& plan_output = global.make<String>(debug_string(target, retained));
    require_graph(
        plan_output.find("visited_nodes: {") != String::npos &&
            plan_output.find("\"Root\", \"u:7\"") != String::npos &&
            plan_output.find("\"Other\", \"u:9\"") != String::npos,
        "unordered visited diagnostics preserve every member without prescribing iteration order");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        graph_model_lifetimes();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime graph model lifetime contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
