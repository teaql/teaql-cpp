#include "../../tests/support/audited_save_fixture.hpp"
#include "../../tests/support/graph_saver_fixture.hpp"
#include "../../tests/support/save_preparation_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static InMemoryMetadataStore& metadata(UserContext& user) {
    auto& store = user.make<InMemoryMetadataStore>();
    auto d = audited_fixture::Model::entity_descriptor(user.memory_resource());
    d.table("orders");
    store.register_entity(std::move(d));
    store.register_entity(save_fixture::Model::entity_descriptor(user.memory_resource()));
    user.set_metadata(user, store);
    user.disable_sql_log();
    return store;
}

static audited_fixture::Model input(Memory r, const EntityRuntimeState* root = nullptr) {
    audited_fixture::Model m(std::allocator_arg, Allocator(r));
    m.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    m.values.fields.emplace(String("version", r), Value(r, std::int64_t(3)));
    m.values.fields.emplace(String("name", r), Value(r, "input"));
    m.fresh = true;
    if (root) {
        m.runtime.emplace(std::allocator_arg, Allocator(r), *root);
    }
    return m;
}

static void entry_order_and_evidence() {
    GlobalContext global;
    for (bool ledger : {false, true}) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        metadata(*user);
        auto& state = user->make<mutation_fixture::State>();
        auto& checker = user->make_resource<audited_fixture::Checker>(state, "error");
        auto& registry = user->make<InMemoryCheckerRegistry>();
        registry.register_checker(*user, checker);
        user->set_checker_registry(*user, registry);
        auto audited =
            Audited<audited_fixture::Model>(std::allocator_arg, Allocator(r), input(r), "reason");
        auto& work = ledger
                         ? save_audited_ledger_entity(std::move(audited), *user)
                         : AuditedSaveExt<audited_fixture::Model>::save(std::move(audited), *user);
        auto& result = work.wait();
        check(!result && checker.calls == unsigned(!ledger),
              "plain preflight precedes missing saver; ledger lookup precedes preflight");
        check(user->last_fix_evidence().empty(),
              "missing saver/plain entry does not publish ledger Fix evidence");
    }
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    auto& controls = user->make<saver_fixture::Controls>();
    auto executor = user->make_resource_ref<saver_fixture::Ambient>(state, controls);
    user->register_executor(executor);
    auto& checker = user->make_resource<audited_fixture::Checker>(state, "error");
    auto& registry = user->make<InMemoryCheckerRegistry>();
    registry.register_checker(*user, checker);
    user->set_checker_registry(*user, registry);
    auto audited =
        Audited<audited_fixture::Model>(std::allocator_arg, Allocator(r), input(r), "reason");
    auto& result = save_audited_ledger_entity(std::move(audited), *user).wait();
    check(!result && user->last_fix_evidence().size() == 1 && !user->get(executor).transaction,
          "automatic ledger preflight failure publishes evidence before opening a transaction");
}

static void cancelled_automatic_entry() {
    GlobalContext global;
    unsigned cancelled = 0;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    metadata(*user);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.read_pending = true;
    state.external_cancel = &cancelled;
    auto& controls = user->make<saver_fixture::Controls>();
    auto executor = user->make_resource_ref<saver_fixture::Ambient>(state, controls);
    user->register_executor(executor);
    auto& root = user->make<EntityRuntimeState>();
    EntityKey identity(std::allocator_arg, Allocator(r), "Order", Value(r, std::uint64_t(7)));
    root.mark_as_new(identity);
    root.set(identity, "name", Value(r, "pending"));
    auto audited = Audited<audited_fixture::Model>(
        std::allocator_arg, Allocator(r), input(r, &root), "reason");
    auto& work = save_audited_ledger_entity(std::move(audited), *user);
    check(!work.poll(), "automatic audited entry suspends at authoritative read");
    work.cancel();
    check(cancelled == 1 && controls.abandoned == 1 && root.is_new(identity),
          "cancellation joins read, releases transaction and preserves uncommitted intent");
    check(!detail::current_mutation_governance(*user), "cancel restores governance activation");
}

