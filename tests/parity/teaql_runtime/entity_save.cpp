// Direct pinned save preparation, readback and governance source contracts.
#include <teaql/runtime/transaction.hpp>
#include "../../../examples/models.hpp"
#include <algorithm>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class T>
struct Ready {
    using allocator_type = Allocator;
    Result<T, String> result;

    Ready(std::allocator_arg_t, Allocator a, T value)
        : result(Result<T, String>::ok(a.resource(), std::move(value))) {}

    Ready(std::allocator_arg_t, Allocator a, Result<T, String> value)
        : result(std::allocator_arg, a, std::move(value)) {}

    std::optional<Result<T, String>> poll(WakeSignal&) {
        return detail::clone_in(result.memory_resource(), result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct State {
    using allocator_type = Allocator;
    Record row;
    Vector<String> calls;
    bool fail_readback = false;
    bool fail_commit = false;

    State(std::allocator_arg_t, Allocator a) : row(a.resource()), calls(a.resource()) {}

    Memory memory_resource() const {
        return row.get_allocator().resource();
    }

    bool called(std::string_view name) const {
        return std::ranges::any_of(calls, [&](const auto& call) {
            return call == name;
        });
    }

    void reset_row() {
        auto owner = memory_resource();
        row.insert_or_assign(String("id", owner), Value(owner, std::int64_t(1)));
        row.insert_or_assign(String("version", owner), Value(owner, std::int64_t(1)));
        row.insert_or_assign(String("name", owner), Value(owner, "before"));
    }
};

static DataServiceCapabilities capabilities() {
    DataServiceCapabilities result;
    result.query = true;
    result.mutation = true;
    result.transaction = true;
    return result;
}

static AsyncOperation<Result<QueryResult, String>>& read(Memory owner, State& state, bool ambient) {
    state.calls.emplace_back(ambient ? "ambient-query" : "transaction-query");
    bool empty = !ambient && state.fail_readback && state.called("transaction-mutate");
    QueryResult result(std::allocator_arg,
                       Allocator(owner),
                       ExecutionMetadata::unrecorded_query(owner, empty ? 0 : 1));
    if (!empty) {
        result.rows.emplace_back(state.row);
    }
    return make_operation<Result<QueryResult, String>, Ready<QueryResult>>(owner,
                                                                           std::move(result));
}

// Controlled provider matches the original fixture. Saver, ledger, governance,
// authoritative-read and transaction orchestration remain production runtime code.
struct Tx final : DataServiceTransaction<String> {
    using allocator_type = Allocator;
    State& state;

    Tx(std::allocator_arg_t, Allocator a, State& state)
        : DataServiceTransaction<String>(a.resource()), state(state) {}

    DataServiceCapabilities capabilities() const override {
        return ::capabilities();
    }

protected:
    AsyncOperation<Result<QueryResult, String>>& query_transaction(Memory owner,
                                                                   QueryRequest) override {
        return read(owner, state, false);
    }

    AsyncOperation<Result<MutationResult, String>>&
    mutate_transaction(Memory owner, MutationRequest request) override {
        state.calls.emplace_back("transaction-mutate");
        if (auto* update = std::get_if<UpdateCommand>(&request.data)) {
            check(update->expected_version_value == 1,
                  "original transactional update requires version1");
            for (const auto& [field, value] : update->values.fields) {
                state.row.insert_or_assign(String(field, state.memory_resource()),
                                           Value(state.memory_resource(), value));
            }
        } else if (auto* deletion = std::get_if<DeleteCommand>(&request.data)) {
            check(deletion->expected_version_value == 1,
                  "original transactional deletion requires version1");
            state.row.insert_or_assign(String("version", state.memory_resource()),
                                       Value(state.memory_resource(), std::int64_t(-2)));
        } else {
            throw std::logic_error("unexpected original transactional mutation");
        }
        MutationResult result(
            std::allocator_arg, Allocator(owner), 1, ExecutionMetadata::unrecorded_query(owner, 0));
        return make_operation<Result<MutationResult, String>, Ready<MutationResult>>(
            owner, std::move(result));
    }

    AsyncOperation<Result<std::monostate, String>>& commit_transaction(Memory owner) override {
        if (state.fail_commit) {
            state.calls.emplace_back("commit-failed");
            return make_operation<Result<std::monostate, String>, Ready<std::monostate>>(
                owner,
                Result<std::monostate, String>::err(owner,
                                                    String("simulated commit failure", owner)));
        }
        state.calls.emplace_back("commit");
        state.row.insert_or_assign(String("version", state.memory_resource()),
                                   Value(state.memory_resource(), std::int64_t(3)));
        state.row.insert_or_assign(String("name", state.memory_resource()),
                                   Value(state.memory_resource(), "other writer"));
        return make_operation<Result<std::monostate, String>, Ready<std::monostate>>(
            owner, std::monostate{});
    }

    AsyncOperation<Result<std::monostate, String>>& rollback_transaction(Memory owner) override {
        state.calls.emplace_back("rollback");
        state.row.insert_or_assign(String("version", state.memory_resource()),
                                   Value(state.memory_resource(), std::int64_t(1)));
        state.row.insert_or_assign(String("name", state.memory_resource()),
                                   Value(state.memory_resource(), "before"));
        return make_operation<Result<std::monostate, String>, Ready<std::monostate>>(
            owner, std::monostate{});
    }

    void cancel_transaction() noexcept override {}
};

struct Ambient final : QueryExecutor<String>,
                       MutationExecutor<String>,
                       TransactionExecutor<String> {
    State& state;

    explicit Ambient(State& state) : state(state) {}

    DataServiceCapabilities capabilities() const override {
        return ::capabilities();
    }

    AsyncOperation<Result<QueryResult, String>>& query(Memory owner, QueryRequest) override {
        return read(owner, state, true);
    }

    AsyncOperation<Result<MutationResult, String>>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("original ledger writes must use the transaction executor");
    }

    AsyncOperation<Result<TransactionHandle<String>, String>>& begin(Memory owner) override {
        state.calls.emplace_back("begin");
        auto& tx = detail::async_arena(owner).make_resource<Tx>(state);
        auto handle = TransactionHandle<String>(std::allocator_arg, Allocator(owner), tx);
        return make_operation<Result<TransactionHandle<String>, String>,
                              Ready<TransactionHandle<String>>>(owner, std::move(handle));
    }
};

struct CaptureEvents final : RawAuditEventSink {
    using allocator_type = Allocator;
    Vector<RawAuditEvent> events;
    bool fail = false;

    CaptureEvents(std::allocator_arg_t, Allocator a) : events(a.resource()) {}

    Result<std::monostate, RuntimeError>
    on_event(Memory owner, const UserContext&, const RawAuditEvent& event) const override {
        const_cast<CaptureEvents*>(this)->events.emplace_back(event);
        if (fail) {
            return Result<std::monostate, RuntimeError>::err(
                owner, RuntimeError::event(owner, "simulated sink failure"));
        }
        return Result<std::monostate, RuntimeError>::ok(owner, {});
    }
};

struct DenyTaskPolicy final : MutationPolicy {
    MutationPolicyIdentity identity(Memory owner) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(owner), "task-policy", "1", "sha256:deny-task");
    }

