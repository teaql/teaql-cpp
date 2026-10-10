// Direct pinned root mutation-event, ledger-order and relation-load contracts.
#include "root_fixture.hpp"
#include <teaql/runtime/detail/relation_hydration.hpp>

using namespace original_runtime_root;

static const Value* field(const Record& values, std::string_view name) {
    auto found = values.find(name);
    return found == values.end() ? nullptr : &found->second;
}

static const Value* field(const std::optional<Record>& values, std::string_view name) {
    return values ? field(*values, name) : nullptr;
}

static void field_is(const Record& values, std::string_view name, const Value& expected) {
    auto* actual = field(values, name);
    check(actual && *actual == expected,
          "original event field is present with its exact Value variant");
}

struct RecordingEventSink final : RawAuditEventSink {
    Vector<RawAuditEvent>& events;

    explicit RecordingEventSink(Vector<RawAuditEvent>& events) : events(events) {}

    RuntimeHookResult
    on_event(Memory owner, const UserContext&, const RawAuditEvent& event) const override {
        events.emplace_back(event);
        return RuntimeHookResult::ok(owner, {});
    }
};

struct RecordingSafeEventSink final : SafeAuditEventSink {
    Vector<SafeAuditEvent>& events;

    explicit RecordingSafeEventSink(Vector<SafeAuditEvent>& events) : events(events) {}

    RuntimeHookResult
    on_safe_event(Memory owner, const UserContext&, const SafeAuditEvent& event) const override {
        events.emplace_back(event);
        return RuntimeHookResult::ok(owner, {});
    }
};

static void user_context_event_sink_receives_data_service_mutation_events() {
    Fixture f;
    auto descriptor = entity(f.owner);
    descriptor.mask_fields({"name"});
    f.store.with_entity(std::move(descriptor));
    f.generator(88);
    Record row(f.owner);
    row.emplace("id", Value(f.owner, std::uint64_t(88)));
    row.emplace("version", Value(f.owner, std::int64_t(1)));
    row.emplace("name", Value(f.owner, "old"));
    f.executor.rows.emplace_back(row);
    auto& events = f.user->make<Vector<RawAuditEvent>>();
    auto& safe_events = f.user->make<Vector<SafeAuditEvent>>();
    auto& sink = f.user->make_resource<RecordingEventSink>(events);
    auto& safe_sink = f.user->make_resource<RecordingSafeEventSink>(safe_events);
    f.user->with_event_sink(*f.user, sink).with_custom_event_sink(*f.user, safe_sink);
    auto& service = f.service();
    auto created = service.insert_command();
    created.value("name", "created");
    check(bool(Access::insert_internal(service, created).wait()),
          "original internal insert succeeds");
    auto updated = service.update_command(std::uint64_t(88));
    updated.expected_version(1).value("name", "updated");
    check(bool(Access::update_internal(service, updated).wait()),
          "original internal update succeeds");
    auto deleted = service.delete_command(std::uint64_t(88));
    deleted.expected_version(2);
    check(bool(Access::delete_internal(service, deleted).wait()),
          "original internal delete succeeds");
    auto recovered = service.recover_command(std::uint64_t(88), -3);
    check(bool(Access::recover_internal(service, recovered).wait()),
          "original internal recover succeeds");
    check(events.size() == 4, "original raw event count");
    check(events[0].kind == RawAuditEventKind::Created && events[0].entity == "Order",
          "original created kind/entity");
    field_is(events[0].values, "id", Value(f.owner, std::uint64_t(88)));
    check(events[1].kind == RawAuditEventKind::Updated, "original updated kind");
    field_is(events[1].values, "id", Value(f.owner, std::uint64_t(88)));
    field_is(events[1].values, "version", Value(f.owner, std::int64_t(2)));
    const std::array<std::string_view, 1> names{"name"};
    check(std::ranges::equal(events[1].updated_fields, names), "original exact updated-field list");
    check(!field(events[1].old_values, "name"),
          "blind update does not dynamically fetch old values");
    auto* new_name = field(events[1].new_values, "name");
    check(new_name && *new_name == Value(f.owner, "updated"), "original exact new updated name");
    check(events[1].changes.size() == 1 && events[1].changes[0].field == "name" &&
              !events[1].changes[0].old_value && events[1].changes[0].new_value &&
              *events[1].changes[0].new_value == Value(f.owner, "updated"),
          "original blind-update change payload and optional presence");
    check(events[2].kind == RawAuditEventKind::Deleted && !events[2].old_values &&
              !events[2].new_values,
          "original deleted event has no fetched old/new snapshot");
    check(events[3].kind == RawAuditEventKind::Recovered && !field(events[3].old_values, "version"),
          "original recovered kind and absent fetched old version");
    auto* version = field(events[3].new_values, "version");
    check(version && *version == Value(f.owner, std::int64_t(4)) && !events[3].changes.empty() &&
              events[3].changes[0].field == "version",
          "original recovery version/change field");
    check(safe_events.size() == 4 && safe_events[0].kind == RawAuditEventKind::Created,
          "original custom safe event count and created kind");
    auto name =
        std::ranges::find(safe_events[0].fields, std::string_view("name"), &SafeAuditField::name);
    check(name != safe_events[0].fields.end() && name->masked &&
              (!name->value || *name->value != "created"),
          "original changed-name field is present and masked in the application projection");
}

