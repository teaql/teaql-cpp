#include <teaql/runtime/graph.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
#include <iostream>
#include <type_traits>
using namespace teaql;

static void check(bool v, const char* message) {
    if (!v) {
        throw std::runtime_error(message);
    }
}

template <class F>
void rejects(F f) {
    bool caught = false;
    try {
        f();
    } catch (const std::logic_error&) {
        caught = true;
    }
    check(caught, "invalid trace borrowing rejected");
}

static_assert(!std::is_same_v<EntityValues, MutationValues>);
static_assert(!std::is_same_v<EntityValues, EntitySnapshot>);

static GraphNode node(Memory r) {
    GraphNode root(std::allocator_arg, Allocator(r), "Order with a long entity name");
    root.value("id", 7)
        .value("name", "a long value allocated inside the user context")
        .with_comment("");
    root.dirty_fields.emplace(r);
    root.dirty_fields->emplace("long dirty field allocated inside the user context");
    root.original_values.emplace(r);
    root.original_values->fields.emplace(String("original", r),
                                         Value(r, "old value allocated inside user"));
    GraphNode child(std::allocator_arg, Allocator(r), "Child");
    child.value("id", Value(r));
    root.relation("children", child).with_relations("empty", {});
    return root;
}

static void lifetimes() {
    GlobalContext global;
    auto source = global.make_user_context();
    auto target = global.make_user_context();
    auto r = source->memory_resource();
    auto& root = source->make<GraphNode>(node(r));
    auto& retained = global.make<GraphNode>(root);
    auto& module = source->make<RuntimeModule>();
    module.initial_graph(root).root_graph(root);
    module.apply_to(*target);
    root.entity = "changed after install";
    check(target->initial_graphs().size() == 1 && target->root_graphs().size() == 1,
          "module installs both graph collections");
    auto& first =
        source->make<TraceNode>(TraceKind::Entity, "Order", "root comment with allocated text", 7);
    auto& last =
        source->make<TraceNode>(TraceKind::Comment, "Child", "child comment with allocated text");
    auto& parent = source->make<TraceScopeToken>(nullptr, first, 1);
    auto& token = source->make<TraceScopeToken>(&parent, last, 2);
    auto& plan = source->make<GraphMutationPlan>();
    auto& copied = global.make<GraphMutationPlan>();
    {
        MutationValues values(r);
        values.fields.emplace(String("payload", r), Value(r, "payload retained after close"));
        Vector<String> fields(r);
        fields.emplace_back("name");
        std::optional<EntitySnapshot> old;
        old.emplace(r);
        old->fields.emplace(String("name", r), Value(r, "old"));
        plan.planned_root = detail::clone_in(r, root);
        plan.visited_nodes.emplace(String("Order", r),
                                   String("identity with a long allocated value", r));
        plan.push("Order", GraphMutationKind::Update, values, fields, &token, old);
        plan.push("Order", GraphMutationKind::Update, values, fields, &token, old);
        plan.rebuild_batches();
        check(plan.items[0].scope_token == &token && plan.batches[0].items[0].scope_token == &token,
              "same-domain token copies preserve constant-time borrowing");
        copied = detail::clone_in(global.memory_resource(), plan);
        check(copied == plan, "deep-copied plans retain value equality");
        check(copied.items[0].scope_token != &token &&
                  copied.items[0].scope_token->parent() != &parent,
              "cross-domain tokens recursively clone parents");
        rejects([&] {
            global.make<TraceScopeToken>(&token, last, 3);
        });
        rejects([&] {
            target->make<TraceScopeToken>(&token, last, 3);
        });
    } // Caller-owned PMR values destruct before their resource closes.
    source->close();
    check(retained.entity == "Order with a long entity name" && retained.comment &&
              retained.comment->empty(),
          "Global graph retained original content including Some empty comment");
    check(retained.dirty_fields->begin()->get_allocator().resource() == global.memory_resource(),
          "nested set allocator propagated");
    check(
        retained.relations.find(std::string_view("children"))->second[0].values.memory_resource() ==
            global.memory_resource(),
        "nested graph values copied");
    check(copied.items[0].scope_token->recover_trace_chain(global.memory_resource()).size() == 2,
          "persistent trace remains valid after source close");
    check(copied.visited_nodes.begin()->second.get_allocator().resource() ==
              global.memory_resource(),
          "visited identity strings propagated");
    check(target->initial_graphs()[0].original_values->fields.size() == 1,
          "Context graph snapshot independent of source module");
    target->set_root_graphs(target->root_graphs());
    check(target->root_graphs().size() == 1,
          "replacing from own borrowed slice deep-copies before replacement");
    auto& empty = global.make<RuntimeModule>();
    empty.apply_to(*target);
    check(target->initial_graphs().empty() && target->root_graphs().empty(),
          "empty module replaces graphs");
    check(source->allocation_stats().live_blocks == 0, "all source graph blocks returned");
}

