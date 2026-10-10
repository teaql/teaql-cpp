#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/context.hpp>
#include "parity/teaql-runtime/entity_runtime_contracts.hpp"
#include <iostream>
using namespace teaql;
static_assert(std::is_copy_constructible_v<RelationHandle<int>>);
static_assert(!std::is_constructible_v<RelationHandle<int>, LoadedRelation, const int*>);

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static EntityKey key(Memory r, std::string_view name = "Child", std::uint64_t id = 7) {
    return EntityKey(std::allocator_arg, Allocator(r), name, Value(r, id));
}

struct Node {
    using allocator_type = Allocator;
    EntityRuntimeState state;
    String name;

    Node(std::allocator_arg_t, Allocator a, const EntityRuntimeState& state, std::string_view name)
        : state(std::allocator_arg, a, state), name(name, a.resource()) {}

    Node(std::allocator_arg_t, Allocator a, const Node& other)
        : Node(std::allocator_arg, a, other.state, other.name) {}

    Node(const Node&) = delete;
    Node(Node&&) noexcept = default;

    Node& operator=(Node&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }
};

static void ledger_lifetimes() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& root = user->make<EntityRuntimeState>();
    auto& alias = user->make<EntityRuntimeState>(root);
    check(root == alias, "copies share the lazy ledger before mutation");
    {
        Record values(r);
        values.emplace(String("id", r), Value(r, std::int64_t(7)));
        values.emplace(String("version", r), Value(r, std::int64_t(4)));
        CompactRow row(std::allocator_arg, Allocator(r), values);
        root.set_original_compact_row("Child", row);
        check(root.get_original_version(key(r)) == 4 &&
                  !detail::EntityRuntimeAccess::has_mutation_context(root),
              "loaded snapshot reads do not allocate ledger");
    }
    check(!alias.original_snapshot(), "loaded snapshots are per view");
    alias.set(key(r), "payload", Value(r, String(100, 'x', r)));
    check(root.get(key(r), "payload")->try_text()->size() == 100, "lazy shared write visible");
    root.set_comment("persisted intent");
    root.mark_as_new(key(r));
    root.push_change_set();
    root.set(key(r), "upper", Value(r, true));
    auto& retained = global.make<EntityRuntimeState>(root);
    check(!(retained == root), "cross-domain ledger copied, not borrowed");
    root.mark_as_delete(key(r));
    check(root.changed_field_names(key(r)).empty() && root.is_new(key(r)),
          "delete clears all stack levels but retains new marker");
    user->close();
    check(user->allocation_stats().live_blocks == 0, "all User blocks reclaimed");
    auto gr = global.memory_resource();
    check(retained.get(key(gr), "payload")->try_text()->size() == 100 &&
              retained.get(key(gr), "upper")->try_bool() == true,
          "deep-copy values survive source close");
    check(retained.get_original_version(key(gr)) == 4, "loaded row copied with ledger");
    retained.clear_committed();
    check(retained.current_change_set().is_empty() && !retained.get_comment() &&
              retained.get_original_version(key(gr)) == 4,
          "commit clears intent but preserves loaded row");
}

static void composition() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& target = user->make<EntityRuntimeState>();
    auto& source = user->make<EntityRuntimeState>();
    auto k = key(r);
    target.set_original_version(k, 2);
    source.set(k, "text", Value(r, "keep source"));
    source.set_original_version(k, 3);
    auto error = detail::EntityRuntimeAccess::compose(target, source);
    check(!error && target.current_change_set().is_empty() && source.get(k, "text"),
          "conflicting compose is atomic and source remains intact");
    target.adopt_mutations_from(source);
    check(detail::EntityRuntimeAccess::first_composition_error(target).has_value(),
          "void attachment retains conflict");
    target.clear_committed();
    check(detail::EntityRuntimeAccess::first_composition_error(target).has_value(),
          "source clear_committed retains composition error");
    check(bool(detail::EntityRuntimeAccess::compose(target, source)),
          "cleared original version can accept source intent");
    source.set(k, "text", Value(r, "changed later"));
    check(target.get(k, "text")->try_text() == "keep source",
          "composition copies without aliasing source ledger");
    auto& empty = user->make<EntityRuntimeState>();
    auto& fresh = user->make<EntityRuntimeState>();
    empty.set_original_compact_row("Child", CompactRow(std::allocator_arg, Allocator(r)));
    fresh.adopt_mutations_from(empty);
    check(!detail::EntityRuntimeAccess::has_mutation_context(fresh),
          "pristine source does not materialize target ledger");
}

