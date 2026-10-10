#include <teaql/runtime.hpp>
#include <teaql/runtime/relation_model_debug.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& relation = user->make<RelationDescriptor>("items", "Item");
    relation.local_key = "id";
    relation.foreign_key = "order_id";
    relation.many = true;
    auto source = user->make_ref<RelationLoadPlan>("Order", relation);
    auto& plan = user->get(source);
    plan.path.assign(512, 'p');
    plan.query.emplace(std::allocator_arg, Allocator(owner), "Item");
    plan.query->and_filter(Expr::eq(owner, "name", Value(owner, String(512, 'v', owner))));
    plan.query->comment("reason🧊").limit(UINT64_MAX).offset(17).top_n_probe_parent_threshold(7);
    auto& child_relation = user->make<RelationDescriptor>("parts", "Part");
    auto& child = plan.children.emplace_back("Item", child_relation);
    child.query.emplace(std::allocator_arg, Allocator(owner), "Part");
    child.query->comment(String(512, 'c', owner));
    auto kept = user->make_global<RelationLoadPlan>(plan);
    user.reset();
    check(global.live_user_bytes() == 0, "Source relation-plan User is fully reclaimed");
    auto next = global.make_user_context();
    const auto& retained = next->get(next->rebind_global(kept));
    check(retained.path.get_allocator().resource() == global.memory_resource() &&
              retained.query->memory_resource() == global.memory_resource() &&
              retained.children[0].query->memory_resource() == global.memory_resource(),
          "Full recursive relation plans and query payloads use the destination scope");
    {
        auto debug = debug_string(next->memory_resource(), retained, DebugOptions{.pretty = true});
        auto copy = detail::clone_in(next->memory_resource(), retained);
        check(debug.find("parts") != debug.npos && debug.find("reason") != debug.npos &&
                  copy == retained && debug.get_allocator().resource() == next->memory_resource(),
              "Retained recursive diagnostics and explicit copies preserve actual query fields");
    }
    bool rejected = false;
    try {
        (void)next->get(source);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Expired plan reference rejects before traversal or diagnostics");
    std::cout << "Recursive relation model ownership contracts pass\n";
}
