#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::logic_error(message);
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& root = user_context->make<EntityRuntimeState>();
        {
            EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
            SmartList<String> children(std::allocator_arg, Allocator(r));
            children.emplace_back("a long source-owned child value for identity verification");
            builder.install_relation_list("Owner", 10, "class", std::move(children));
            builder.install_relation_option<String>(
                "Entry", 20, "namespace", String("an independently owned namespace value", r));
            builder.install_relation_option<String>("Owner", 10, "operator", std::nullopt);
            check(bool(root.freeze_graph(std::move(builder))), "Publish the real runtime graph");
        }
        auto children = root.relation_list<String>("Owner", 10, "class");
        check(children.is_loaded() && children.value()->len() == 1,
              "Canonical keyword relation names are ordinary runtime keys");
        check(!root.relation_list<String>("Owner", 10, "model_class").is_loaded(),
              "The runtime does not invent C++ escaped relation names");
        check(root.relation_option<String>("Entry", 20, "namespace").is_loaded(),
              "Forward relation identity remains independent of a C++ accessor name");
        check(root.relation_option<String>("Owner", 10, "operator").is_empty(),
              "Loaded NULL remains different from a missing escaped-name relation");
        auto& retained = global.make<EntityRuntimeState>(root);
        user_context.reset();
        check(global.live_user_bytes() == 0, "All source User storage must be reclaimed");
        bool expired = false;
        try {
            (void)children.value();
        } catch (const ReferenceError&) {
            expired = true;
        }
        check(expired, "An ordinary relation handle expires with its Context");
        auto copied = retained.relation_list<String>("Owner", 10, "class");
        check(copied.value()->data[0].get_allocator().resource() == global.memory_resource(),
              "Explicit retention owns the nested string in its destination scope");
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