static void graph_and_copy() {
    GlobalContext global;
    auto source = global.make_user_context();
    auto r = source->memory_resource();
    auto& root = source->make<EntityRuntimeState>();
    {
        EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
        builder.install(1, Node(std::allocator_arg, Allocator(r), root, "first"));
        builder.install(2, Node(std::allocator_arg, Allocator(r), root, "second"));
        SmartList<Node> list(std::allocator_arg, Allocator(r));
        list.emplace_back(root, "list child");
        list.with_summary("long", String(100, 's', r));
        builder.install_relation_list("PurchaseOrder", 1, "children", std::move(list));
        builder.install_relation_option<Node>("PurchaseOrder", 1, "null", std::nullopt);
        check(builder.entity_count() == 2 && builder.relation_list_count() == 2,
              "graph contains typed tables and independent relation views");
        check(bool(root.freeze_graph(std::move(builder))), "freeze publishes graph once");
    }
    auto* first = root.resolve_entity<Node>(1);
    auto* second = root.resolve_entity<Node>(2);
    check(first && second && first->state == root && second->state == root,
          "graph entities share root ledger");
    auto view = root.relation_list<Node>("purchase_order", 1, "children");
    check(view.state() == LoadedRelation::Loaded && view.value()->data[0].name == "list child",
          "relation owner canonicalized");
    check(root.relation_option<Node>("PurchaseOrder", 1, "null").is_empty(),
          "loaded null remains distinct from missing");
    check(!root.relation_list<Node>("PurchaseOrder", 1, "missing").is_loaded(),
          "no implicit relation load");
    bool mismatch = false;
    try {
        root.relation_list<std::uint64_t>("PurchaseOrder", 1, "children");
    } catch (const std::logic_error&) {
        mismatch = true;
    }
    check(mismatch && !root.resolve_relation_list<std::uint64_t>("PurchaseOrder", 1, "children"),
          "strict handle mismatch panics while optional resolver returns None");
    auto& copy = global.make<EntityRuntimeState>(root);
    source->close();
    auto gr = global.memory_resource();
    auto* a = copy.resolve_entity<Node>(1);
    auto* b = copy.resolve_entity<Node>(2);
    check(a && b && a->state == copy && b->state == copy,
          "cross-domain cyclic graph preserves ledger aliases");
    check(a->state.resolve_entity<Node>(2) == b,
          "cross-domain back-reference resolves copied graph");
    a->state.set(key(gr), "name", Value(gr, "shared copied ledger"));
    check(copy.get(key(gr), "name")->try_text() == "shared copied ledger",
          "copied graph references same copied ledger");
    check(copy.relation_list<Node>("PurchaseOrder", 1, "children")
                  .value()
                  ->summary.at(String("long", gr))
                  .memory_resource() == gr,
          "nested relation summary propagates allocator");
    {
        EntityGraphBuilder rejected(std::allocator_arg, Allocator(gr));
        rejected.install(3, std::uint64_t(42));
        auto result = copy.freeze_graph(std::move(rejected));
        check(!result && result.error().entity_count() == 1 &&
                  !copy.resolve_entity<std::uint64_t>(3),
              "second freeze returns intact builder");
    }
}

struct Resource {
    using allocator_type = Allocator;
    Memory r;
    int* destroyed;
    bool active = true;

    Resource(std::allocator_arg_t, Allocator a, int& count) : r(a.resource()), destroyed(&count) {}