static void sqlite_commit_and_reconstruction() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& store = metadata(*user);
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (auto statement : {"CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
                           "INSERT INTO orders VALUES(7,3,'original')"}) {
        sql.sql = statement;
        check(bool(transport.execute(r, sql)), "SQLite automatic audited setup");
    }
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& cache = user->make<SqlExecutorState>();
    using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
    auto executor = user->make_ref<Executor>(dialect, transport, store, cache);
    user->register_executor(executor);
    auto& root = user->make<EntityRuntimeState>();
    auto identity = user->make_ref<EntityKey>("Order", Value(r, std::uint64_t(7)));
    root.set_original_version(user->get(identity), 3);
    root.set(user->get(identity), "name", Value(r, "committed"));
    auto& model = user->make<audited_fixture::Model>(input(r, &root));
    model.fresh = false;
    auto& audited = user->make<Audited<audited_fixture::Model>>(std::move(model), "reason");
    auto& result = save_audited_ledger_entity(std::move(audited), *user).wait();
    check(result && result.value().restored &&
              result.value().values.fields.at(String("version", r)).try_i64() == 4 &&
              root.current_change_set().changes().empty(),
          "automatic ledger entry commits authoritative row and restores safe runtime state");
    auto& retained = global.make<audited_fixture::Model>(result.value());
    // The ordinary extension must support an Entity that has no ledger API.
    static_assert(!LedgerEntity<save_fixture::Model>);
    store.register_entity(save_fixture::Model::entity_descriptor(r));
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.rows.emplace_back();
    state.rows.back().emplace(String("id", r), Value(r, std::uint64_t(1)));
    state.rows.back().emplace(String("version", r), Value(r, std::int64_t(1)));
    state.rows.back().emplace(String("name", r), Value(r, "authoritative non-ledger"));
    auto& controls = user->make<saver_fixture::Controls>();
    auto fake = user->make_resource_ref<saver_fixture::Ambient>(state, controls);
    user->register_executor(fake);
    auto& plain = user->make<save_fixture::Model>();
    plain.fresh = true;
    plain.values.fields.emplace(String("id", r), Value(r, std::uint64_t(1)));
    auto& normal = user->make<Audited<save_fixture::Model>>(std::move(plain), "plain reason");
    auto& plain_result = AuditedSaveExt<save_fixture::Model>::save(std::move(normal), *user).wait();
    if (!plain_result) {
        throw std::runtime_error(std::string(plain_result.error().to_string(r)));
    }
    check(bool(plain_result), "plain extension preserves the non-ledger Entity abstraction");
    user.reset();
    check(retained.values.fields.at(String("name", global.memory_resource())).try_text() ==
                  "committed" &&
              retained.runtime,
          "explicit retained typed result survives actual User destruction");
}

struct DynamicResultOperation {
    using allocator_type = Allocator;
    detail::GraphSaveResult result;

    DynamicResultOperation(std::allocator_arg_t, Allocator a, GraphNode node)
        : result(detail::GraphSaveResult::ok(a.resource(), std::move(node))) {}