    MutationDecision
    review(Memory owner, const UserContext&, const MutationPlan& plan) const override {
        check(plan.request_key == "Task.saveGraph" && plan.operations.size() == 1,
              "original policy request and operation count");
        Vector<String> fields(owner);
        fields.emplace_back("Task.name");
        return MutationDecision::deny(owner, "TASK-DENIED", "task mutation denied", fields);
    }
};

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory owner = user->memory_resource();
    State& state = user->make<State>();
    CaptureEvents& capture = user->make_resource<CaptureEvents>();
    EntityRuntimeState& root = user->make<EntityRuntimeState>();
    EntityKey key{std::allocator_arg, Allocator(owner), "Task", Value(owner, std::int64_t(1))};
    GraphNode node{std::allocator_arg, Allocator(owner), "Task"};
    detail::GraphSaverFor<Ambient>& saver = user->make<detail::GraphSaverFor<Ambient>>();

    explicit Fixture(bool events = false) {
        root.set_comment("Inspect native ledger save");
        user->disable_sql_log();
        auto& metadata = user->make<InMemoryMetadataStore>();
        EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "Task");
        descriptor.property("id", DataType::I64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        metadata.register_entity(std::move(descriptor));
        user->set_metadata(*user, metadata);
        auto& ambient = user->make_resource<Ambient>(state);
        user->insert_resource(*user, ambient);
        if (events) {
            user->set_event_sink(*user, capture);
        }
        state.reset_row();
        root.set_original_version(key, 1);
        node.value("id", std::int64_t(1))
            .value("version", std::int64_t(1))
            .value("name", "before")
            .with_comment("Inspect native ledger save");
    }
};

static const Value* field(const EntityValues& values, std::string_view name) {
    auto found = values.fields.find(name);
    return found == values.fields.end() ? nullptr : &found->second;
}