static void trace_and_conversion() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    Record record(r);
    record.emplace(String("z", r), Value(r, 1));
    record.emplace(String("a", r), Value(r));
    CompactRow row(std::allocator_arg, Allocator(r), record);
    EntityValues values(std::allocator_arg, Allocator(r), row);
    auto mutation = values.into_mutation_values(r);
    EntityValues back(std::allocator_arg, Allocator(r), mutation);
    check(values == back && values.fields == record,
          "row and command conversions preserve independent entity state");
    Vector<String> excluded(r);
    excluded.emplace_back("z");
    excluded.emplace_back("missing");
    auto fields = sorted_update_fields(r, values, excluded);
    check(fields.size() == 1 && fields[0] == "a",
          "sorted field filter uses exact exclusions including null values");
    TraceNode first(std::allocator_arg, Allocator(r), TraceKind::Entity, "Order", "first");
    TraceNode empty(std::allocator_arg, Allocator(r), TraceKind::Relation, "Child", "");
    TraceNode spaces(std::allocator_arg, Allocator(r), TraceKind::Comment, "Child", "  ");
    ScopedCommentNode a(std::allocator_arg, Allocator(r), nullptr, first),
        b(std::allocator_arg, Allocator(r), &a, empty),
        c(std::allocator_arg, Allocator(r), &b, spaces);
    auto chain = c.to_trace_chain(r);
    check(chain.size() == 2 && chain[0].comment == "first" && chain[1].comment == "  ",
          "skip only empty comments; whitespace is meaningful");
    check(graph_mutation_kind_for_update(true) == GraphMutationKind::Update &&
              graph_mutation_kind_for_update(false) == GraphMutationKind::Create,
          "mutation kind selection");
    GraphMutationPlan overflow(std::allocator_arg, Allocator(r));
    overflow.next_item_index = ~std::uint64_t(0);
    bool threw = false;
    try {
        overflow.push("Order", GraphMutationKind::Create, mutation, {});
    } catch (const std::overflow_error&) {
        threw = true;
    }
#ifndef NDEBUG
    check(threw && overflow.items.empty() && overflow.next_item_index == ~std::uint64_t(0),
          "Debug preserves checked source overflow before item insertion");
#else
    check(!threw && overflow.items.size() == 1 && overflow.next_item_index == 0 &&
              overflow.items[0].item_index == ~std::uint64_t(0),
          "Release preserves source wrapping counter");
#endif
    auto& p = user->make<TraceScopeToken>(nullptr, first, 0);
    auto& q = user->make<TraceScopeToken>(&p, empty, 1);
    auto& t = user->make<TraceScopeToken>(&q, spaces, 2);
    check(t.recover_trace_chain(r) == chain, "persistent and stack trace ordering match");
}

int main() {
    try {
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        lifetimes();
        trace_and_conversion();
        std::pmr::set_default_resource(old);
        std::cout << "Runtime graph contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
