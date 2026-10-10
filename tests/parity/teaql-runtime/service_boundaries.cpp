// Direct pinned runtime-root service, hook, policy and ID-preparation contracts.
#include "root_fixture.hpp"

using namespace original_runtime_root;

static const Value* field(const InsertCommand& command, std::string_view name) {
    auto found = command.values.fields.find(name);
    return found == command.values.fields.end() ? nullptr : &found->second;
}

static void field_is(const InsertCommand& command, std::string_view name, const Value& expected) {
    auto* actual = field(command, name);
    check(actual && *actual == expected,
          "original prepared command retains exact field Value variant");
}

static void data_service_returns_optimistic_lock_conflict() {
    Fixture f(0);
    auto borrow = f.user->require_resource<StubExecutor>();
    auto& service = f.user->make<detail::RuntimeDataService<InMemoryMetadataStore, StubExecutor>>(
        f.store, borrow.value());
    UpdateCommand command(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(1));
    command.expected_version(3).value("name", "next");
    const auto& result = service.update(command).wait();
    check(!result && result.error().kind() == DataServiceError<StubError>::Kind::Runtime &&
              result.error().runtime_error().kind() == RuntimeError::Kind::OptimisticLockConflict,
          "original affected0/version3 update returns typed optimistic-lock conflict");
}

static void user_context_builds_context_data_service() {
    Fixture f;
    auto& events = f.user->make<Vector<String>>();
    auto& telemetry = f.user->make<RecordingTelemetry>(events);
    f.user->with_runtime_telemetry(*f.user, telemetry);
    auto executor = detail::ContextEntityAccess::require_executor<StubExecutor>(*f.user);
    check(bool(executor), "original Context resolves registered executor");
    auto& service =
        f.user->make<detail::ContextDataService<StubExecutor>>(*f.user, executor.value());
    UpdateCommand command(std::allocator_arg, Allocator(f.owner), "Order", std::uint64_t(1));
    command.expected_version(3).value("name", "next");
    const auto& result = service.update(command).wait();
    check(result && result.value() == 1, "original Context update affected row count");
    const std::array<std::string_view, 4> expected{
        "start:mutation", "start:provider", "success", "success"};
    check(std::ranges::equal(events, expected),
          "complete original mutation/provider telemetry ordering");
}

static void user_context_resolves_entity_data_service_by_entity_type() {
    Fixture f;
    auto& service = f.service();
    check(service.entity() == "Order" && service.select().entity == "Order",
          "original named entity/select binding");
    auto insert = service.insert_command();
    insert.value("id", std::uint64_t(1)).value("version", std::int64_t(1)).value("name", "n");
    const auto& result = Access::insert_internal(service, insert).wait();
    check(result && result.value() == 1, "original bound entity insert affected count");
}

static void entity_data_service_applies_behavior_hooks() {
    Fixture f(1, true);
    f.behavior<OrderBehavior>();
    auto& service = f.service();
    auto insert = service.insert_command();
    insert.value("id", std::uint64_t(1)).value("name", "n");
    const auto& result = Access::insert_internal(service, insert).wait();
    check(result && result.value() == 1, "original insert passes default-version behavior");
    const std::array<std::string_view, 1> expected{"lines"};
    check(std::ranges::equal(service.relation_loads(), expected),
          "original behavior relation loads");
}

static void entity_data_service_applies_request_policy_after_behavior_hooks() {
    Fixture f(1, true);
    f.behavior<OrderBehavior>();
    auto& policy = f.user->make_resource<TenantRequestPolicy>();
    f.user->with_request_policy(*f.user, policy);
    auto& tenant = f.user->make<std::uint64_t>(9);
    f.user->insert_named_resource("tenant_id", *f.user, tenant);
    auto& service = f.service();
    auto insert = service.insert_command();
    insert.value("id", std::uint64_t(1)).value("name", "n");
    auto prepared = service.prepare_insert_command(insert);
    check(bool(prepared), "original tenant insert preparation succeeds");
    field_is(prepared.value(), "version", Value(f.owner, std::int64_t(9)));
}

template <class T>
static void
assert_query_policy_probe(AsyncOperation<Result<T, DataServiceError<StubError>>>& work) {
    const auto& result = work.wait();
    check(!result && result.error().kind() == DataServiceError<StubError>::Kind::Runtime &&
              result.error().runtime_error().kind() == RuntimeError::Kind::Policy &&
              result.error().runtime_error().message() == "QUERY_POLICY_PROBE",
          "original public query rejects with exact typed policy probe");
}

