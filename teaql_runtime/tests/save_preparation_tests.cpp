#include "../../tests/support/save_preparation_fixture.hpp"
#include <teaql/runtime/module.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static EntityDescriptor descriptor(Memory r, std::string_view name) {
    EntityDescriptor out(std::allocator_arg, Allocator(r), name);
    out.property("id", DataType::U64).id();
    out.property("name", DataType::Text);
    return out;
}

static void install(UserContext& user) {
    auto r = user.memory_resource();
    auto& module = user.make<RuntimeModule>();
    auto parent = descriptor(r, "Parent");
    parent.relation("kids", "Child").local("id").foreign("parent_id");
    auto child = descriptor(r, "Child");
    child.relation("leaves", "Leaf").local("id").foreign("parent_id");
    module.descriptor(std::move(parent))
        .descriptor(std::move(child))
        .descriptor(descriptor(r, "Leaf"))
        .apply_to(user);
}

static void extraction_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& model = user->make<save_fixture::Model>();
    model.values.fields.emplace(String("id", r), Value(r, std::uint64_t{1}));
    auto& leaf = user->make<Record>();
    leaf.emplace(String("id", r), Value(r, std::uint64_t{3}));
    leaf.emplace(String("name", r),
                 Value(r, "Long nested value whose characters must survive the source Context"));
    auto& child = user->make<Record>();
    child.emplace(String("id", r), Value(r, std::uint64_t{2}));
    child.emplace(String("leaves", r), Value(r, leaf));
    model.values.fields.emplace(String("kids", r), Value(r, child));
    model.dirty.emplace(r);
    model.dirty->emplace("name");
    model.comment.emplace("Handwritten audited comment retained explicitly", r);
    auto& graph = user->make<Result<GraphNode, RuntimeError>>(
        graph_node_from_entity(*user, detail::clone_in(r, model)));
    check(graph.is_ok() && graph.value()
                                   .relations.at(String("kids", r))
                                   .front()
                                   .relations.at(String("leaves", r))
                                   .size() == 1,
          "public typed extraction recursively builds metadata-controlled relations");
    auto copy = user->make_global<GraphNode>(graph.value());
    auto locations = user->make_global<detail::LedgerObjectLocations>(
        detail::ledger_object_locations(r, graph.value()));
    user.reset();
    auto next = global.make_user_context();
    auto& retained = next->get(next->rebind_global(copy));
    auto& deepest =
        retained.relations.find("kids")->second.front().relations.find("leaves")->second.front();
    check(deepest.values.fields.find("name")->second.try_text()->starts_with("Long nested"),
          "all nested data is explicitly copied into Global before source destruction");
    auto& paths = next->get(next->rebind_global(locations));
    EntityKey key(std::allocator_arg,
                  Allocator(next->memory_resource()),
                  "Leaf",
                  Value(next->memory_resource(), std::uint64_t{3}));
    check(paths.at(key).model_path(next->memory_resource()) == "kids[0].leaves[0]",
          "retained ledger locations preserve model paths");
}

static void preflight_and_partial_failure() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& registry = user->make<InMemoryCheckerRegistry>();
    auto& parent = user->make<save_fixture::Checker>("Parent", "fill");
    auto& child = user->make<save_fixture::Checker>("Child", "error");
    registry.register_checker(*user, parent);
    registry.register_checker(*user, child);
    user->set_checker_registry(*user, registry);
    auto& root = user->make<EntityRuntimeState>();
    auto& node = user->make<GraphNode>("Parent");
    node.value("id", std::uint64_t{1});
    node.dirty_fields.emplace(r);
    node.dirty_fields->emplace("name");
    GraphNode first(std::allocator_arg, Allocator(r), "Child");
    first.value("id", std::uint64_t{2});
    node.relation("kids", first);
    node.relation("kids", first);
    auto result = detail::preflight_graph(*user, node, ObjectLocation::root(r), &root);
    check(!result && parent.calls.size() == 1 && child.calls.size() == 1 &&
              child.calls[0] == "kids[0]",
          "preflight stops on first child failure after completing parent fix");
    check(!node.values.fields.contains(check_object_status_field) &&
              !node.values.fields.contains("_dirty_fields"),
          "normal check errors clear preflight markers");
    EntityKey key(std::allocator_arg, Allocator(r), "Parent", Value(r, std::uint64_t{1}));
    auto changed = root.get(key, "name");
    check(changed && changed->try_text() == "fixed" &&
              result.error().check_results()[0].location.instance_path(r) == "/kids/0/name",
          "parent fix remains in the ledger and child diagnostics retain location");
}

static void ledger_discovery_and_merge() {
    GlobalContext global;
    auto user = global.make_user_context();
    install(*user);
    auto r = user->memory_resource();
    auto& root = user->make<EntityRuntimeState>();
    EntityKey child(std::allocator_arg, Allocator(r), "Child", Value(r, std::uint64_t{2}));
    EntityKey leaf(std::allocator_arg, Allocator(r), "Leaf", Value(r, std::uint64_t{3}));
    root.set(child, "parent_id", Value(r, std::uint64_t{1}));
    root.set(child, "name", Value(r, "child fix"));
    root.set_original_version(child, 5);
    root.set(leaf, "parent_id", Value(r, std::uint64_t{2}));
    root.mark_as_new(leaf);
    auto& graph = user->make<GraphNode>("Parent");
    graph.value("id", std::uint64_t{1});
    EntityRuntimeState::Keys visited(r);
    visited.emplace("Parent", Value(r, std::uint64_t{1}));
    check(bool(detail::hydrate_ledger_relations(*user, root, graph, visited)) &&
              visited.size() == 3,
          "ledger changes discover nested children with one shared visited set");
    auto& found = graph.relations.find("kids")->second.front();
    check(found.operation == GraphOperation::Upsert &&
              found.relations.find("leaves")->second.front().operation == GraphOperation::Create,
          "original version and new-key roles select the source graph operations");
    auto& target = user->make<EntityRuntimeState>();
    found.dirty_fields.emplace(r);
    found.dirty_fields->emplace("name");
    found.original_values.emplace(r);
    found.original_values->fields.emplace(String("version", r), Value(r, std::int64_t{5}));
    check(bool(detail::merge_relation_mutations_into_root(target, graph)) &&
              target.get(child, "name") && target.is_new(leaf) &&
              target.get_original_version(child) == 5,
          "relation merge preserves dirty fields, versions and create roles");
    GraphNode broken(std::allocator_arg, Allocator(r), "Child");
    graph.relation("z", broken);
    auto error = detail::merge_relation_mutations_into_root(target, graph);
    check(!error && target.get(child, "name"),
          "later malformed child does not roll back already merged ledger state");
}

int main() {
    auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        extraction_and_retention();
        preflight_and_partial_failure();
        ledger_discovery_and_merge();
        std::pmr::set_default_resource(old);
        std::cout << "Save preparation/ownership contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(old);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