static void ledger_save_returns_transaction_snapshot_before_concurrent_commit_race() {
    Fixture f(true);
    f.root.set(f.key, "name", Value(f.owner, "updated"));
    const auto& result = f.saver.save_ledger_dyn(*f.user, std::move(f.node), f.root).wait();
    check(bool(result), "original ledger save succeeds");
    check(field(result.value().values, "version") &&
              *field(result.value().values, "version") == Value(f.owner, std::int64_t(2)) &&
              field(result.value().values, "name") &&
              *field(result.value().values, "name") == Value(f.owner, "updated"),
          "original returned snapshot is authoritative before concurrent commit visibility");
    auto version = f.state.row.find(std::string_view("version"));
    check(version != f.state.row.end() && version->second == Value(f.owner, std::int64_t(3)) &&
              f.state.calls.back() == "commit" && !f.state.called("ambient-query"),
          "original state sees later version3 but no post-commit ambient query");
    check(!f.capture.events.empty() && f.capture.events[0].mutation_governance,
          "original audit event retains governance");
    const auto& governance = *f.capture.events[0].mutation_governance;
    check(governance.request_key == "Task.saveGraph" &&
              governance.source == MutationPolicySource::GeneratedDefault &&
              governance.operations.size() == 1,
          "all original governance assertions");
}

static void failed_authoritative_readback_rolls_back_before_reporting_failure() {
    Fixture f;
    f.state.fail_readback = true;
    f.root.set(f.key, "name", Value(f.owner, "updated"));
    const auto& result = f.saver.save_ledger_dyn(*f.user, std::move(f.node), f.root).wait();
    check(!result &&
              result.error().to_string(f.owner).find("could not be read back") != String::npos,
          "original missing authoritative row is a save failure");
    check(f.state.calls.back() == "rollback" && !f.state.called("commit") &&
              f.state.row.find(std::string_view("version"))->second ==
                  Value(f.owner, std::int64_t(1)) &&
              f.root.get_original_version(f.key) == 1,
          "all original rollback and retryable version assertions");
}

static void soft_delete_returns_authoritative_tombstone_before_commit() {
    Fixture f;
    f.root.mark_as_delete(f.key);
    const auto& result = f.saver.save_ledger_dyn(*f.user, std::move(f.node), f.root).wait();
    check(result && field(result.value().values, "version") &&
              *field(result.value().values, "version") == Value(f.owner, std::int64_t(-2)) &&
              f.state.calls.back() == "commit" && !f.state.called("ambient-query"),
          "original tombstone version is read before commit without ambient reread");
}

static void cancelled_new_root_cannot_return_a_fictitious_persisted_entity() {
    Fixture f;
    f.state.row.clear();
    f.key =
        EntityKey(std::allocator_arg, Allocator(f.owner), "Task", Value(f.owner, std::int64_t(7)));
    f.root.clear_committed();
    f.root.mark_as_new(f.key);
    f.root.mark_as_delete(f.key);
    f.node = GraphNode(std::allocator_arg, Allocator(f.owner), "Task");
    f.node.value("id", std::int64_t(7))
        .value("version", std::int64_t(0))
        .value("name", "cancelled")
        .with_comment("Inspect cancelled native root");
    const auto& result = f.saver.save_ledger_dyn(*f.user, std::move(f.node), f.root).wait();
    check(!result && result.error().to_string(f.owner).find("cancelled new root") != String::npos &&
              !f.state.called("transaction-mutate") && !f.state.called("commit"),
          "all original cancelled-root failure and non-persistence assertions");
}

static void mutation_policy_denial_happens_before_transaction_begin() {
    Fixture f;
    auto policy = f.user->make_resource_ref<DenyTaskPolicy>();
    auto registry = f.user->make_ref<FunctionMutationPolicyRegistry>(
        [&f, policy](Memory owner,
                     std::string_view request) -> std::optional<MutationPolicyBorrow> {
            if (request != "Task.saveGraph") {
                return {};
            }
            return MutationPolicyBorrow(std::allocator_arg,
                                        Allocator(owner),
                                        f.user->get(policy),
                                        detail::resource_arena(f.owner),
                                        detail::reference_slot(policy.id()));
        });
    f.user->set_mutation_policy_registry(registry);
    f.root.set(f.key, "name", Value(f.owner, "denied"));
    const auto& result = f.saver.save_ledger_dyn(*f.user, std::move(f.node), f.root).wait();
    check(!result && result.error().to_string(f.owner).find("TASK-DENIED") != String::npos &&
              f.state.calls.empty(),
          "original denial code is returned before every provider/transaction call");
}

