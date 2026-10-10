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

    CaptureEvents(std::allocator_arg_t, Allocator a) : events(a.resource()) {}

    Result<std::monostate, RuntimeError>
    on_event(Memory owner, const UserContext&, const RawAuditEvent& event) const override {
        const_cast<CaptureEvents*>(this)->events.emplace_back(event);
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
        node.value("id", std::int64_t(1)).value("version", std::int64_t(1)).value("name", "before");
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
        .value("name", "cancelled");
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

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scalar_change_keeps_snapshot_but_relation_changes_invalidate_it();
        ledger_save_returns_transaction_snapshot_before_concurrent_commit_race();
        failed_authoritative_readback_rolls_back_before_reporting_failure();
        soft_delete_returns_authoritative_tombstone_before_commit();
        cancelled_new_root_cannot_return_a_fictitious_persisted_entity();
        mutation_policy_denial_happens_before_transaction_begin();
        nested_ledger_entity_keeps_model_and_json_error_paths();
        graph_preflight_preserves_dirty_fields_for_typed_checker();
        std::cout << "Original eight entity-save contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
