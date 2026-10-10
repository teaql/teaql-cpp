#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct OwnedEntity : EntityDefaults<OwnedEntity> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Dummy";
    MutationValues values;

    OwnedEntity(std::allocator_arg_t, Allocator a, MutationValues&& input)
        : values(std::allocator_arg, a, std::move(input)) {}

    OwnedEntity(OwnedEntity&&) noexcept = default;

    OwnedEntity(std::allocator_arg_t, Allocator a, OwnedEntity&& other)
        : values(std::allocator_arg, a, std::move(other.values)) {}

    OwnedEntity(const OwnedEntity&) = delete;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "Dummy");
        descriptor.property("id", DataType::I64).id();
        return descriptor;
    }

    static Result<OwnedEntity, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Outcome = Result<OwnedEntity, EntityError>;
        return Outcome::ok(owner,
                           OwnedEntity(std::allocator_arg,
                                       Allocator(owner),
                                       MutationValues(owner, std::move(row).into_map(owner))));
    }

    MutationValues into_values() && {
        return std::move(values);
    }
};

static OwnedEntity entity(Memory owner, std::int64_t id) {
    MutationValues values(owner);
    values.fields.emplace("id", Value(owner, id));
    return OwnedEntity(std::allocator_arg, Allocator(owner), std::move(values));
}

// Exact original entity_graph.rs builder inputs and assertions.
static void original_builder_contract() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto graph = EntityGraph::from_entity(owner, entity(owner, 1))
                     .comment("Parent creation")
                     .child("dummy_items",
                            EntityGraph::from_entity(owner, entity(owner, 2))
                                .comment("Child deletion")
                                .delete_())
                     .build();
    const auto& root = graph.root;
    check(root.entity_type == "Dummy" && root.comment == "Parent creation" &&
              root.operation == EntityGraphOperation::Save && root.children.size() == 1,
          "original parent annotation and operation assertions");
    const auto& [name, child] = root.children[0];
    check(name == "dummy_items" && child.entity_type == "Dummy" &&
              child.comment == "Child deletion" && child.operation == EntityGraphOperation::Delete,
          "original child relation, annotation and deletion assertions");
}

static void owning_and_retention_contracts() {
    GlobalContext global;
    Ref<core::EntityGraph> retained;
    Ref<String> output;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        MutationValues values(owner);
        values.fields.emplace("payload", Value(owner, String(8192, 'v', owner)));
        auto node_pointer = &*values.fields.begin();
        auto text_pointer = std::get<String>(values.fields.begin()->second.data).data();
        OwnedEntity input(std::allocator_arg, Allocator(owner), std::move(values));
        auto graph =
            EntityGraph::from_entity(owner, std::move(input))
                .comment("")
                .child("duplicate", EntityGraph::from_entity(owner, entity(owner, 2)).delete_())
                .child("duplicate", EntityGraph::from_entity(owner, entity(owner, 3)).comment(""))
                .build();
        check(&*graph.root.values.fields.begin() == node_pointer &&
                  std::get<String>(graph.root.values.fields.begin()->second.data).data() ==
                      text_pointer,
              "entity-to-input-graph construction transfers same-owner map nodes and long strings");
        check(graph.root.comment && graph.root.comment->empty() &&
                  graph.root.children.size() == 2 &&
                  graph.root.children[0].first == graph.root.children[1].first,
              "explicit empty comments and ordered duplicate relation names remain distinct");
        auto before = user->allocation_stats().issued_objects;
        String text(owner);
        core::DebugWriter writer(text, {.pretty = true});
        core::write_debug(writer, graph.root);
        check(user->allocation_stats().issued_objects == before &&
                  text.find("MutationValues(") != String::npos,
              "qualified public node diagnostics preserve mutation role with payload-only scratch");
        retained = user->make_global<core::EntityGraph>(graph);
        output = user->make_global<String>(text);
    }
    check(global.live_user_bytes() == 0,
          "actual source User destruction releases every input block");
    auto next = global.make_user_context();
    const auto& graph = next->get(next->rebind_global(retained));
    auto owner = global.memory_resource();
    const auto& [relation, child] = graph.root.children[0];
    check(graph.root.memory_resource() == owner && graph.root.values.memory_resource() == owner &&
              graph.root.comment->get_allocator().resource() == owner &&
              relation.get_allocator().resource() == owner && child.memory_resource() == owner &&
              child.values.memory_resource() == owner &&
              std::get<String>(graph.root.values.fields.begin()->second.data)
                      .get_allocator()
                      .resource() == owner,
          "explicit retained input graph uses only Global for every nested owner");
    check(debug_string(next->memory_resource(), graph.root, {.pretty = true}) ==
              next->get(next->rebind_global(output)),
          "complete recursive input-node diagnostics survive actual source User deletion");
    core::EntityGraphNode changed(
        std::allocator_arg, Allocator(next->memory_resource()), graph.root);
    changed.children[0].first = "changed";
    changed.comment.emplace("changed", next->memory_resource());
    check(graph.root.children[0].first == "duplicate" && graph.root.comment->empty(),
          "node Clone edits do not change retained snapshots");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        original_builder_contract();
        owning_and_retention_contracts();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