static void scalar_change_keeps_snapshot_but_relation_changes_invalidate_it() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(owner), "Order");
    descriptor.relation("customer", "Customer").local("customer_id");
    EntityKey order(std::allocator_arg, Allocator(owner), "Order", Value(owner, std::int64_t(1)));
    EntityKey child(
        std::allocator_arg, Allocator(owner), "OrderLine", Value(owner, std::int64_t(2)));
    auto& root = user->make<EntityRuntimeState>();
    root.set_comment("Inspect native ledger validation");
    root.set(order, "total_amount", Value(owner, std::int64_t(100)));
    check(detail::can_preserve_loaded_relations(root, order, descriptor),
          "original scalar-only snapshot can be reused");
    root.set(child, "sku", Value(owner, "CHANGED"));
    check(!detail::can_preserve_loaded_relations(root, order, descriptor),
          "original child mutation invalidates snapshot reuse");
    auto& fk = user->make<EntityRuntimeState>();
    fk.set(order, "customer_id", Value(owner, std::int64_t(3)));
    check(!detail::can_preserve_loaded_relations(fk, order, descriptor),
          "original local FK mutation invalidates snapshot reuse");
}

static void nested_ledger_entity_keeps_model_and_json_error_paths() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    GraphNode order(std::allocator_arg, Allocator(owner), "Order");
    order.value("id", std::uint64_t(7));
    GraphNode line(std::allocator_arg, Allocator(owner), "OrderLine");
    line.value("id", std::uint64_t(9));
    order.relation("line_items", line);
    auto locations = detail::ledger_object_locations(owner, order);
    EntityKey parent(std::allocator_arg, Allocator(owner), "Order", Value(owner, std::uint64_t(7)));
    EntityKey child(
        std::allocator_arg, Allocator(owner), "OrderLine", Value(owner, std::uint64_t(9)));
    auto root = locations.find(parent);
    auto nested = locations.find(child);
    check(root != locations.end() && root->second.is_root() && nested != locations.end() &&
              nested->second.model_path(owner) == "line_items[0]" &&
              nested->second.instance_path(owner) == "/lineItems/0",
          "all original nested model and JSON path assertions");
}

struct DirtyTrackedOrder : example::Model<DirtyTrackedOrder> {
    static constexpr std::string_view ENTITY_NAME = "DirtyTrackedOrder";
    std::int64_t revision = 0;
    String code;

    DirtyTrackedOrder(std::allocator_arg_t, Allocator a)
        : Model(std::allocator_arg, a), code(a.resource()) {}

    DirtyTrackedOrder(std::allocator_arg_t, Allocator a, const DirtyTrackedOrder& other)
        : Model(std::allocator_arg, a, other), revision(other.revision),
          code(other.code, a.resource()) {}

    DirtyTrackedOrder(const DirtyTrackedOrder&) = delete;
    DirtyTrackedOrder(DirtyTrackedOrder&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("dirty_tracked_order");
        result.property("id", DataType::U64).id().not_null();
        result.property("version", DataType::I64).version().not_null();
        result.property("code", DataType::Text).not_null();
        return result;
    }

    static Result<DirtyTrackedOrder, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<DirtyTrackedOrder, EntityError>;
        DirtyTrackedOrder result(std::allocator_arg, Allocator(owner));
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto version = example::decode_field<std::int64_t>(owner, row, ENTITY_NAME, "version");
        auto code = example::decode_field<String>(owner, row, ENTITY_NAME, "code");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!version) {
            return Output::err(owner, std::move(version).error());
        }
        if (!code) {
            return Output::err(owner, std::move(code).error());
        }
        result.id = id.value();
        result.revision = version.value();
        result.code = std::move(code).value();
        result.root.set_original_compact_row(ENTITY_NAME, row);
        return Output::ok(owner, std::move(result));
    }

    MutationValues into_values() && {
        auto owner = memory_resource();
        MutationValues result(owner);
        result.fields.emplace("id", Value(owner, id));
        result.fields.emplace("version", Value(owner, revision));
        auto value = root.get(entity_key(), "code");
        result.fields.emplace("code", value ? std::move(*value) : Value(owner, code));
        return result;
    }
};

struct DirtyFieldProbe final : TypedChecker<DirtyTrackedOrder> {
    bool observed = false;

