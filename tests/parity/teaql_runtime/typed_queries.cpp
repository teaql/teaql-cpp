// Direct pinned runtime-root typed decoding and identity-graph getter contracts.
#include "typed_models.hpp"
#include "queue_fixture.hpp"
#include <teaql/runtime/module.hpp>

using namespace original_runtime_typed;
using namespace original_runtime_queue;

template <class T, class E>
static auto& basic(const EntityDataService<E>& service, const SelectQuery& query) {
    using Output = Result<SmartList<T>, DataServiceError<typename E::Error>>;
    return make_operation<Output, detail::EntityBasicListOperation<E, T>>(
        service.memory_resource(), service, query);
}

template <class T, class E>
static auto& enhanced(const EntityDataService<E>& service, const SelectQuery& query) {
    using Output = Result<SmartList<T>, DataServiceError<typename E::Error>>;
    return make_operation<Output, detail::EnhancedEntitiesOperation<E, T>>(
        service.memory_resource(), service, query, std::span<const RelationAggregate>{});
}

static void relation_enhancement_wraps_inverse_many_relation_as_list() {
    Fixture f;
    f.store.with_entity(OrderLineWithProductEntityRow::entity_descriptor(f.owner));
    f.store.with_entity(ProductWithLinesEntityRow::entity_descriptor(f.owner));
    auto& executor = f.user->make<QueueExecutor>();
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(
        record(f.owner,
               {{"id", Value(f.owner, std::uint64_t(11))},
                {"order_id", Value(f.owner, std::uint64_t(7))},
                {"name", Value(f.owner, "line")},
                {"product_id", Value(f.owner, std::uint64_t(101))}}));
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(record(
        f.owner, {{"id", Value(f.owner, std::uint64_t(101))}, {"name", Value(f.owner, "sku")}}));
    f.user->insert_resource(*f.user, executor);
    auto bound = f.user->entity_data_service<QueueExecutor>("OrderLine");
    check(bool(bound), "original OrderLine service binding");
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "OrderLine");
    query.relation("product");
    const auto& rows = enhanced<OrderLineWithProductEntityRow>(bound.value().get(), query).wait();
    check(rows && !rows.value().data.empty() && rows.value().data[0].product,
          "original enhanced row has the related product");
    const auto& product = *rows.value().data[0].product;
    check(product.lines.data.size() == 1 && product.lines.data[0].id == 11,
          "original inverse-many relation is a list containing line11");
}

struct ModuleFixture {
    GlobalContext global;
    RuntimeModule& module = global.make<RuntimeModule>();
    std::unique_ptr<UserContext> user;
    Memory owner = nullptr;

    void apply() {
        user = module.into_context(global);
        owner = user->memory_resource();
        auto& dialect = user->make_resource<TestDialect>();
        user->insert_resource(*user, dialect);
    }
};

static void generated_to_one_getter_resolves_from_runtime_module_identity_graph() {
    ModuleFixture f;
    f.module.entity<FlatTripRow>().entity<FlatVendorRow>();
    f.apply();
    auto& executor = f.user->make<QueueExecutor>();
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(
        record(f.owner,
               {{"id", Value(f.owner, std::uint64_t(11))},
                {"vendor_id", Value(f.owner, std::uint64_t(101))}}));
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(record(
        f.owner, {{"id", Value(f.owner, std::uint64_t(101))}, {"name", Value(f.owner, "Acme")}}));
    f.user->insert_resource(*f.user, executor);
    auto bound = f.user->entity_data_service<QueueExecutor>("FlatTrip");
    check(bool(bound), "original FlatTrip module service binding");
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "FlatTrip");
    query.relation("vendor");
    const auto& rows = enhanced<FlatTripRow>(bound.value().get(), query).wait();
    check(rows && !rows.value().data.empty(), "original flat Trip fetch succeeds");
    const auto& trip = rows.value().data[0];
    check(
        !trip.vendor_value && trip.vendor() && trip.vendor()->name == "Acme",
        "original scalar relation remains absent while getter resolves the actual identity graph");
}

