// Direct pinned root dynamic-property, relation-aggregate and cache contracts.
#include "queue_fixture.hpp"
#include <teaql/runtime/cache.hpp>

using namespace original_runtime_queue;

struct OrderAggregateDynamic : EntityDefaults<OrderAggregateDynamic> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "OrderAggregate";
    std::uint64_t id = 0;
    Record dynamic;

    OrderAggregateDynamic(std::allocator_arg_t, Allocator a) : dynamic(a.resource()) {}

    OrderAggregateDynamic(std::allocator_arg_t, Allocator a, const OrderAggregateDynamic& other)
        : id(other.id), dynamic(other.dynamic, a.resource()) {}

    OrderAggregateDynamic(const OrderAggregateDynamic&) = delete;
    OrderAggregateDynamic(OrderAggregateDynamic&&) noexcept = default;

    Memory memory_resource() const {
        return dynamic.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("orders");
        result.property("id", DataType::U64).id().not_null();
        return result;
    }

    static Result<OrderAggregateDynamic, EntityError> from_compact_row(Memory owner,
                                                                       CompactRow row) {
        using Output = Result<OrderAggregateDynamic, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        OrderAggregateDynamic result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.dynamic = row.into_map(owner);
        result.dynamic.erase(String("id", owner));
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields = std::move(dynamic);
        result.fields.insert_or_assign(String("id", memory_resource()),
                                       Value(memory_resource(), id));
        return result;
    }
};

static const Value* field(const Record& fields, std::string_view name) {
    auto found = fields.find(name);
    return found == fields.end() ? nullptr : &found->second;
}

static void field_is(const CompactRow& row, std::string_view name, const Value& expected) {
    auto* actual = row.get(name);
    check(actual && *actual == expected, "original aggregate field has its exact Value variant");
}

static void entity_data_service_collects_dynamic_properties_for_aggregate_output() {
    Fixture f;
    f.store.with_entity(OrderAggregateDynamic::entity_descriptor(f.owner));
    auto& registry = f.user->make<InMemoryEntityRegistry>();
    registry.with_entity("OrderAggregate");
    f.user->with_entity_registry(*f.user, registry);
    f.executor.rows.emplace_back(record(f.owner,
                                        {{"id", Value(f.owner, std::uint64_t(1))},
                                         {"lineCount", Value(f.owner, std::int64_t(3))},
                                         {"amount", Value(f.owner, 18.5)}}));
    auto bound = f.user->entity_data_service<StubExecutor>("OrderAggregate");
    check(bool(bound), "original dynamic entity service binding");
    using Output = Result<SmartList<OrderAggregateDynamic>, DataServiceError<StubError>>;
    auto& work =
        make_operation<Output,
                       detail::EntityBasicListOperation<StubExecutor, OrderAggregateDynamic>>(
            f.owner, bound.value().get(), bound.value().get().select());
    const auto& result = work.wait();
    check(result && result.value().data.size() == 1 && result.value().data[0].id == 1,
          "original aggregate model row count and ID");
    const auto& model = result.value().data[0];
    auto* count = field(model.dynamic, "lineCount");
    auto* amount = field(model.dynamic, "amount");
    check(count && *count == Value(f.owner, std::int64_t(3)) && amount &&
              *amount == Value(f.owner, 18.5),
          "original dynamic properties preserve I64 and F64");
    auto copy = detail::clone_in(f.owner, model);
    auto actual = std::move(copy).into_json(f.owner);
    auto expected = Json::parse(f.owner, R"({"id":1,"lineCount":3,"amount":18.5})");
    check(expected && actual == *expected, "complete original flattened dynamic-property JSON");
}

static QueueExecutor& queue(Fixture& f) {
    auto& executor = f.user->make<QueueExecutor>();
    f.user->insert_resource(*f.user, executor);
    return executor;
}

static EntityDataService<QueueExecutor>& service(Fixture& f) {
    auto bound = f.user->entity_data_service<QueueExecutor>("Order");
    check(bool(bound), "original queue entity-service binding");
    return bound.value().get();
}

static void parent_batch(Fixture& f, QueueExecutor& executor, bool two = true) {
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(record(f.owner,
                                                {{"id", Value(f.owner, std::uint64_t(1))},
                                                 {"version", Value(f.owner, std::int64_t(1))},
                                                 {"name", Value(f.owner, "first")}}));
    if (two) {
        executor.batches.back().emplace_back(record(f.owner,
                                                    {{"id", Value(f.owner, std::uint64_t(2))},
                                                     {"version", Value(f.owner, std::int64_t(1))},
                                                     {"name", Value(f.owner, "second")}}));
    }
}

static void aggregate_batch(Fixture& f, QueueExecutor& executor, bool storage = false) {
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(
        record(f.owner,
               {{storage ? "order_ref" : "order_id",
                 storage ? Value(f.owner, std::int64_t(1)) : Value(f.owner, std::uint64_t(1))},
                {"lineCount", Value(f.owner, std::int64_t(3))}}));
}

static SelectQuery parent_query(Fixture& f) {
    SelectQuery result(std::allocator_arg, Allocator(f.owner), "Order");
    result.project("id").project("version").project("name");
    return result;
}

static RelationAggregate aggregate(Fixture& f) {
    SelectQuery child(std::allocator_arg, Allocator(f.owner), "OrderLine");
    return RelationAggregate(
        std::allocator_arg, Allocator(f.owner), "lines", "lineCount", child, true);
}

static auto& relation_rows(const EntityDataService<QueueExecutor>& service,
                           const SelectQuery& query,
                           const RelationAggregate& aggregate) {
    return make_operation<detail::RelationRowsResult<QueueExecutor>,
                          detail::EntityRowsOperation<QueueExecutor>>(
        service.memory_resource(),
        service,
        query,
        std::span<const RelationAggregate>(&aggregate, 1),
        true);
}