    void check_and_fix_typed(const UserContext&,
                             DirtyTrackedOrder& entity,
                             CheckObjectStatus status,
                             const ObjectLocation&,
                             CheckResults&) const override {
        auto fields = entity.dirty_fields();
        const_cast<DirtyFieldProbe*>(this)->observed =
            status == CheckObjectStatus::Update && fields && fields->contains("code");
    }
};

static void graph_preflight_preserves_dirty_fields_for_typed_checker() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& metadata = user->make<InMemoryMetadataStore>();
    metadata.register_entity(DirtyTrackedOrder::entity_descriptor(owner));
    user->set_metadata(*user, metadata);
    auto& probe = user->make_resource<DirtyFieldProbe>();
    auto& checker =
        user->make<TypedEntityChecker<DirtyTrackedOrder, DirtyFieldProbe>>(*user, probe);
    auto& registry = user->make<InMemoryCheckerRegistry>();
    registry.register_checker(*user, checker);
    user->set_checker_registry(*user, registry);
    Record row(owner);
    row.emplace("id", Value(owner, std::uint64_t(7)));
    row.emplace("version", Value(owner, std::int64_t(3)));
    row.emplace("code", Value(owner, "ORD-OLD"));
    auto entity = DirtyTrackedOrder::from_compact_row(
        owner, CompactRow(std::allocator_arg, Allocator(owner), row));
    check(bool(entity), "original tracked entity materializes");
    entity.value().root.set(entity.value().entity_key(), "code", Value(owner, "ORD-NEW"));
    auto node = graph_node_from_entity(*user, std::move(entity).value());
    check(node && detail::preflight_graph(*user, node.value(), ObjectLocation::root(owner)),
          "original dirty graph preflight succeeds");
    check(probe.observed && !node.value().values.fields.contains(std::string_view("_dirty_fields")),
          "original typed checker sees update dirty fields and final graph removes the marker");
}

static void failed_commit_does_not_deliver_audit_or_clear_retryable_ledger() {
    Fixture f(true);
    f.state.fail_commit = true;
    f.root.set(f.key, "name", Value(f.owner, "updated"));
    const auto& result = f.saver.save_ledger_dyn(*f.user, std::move(f.node), f.root).wait();
    check(!result &&
              result.error().to_string(f.owner).find("simulated commit failure") != String::npos,
          "original commit failure is returned");
    check(f.state.called("commit-failed") && f.state.called("transaction-mutate") &&
              f.state.called("transaction-query"),
          "commit failure follows mutation and readback");
    check(f.capture.events.empty(), "failed commit does not deliver audit");
    check(!f.root.current_change_set().changes().empty(),
          "failed commit retains retryable scalar changes");
}

static void audit_sink_failure_is_reported_as_already_committed_without_replaying_ledger() {
    Fixture f(true);
    f.capture.fail = true;
    f.root.set(f.key, "name", Value(f.owner, "updated"));
    const auto& result = f.saver.save_ledger_dyn(*f.user, std::move(f.node), f.root).wait();
    check(!result && result.error().kind() == RuntimeError::Kind::AuditAfterCommit &&
              result.error().to_string(f.owner).find("AUDIT_DELIVERY_AFTER_COMMIT") != String::npos,
          "original audit failure identifies an already committed operation");
    check(f.state.called("commit") && !f.state.called("rollback"),
          "audit failure cannot roll back the commit");
    check(f.capture.events.size() == 1 && f.root.current_change_set().changes().empty(),
          "committed ledger is cleared even when its audit sink fails");
}

struct TraceFixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user_context = global.make_user_context();
    Memory owner = user_context->memory_resource();
    EntityRuntimeState& root = user_context->make<EntityRuntimeState>();

    EntityKey key(std::string_view entity, std::uint64_t id) {
        return EntityKey(std::allocator_arg, Allocator(owner), entity, Value(owner, id));
    }

    GraphNode& node(std::string_view entity, std::uint64_t id, std::string_view reason = {}) {
        auto& value = user_context->make<GraphNode>(entity);
        value.value("id", id);
        if (!reason.empty()) {
            value.with_comment(reason);
        }
        return value;
    }

    void capture(const GraphNode& graph) {
        check(bool(detail::capture_ledger_trace_scopes(root, graph)), "original trace capture");
    }

    struct Expected {
        std::string_view entity;
        std::optional<std::uint64_t> id;
        std::string_view reason;
    };

    void
    trace(std::string_view entity, std::uint64_t id, std::initializer_list<Expected> expected) {
        auto actual = root.get_trace_chain(key(entity, id));
        check(actual.size() == expected.size(), "original trace length");
        auto position = actual.begin();
        for (const auto& item : expected) {
            check(position->kind == TraceKind::AuditReason &&
                      position->entity_type == item.entity && position->entity_id == item.id &&
                      position->comment == item.reason,
                  "original trace kind, identity and reason");
            ++position;
        }
    }
};