static void generated_to_many_getter_uses_adjacency_and_mutation_copies_on_write() {
    ModuleFixture f;
    f.module.entity<FlatFleetRow>().entity<FlatFleetTripRow>();
    f.apply();
    auto& executor = f.user->make<QueueExecutor>();
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(
        record(f.owner, {{"id", Value(f.owner, std::uint64_t(7))}}));
    executor.batches.emplace_back();
    for (const auto& [id, name] : {std::pair{11u, "first"}, {12u, "second"}}) {
        executor.batches.back().emplace_back(record(f.owner,
                                                    {{"id", Value(f.owner, std::uint64_t(id))},
                                                     {"fleet_id", Value(f.owner, std::uint64_t(7))},
                                                     {"name", Value(f.owner, name)}}));
    }
    f.user->insert_resource(*f.user, executor);
    auto bound = f.user->entity_data_service<QueueExecutor>("FlatFleet");
    check(bool(bound), "original FlatFleet module service binding");
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "FlatFleet");
    query.relation("trip_list");
    auto& result = enhanced<FlatFleetRow>(bound.value().get(), query).wait();
    check(result && !result.value().data.empty(), "original flat Fleet fetch succeeds");
    auto fleet = detail::clone_in(f.owner, result.value().data[0]);
    check(!fleet.trip_list_value.is_loaded && fleet.trip_list().data.size() == 2 &&
              fleet.trip_list().data[1].name == "second",
          "original getter uses immutable adjacency");
    FlatFleetTripRow third(std::allocator_arg, Allocator(f.owner));
    third.id = 13;
    third.fleet_id = 7;
    third.name = "third";
    fleet.trip_list_mut().push(std::move(third));
    check(fleet.trip_list_value.is_loaded && fleet.trip_list().data.size() == 3,
          "original mutation materializes the local relation list");
    const auto* graph =
        fleet.root.resolve_relation_list<FlatFleetTripRow>("FlatFleet", 7, "trip_list");
    check(graph && graph->data.size() == 2,
          "local copy-on-write does not mutate immutable graph adjacency");
}

static void entity_data_service_fetches_smart_list_of_entities() {
    Fixture f;
    f.executor.rows.emplace_back(record(f.owner,
                                        {{"id", Value(f.owner, std::uint64_t(7))},
                                         {"version", Value(f.owner, std::int64_t(2))},
                                         {"name", Value(f.owner, "typed")}}));
    auto& service = f.service();
    const auto& rows = basic<OrderEntity>(service, service.select()).wait();
    check(rows && rows.value().data.size() == 1 && rows.value().data[0].id == 7 &&
              rows.value().data[0].revision == 2 && rows.value().data[0].name == "typed",
          "original complete handwritten entity and SmartList assertions");
}

static void typed_entity_fetch_restores_id_and_version_to_reduced_projection() {
    Fixture f;
    auto& executor = f.user->make<QueueExecutor>(true);
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(record(f.owner,
                                                {{"id", Value(f.owner, std::uint64_t(7))},
                                                 {"version", Value(f.owner, std::int64_t(2))},
                                                 {"name", Value(f.owner, "typed")}}));
    f.user->insert_resource(*f.user, executor);
    auto bound = f.user->entity_data_service<QueueExecutor>("Order");
    check(bool(bound), "original reduced-projection capture binding");
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "Order");
    query.project("name");
    const auto& rows = basic<OrderEntity>(bound.value().get(), query).wait();
    const auto& enhanced_rows = enhanced<OrderEntity>(bound.value().get(), query).wait();
    check(rows && rows.value().data.size() == 1 && enhanced_rows &&
              enhanced_rows.value().data.size() == 1,
          "original basic and enhanced typed query row counts");
    const std::array<std::string_view, 3> expected{"name", "id", "version"};
    check(executor.queries.size() == 2 &&
              std::ranges::equal(executor.queries[0].projection, expected) &&
              std::ranges::equal(executor.queries[1].projection, expected),
          "both original queries restore complete ordered identity/version projection");
}

static void entity_data_service_fetches_smart_list_of_derived_entities() {
    Fixture f;
    f.store.with_entity(CatalogProductRow::entity_descriptor(f.owner));
    auto& registry = f.user->make<InMemoryEntityRegistry>();
    registry.with_entity("CatalogProduct");
    f.user->with_entity_registry(*f.user, registry);
    f.executor.rows.emplace_back(record(
        f.owner, {{"id", Value(f.owner, std::uint64_t(9))}, {"name", Value(f.owner, "derived")}}));
    auto bound = f.user->entity_data_service<StubExecutor>("CatalogProduct");
    check(bool(bound), "original CatalogProduct service binding");
    const auto& rows =
        basic<CatalogProductRow>(bound.value().get(), bound.value().get().select()).wait();
    check(rows && rows.value().data.size() == 1 && rows.value().data[0].id == 9 &&
              rows.value().data[0].name == "derived",
          "original complete private derived model and SmartList assertions");
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
        relation_enhancement_wraps_inverse_many_relation_as_list();
        generated_to_one_getter_resolves_from_runtime_module_identity_graph();
        generated_to_many_getter_uses_adjacency_and_mutation_copies_on_write();
        entity_data_service_fetches_smart_list_of_entities();
        typed_entity_fetch_restores_id_and_version_to_reduced_projection();
        entity_data_service_fetches_smart_list_of_derived_entities();
        std::cout << "Six original runtime-root typed-query contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
