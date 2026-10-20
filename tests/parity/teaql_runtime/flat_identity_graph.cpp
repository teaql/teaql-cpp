// Rust 5.0.6 flat-identity contracts, using Context-owned payloads.
// LoadedOrder is a handwritten model adapter, not generated-macro evidence.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Vendor {
    using allocator_type = Allocator;
    std::uint64_t id;
    String name;

    Vendor(std::allocator_arg_t, Allocator a, std::uint64_t id, std::string_view name)
        : id(id), name(name, a.resource()) {}

    Vendor(std::allocator_arg_t, Allocator a, const Vendor& other)
        : Vendor(std::allocator_arg, a, other.id, other.name) {}

    Vendor(Vendor&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return name.get_allocator().resource();
    }
};

static CompactRow
snapshot_row(Memory r, std::uint64_t id, std::int64_t version, std::string_view name) {
    Record fields(r);
    fields.emplace(String("id", r), Value(r, id));
    fields.emplace(String("version", r), Value(r, version));
    fields.emplace(String("name", r), Value(r, name));
    return CompactRow(std::allocator_arg, Allocator(r), std::move(fields));
}

// Only model plumbing lives here. Graph sharing, snapshots and mutation ledgers
// are supplied by EntityDefaults and EntityRuntimeState.
struct LoadedOrder : EntityDefaults<LoadedOrder> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "LoadedOrder";
    EntityRuntimeState state;

    LoadedOrder(std::allocator_arg_t, Allocator a, CompactRow row) : state(std::allocator_arg, a) {
        state.set_original_compact_row(ENTITY_NAME, std::move(row));
    }

    LoadedOrder(std::allocator_arg_t, Allocator a, const LoadedOrder& other)
        : state(std::allocator_arg, a, other.state) {}

    LoadedOrder(LoadedOrder&&) noexcept = default;

    Memory memory_resource() const {
        return state.memory_resource();
    }

    static Result<LoadedOrder, EntityError> from_compact_row(Memory r, CompactRow row) {
        return Result<LoadedOrder, EntityError>::ok(
            r, LoadedOrder(std::allocator_arg, Allocator(r), std::move(row)));
    }

    void on_loaded(AnyRef context) {
        if (auto* source = context.get<EntityRuntimeState>()) {
            state = state.with_shared_graph(*source);
        }
    }

    void setComment(std::string_view text) {
        state.set_comment(text);
    }
};

static void hydration_keeps_independent_mutation_ownership(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& graph = user_context.make<EntityRuntimeState>();
    EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
    builder.install(1, Vendor(std::allocator_arg, Allocator(r), 1, "Read-only Vendor"));
    check(bool(graph.freeze_graph(std::move(builder))), "freeze hydration graph");
    auto left = LoadedOrder::from_compact_row_with_context(
        r, snapshot_row(r, 1, 3, "loaded order"), AnyRef(graph));
    auto right = LoadedOrder::from_compact_row_with_context(
        r, snapshot_row(r, 2, 8, "loaded order"), AnyRef(graph));
    check(bool(left) && bool(right), "decode both orders");
    auto& a = left.value().state;
    auto& b = right.value().state;
    check(a != b, "hydration retains independent ledger owners");
    check(a.resolve_entity<Vendor>(1) && a.resolve_entity<Vendor>(1) == b.resolve_entity<Vendor>(1),
          "hydrated graph is shared");
    left.value().setComment("revise order A");
    right.value().setComment("revise order B");
    EntityKey ka(std::allocator_arg, Allocator(r), "LoadedOrder", Value(r, std::uint64_t(1)));
    EntityKey kb(std::allocator_arg, Allocator(r), "LoadedOrder", Value(r, std::uint64_t(2)));
    a.set(ka, "name", Value(r, "A"));
    b.set(kb, "name", Value(r, "B"));
    check(!a.get(kb, "name") && !b.get(ka, "name"), "hydrated changes remain local");
    check(!a.get_entity_comment(kb) && !b.get_entity_comment(ka), "hydrated intent remains local");
    check(a.get_original_version(ka) == 3 && b.get_original_version(kb) == 8,
          "hydration preserves original versions");
    check(graph.current_change_set().changes().empty() && !graph.get_entity_comment(ka) &&
              !graph.get_entity_comment(kb),
          "decode source acquires no mutations or intent");
}

struct Platform {
    using allocator_type = Allocator;
    EntityRuntimeState state;

    Platform(std::allocator_arg_t, Allocator a, const EntityRuntimeState& source)
        : state(std::allocator_arg, a, source) {}

    Platform(std::allocator_arg_t, Allocator a, const Platform& source)
        : state(std::allocator_arg, a, source.state) {}

    Platform(Platform&&) noexcept = default;

    Memory memory_resource() const {
        return state.memory_resource();
    }
};