static EntityKey key(Memory owner, std::string_view entity, std::uint64_t id) {
    return EntityKey(std::allocator_arg, Allocator(owner), entity, Value(owner, id));
}

static EntityValues
values(Memory owner, std::initializer_list<std::pair<std::string_view, std::uint64_t>> fields) {
    EntityValues result(std::allocator_arg, Allocator(owner));
    for (const auto& [name, value] : fields) {
        result.fields.emplace(String(name, owner), Value(owner, value));
    }
    return result;
}

static void ledger_insert_order_places_foreign_key_parent_before_child() {
    Fixture f(1, true);
    auto& service =
        EntityDataService<StubExecutor>::for_executor(*f.user, "OrderLine", *f.user, f.executor);
    auto line = key(f.owner, "OrderLine", 10);
    auto product = key(f.owner, "Product", 88);
    detail::LedgerChanges changes(f.owner);
    changes.emplace(detail::clone_in(f.owner, line),
                    values(f.owner, {{"id", 10}, {"product_id", 88}}));
    changes.emplace(detail::clone_in(f.owner, product), values(f.owner, {{"id", 88}}));
    Vector<EntityKey> input(f.owner);
    input.emplace_back(line);
    input.emplace_back(product);
    auto ordered = Access::order_new_ledger_keys(service, input, changes);
    check(ordered && ordered.value().size() == 2 && ordered.value()[0] == product &&
              ordered.value()[1] == line,
          "original foreign parent precedes the child in the complete ordered vector");
}

static void ledger_insert_order_honors_reverse_many_and_forward_parents() {
    Fixture f(1, true);
    auto& service =
        EntityDataService<StubExecutor>::for_executor(*f.user, "OrderLine", *f.user, f.executor);
    auto order = key(f.owner, "Order", 77);
    auto line = key(f.owner, "OrderLine", 10);
    auto product = key(f.owner, "Product", 88);
    detail::LedgerChanges changes(f.owner);
    changes.emplace(detail::clone_in(f.owner, order), values(f.owner, {{"id", 77}}));
    changes.emplace(detail::clone_in(f.owner, product), values(f.owner, {{"id", 88}}));
    changes.emplace(detail::clone_in(f.owner, line),
                    values(f.owner, {{"id", 10}, {"order_id", 77}, {"product_id", 88}}));
    Vector<EntityKey> input(f.owner);
    input.emplace_back(line);
    input.emplace_back(product);
    input.emplace_back(order);
    auto ordered = Access::order_new_ledger_keys(service, input, changes);
    check(ordered && ordered.value().size() == 3 && ordered.value()[0] == order &&
              ordered.value()[1] == product && ordered.value()[2] == line,
          "original reverse-many and forward parents retain complete deterministic order");
}