static void captures_nested_ledger_ancestry_including_deleted_child_and_same_numeric_id() {
    TraceFixture f;
    auto& graph = f.node("Order", 100, "submit order");
    graph.relation("items", f.node("OrderItem", 201));
    auto& removed = f.node("OrderItem", 202, "remove unavailable item");
    removed.operation = GraphOperation::Remove;
    graph.relation("items", removed);
    auto& payment = f.node("Payment", 100, "authorize payment");
    payment.relation("attempts", f.node("PaymentAttempt", 401));
    graph.relation("payment", payment);
    graph.relation("shipment", f.node("Shipment", 501, "dispatch shipment"));
    f.root.mark_as_delete(f.key("OrderItem", 202));
    f.capture(graph);
    TraceFixture::Expected order{"Order", 100, "submit order"};
    TraceFixture::Expected paid{"Payment", 100, "authorize payment"};
    f.trace("Order", 100, {order});
    f.trace("OrderItem", 201, {order});
    f.trace("Payment", 100, {order, paid});
    f.trace("PaymentAttempt", 401, {order, paid});
    f.trace("Shipment", 501, {order, {"Shipment", 501, "dispatch shipment"}});
    f.trace("OrderItem", 202, {order, {"OrderItem", 202, "remove unavailable item"}});
}

static void retry_rebuilds_request_scopes_instead_of_appending_or_reusing_old_root_intent() {
    TraceFixture f;
    auto& graph = f.node("Order", 100, "first attempt");
    graph.relation("payment", f.node("Payment", 301, "authorize payment"));
    f.capture(graph);
    graph.set_comment("retry after review");
    f.capture(graph);
    f.trace("Payment",
            301,
            {{"Order", 100, "retry after review"}, {"Payment", 301, "authorize payment"}});
    f.root.clear_committed();
    f.trace("Payment", 301, {});
}

static void allocation_rebinds_owned_ancestors_without_assigning_inherited_siblings() {
    TraceFixture f;
    auto& graph = f.node("Order", 0, "submit late order");
    graph.relation("items", f.node("OrderItem", 0));
    auto& payment = f.node("Payment", 0, "authorize late payment");
    payment.relation("attempts", f.node("PaymentAttempt", 401));
    graph.relation("payment", payment);
    f.root.mark_as_new(f.key("Shipment", 501));
    f.root.set_entity_comment(f.key("Shipment", 501), "dispatch standalone shipment");
    f.capture(graph);
    std::pmr::map<EntityKey, Value> ids(f.owner);
    ids.emplace(f.key("Order", 0), Value(f.owner, std::uint64_t{100}));
    ids.emplace(f.key("OrderItem", 0), Value(f.owner, std::uint64_t{201}));
    ids.emplace(f.key("Payment", 0), Value(f.owner, std::uint64_t{301}));
    detail::EntityRuntimeAccess::enrich_allocated_trace_scopes(f.root, ids);
    TraceFixture::Expected order{"Order", 100, "submit late order"};
    f.trace("OrderItem", 0, {order});
    f.trace("PaymentAttempt", 401, {order, {"Payment", 301, "authorize late payment"}});
    f.trace("Shipment", 501, {order, {"Shipment", 501, "dispatch standalone shipment"}});
    graph.set_comment("retry late order");
    f.capture(graph);
    ids.clear();
    ids.emplace(f.key("Order", 0), Value(f.owner, std::uint64_t{102}));
    ids.emplace(f.key("Payment", 0), Value(f.owner, std::uint64_t{303}));
    detail::EntityRuntimeAccess::enrich_allocated_trace_scopes(f.root, ids);
    f.trace("PaymentAttempt",
            401,
            {{"Order", 102, "retry late order"}, {"Payment", 303, "authorize late payment"}});
    check(graph.id() && *graph.id() == Value(f.owner, std::uint64_t{0}),
          "allocation does not replace original graph identity");
    f.root.clear_committed();
    f.trace("PaymentAttempt", 401, {});
}

