#include <teaql/runtime/entity_save.hpp>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void ordering_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& metadata = user->make<InMemoryMetadataStore>();
    EntityDescriptor child(std::allocator_arg, Allocator(r), "AChild");
    RelationDescriptor parent(std::allocator_arg, Allocator(r), "parent", "ZParent");
    parent.local("parent_id");
    child.relations.emplace_back(parent);
    child.relations.emplace_back(parent);
    metadata.register_entity(std::move(child));
    metadata.register_entity(EntityDescriptor(std::allocator_arg, Allocator(r), "ZParent"));
    user->set_metadata(*user, metadata);
    auto& keys = user->make<Vector<EntityKey>>();
    keys.emplace_back("AChild", Value(r, std::uint64_t(2)));
    keys.emplace_back("ZParent", Value(r, std::uint64_t(1)));
    auto& changes = user->make<detail::LedgerChanges>();
    EntityValues record(std::allocator_arg, Allocator(r));
    record.fields.emplace(String("parent_id", r), Value(r, std::uint64_t(1)));
    changes.emplace(detail::clone_in(r, keys[0]), std::move(record));
    Vector<EntityKey>* retained;
    {
        auto result = detail::order_new_ledger_keys(*user, keys, changes);
        check(result && result.value().size() == 2 && result.value()[0].entity == "ZParent" &&
                  result.value()[1].entity == "AChild",
              "to-one dependency precedes child; duplicate edges do not block completion");
        retained = &global.make<Vector<EntityKey>>(result.value());
        check(result.value().get_allocator().resource() == r &&
                  result.value()[0].memory_resource() == r,
              "ordered keys and nested identities remain User-owned");
    }
    keys.emplace_back("ZParent", Value(r, std::int64_t(1)));
    {
        auto result = detail::order_new_ledger_keys(*user, keys, changes);
        check(result && result.value().size() == 2 && result.value()[0].entity == "AChild" &&
                  std::holds_alternative<std::int64_t>(result.value()[1].id.data),
              "last canonical representative retains its Value and prevents a mixed-type edge");
    }
    user.reset();
    check(retained->size() == 2 && retained->front().entity == "ZParent" &&
              retained->front().memory_resource() == global.memory_resource(),
          "explicit Global result copy survives actual User destruction");
}

static void metadata_and_cycles() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& metadata = user->make<InMemoryMetadataStore>();
    user->set_metadata(*user, metadata);
    auto& keys = user->make<Vector<EntityKey>>();
    auto& changes = user->make<detail::LedgerChanges>();
    check(bool(detail::order_new_ledger_keys(*user, keys, changes)),
          "empty key set requires no entity descriptor");
    keys.emplace_back("A", Value(r, std::uint64_t(1)));
    check(!detail::order_new_ledger_keys(*user, keys, changes),
          "missing descriptor rejects a key even without changes");
    keys.emplace_back("B", Value(r, std::uint64_t(1)));
    for (auto name : {"A", "B"}) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), name);
        descriptor.relations.emplace_back("link", name == std::string_view("A") ? "B" : "A");
        descriptor.relations.back().local("link");
        metadata.register_entity(std::move(descriptor));
    }
    for (const auto& key : keys) {
        EntityValues values(std::allocator_arg, Allocator(r));
        values.fields.emplace(String("link", r), Value(r, std::uint64_t(1)));
        changes.emplace(detail::clone_in(r, key), std::move(values));
    }
    auto cycle = detail::order_new_ledger_keys(*user, keys, changes);
    check(!cycle && cycle.error().message() ==
                        "new entity graph contains a required foreign-key cycle among: A, B",
          "cycle error reports unique sorted blocked entities");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ordering_and_retention();
        metadata_and_cycles();
        std::pmr::set_default_resource(previous);
        std::cout << "Ledger dependency contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