    Resource(std::allocator_arg_t, Allocator a, Resource&& other)
        : r(a.resource()), destroyed(other.destroyed), active(std::exchange(other.active, false)) {
        if (r != other.r) {
            throw std::invalid_argument("resource cannot migrate");
        }
    }

    Resource(Resource&& other) noexcept
        : r(other.r), destroyed(other.destroyed), active(std::exchange(other.active, false)) {}

    Resource(const Resource&) = delete;

    ~Resource() {
        if (active) {
            ++*destroyed;
        }
    }
};

struct LedgerModel : EntityDefaults<LedgerModel>, LedgerEntityDefaults<LedgerModel> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "LedgerModel";
    Memory r;
    const EntityRuntimeState* state;

    LedgerModel(std::allocator_arg_t, Allocator a, const EntityRuntimeState* state = nullptr)
        : r(a.resource()), state(state) {}

    LedgerModel(std::allocator_arg_t, Allocator a, const LedgerModel& other)
        : r(a.resource()), state(other.state) {
        if (state && r != other.r) {
            state = &detail::resource_arena(r).make<EntityRuntimeState>(*state);
        }
    }

    Memory memory_resource() const {
        return r;
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return EntityDescriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
    }

    static Result<LedgerModel, EntityError> from_compact_row(Memory r, CompactRow) {
        return Result<LedgerModel, EntityError>::ok(r,
                                                    LedgerModel(std::allocator_arg, Allocator(r)));
    }

    MutationValues into_values() && {
        return MutationValues(r);
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        if (!state) {
            return {};
        }
        return detail::clone_in(r, *state);
    }
};

static_assert(LedgerEntity<LedgerModel>);

static void public_composition() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& target = user->make<EntityRuntimeState>();
    auto& source = user->make<EntityRuntimeState>();
    auto& receiver = user->make<LedgerModel>(&target);
    auto& sender = user->make<LedgerModel>(&source);
    auto& missing = user->make<LedgerModel>();
    auto first = missing.include_pending_mutations_from(missing);
    check(!first && first.error().kind == LedgerCompositionError::Kind::MissingTargetState,
          "public composition checks target first");
    auto second = receiver.include_pending_mutations_from(missing);
    check(!second && second.error().to_string(r) == "source entity has no mutation ledger",
          "public composition reports missing source");
    source.set(key(r), "name", Value(r, "typed composition"));
    check(bool(receiver.include_pending_mutations_from(sender)),
          "public LedgerEntity default composes states");
    check(target.get(key(r), "name")->try_text() == "typed composition",
          "typed default preserves mutation intent");
}

static void eager_payload_cleanup() {
    int count = 0;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& root = user->make<EntityRuntimeState>();
    {
        EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
        builder.install(1, Resource(std::allocator_arg, Allocator(r), count));
        builder.install(1, Resource(std::allocator_arg, Allocator(r), count));
        check(count == 1, "replacing graph entity destroys native payload once");
        SmartList<Resource> native_list(std::allocator_arg, Allocator(r));
        native_list.emplace_back(count);
        builder.install_relation_list("Owner", 1, "resources", std::move(native_list));
        check(bool(root.freeze_graph(std::move(builder))), "freeze accepts move-only resources");
    }
    bool denied = false;
    try {
        global.make<EntityRuntimeState>(root);
    } catch (const std::invalid_argument&) {
        denied = true;
    }
    check(denied, "cross-domain copy never silently retains non-copyable native payload");
    check(detail::resource_arena(global.memory_resource()).stats().live_blocks > 0,
          "Global remains usable after failed copy");
    check(global.make<String>("still alive") == "still alive",
          "copy scope restored after exception");
    user->close();
    check(count == 3 && user->allocation_stats().live_blocks == 0,
          "native graph payload cleaned before User arena reclamation");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        ledger_lifetimes();
        composition();
        public_composition();
        graph_and_copy();
        eager_payload_cleanup();
        teaql::test::source_entity_runtime::run_original_contracts();
        std::pmr::set_default_resource(previous);
        std::cout << "Entity identity graph and ledger contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