static void allocation_uses_scope_ownership_when_unassigned_same_type_keys_differ() {
    TraceFixture f;
    auto& graph = f.node("Order", 100, "submit order");
    auto& first = f.user_context->make<GraphNode>("Payment");
    first.value("id", Value(f.owner)).with_comment("first payment");
    graph.relation("payments", first);
    graph.relation("payments", f.node("Payment", 0, "second payment"));
    f.capture(graph);
    EntityKey null_key(std::allocator_arg, Allocator(f.owner), "Payment", Value(f.owner));
    auto zero_key = f.key("Payment", 0);
    std::pmr::map<EntityKey, Value> ids(f.owner);
    ids.emplace(null_key, Value(f.owner, std::uint64_t{301}));
    ids.emplace(zero_key, Value(f.owner, std::uint64_t{302}));
    detail::EntityRuntimeAccess::enrich_allocated_trace_scopes(f.root, ids);
    check(f.root.get_trace_chain(null_key).back().entity_id == 301 &&
              f.root.get_trace_chain(zero_key).back().entity_id == 302,
          "same entity NULL and zero scopes retain independent allocation owners");
}

static void install_trace_relations(TraceFixture& f, bool payment_graph) {
    auto& metadata = f.user_context->make<InMemoryMetadataStore>();
    EntityDescriptor order(std::allocator_arg, Allocator(f.owner), "Order");
    order.property("id", DataType::U64).id();
    auto& relation = order.relation(payment_graph ? "payments" : "items",
                                    payment_graph ? "Payment" : "OrderItem");
    relation.local_key = "id";
    relation.foreign_key = "order";
    relation.many = true;
    metadata.register_entity(std::move(order));
    EntityDescriptor child(
        std::allocator_arg, Allocator(f.owner), payment_graph ? "Payment" : "OrderItem");
    child.property("id", DataType::U64).id();
    if (payment_graph) {
        auto& attempts = child.relation("attempts", "PaymentAttempt");
        attempts.local_key = "id";
        attempts.foreign_key = "payment";
        attempts.many = true;
        EntityDescriptor attempt(std::allocator_arg, Allocator(f.owner), "PaymentAttempt");
        attempt.property("id", DataType::U64).id();
        metadata.register_entity(std::move(attempt));
    }
    metadata.register_entity(std::move(child));
    f.user_context->set_metadata(*f.user_context, metadata);
}

static void
composed_deleted_child_is_discovered_from_loaded_snapshot_without_field_changes_or_io() {
    TraceFixture f;
    auto& child = f.user_context->make<EntityRuntimeState>();
    auto key = f.key("OrderItem", 202);
    Record row(f.owner);
    row.emplace("id", Value(f.owner, std::uint64_t{202}));
    row.emplace("version", Value(f.owner, std::int64_t{3}));
    row.emplace("order", Value(f.owner, std::uint64_t{100}));
    child.set_original_compact_row("OrderItem",
                                   CompactRow(std::allocator_arg, Allocator(f.owner), row));
    child.set_entity_comment(key, "remove unavailable item");
    child.mark_as_delete(key);
    f.root.adopt_mutations_from(child);
    check(!detail::EntityRuntimeAccess::first_composition_error(f.root) &&
              f.root.current_change_set().changes().empty(),
          "deletion composition needs no scalar change");
    install_trace_relations(f, false);
    auto& graph = f.node("Order", 100, "submit order");
    EntityRuntimeState::Keys visited(f.owner);
    visited.emplace(f.key("Order", 100));
    check(bool(detail::hydrate_ledger_relations(*f.user_context, f.root, graph, visited)),
          "hydrate deletion without an executor");
    auto& removed = graph.relations.at(String("items", f.owner)).at(0);
    check(removed.operation == GraphOperation::Remove && removed.comment &&
              *removed.comment == "remove unavailable item",
          "discovered child retains removal and its reason");
    f.capture(graph);
    f.trace("OrderItem",
            202,
            {{"Order", 100, "submit order"}, {"OrderItem", 202, "remove unavailable item"}});
}