static void ledger_insert_order_reports_required_foreign_key_cycle_before_sql() {
    Fixture f;
    for (auto name : {"Alpha", "Beta"}) {
        const bool alpha = name == std::string_view("Alpha");
        EntityDescriptor descriptor(std::allocator_arg, Allocator(f.owner), name);
        descriptor.property("id", DataType::U64).id();
        descriptor.property(alpha ? "beta_id" : "alpha_id", DataType::U64).not_null();
        descriptor.relation(alpha ? "beta" : "alpha", alpha ? "Beta" : "Alpha")
            .local(alpha ? "beta_id" : "alpha_id")
            .foreign("id");
        f.store.with_entity(std::move(descriptor));
    }
    auto& service =
        EntityDataService<StubExecutor>::for_executor(*f.user, "Alpha", *f.user, f.executor);
    auto alpha = key(f.owner, "Alpha", 1);
    auto beta = key(f.owner, "Beta", 2);
    detail::LedgerChanges changes(f.owner);
    changes.emplace(detail::clone_in(f.owner, alpha), values(f.owner, {{"id", 1}, {"beta_id", 2}}));
    changes.emplace(detail::clone_in(f.owner, beta), values(f.owner, {{"id", 2}, {"alpha_id", 1}}));
    Vector<EntityKey> input(f.owner);
    input.emplace_back(alpha);
    input.emplace_back(beta);
    auto error = Access::order_new_ledger_keys(service, input, changes);
    check(!error && error.error().kind() == RuntimeError::Kind::Graph &&
              error.error().message().find("Alpha") != std::string_view::npos &&
              error.error().message().find("Beta") != std::string_view::npos,
          "original required cycle retains both entity names in a Graph error");
    check(f.executor.queries == 0 && f.executor.mutations == 0, "ordering rejection precedes SQL");
}

static void entity_data_service_builds_relation_plans() {
    Fixture f(1, true);
    f.behavior<OrderBehavior>();
    auto plans = f.service().relation_plans();
    check(plans && plans.value().size() == 1, "original relation-plan count");
    const auto& plan = plans.value()[0];
    check(plan.relation_name == "lines" && plan.target_entity == "OrderLine" &&
              plan.local_key == "id" && plan.foreign_key == "order_id" && plan.many,
          "complete original behavior relation plan fields");
}

static Vector<CompactRow> parents(Memory owner, std::initializer_list<std::uint64_t> ids) {
    Vector<CompactRow> result(owner);
    for (auto id : ids) {
        Record row(owner);
        row.emplace("id", Value(owner, id));
        result.emplace_back(row);
    }
    return result;
}

static void entity_data_service_builds_relation_query_from_parent_rows() {
    Fixture f(1, true);
    f.behavior<OrderBehavior>();
    auto rows = parents(f.owner, {11, 12, 11});
    auto query = f.service().relation_query("lines", rows);
    check(query && query.value().predicate && query.value().predicate->kind == Expr::Kind::Binary &&
              query.value().predicate->args.size() == 2 &&
              query.value().predicate->args[1].kind == Expr::Kind::Value,
          "original relation query contains a binary literal predicate");
    auto* ids = std::get_if<Value::List>(&query.value().predicate->args[1].literal.data);
    check(ids && ids->size() == 2 && (*ids)[0] == Value(f.owner, std::uint64_t(11)) &&
              (*ids)[1] == Value(f.owner, std::uint64_t(12)),
          "complete original deduplicated relation identities preserve U64 and order");
}

template <class E>
static auto& enhance(const EntityDataService<E>& service,
                     Vector<CompactRow>& rows,
                     std::span<const RelationLoadPlan> plans) {
    detail::EntityRelationFetch<E> fetch(
        std::allocator_arg,
        Allocator(service.memory_resource()),
        [](const EntityDataService<E>& child, SelectQuery query) -> auto& {
            return Access::fetch_all(child, query);
        });
    return detail::enhance_relation_plans(service, rows, plans, fetch);
}

static const Value::List& relation(const CompactRow& row) {
    auto* value = row.get("lines");
    check(value && std::holds_alternative<Value::List>(value->data),
          "original relation payload is a List");
    return std::get<Value::List>(value->data);
}

static void entity_data_service_enhances_parent_rows_with_relations() {
    Fixture f(1, true);
    f.behavior<OrderBehavior>();
    for (const auto& [id, order, name] :
         {std::tuple{101u, 11u, "l1"}, {102u, 11u, "l2"}, {201u, 12u, "l3"}}) {
        Record row(f.owner);
        row.emplace("id", Value(f.owner, std::uint64_t(id)));
        row.emplace("order_id", Value(f.owner, std::uint64_t(order)));
        row.emplace("name", Value(f.owner, name));
        f.executor.rows.emplace_back(row);
    }
    auto& events = f.user->make<Vector<String>>();
    auto& telemetry = f.user->make<RecordingTelemetry>(events);
    f.user->with_runtime_telemetry(*f.user, telemetry);
    auto rows = parents(f.owner, {11, 12, 13});
    auto& service = f.service();
    auto plans = service.relation_plans();
    check(bool(plans), "original relation plans resolve");
    check(bool(enhance(service, rows, plans.value()).wait()), "actual relation hydration succeeds");
    check(relation(rows[0]).size() == 2 && relation(rows[1]).size() == 1,
          "original two parent relation cardinalities");
    check(std::ranges::find(events, std::string_view("start:relation_load")) != events.end(),
          "original relation-load telemetry start is present");
}

