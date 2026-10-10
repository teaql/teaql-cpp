#include <teaql/runtime/detail/context/entity.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include "../../tests/support/entity_decoder_fixture.hpp"
#include <iostream>
using namespace teaql;
using namespace decoder_fixture;
using Access = detail::ContextEntityAccess;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejected(F&& f, const char* message) {
    bool failed = false;
    try {
        f();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

static CompactRow row(Memory r, std::uint64_t id = 1) {
    Record values(r);
    values.emplace(String("id", r), Value(r, id));
    values.emplace(String("label", r), Value(r, "long model payload owned by explicit context"));
    return CompactRow(std::allocator_arg, Allocator(r), values);
}

static void decoding_and_copies() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& root = user->make<EntityRuntimeState>();
    auto& registry = user->make<InMemoryEntityGraphDecoderRegistry>();
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    check(!Access::has_entity_graph_decoder(*user, "Item"),
          "new Context has empty decoder registry");
    auto missing = Access::decode_entity_graph_compact(*user, "Item", row(r), root, graph);
    check(!missing && missing.error().message ==
                          "entity has no compact identity graph decoder in RuntimeModule",
          "Context forwards exact missing decoder error");
    registry.register_entity<Item<>>();
    user->set_entity_graph_decoder_registry(registry);
    registry.register_entity<Item<1>>();
    Vector<CompactRow> rows(r);
    rows.emplace_back(row(r, 1));
    rows.emplace_back(row(r, 2));
    check(bool(Access::decode_entity_graph_compact_batch(
              *user, "Item", std::move(rows), root, graph)),
          "Context batch decoder succeeds");
    rows.emplace_back(row(r, 1));
    rows.emplace_back(row(r, 2));
    check(bool(Access::decode_entity_graph_compact_list(
              *user, "Item", std::move(rows), root, graph, "PurchaseOrder", 9, "items")),
          "Context list decoder succeeds");
    rows = Vector<CompactRow>(r);
    check(bool(Access::decode_entity_graph_compact_option(
              *user, "Item", std::move(rows), root, graph, "PurchaseOrder", 9, "optional")),
          "Context option decoder succeeds");
    check(bool(root.freeze_graph(std::move(graph))), "decoded graph freezes");
    auto* first = root.resolve_entity<Item<>>(1);
    auto* second = root.resolve_entity<Item<>>(2);
    check(first && second && !root.resolve_entity<Item<1>>(1),
          "Context owns registry copy independent of source replacement");
    check(first->state && *first->state == *second->state && *first->state != root,
          "batch ledger shared within decode and independent from root");
    check(first->state->resolve_entity<Item<>>(2) == second,
          "weak decoded state resolves frozen graph");
    check(root.relation_option<Item<>>("purchase_order", 9, "optional").state() ==
              LoadedRelation::Empty,
          "installed empty option distinct from missing");
    EntityKey key(std::allocator_arg, Allocator(r), "Item", Value(r, std::uint64_t(1)));
    first->state->set(key, "label", Value(r, "changed"));
    check(second->state->get(key, "label").has_value() && !root.get(key, "label"),
          "batch mutations share only their fresh ledger");
    auto& retained = global.make<EntityRuntimeState>(root);
    auto& registry_copy = global.make<InMemoryEntityGraphDecoderRegistry>(registry);
    user->close();
    check(user->allocation_stats().live_blocks == 0, "User close reclaims every arena block");
    auto* a = retained.resolve_entity<Item<>>(1);
    auto* b = retained.resolve_entity<Item<>>(2);
    auto g = global.memory_resource();
    check(a && b && a->memory_resource() == g && a->row.get("label")->memory_resource() == g,
          "explicit Global graph copy retains nested row payloads");
    check(a->state->resolve_entity<Item<>>(2) == b && *a->state == *b->state,
          "deep graph copy preserves child backreferences and ledger aliases");
    auto sibling = global.make_user_context();
    sibling->set_entity_graph_decoder_registry(registry_copy);
    check(Access::has_entity_graph_decoder(*sibling, "Item"),
          "decoder functions survive source registry and Context close");
    auto& other = sibling->make<EntityRuntimeState>();
    EntityGraphBuilder mixed(std::allocator_arg, Allocator(g));
    rejected(
        [&] {
            registry_copy.decode_compact("Item", row(g), other, mixed);
        },
        "cross-domain root requires explicit copy before decoding");
    check(mixed.entity_count() == 0, "domain rejection happens before graph mutation");
    rejected(
        [&] {
            Access::has_entity_graph_decoder(*user, "Item");
        },
        "closed Context refuses decoder access");
}

struct ScopedPolicy : RequestPolicy {
    unsigned& destroyed;

    explicit ScopedPolicy(unsigned& n) : destroyed(n) {}

    ~ScopedPolicy() {
        ++destroyed;
    }

    RuntimeHookResult enforce_select(Memory r, const UserContext&, SelectQuery& q) const override {
        q.entity = "policy";
        return RuntimeHookResult::err(r, RuntimeError::policy(r, "policy denied"));
    }
};

struct ScopedBehavior : NamedBehavior {
    unsigned& destroyed;

    ScopedBehavior(std::allocator_arg_t, Allocator a, unsigned& n)
        : NamedBehavior(std::allocator_arg, a, "scoped relation"), destroyed(n) {}

    ~ScopedBehavior() {
        ++destroyed;
    }

    RuntimeHookResult before_select(Memory r, const UserContext&, SelectQuery& q) const override {
        q.entity = "behavior";
        return RuntimeHookResult::ok(r, {});
    }
};

static void registrations_and_hooks() {
    unsigned policies = 0, behaviors = 0;
    GlobalContext global;
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    auto r = user->memory_resource();
    auto g = global.memory_resource();
    auto& module = user->make<RuntimeModule>();
    auto& b = user->make<ScopedBehavior>(behaviors);
    auto& policy = user->make_resource<ScopedPolicy>(policies);
    check(!user->entity_data_service_behavior("Item") && !Access::request_policy(*user),
          "Context has no behavior or request policy by default");
    user->with_request_policy(*user, policy);
    module.behavior("OnlyBehavior", *user, b)
        .entity_with_behavior<Item<2>>(*user, b)
        .entity<Item<>>();
    module.apply_to(*user);
    check(user->has_entity_data_service("Alias") &&
              Access::has_entity_graph_decoder(*user, "Item") &&
              !Access::has_entity_graph_decoder(*user, "Alias"),
          "module preserves descriptor-name and ENTITY_NAME registration keys");
    check(user->entity_data_service_behavior("Alias") &&
              !user->has_entity_data_service("OnlyBehavior") &&
              user->entity_data_service_behavior("OnlyBehavior"),
          "behavior-only registration does not manufacture entity service");
    check(Access::request_policy(*user) == &policy,
          "module application preserves installed request policy");
    auto& q = user->make<SelectQuery>("Item");
    auto hook = user->entity_data_service_behavior("Alias")->get().before_select(r, *user, q);
    check(hook && q.entity == "behavior", "custom behavior mutates caller query");
    auto denied = Access::request_policy(*user)->enforce_select(r, *user, q);
    check(!denied && denied.error().kind() == RuntimeError::Kind::Policy &&
              denied.error().message() == "policy denied" && q.entity == "policy",
          "policy result and in-place changes are preserved");
    auto& old = sibling->make<InMemoryMetadataStore>();
    old.register_entity(
        EntityDescriptor(std::allocator_arg, Allocator(sibling->memory_resource()), "Existing"));
    sibling->set_metadata(*sibling, old);
    rejected(
        [&] {
            module.apply_to(*sibling);
        },
        "module cannot carry User behavior into sibling");
    check(sibling->entity("Existing") && !sibling->entity("Alias"),
          "failed module scope validation leaves installed Context registries intact");
    rejected(
        [&] {
            global.make<RuntimeModule>(module);
        },
        "Global module cannot retain User behavior");
    rejected(
        [&] {
            sibling->set_request_policy(*user, policy);
        },
        "sibling cannot borrow User policy");
    auto& registry = user->make<InMemoryEntityDataServiceBehaviorRegistry>();
    registry.register_behavior("Item", *user, b);
    auto borrow = registry.behavior(r, "Item");
    rejected(
        [&] {
            global.make<EntityBehaviorBorrow>(*borrow);
        },
        "behavior borrow cannot extend User lifetime");
    auto& shared = global.make<NamedBehavior>("global relation");
    registry.register_behavior("Item", global, shared);
    auto& copy = global.make<InMemoryEntityDataServiceBehaviorRegistry>(registry);
    check(copy.behavior(g, "Item")->get().relation_loads(g, *user)[0] == "global relation",
          "replacement and registry copy retain checked Global borrow");
    user->clear_request_policy();
    check(!Access::request_policy(*user) && policies == 0,
          "clear policy detaches borrow; owning Context controls destruction");
    RequestPolicy default_policy;
    EntityDataServiceBehavior default_behavior;
    auto& insert = user->make<InsertCommand>("Item");
    auto& update = user->make<UpdateCommand>("Item", 1);
    auto& del = user->make<DeleteCommand>("Item", 1);
    auto& recover = user->make<RecoverCommand>("Item", 1, 2);
    check(default_policy.enforce_select(r, *user, q) &&
              default_policy.enforce_insert(r, *user, insert) &&
              default_policy.enforce_update(r, *user, update) &&
              default_policy.enforce_delete(r, *user, del) &&
              default_policy.enforce_recover(r, *user, recover),
          "all five policy defaults allow");
    check(default_behavior.before_select(r, *user, q) &&
              default_behavior.before_insert(r, *user, insert) &&
              default_behavior.before_update(r, *user, update) &&
              default_behavior.before_delete(r, *user, del) &&
              default_behavior.before_recover(r, *user, recover) &&
              default_behavior.relation_loads(r, *user).empty(),
          "all behavior defaults allow and request no relations");
    check(q.entity == "policy" && insert.values.fields.empty() && update.values.fields.empty() &&
              del.soft_delete && recover.expected_version == 2,
          "defaults leave commands unchanged");
    user->close();
    check(policies == 1 && behaviors == 1,
          "resource and managed behavior destructors run before arena reclamation");
    check(copy.behavior(sibling->memory_resource(), "Item")
                  ->get()
                  .relation_loads(sibling->memory_resource(), *sibling)[0] == "global relation",
          "Global behavior registry survives User close");
    rejected(
        [&] {
            user->clear_request_policy();
        },
        "closed Context refuses policy mutation");
}

static void close_reads_empty_registries() {
    GlobalContext global;
    auto user = global.make_user_context();
    bool checked = false;

    struct Evidence {
        UserContext& context;
        bool& checked;
    } evidence{*user, checked};

    Arena::Quiescence stop;
    stop.attach(detail::resource_arena(user->memory_resource()), &evidence, [](void* raw) noexcept {
        auto& e = *static_cast<Evidence*>(raw);
        try {
            e.checked = !Access::has_entity_graph_decoder(e.context, "Item") &&
                        !e.context.entity_data_service_behavior("Item") &&
                        !Access::request_policy(e.context);
        } catch (...) {
            e.checked = false;
        }
    });
    user->close();
    check(checked, "empty registry reads remain allocation-free during Context quiescence");
}

static void sqlite_rows_to_graph() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& connection = user->make_resource<SqliteConnection>();
    auto& executor = user->make<SqliteMutationExecutor>(connection);
    auto& query = user->make<CompiledQuery>();
    query.sql =
        "SELECT 1 AS id, 'first from SQLite' AS label UNION ALL SELECT 2, 'second from SQLite'";
    auto rows = executor.fetch_all_compact(r, query);
    check(bool(rows), "actual SQLite provider returns CompactRows");
    auto& module = user->make<RuntimeModule>();
    module.entity<Item<>>().apply_to(*user);
    auto& root = user->make<EntityRuntimeState>();
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    check(bool(Access::decode_entity_graph_compact_batch(
              *user, "Item", std::move(rows).value(), root, graph)),
          "SQLite rows decode through runtime registry");
    check(bool(root.freeze_graph(std::move(graph))), "SQLite entity graph freezes");
    auto* second = root.resolve_entity<Item<>>(2);
    check(second && second->row.get("label")->try_text() == "second from SQLite" &&
              second->state->resolve_entity<Item<>>(1),
          "handwritten model views resolve SQLite identities");
    auto& retained = global.make<EntityRuntimeState>(root);
    user->close();
    check(retained.resolve_entity<Item<>>(1)->row.get("label")->try_text() == "first from SQLite",
          "explicit graph retention survives SQLite native teardown");
}

