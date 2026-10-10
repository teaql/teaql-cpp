// Pinned graph-reference contracts adapted to the explicit Context owner.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <iostream>
#include <utility>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Counts {
    unsigned constructed = 0;
    unsigned destroyed = 0;
};

struct GraphChild {
    using allocator_type = Allocator;
    EntityRuntimeState root;
    Counts* counts;
    bool active = true;

    GraphChild(std::allocator_arg_t, Allocator a, EntityRuntimeState&& root, Counts& counts)
        : root(std::allocator_arg, a, std::move(root)), counts(&counts) {
        ++counts.constructed;
    }

    GraphChild(std::allocator_arg_t, Allocator a, const GraphChild& other)
        : root(std::allocator_arg, a, other.root), counts(other.counts) {
        ++counts->constructed;
    }

    GraphChild(GraphChild&& other) noexcept
        : root(std::move(other.root)), counts(other.counts),
          active(std::exchange(other.active, false)) {}

    GraphChild(const GraphChild&) = delete;

    ~GraphChild() {
        if (active) {
            ++counts->destroyed;
        }
    }

    Memory memory_resource() const {
        return root.memory_resource();
    }
};

static void install_child(UserContext& user, Ref<EntityRuntimeState> root, Counts& counts) {
    auto owner = user.memory_resource();
    SmartList<GraphChild> children(std::allocator_arg, Allocator(owner));
    children.emplace_back(detail::EntityRuntimeAccess::fresh_with_weak_graph(owner, user.get(root)),
                          counts);
    // Publication rejection observes the weak role before the graph is frozen.
    EntityGraphBuilder rejected(std::allocator_arg, Allocator(owner));
    rejected.install(9, std::uint64_t(9));
    auto refusal = children.data[0].root.freeze_graph(std::move(rejected));
    check(!refusal && refusal.error().entity_count() == 1,
          "graph-owned weak state cannot publish; rejected builder is preserved");
    EntityGraphBuilder builder(std::allocator_arg, Allocator(owner));
    builder.install_relation_list("Owner", 1, "children", std::move(children));
    check(bool(user.get(root).freeze_graph(std::move(builder))),
          "source owning root publishes child list");
}

static void graph_owned_entities_do_not_keep_the_graph_alive() {
    Counts counts;
    GlobalContext global;
    auto user = global.make_user_context();
    auto root = user->make_ref<EntityRuntimeState>();
    install_child(*user, root, counts);
    const auto* children =
        user->get(root).resolve_relation_list<GraphChild>("Owner", 1, "children");
    check(children && children->data.size() == 1 &&
              children->data[0].root.resolve_relation_list<GraphChild>("Owner", 1, "children") ==
                  children,
          "original weak child resolves the same frozen graph without a copied adjacency");
    user->destroy(root);
    check(counts.constructed == 1 && counts.destroyed == 0,
          "releasing a root view leaves graph storage with its explicit Context owner");
    user->close();
    check(counts.destroyed == 1 && user->allocation_stats().live_bytes == 0 &&
              user->allocation_stats().live_blocks == 0,
          "weak graph cycle cannot retain payloads or blocks past Context close");
    user.reset();
    check(global.live_user_bytes() == 0 && counts.destroyed == counts.constructed,
          "actual User deletion does not leave a graph-reference ownership cycle");
}

static void cloning_a_graph_owned_entity_promotes_its_graph_anchor() {
    Counts counts;
    {
        GlobalContext global;
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        // Observe clone promotion on an unfrozen anchor, where publication
        // authority distinguishes weak preservation from promotion.
        {
            EntityRuntimeState original(std::allocator_arg, Allocator(owner));
            auto weak = detail::EntityRuntimeAccess::fresh_with_weak_graph(owner, original);
            GraphChild child(std::allocator_arg, Allocator(owner), std::move(weak), counts);
            GraphChild promoted(std::allocator_arg, Allocator(owner), child);
            EntityGraphBuilder builder(std::allocator_arg, Allocator(owner));
            builder.install(42, std::uint64_t(42));
            check(bool(promoted.root.freeze_graph(std::move(builder))),
                  "cloning weak child promotes publication authority");
            auto* value = child.root.resolve_entity<std::uint64_t>(42);
            check(value && *value == 42 && original.resolve_entity<std::uint64_t>(42) == value,
                  "promotion shares the original anchor rather than creating an empty graph");
        }
        auto root = user->make_ref<EntityRuntimeState>();
        install_child(*user, root, counts);
        const auto* children =
            user->get(root).resolve_relation_list<GraphChild>("Owner", 1, "children");
        check(children && children->data.size() == 1, "source Owner#1 children list exists");
        auto detached = user->make_ref<GraphChild>(children->data[0]);
        user->destroy(root);
        check(user->get(detached).root.resolve_relation_list<GraphChild>("Owner", 1, "children") ==
                  children,
              "source detached clone retains the same complete frozen graph after root release");
        auto retained = user->make_global<GraphChild>(user->get(detached));
        user->close();
        try {
            (void)user->get(detached);
            throw std::runtime_error("detached clone extended User access past close");
        } catch (const ReferenceError& error) {
            check(error.kind() == ReferenceError::Kind::Closed,
                  "same-domain detached clone expires with User");
        }
        user.reset();
        check(global.live_user_bytes() == 0, "explicit Global copy retains no source User blocks");
        auto next = global.make_user_context();
        auto rebound = next->rebind_global(retained);
        const auto& copied = next->get(rebound);
        const auto* copied_children =
            copied.root.resolve_relation_list<GraphChild>("Owner", 1, "children");
        check(
            copied_children && copied_children->data.size() == 1 &&
                copied_children->memory_resource() == global.memory_resource() &&
                copied_children->data[0].memory_resource() == global.memory_resource() &&
                copied_children->data[0].root.resolve_relation_list<GraphChild>(
                    "Owner", 1, "children") == copied_children,
            "explicit cross-domain copy relocates the list and preserves the cyclic graph anchor");
        next->destroy(rebound);
        check(counts.destroyed + 1 == counts.constructed,
              "destroying retained detached view leaves exactly the Global-owned graph child");
        next.reset();
    }
    check(counts.destroyed == counts.constructed,
          "every graph and detached payload is destroyed exactly once");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    struct Restore {
        Memory previous;

        ~Restore() {
            std::pmr::set_default_resource(previous);
        }
    } restore{previous};

    try {
        graph_owned_entities_do_not_keep_the_graph_alive();
        cloning_a_graph_owned_entity_promotes_its_graph_anchor();
        std::cout << "Graph-reference role and Context lifetime adaptation contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