static void mixed_signed_provider_ids_and_unsigned_generated_references_keep_grandchild_ancestry() {
    TraceFixture f;
    auto& payment = f.user_context->make<EntityRuntimeState>();
    Record row(f.owner);
    row.emplace("id", Value(f.owner, std::int64_t{301}));
    row.emplace("version", Value(f.owner, std::int64_t{1}));
    row.emplace("order", Value(f.owner, std::int64_t{100}));
    payment.set_original_compact_row("Payment",
                                     CompactRow(std::allocator_arg, Allocator(f.owner), row));
    payment.set(f.key("Payment", 301), "reference_code", Value(f.owner, "authorized"));
    payment.set_entity_comment(f.key("Payment", 301), "authorize payment");
    f.root.adopt_mutations_from(payment);
    for (std::uint64_t id : {401, 402}) {
        auto key = f.key("PaymentAttempt", id);
        f.root.mark_as_new(key);
        f.root.set(key, "payment", Value(f.owner, id - 100));
    }
    install_trace_relations(f, true);
    auto& graph = f.node("Order", 100, "submit order");
    EntityRuntimeState::Keys visited(f.owner);
    visited.emplace(f.key("Order", 100));
    check(bool(detail::hydrate_ledger_relations(*f.user_context, f.root, graph, visited)),
          "hydrate mixed signed identities");
    const auto& payments = graph.relations.at(String("payments", f.owner));
    check(payments.size() == 1, "signed loaded FK matches unsigned parent");
    const auto& attempts = payments[0].relations.at(String("attempts", f.owner));
    check(attempts.size() == 1 && attempts[0].id() && attempts[0].id()->try_u64() == 401,
          "only matching unsigned child FK joins signed payment");
    f.capture(graph);
    f.trace("PaymentAttempt",
            401,
            {{"Order", 100, "submit order"}, {"Payment", 301, "authorize payment"}});
    f.trace("PaymentAttempt", 402, {{"Order", 100, "submit order"}});
}

static void graph_and_ledger_savers_reject_missing_intent_before_policy_and_transaction() {
    for (bool logging : {false, true}) {
        Fixture f;
        if (logging) {
            f.user->set_sql_log_options(SqlLogOptions::all());
        }
        auto registry = f.user->make_ref<FunctionMutationPolicyRegistry>(
            [](Memory, std::string_view) -> std::optional<MutationPolicyBorrow> {
                throw std::runtime_error("missing intent reached policy registry");
            });
        f.user->set_mutation_policy_registry(registry);
        for (auto comment : {std::optional<std::string_view>{},
                             std::optional<std::string_view>{""},
                             std::optional<std::string_view>{" \t\r\n"},
                             std::optional<std::string_view>{"\u0085"},
                             std::optional<std::string_view>{"\u00a0"},
                             std::optional<std::string_view>{"\u2003"}}) {
            GraphNode node(std::allocator_arg, Allocator(f.owner), "DirtyTrackedOrder");
            node.value("id", std::uint64_t{1});
            auto& child = f.user->make<GraphNode>("Child");
            child.with_comment("valid child reason");
            node.relation("children", child);
            if (comment) {
                node.set_comment(*comment);
            }
            for (bool ledger : {false, true}) {
                auto copy = detail::clone_in(f.owner, node);
                auto& operation = ledger ? f.saver.save_ledger_dyn(*f.user, std::move(copy), f.root)
                                         : f.saver.save_graph_dyn(*f.user, std::move(copy));
                const auto& result = operation.wait();
                check(!result && result.error().kind() == RuntimeError::Kind::RequestIntent,
                      "missing root intent precedes metadata and execution");
                const auto& error = result.error().request_intent_error();
                check(error.code() == "REQUEST_COMMENT_REQUIRED" && error.field == "comment" &&
                          error.request_kind == RequestKind::Mutation,
                      "original missing mutation intent diagnostic");
                check(f.state.calls.empty(), "missing root intent never reaches provider");
            }
        }
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        composed_deleted_child_is_discovered_from_loaded_snapshot_without_field_changes_or_io();
        mixed_signed_provider_ids_and_unsigned_generated_references_keep_grandchild_ancestry();
        graph_and_ledger_savers_reject_missing_intent_before_policy_and_transaction();
        captures_nested_ledger_ancestry_including_deleted_child_and_same_numeric_id();
        retry_rebuilds_request_scopes_instead_of_appending_or_reusing_old_root_intent();
        allocation_rebinds_owned_ancestors_without_assigning_inherited_siblings();
        allocation_uses_scope_ownership_when_unassigned_same_type_keys_differ();
        scalar_change_keeps_snapshot_but_relation_changes_invalidate_it();
        ledger_save_returns_transaction_snapshot_before_concurrent_commit_race();
        failed_authoritative_readback_rolls_back_before_reporting_failure();
        soft_delete_returns_authoritative_tombstone_before_commit();
        failed_commit_does_not_deliver_audit_or_clear_retryable_ledger();
        audit_sink_failure_is_reported_as_already_committed_without_replaying_ledger();
        cancelled_new_root_cannot_return_a_fictitious_persisted_entity();
        mutation_policy_denial_happens_before_transaction_begin();
        nested_ledger_entity_keeps_model_and_json_error_paths();
        graph_preflight_preserves_dirty_fields_for_typed_checker();
        std::cout << "Original seventeen entity-save contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