static void reference_preserves_snapshot_and_local_intent(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& graph = user_context.make<EntityRuntimeState>();
    auto left = EntityRuntimeState::fresh_with_shared_graph(r, graph);
    auto right = EntityRuntimeState::fresh_with_shared_graph(r, graph);
    auto order_row = [&](std::uint64_t id, std::int64_t version) {
        auto fields = snapshot_row(r, id, version, "order").into_map(r);
        fields.emplace(String("platform_id", r), Value(r, std::uint64_t(1)));
        return CompactRow(std::allocator_arg, Allocator(r), std::move(fields));
    };
    left.set_original_compact_row("CustomerOrder", order_row(1, 3));
    right.set_original_compact_row("CustomerOrder", order_row(2, 8));
    auto& reference = user_context.make<EntityRuntimeState>();
    reference.set_original_compact_row("Platform", snapshot_row(r, 1, 11, "Unmodified Platform"));
    auto before = reference.original_snapshot();
    EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
    builder.install(1, Platform(std::allocator_arg, Allocator(r), reference));
    check(bool(graph.freeze_graph(std::move(builder))), "freeze reference graph");
    auto a = left.resolve_entity<Platform>(1);
    auto b = right.resolve_entity<Platform>(1);
    check(a && a == b, "both roots resolve identical read-only reference");
    check(left != right && left != reference && right != reference, "three independent owners");
    EntityKey ka(std::allocator_arg, Allocator(r), "CustomerOrder", Value(r, std::uint64_t(1)));
    EntityKey kb(std::allocator_arg, Allocator(r), "CustomerOrder", Value(r, std::uint64_t(2)));
    EntityKey kr(std::allocator_arg, Allocator(r), "Platform", Value(r, std::uint64_t(1)));
    left.set(ka, "description", Value(r, "graph A"));
    right.set(kb, "description", Value(r, "graph B"));
    left.set_comment("revise graph A");
    right.set_comment("revise graph B");
    left.set_entity_comment(ka, "local graph A");
    right.set_entity_comment(kb, "local graph B");
    check(!left.get(kb, "description") && !right.get(ka, "description"), "no sibling changes");
    check(!left.get_entity_comment(kb) && !right.get_entity_comment(ka), "no sibling intent");
    check(left.get_original_version(ka) == 3 && right.get_original_version(kb) == 8,
          "root versions survive composition");
    check(!left.get_original_version(kr) && reference.get_original_version(kr) == 11,
          "reference version stays with its owner");
    check(reference.current_change_set().changes().empty() && !reference.get_comment() &&
              reference.new_keys().empty() && reference.deleted_keys().empty(),
          "reference is unchanged");
    check(reference.original_snapshot() == before && a->state == reference && b->state == reference,
          "reference snapshot and runtime identity survive local edits");
}

static void roots_share_entities_without_sharing_ledgers(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& graph_root = user_context.make<EntityRuntimeState>();
    auto& first = user_context.make<EntityRuntimeState>(
        EntityRuntimeState::fresh_with_shared_graph(r, graph_root));
    auto& second = user_context.make<EntityRuntimeState>(
        EntityRuntimeState::fresh_with_shared_graph(r, graph_root));
    EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
    builder.install(7, Vendor(std::allocator_arg, Allocator(r), 7, "Vendor A"));
    check(bool(graph_root.freeze_graph(std::move(builder))), "freeze shared graph");
    auto a = first.resolve_entity<Vendor>(7);
    auto b = second.resolve_entity<Vendor>(7);
    check(a && b && a == b && a->name == "Vendor A", "both roots borrow identical flat entity");
    EntityKey trip(std::allocator_arg, Allocator(r), "NycYellowTrip", Value(r, std::uint64_t(1)));
    first.set(trip, "total_amount", Value(r, std::int64_t(100)));
    auto observed = first.get(trip, "total_amount");
    check(observed && *observed == Value(r, std::int64_t(100)) && !second.get(trip, "total_amount"),
          "shared entity graph does not share independent mutation ledgers");
}

static void same_id_is_namespaced_by_type(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
    builder.install(7, Vendor(std::allocator_arg, Allocator(r), 7, "Vendor"));
    builder.install(7, String("not a vendor", r));
    check(bool(root.freeze_graph(std::move(builder))), "freeze multiple entity types");
    auto vendor = root.resolve_entity<Vendor>(7);
    auto text = root.resolve_entity<String>(7);
    check(vendor && vendor->name == "Vendor" && text && *text == "not a vendor",
          "same numeric ID preserves both payload types");
}

static void stable_typed_to_many_view(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
    SmartList<Vendor> list(std::allocator_arg, Allocator(r));
    list.emplace_back(11, "first");
    list.emplace_back(12, "second");
    builder.install_relation_list("Vendor", 7, "trips", std::move(list));
    check(bool(root.freeze_graph(std::move(builder))), "freeze to-many view");
    auto first = root.resolve_relation_list<Vendor>("Vendor", 7, "trips");
    auto second = root.resolve_relation_list<Vendor>("Vendor", 7, "trips");
    check(first && first == second && first->data.size() == 2 && first->data[1].name == "second",
          "relation list address and ordered content remain stable");
    check(!root.resolve_relation_list<String>("Vendor", 7, "trips"), "wrong typed list is missing");
}

static void loaded_null_is_distinct_from_missing(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
    builder.install_relation_option<Vendor>("Garage", 7, "primary_vehicle", {});
    check(bool(root.freeze_graph(std::move(builder))), "freeze loaded-null view");
    auto option = root.resolve_relation_option<Vendor>("Garage", 7, "primary_vehicle");
    check(option && !*option && root.has_relation_view("Garage", 7, "primary_vehicle") &&
              !root.has_relation_view("Garage", 7, "backup_vehicle"),
          "loaded null versus absent relation");
}

int main() {
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        hydration_keeps_independent_mutation_ownership(*user_context);
        reference_preserves_snapshot_and_local_intent(*user_context);
        roots_share_entities_without_sharing_ledgers(*user_context);
        same_id_is_namespaced_by_type(*user_context);
        stable_typed_to_many_view(*user_context);
        loaded_null_is_distinct_from_missing(*user_context);
        std::cout << "FLAT_IDENTITY_CONTRACTS=6\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