    std::optional<detail::GraphSaveResult> poll(WakeSignal&) {
        return std::move(result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct DynamicResultSaver final : detail::DynGraphSaver {
    using allocator_type = Allocator;

    DynamicResultSaver(std::allocator_arg_t, Allocator) {}

    AsyncOperation<detail::GraphSaveResult>& save_graph_dyn(const UserContext& user_context,
                                                            GraphNode node) const override {
        auto r = user_context.memory_resource();
        auto definitions = DynamicFieldDefinitions::from_fields(r, "Order", "1", {});
        auto& owned = user_context.make<DynamicFieldDefinitions>(definitions.value());
        DynamicFieldValues::Values empty(r);
        auto fields = DynamicFieldValues::from_values(r, owned, empty);
        node.dynamic_fields.reset(
            &user_context.make<DynamicFieldValues>(std::move(fields).value()));
        return make_operation<detail::GraphSaveResult, DynamicResultOperation>(r, std::move(node));
    }

    AsyncOperation<detail::GraphSaveResult>& save_ledger_dyn(
        const UserContext& user_context, GraphNode node, const EntityRuntimeState&) const override {
        return save_graph_dyn(user_context, std::move(node));
    }
};

static void plain_saved_dynamic_result() {
    GlobalContext global;
    for (bool ledger : {false, true}) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        metadata(*user_context);
        auto& saver = user_context->make_resource<DynamicResultSaver>();
        user_context->insert_resource(*user_context, static_cast<detail::DynGraphSaver&>(saver));
        auto audited = Audited<audited_fixture::Model>(
            std::allocator_arg, Allocator(r), input(r), "verify saved dynamic decoding boundary");
        auto& operation = ledger ? save_audited_ledger_entity(std::move(audited), *user_context)
                                 : AuditedSaveExt<audited_fixture::Model>::save(std::move(audited),
                                                                                *user_context);
        const auto& result = operation.wait();
        if (ledger) {
            check(!result &&
                      result.error().to_string(r) ==
                          "graph write error: indexed state missing from dynamic save result",
                  "ledger Save retains the indexed dynamic installation precondition");
        } else {
            check(bool(result) &&
                      result.value().values.fields.at(String("name", r)).try_text() == "input",
                  "plain generic Save decodes its row without installing returned dynamic fields");
        }
    }
}

struct EntryOrderModel : EntityDefaults<EntryOrderModel> {
    static inline std::array<std::string_view, 32> events{};
    static inline unsigned count = 0;

    static void note(std::string_view event) {
        events.at(count++) = event;
    }

    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    MutationValues values;
    std::optional<EntityRuntimeState> runtime;
    std::optional<String> comment;
    std::optional<FieldSet> loaded, dirty;
    std::optional<EntitySnapshot> original;
    bool fresh = false, removed = false, restored = false;
    const DynamicFieldValues* fields = nullptr;

    EntryOrderModel(std::allocator_arg_t, Allocator a) : values(a.resource()) {}

    EntryOrderModel(std::allocator_arg_t, Allocator a, const EntryOrderModel& other)
        : values(std::allocator_arg, a, other.values),
          runtime(detail::clone_in(a.resource(), other.runtime)),
          comment(detail::clone_in(a.resource(), other.comment)),
          loaded(detail::clone_in(a.resource(), other.loaded)),
          dirty(detail::clone_in(a.resource(), other.dirty)),
          original(detail::clone_in(a.resource(), other.original)), fresh(other.fresh),
          removed(other.removed), restored(other.restored), fields(other.fields) {}

    EntryOrderModel(const EntryOrderModel&) = delete;
    EntryOrderModel(EntryOrderModel&&) noexcept = default;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        note("descriptor");
        EntityDescriptor d(std::allocator_arg, Allocator(r), ENTITY_NAME);
        d.property("id", DataType::U64).id();
        d.property("version", DataType::I64).version();
        d.property("name", DataType::Text);
        return d;
    }

    static Result<EntryOrderModel, EntityError> from_compact_row(Memory r, CompactRow row) {
        if (auto* name = row.get("name"); name && name->try_text() == "decode error") {
            return Result<EntryOrderModel, EntityError>::err(
                r, EntityError(std::allocator_arg, Allocator(r), "Order", "decode failed"));
        }
        EntryOrderModel m(std::allocator_arg, Allocator(r));
        m.values.fields = row.into_map(r);
        return Result<EntryOrderModel, EntityError>::ok(r, std::move(m));
    }

    const DynamicFieldValues* dynamic_field_values() const {
        return fields;
    }

    MutationValues into_values() && {
        return std::move(values);
    }

    std::optional<AnyRef> runtime_state_any() const {
        note("generic_carrier");
        return runtime ? std::optional<AnyRef>(std::in_place, *runtime) : std::nullopt;
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        note("ledger_carrier");
        return detail::clone_in(memory_resource(), runtime);
    }

    void set_comment(std::string_view reason) {
        note("audit");
        comment.emplace(reason, memory_resource());
        if (runtime) {
            runtime->set_comment(reason);
        }
    }

    std::optional<String> get_comment() const {
        return detail::clone_in(memory_resource(), comment);
    }

    bool is_field_loaded(std::string_view field) const {
        return !loaded || loaded->contains(field);
    }

    std::optional<FieldSet> dirty_fields() const {
        return detail::clone_in(memory_resource(), dirty);
    }

    std::optional<EntitySnapshot> original_values() const {
        return detail::clone_in(memory_resource(), original);
    }

    bool is_new() const {
        return fresh;
    }

    bool is_marked_as_delete() const {
        return removed;
    }

    void on_loaded(AnyRef source) {
        if (auto* root = source.get<EntityRuntimeState>()) {
            runtime.emplace(std::allocator_arg, Allocator(memory_resource()), *root);
            restored = true;
        }
    }
};

static void save_entry_order() {
    GlobalContext global;
    for (bool ledger : {false, true}) {
        for (bool dynamic : {false, true}) {
            for (bool carrier : {false, true}) {
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto& metadata = user_context->make<InMemoryMetadataStore>();
                metadata.register_entity(EntryOrderModel::entity_descriptor(r));
                user_context->set_metadata(*user_context, metadata);
                EntryOrderModel model(std::allocator_arg, Allocator(r));
                model.values.fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
                model.values.fields.emplace(String("version", r), Value(r, std::int64_t{1}));
                model.values.fields.emplace(String("name", r), Value(r, "loaded"));
                if (carrier) {
                    model.runtime.emplace(std::allocator_arg, Allocator(r));
                }
                if (dynamic) {
                    auto definitions = DynamicFieldDefinitions::from_fields(r, "Order", "1", {});
                    auto& owned = user_context->make<DynamicFieldDefinitions>(definitions.value());
                    DynamicFieldValues::Values empty(r);
                    auto fields = DynamicFieldValues::from_values(r, owned, empty);
                    model.fields =
                        &user_context->make<DynamicFieldValues>(std::move(fields).value());
                }
                EntryOrderModel::count = 0;
                auto audited = Audited<EntryOrderModel>(
                    std::allocator_arg, Allocator(r), std::move(model), "verify save entry order");
                auto& operation =
                    ledger
                        ? save_audited_ledger_entity(std::move(audited), *user_context)
                        : AuditedSaveExt<EntryOrderModel>::save(std::move(audited), *user_context);
                const auto& result = operation.wait();
                check(!result, "entry fixture reaches missing saver or carrier");
                auto error = result.error().to_string(r);
                auto route = error.find("no DynGraphSaver registered") != String::npos
                                 ? "missing_saver"
                                 : "missing_carrier";
                const bool missing_carrier = !ledger && dynamic && !carrier;
                check(std::string_view(route) ==
                          (missing_carrier ? "missing_carrier" : "missing_saver"),
                      "only generic dynamic Save requires a runtime carrier");
                if (missing_carrier) {
                    check(
                        error ==
                            "graph write error: dynamic save requires the runtime mutation ledger carrier",
                        "generic missing-carrier error is preserved");
                }
                Vector<std::string_view> expected(r);
                if (ledger) {
                    expected = {"audit", "ledger_carrier", "descriptor"};
                } else if (dynamic && carrier) {
                    expected = {"descriptor", "audit", "generic_carrier", "descriptor"};
                } else if (dynamic) {
                    expected = {"descriptor", "audit", "generic_carrier"};
                } else {
                    expected = {"descriptor", "audit", "descriptor"};
                }
                check(
                    EntryOrderModel::count == expected.size() &&
                        std::equal(
                            expected.begin(), expected.end(), EntryOrderModel::events.begin()),
                    "generic and ledger entries preserve original descriptor/audit/carrier ordering");
            }
        }
    }
}

int main() {
    save_entry_order();
    plain_saved_dynamic_result();
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        entry_order_and_evidence();
        cancelled_automatic_entry();
        sqlite_commit_and_reconstruction();
        std::pmr::set_default_resource(previous);
        std::cout << "Automatic audited contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