static void public_query_families_cannot_bypass_request_policy() {
    Fixture f(0, true);
    auto& policy = f.user->make_resource<DenySelectRequestPolicy>();
    f.user->with_request_policy(*f.user, policy);
    auto& service = f.service();
    PurposedSelectQuery query(
        std::allocator_arg, Allocator(f.owner), service.select(), "verify query policy coverage");
    SelectQuery child(std::allocator_arg, Allocator(f.owner), "OrderLine");
    RelationAggregate relation(
        std::allocator_arg, Allocator(f.owner), "lines", "lineCount", child, true);
    auto aggregates = std::span<const RelationAggregate>(&relation, 1);
    assert_query_policy_probe(service.fetch_all(query));
    assert_query_policy_probe(service.fetch_all_owned(detail::clone_in(f.owner, query)));
    assert_query_policy_probe(service.fetch_stream(query));
    assert_query_policy_probe(service.fetch_smart_list(query));
    assert_query_policy_probe(service.fetch_smart_list_with_relation_aggregates(query, aggregates));
    assert_query_policy_probe(service.fetch_entities<Order>(query));
    assert_query_policy_probe(service.fetch_enhanced_entities<Order>(query));
    assert_query_policy_probe(
        service.fetch_enhanced_entities_with_relation_aggregates<Order>(query, aggregates));
    assert_query_policy_probe(service.fetch_enhanced_entities_with_relation_aggregates_owned<Order>(
        detail::clone_in(f.owner, query), aggregates));
    auto aggregate = service.select();
    aggregate.aggregate(Aggregate::count(f.owner, "orderCount"));
    PurposedSelectQuery count(std::allocator_arg,
                              Allocator(f.owner),
                              aggregate,
                              "verify aggregate query policy coverage");
    assert_query_policy_probe(service.fetch_all(count));
    check(f.executor.queries == 0 && f.executor.streams == 0 && f.executor.mutations == 0,
          "none of the original ten rejected query calls reaches a provider");
}

static void entity_data_service_prepares_insert_command_with_generated_id() {
    Fixture f(1, true);
    f.behavior<OrderBehavior>();
    f.generator(42);
    auto& service = f.service();
    auto insert = service.insert_command();
    insert.value("id", std::uint64_t(0)).value("name", "n");
    auto prepared = service.prepare_insert_command(insert);
    check(bool(prepared), "original ID generation preparation succeeds");
    field_is(prepared.value(), "id", Value(f.owner, std::uint64_t(42)));
    field_is(prepared.value(), "version", Value(f.owner, std::int64_t(1)));
    field_is(prepared.value(), "name", Value(f.owner, "n"));
    auto zero = service.insert_command();
    zero.value("id", std::uint64_t(0))
        .value("version", std::int64_t(0))
        .value("name", "zero-version");
    auto prepared_zero = service.prepare_insert_command(zero);
    check(bool(prepared_zero), "original zero-version preparation succeeds");
    field_is(prepared_zero.value(), "version", Value(f.owner, std::int64_t(1)));
}

static void custom_user_context_can_drive_insert_preparation() {
    Fixture f;
    f.behavior<ContextAwareOrderBehavior>();
    f.generator(99);
    auto& tenant = f.user->make<String>("acme");
    auto& version = f.user->make<std::int64_t>(7);
    f.user->insert_named_resource("tenant", *f.user, tenant);
    f.user->insert_named_resource("initial_version", *f.user, version);
    f.user->put_local("trace_id", "req-9");
    auto& service = f.service();
    auto prepared = service.prepare_insert_command(service.insert_command());
    check(bool(prepared), "original Context-aware preparation succeeds");
    field_is(prepared.value(), "id", Value(f.owner, std::uint64_t(99)));
    field_is(prepared.value(), "version", Value(f.owner, std::int64_t(7)));
    field_is(prepared.value(), "name", Value(f.owner, "acme:req-9"));
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
        data_service_returns_optimistic_lock_conflict();
        user_context_builds_context_data_service();
        user_context_resolves_entity_data_service_by_entity_type();
        entity_data_service_applies_behavior_hooks();
        entity_data_service_applies_request_policy_after_behavior_hooks();
        public_query_families_cannot_bypass_request_policy();
        entity_data_service_prepares_insert_command_with_generated_id();
        custom_user_context_can_drive_insert_preparation();
        std::cout << "Eight original runtime-root service-boundary contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
