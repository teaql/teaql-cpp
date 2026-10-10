// All four original flat-identity-graph contracts, using Context-owned payloads.
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

static void roots_share_entities_without_sharing_ledgers(UserContext& user) {
    auto r = user.memory_resource();
    auto& graph_root = user.make<EntityRuntimeState>();
    auto& first =
        user.make<EntityRuntimeState>(EntityRuntimeState::fresh_with_shared_graph(r, graph_root));
    auto& second =
        user.make<EntityRuntimeState>(EntityRuntimeState::fresh_with_shared_graph(r, graph_root));
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

static void same_id_is_namespaced_by_type(UserContext& user) {
    auto r = user.memory_resource();
    auto& root = user.make<EntityRuntimeState>();
    EntityGraphBuilder builder(std::allocator_arg, Allocator(r));
    builder.install(7, Vendor(std::allocator_arg, Allocator(r), 7, "Vendor"));
    builder.install(7, String("not a vendor", r));
    check(bool(root.freeze_graph(std::move(builder))), "freeze multiple entity types");
    auto vendor = root.resolve_entity<Vendor>(7);
    auto text = root.resolve_entity<String>(7);
    check(vendor && vendor->name == "Vendor" && text && *text == "not a vendor",
          "same numeric ID preserves both payload types");
}

static void stable_typed_to_many_view(UserContext& user) {
    auto r = user.memory_resource();
    auto& root = user.make<EntityRuntimeState>();
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

static void loaded_null_is_distinct_from_missing(UserContext& user) {
    auto r = user.memory_resource();
    auto& root = user.make<EntityRuntimeState>();
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
        auto user = global.make_user_context();
        roots_share_entities_without_sharing_ledgers(*user);
        same_id_is_namespaced_by_type(*user);
        stable_typed_to_many_view(*user);
        loaded_null_is_distinct_from_missing(*user);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