static void entity_data_service_executes_relation_aggregates_into_dynamic_properties() {
    Fixture f(1, true);
    auto& executor = queue(f);
    parent_batch(f, executor);
    aggregate_batch(f, executor);
    auto query = parent_query(f);
    auto relation = aggregate(f);
    const auto& result = relation_rows(service(f), query, relation).wait();
    check(result && result.value().size() == 2, "original parent aggregate rows");
    field_is(result.value()[0], "lineCount", Value(f.owner, std::int64_t(3)));
    field_is(result.value()[1], "lineCount", Value(f.owner, std::uint64_t(0)));
    check(executor.queries.size() == 2 && executor.queries[1].entity == "OrderLine",
          "original exactly two queries and OrderLine aggregate target");
}

static void entity_data_service_maps_relation_aggregate_storage_key_to_property_key() {
    Fixture f(1, true);
    auto line = line_entity(f.owner);
    auto property =
        std::ranges::find(line.properties, std::string_view("order_id"), &PropertyDescriptor::name);
    check(property != line.properties.end(), "original FK property exists");
    property->column_name = "order_ref";
    f.store.with_entity(std::move(line));
    auto& executor = queue(f);
    parent_batch(f, executor, false);
    aggregate_batch(f, executor, true);
    auto query = parent_query(f);
    auto relation = aggregate(f);
    const auto& result = relation_rows(service(f), query, relation).wait();
    check(result && !result.value().empty(), "original physical-FK aggregate succeeds");
    field_is(result.value()[0], "lineCount", Value(f.owner, std::int64_t(3)));
    check(executor.queries.size() > 1 && executor.queries[1].entity == "OrderLine",
          "original physical-key query target");
}

static SelectQuery count_query(Fixture& f) {
    SelectQuery result(std::allocator_arg, Allocator(f.owner), "Order");
    result.count("count").enable_aggregation_cache_for(60'000);
    return result;
}

static void entity_data_service_uses_aggregation_cache_when_resource_is_registered() {
    Fixture f;
    auto& executor = queue(f);
    executor.batches.emplace_back();
    executor.batches.back().emplace_back(
        record(f.owner, {{"count", Value(f.owner, std::int64_t(2))}}));
    auto& cache = f.user->make<InMemoryAggregationCache>();
    f.user->insert_resource(*f.user, cache);
    auto& events = f.user->make<Vector<String>>();
    auto& telemetry = f.user->make<RecordingTelemetry>(events);
    f.user->with_runtime_telemetry(*f.user, telemetry);
    auto& repo = service(f);
    auto query = count_query(f);
    const auto& first = Access::fetch_all(repo, query).wait();
    const auto& second = Access::fetch_all(repo, query).wait();
    check(first && second && first.value() == second.value() && executor.queries.size() == 1,
          "original complete cached rows and one provider query");
    check(std::ranges::count(events, std::string_view("start:cache")) == 2 &&
              std::ranges::count(events, std::string_view("start:provider")) == 1,
          "original cache/provider telemetry counts");
}

static void aggregation_cache_is_namespaced_and_invalidated_after_write() {
    Fixture f;
    auto& executor = queue(f);
    for (std::int64_t count : {2, 3}) {
        executor.batches.emplace_back();
        executor.batches.back().emplace_back(record(f.owner, {{"count", Value(f.owner, count)}}));
    }
    auto& cache = f.user->make<InMemoryAggregationCache>("tenant-a");
    f.user->insert_resource(*f.user, static_cast<AggregationCacheBackend&>(cache));
    auto& repo = service(f);
    auto query = count_query(f);
    const auto& first = Access::fetch_all(repo, query).wait();
    const auto& cached = Access::fetch_all(repo, query).wait();
    InsertCommand insert(std::allocator_arg, Allocator(f.owner), "Order");
    insert.value("id", std::uint64_t(9)).value("version", std::int64_t(1)).value("name", "new");
    check(bool(Access::insert_internal(repo, insert).wait()),
          "original cache-invalidating insert succeeds");
    const auto& refreshed = Access::fetch_all(repo, query).wait();
    check(first && cached && refreshed && first.value() == cached.value() &&
              cached.value() != refreshed.value() && executor.queries.size() == 2,
          "original namespaced cache is invalidated after write");
}

static void aggregation_cache_propagates_to_relation_aggregates() {
    Fixture f(1, true);
    auto& executor = queue(f);
    parent_batch(f, executor);
    aggregate_batch(f, executor);
    auto& cache = f.user->make<InMemoryAggregationCache>();
    f.user->insert_resource(*f.user, cache);
    auto& repo = service(f);
    auto query = parent_query(f);
    query.enable_aggregation_cache_for(60'000).propagate_aggregation_cache(60'000);
    auto relation = aggregate(f);
    const auto& first = relation_rows(repo, query, relation).wait();
    const auto& second = relation_rows(repo, query, relation).wait();
    check(
        first && second && executor.queries.size() == 2 && first.value() == second.value(),
        "original parent/child cache propagation preserves full rows and only two provider queries");
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
        entity_data_service_collects_dynamic_properties_for_aggregate_output();
        entity_data_service_executes_relation_aggregates_into_dynamic_properties();
        entity_data_service_maps_relation_aggregate_storage_key_to_property_key();
        entity_data_service_uses_aggregation_cache_when_resource_is_registered();
        aggregation_cache_is_namespaced_and_invalidated_after_write();
        aggregation_cache_propagates_to_relation_aggregates();
        std::cout << "Six original runtime-root aggregate/cache contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