struct CapturingQueryExecutor final : QueryExecutor<StubError>, MutationExecutor<StubError> {
    using allocator_type = Allocator;
    Memory owner;
    Vector<Record> rows;
    Vector<SelectQuery> queries;

    CapturingQueryExecutor(std::allocator_arg_t, Allocator a)
        : owner(a.resource()), rows(owner), queries(owner) {}

    Memory memory_resource() const noexcept {
        return owner;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, StubError>& query(Memory target,
                                                              QueryRequest request) override {
        queries.emplace_back(request.query);
        const auto now = std::chrono::system_clock::now();
        ExecutionMetadata metadata(
            std::allocator_arg, Allocator(target), DataServiceOperation::Query, now, now);
        metadata.backend = "capture";
        metadata.result_count = rows.size();
        QueryResult result(std::allocator_arg, Allocator(target), std::move(metadata));
        for (const auto& row : rows) {
            result.rows.emplace_back(row);
        }
        return make_operation<Result<QueryResult, StubError>, Ready<QueryResult>>(
            target, std::move(result));
    }

    DataServiceOperationResult<MutationResult, StubError>& mutate(Memory,
                                                                  MutationRequest) override {
        throw std::logic_error("original relation query fixture does not mutate");
    }
};

static void topn_006_008_009_relation_is_stable_empty_safe_and_count_free() {
    Fixture f(0, true);
    auto& executor = f.user->make<CapturingQueryExecutor>();
    for (const auto& [order, first] : {std::pair{11u, 101u}, {12u, 201u}}) {
        for (std::uint64_t rank = 1; rank <= 3; ++rank) {
            Record row(f.owner);
            row.emplace("id", Value(f.owner, std::uint64_t(first) + rank - 1));
            row.emplace("order_id", Value(f.owner, std::uint64_t(order)));
            row.emplace(String(PARTITION_RANK_PROPERTY, f.owner), Value(f.owner, rank));
            executor.rows.emplace_back(row);
        }
    }
    f.user->insert_resource(*f.user, executor);
    auto bound = f.user->entity_data_service<CapturingQueryExecutor>("Order");
    check(bool(bound), "original capture executor is bound through Context");
    auto& service = bound.value().get();
    auto rows = parents(f.owner, {11, 12});
    SelectQuery query(std::allocator_arg, Allocator(f.owner), "Order");
    SelectQuery child(std::allocator_arg, Allocator(f.owner), "OrderLine");
    child.order_desc("name").limit(3);
    query.relation_query("lines", child);
    auto plans = detail::build_relation_plans_from_loads(*f.user, query.entity, query.relations);
    check(bool(plans), "original query relation plans resolve");
    check(bool(enhance(service, rows, plans.value()).wait()), "actual TOPN hydration succeeds");
    check(executor.queries.size() == 1 && executor.queries[0].aggregates.empty(),
          "original TOPN-009 issues exactly one non-count query");
    const auto& captured = executor.queries[0];
    check(captured.partition_field && *captured.partition_field == "order_id" && captured.slice &&
              captured.slice->limit == 3 && captured.order_by.size() == 2 &&
              captured.order_by[0] == OrderBy::desc(f.owner, "name") &&
              captured.order_by[1] == OrderBy::asc(f.owner, "id"),
          "all original partition, limit and stable ordering assertions");
    for (const auto& parent : rows) {
        const auto& lines = relation(parent);
        check(lines.size() == 3, "original two parents each retain three children");
        for (const auto& line : lines) {
            auto* object = std::get_if<Value::Object>(&line.data);
            check(object && !object->contains(PARTITION_RANK_PROPERTY),
                  "internal partition rank is absent from every child object");
        }
    }
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
        user_context_event_sink_receives_data_service_mutation_events();
        ledger_insert_order_places_foreign_key_parent_before_child();
        ledger_insert_order_honors_reverse_many_and_forward_parents();
        ledger_insert_order_reports_required_foreign_key_cycle_before_sql();
        entity_data_service_builds_relation_plans();
        entity_data_service_builds_relation_query_from_parent_rows();
        entity_data_service_enhances_parent_rows_with_relations();
        topn_006_008_009_relation_is_stable_empty_safe_and_count_free();
        std::cout << "Eight original runtime-root event/order/relation contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