static void module_clone_after_user_deletion() {
    GlobalContext global;
    auto source = global.make_user_context();
    auto& first = global.make<NamedBehavior>("first global behavior with a long retained name");
    auto& replacement = global.make<NamedBehavior>("replacement global behavior");
    auto& module = source->make<RuntimeModule>();
    module.entity_with_behavior<Item<2>>(global, first)
        .entity_with_behavior<Item<0>>(global, first)
        .entity<Item<1>>();
    auto& retained = global.make<RuntimeModule>(module);
    module.entity<Item<2>>().behavior("Item", global, replacement);
    source.reset();

    auto target = retained.into_context(global);
    auto owner = target->memory_resource();
    check(target->entity("Alias") && target->entity("Item") &&
              target->entity("Item")->table_name == "alternate_items" &&
              target->entity("Item")->memory_resource() == owner,
          "Global module clone retains independent metadata after actual source User deletion");
    check(target->entity_data_service_behavior("Item")->get().relation_loads(owner, *target)[0] ==
              "first global behavior with a long retained name",
          "entity-only replacement preserves independently registered behavior");
    check(Access::has_entity_graph_decoder(*target, "Item") &&
              !Access::has_entity_graph_decoder(*target, "Alias"),
          "retained typed registration keeps type keys separate from descriptor keys");
    auto& root = target->make<EntityRuntimeState>();
    {
        EntityGraphBuilder graph(std::allocator_arg, Allocator(owner));
        check(bool(Access::decode_entity_graph_compact(*target, "Item", row(owner), root, graph)),
              "cloned module decoder runs after its source User is destroyed");
        check(bool(root.freeze_graph(std::move(graph))), "cloned decoder graph freezes");
    }
    check(root.resolve_entity<Item<1>>(1) && !root.resolve_entity<Item<2>>(1),
          "decoder replacement order is retained in the module snapshot");

    retained.entity<Item<2>>().behavior("Item", global, replacement);
    auto next = retained.into_context(global);
    check(target->entity_data_service_behavior("Item")->get().relation_loads(owner, *target)[0] ==
                  "first global behavior with a long retained name" &&
              next->entity_data_service_behavior("Item")->get().relation_loads(
                  next->memory_resource(), *next)[0] == "replacement global behavior",
          "installed Context snapshot stays independent of later module behavior replacement");
    auto& empty = global.make<RuntimeModule>();
    empty.apply_to(*target);
    check(!target->entity("Item") && !target->has_entity_data_service("Alias") &&
              !target->entity_data_service_behavior("Item") &&
              !Access::has_entity_graph_decoder(*target, "Item"),
          "empty module replaces every independent metadata, entity, behavior and decoder role");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        decoding_and_copies();
        module_clone_after_user_deletion();
        registrations_and_hooks();
        close_reads_empty_registries();
        sqlite_rows_to_graph();
        std::pmr::set_default_resource(previous);
        std::cout << "Entity decoder, behavior and policy contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
