#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include "entity_decoder_fixture.hpp"
#include <iostream>
using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
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

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T value;

    Ready(std::allocator_arg_t, Allocator a, T value)
        : value(detail::move_in(a.resource(), std::move(value))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(value);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Executor : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    unsigned queries = 0, cancelled = 0;
    bool pending = false, fail = false, bad_model = false;
    QueryRequest* seen = nullptr;
    WakeSignal* child_wake = nullptr;
    unsigned* external_cancel = nullptr;

    Executor(std::allocator_arg_t, Allocator a) : r(a.resource()) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        DataServiceCapabilities c;
        c.query = c.mutation = true;
        return c;
    }

    struct QueryWork {
        using allocator_type = Allocator;
        Memory r;
        Executor& executor;

        QueryWork(std::allocator_arg_t, Allocator a, Executor& e) : r(a.resource()), executor(e) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal& wake) {
            executor.child_wake = &wake;
            if (executor.pending)
                return {};
            if (executor.fail)
                return Result<QueryResult, String>::err(r, String("executor failed", r));
            QueryResult result(
                std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, 1));
            Record values(r);
            values.emplace(String("id", r), Value(r, std::uint64_t(1)));
            values.emplace(String("fail", r), Value(r, executor.bad_model));
            result.rows.emplace_back(values);
            return Result<QueryResult, String>::ok(r, std::move(result));
        }

        void finish_and_wait() noexcept {
            executor.child_wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            ++executor.cancelled;
            if (executor.external_cancel)
                ++*executor.external_cancel;
            executor.child_wake = nullptr;
        }
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory target,
                                                           QueryRequest request) override {
        ++queries;
        seen = &detail::resource_arena(target).make<QueryRequest>(std::move(request));
        return make_operation<Result<QueryResult, String>, QueryWork>(target, *this);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

struct Spy : MetadataStore {
    using allocator_type = Allocator;
    Memory r;
    mutable unsigned logs = 0;
    bool capture = true, debug = false;

    Spy(std::allocator_arg_t, Allocator a) : r(a.resource()) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    const EntityDescriptor* entity(std::string_view) const override {
        return nullptr;
    }

    Vector<const EntityDescriptor*> all_entities(Memory target) const override {
        return Vector<const EntityDescriptor*>(target);
    }

    bool capture_execution_metadata() const override {
        return capture;
    }

    bool capture_query_debug() const override {
        return debug;
    }

    void record_metadata_log(const ExecutionMetadata&) const override {
        ++logs;
    }
};

static void binding_and_base_queries() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    auto r = user->memory_resource();
    auto missing = user->entity_data_service<Executor>("Item");
    check(!missing && missing.error().kind() == ContextError::Kind::MissingEntityDataService,
          "entity service check precedes metadata and executor");
    auto& registry = user->make<InMemoryEntityRegistry>();
    registry.register_entity("Item");
    user->set_entity_registry(*user, registry);
    auto no_metadata = user->entity_data_service<Executor>("Item");
    check(!no_metadata && no_metadata.error().kind() == ContextError::Kind::MissingResource &&
              no_metadata.error().name() == "metadata",
          "registered-only service still requires metadata before executor");
    auto& store = user->make<InMemoryMetadataStore>();
    store.register_entity(decoder_fixture::Item<>::entity_descriptor(r));
    user->set_metadata(*user, store);
    check(user->entity_data_service<Executor>("Item").error().kind() ==
              ContextError::Kind::MissingTypedResource,
          "executor lookup preserves typed error");
    auto& executor = user->make<Executor>();
    user->insert_resource(*user, executor);
    auto repo = user->entity_data_service<Executor>("Item");
    check(repo && repo.value().get().entity() == "Item",
          "Context binds registered entity and executor");
    auto& service = repo.value().get();
    check(service.select().entity == "Item" && service.insert_command().entity == "Item" &&
              service.update_command(7).id.try_i64() == 7 &&
              service.delete_command(7).soft_delete &&
              service.recover_command(7, -2).expected_version == -2,
          "independent command factories preserve defaults");
    rejected(
        [&] {
            global.make<EntityDataService<Executor>>(service);
        },
        "service cannot retain User Context in Global");
    rejected(
        [&] {
            EntityDataService<Executor>::for_executor(*sibling, "Item", *user, executor);
        },
        "explicit binding rejects sibling-owned executor");
    auto& spy = global.make<Spy>();
    ResourceBorrow<Executor> borrow(
        std::allocator_arg, Allocator(r), executor, detail::resource_arena(r));
    auto& base = user->make<detail::RuntimeDataService<Spy, Executor>>(spy, borrow);
    auto& query = user->make<SelectQuery>("Item");
    query.comment("business");
    query.trace_chain.emplace_back(TraceKind::Request, "Item", "request");
    auto& all = base.fetch_all(query);
    check(executor.queries == 0, "runtime query remains lazy until polled");
    check(all.wait() && spy.logs == 0,
          "fetch_all forwards diagnostics without duplicate final metadata");
    check(executor.seen->trace_chain == query.trace_chain &&
              executor.seen->comment == query.comment_text &&
              executor.seen->capture_execution_metadata && !executor.seen->capture_debug_query,
          "base adapter retains request trace, comment and separate capture flags");
    auto& list = base.fetch_smart_list(query);
    check(list.wait() && list.wait().value().is_loaded && spy.logs == 1,
          "SmartList records returned metadata and marks rows loaded");
    auto& entities = base.fetch_entities<decoder_fixture::Item<>>(query);
    check(entities.wait() && !entities.wait().value()[0].state && spy.logs == 2,
          "base Entity decoding does not pretend to attach entity graph context");
    auto& retained = global.make<SmartList<decoder_fixture::Item<>>>(entities.wait().value());
    executor.bad_model = true;
    auto& bad = base.fetch_entities<decoder_fixture::Item<>>(query);
    check(!bad.wait() && bad.wait().error().kind() == DataServiceError<String>::Kind::Entity &&
              spy.logs == 3,
          "model failure occurs after returned metadata recording");
    executor.bad_model = false;
    executor.fail = true;
    auto& error = base.fetch_all(query);
    check(!error.wait() && error.wait().error().executor_error() == "executor failed" &&
              spy.logs == 3,
          "executor errors preserve original type without invented success metadata");
    executor.fail = false;
    executor.pending = true;
    auto& pending = base.fetch_all(query);
    check(!pending.poll(), "pending provider propagates Pending");
    auto ticket = pending.wake_signal().ticket();
    executor.child_wake->notify();
    check(pending.wake_signal().ticket() != ticket, "child completion forwards parent wake");
    pending.cancel();
    check(executor.cancelled == 1, "cancellation reaches provider");
    auto& closing = base.fetch_all(query);
    check(!closing.poll(), "second pending query reaches provider");
    unsigned stopped = 0;
    executor.external_cancel = &stopped;
    user->close();
    check(user->allocation_stats().live_blocks == 0,
          "runtime adapter host blocks reclaimed after quiescence");
    // executor is a User borrow and intentionally not read after close.
    check(stopped == 1, "Context close cancels provider before destroying executor");
    check(retained[0].id_value().try_u64() == 1,
          "Global copy of non-assignable Entity list survives User close");
}

struct Behavior : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    Memory r;
    Vector<String>& calls;
    bool fail = false;

    Behavior(std::allocator_arg_t, Allocator a, Vector<String>& calls)
        : r(a.resource()), calls(calls) {}

    Memory memory_resource() const {
        return r;
    }

    RuntimeHookResult
    before_select(Memory target, const UserContext&, SelectQuery& q) const override {
        calls.emplace_back("behavior");
        if (fail)
            return RuntimeHookResult::err(target, RuntimeError::behavior(target, "stop"));
        q.entity = "Intermediate";
        q.relations.emplace_back("child");
        q.trace_chain.emplace_back(TraceKind::Relation, "behavior", "hook");
        return RuntimeHookResult::ok(target, {});
    }

    RuntimeHookResult
    before_update(Memory target, const UserContext&, UpdateCommand& q) const override {
        calls.emplace_back("update behavior");
        q.entity = "Intermediate";
        return RuntimeHookResult::ok(target, {});
    }

    Vector<String> relation_loads(Memory target, const UserContext&) const override {
        Vector<String> out(target);
        out.emplace_back("child");
        return out;
    }
};

struct Policy : RequestPolicy {
    using allocator_type = Allocator;
    Memory r;
    Vector<String>& calls;

    Policy(std::allocator_arg_t, Allocator a, Vector<String>& calls)
        : r(a.resource()), calls(calls) {}

    Memory memory_resource() const {
        return r;
    }

    RuntimeHookResult
    enforce_select(Memory target, const UserContext&, SelectQuery& q) const override {
        calls.emplace_back("policy");
        q.entity = "Final";
        return RuntimeHookResult::ok(target, {});
    }

    RuntimeHookResult
    enforce_update(Memory target, const UserContext&, UpdateCommand& q) const override {
        calls.emplace_back("update policy");
        q.entity = "Final";
        return RuntimeHookResult::ok(target, {});
    }
};

struct Schema final : runtime::SchemaProvider {
    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& c,
                                                       const SchemaInvocation&) const override {
        return make_operation<RuntimeSchemaResult, Ready<RuntimeSchemaResult>>(
            c.memory_resource(), RuntimeSchemaResult::ok(c.memory_resource(), {}));
    }
};

static GeneratedSchemaBootstrapFuture& bootstrap(const UserContext& c) {
    auto* service = c.get_resource<EntityDataService<Executor>>();
    auto result = Access::prepare_select(*service, service->select());
    check(result && result.value().entity == "Final" && result.value().relations.empty(),
          "bootstrap bypasses behavior while policy still applies");
    check(service->relation_loads().empty(), "bootstrap skips behavior relation requests");
    return make_operation<RuntimeSchemaResult, Ready<RuntimeSchemaResult>>(
        c.memory_resource(), RuntimeSchemaResult::ok(c.memory_resource(), {}));
}

static void hooks_and_metadata() {
    GlobalContext g;
    auto c = g.make_user_context();
    auto r = c->memory_resource();
    auto& calls = c->make<Vector<String>>();
    auto& behavior = c->make<Behavior>(calls);
    auto& policy = c->make<Policy>(calls);
    auto& module = c->make<RuntimeModule>();
    auto& e = c->make<EntityDescriptor>("Final");
    e.relation("child", "Child").local_key = "final_key";
    auto& child = c->make<EntityDescriptor>("Child");
    child.property("id", DataType::U64).is_id = true;
    module.descriptor(detail::clone_in(r, e))
        .descriptor(detail::clone_in(r, child))
        .behavior("QueryEntity", *c, behavior)
        .behavior("Repo", *c, behavior)
        .generated_schema_bootstrap(bootstrap)
        .apply_to(*c);
    c->set_request_policy(*c, policy);
    auto& executor = c->make<Executor>();
    auto& service = EntityDataService<Executor>::for_executor(*c, "Repo", *c, executor);
    TraceChain trace(r);
    trace.emplace_back(TraceKind::Request, "repository", "outer");
    service.with_trace_context(trace);
    auto& query = c->make<SelectQuery>("QueryEntity");
    query.trace_chain.emplace_back(TraceKind::Comment, "query", "inner");
    auto prepared = Access::prepare_select(service, query);
    check(prepared && prepared.value().entity == "Final" &&
              prepared.value().projection.size() == 1 &&
              prepared.value().projection[0] == "final_key",
          "behavior then policy then final descriptor projection");
    check(prepared.value().trace_chain.size() == 3 &&
              prepared.value().trace_chain[0].entity_type == "repository" &&
              prepared.value().trace_chain[1].entity_type == "query",
          "repository trace prepends before hook trace");
    check(query.entity == "QueryEntity" && query.relations.empty() && query.projection.empty(),
          "caller query remains unchanged");
    check(calls.size() == 2 && calls[0] == "behavior" && calls[1] == "policy",
          "query dispatch order preserved");
    behavior.fail = true;
    calls.clear();
    auto denied = Access::prepare_select(service, query);
    check(!denied && denied.error().kind() == RuntimeError::Kind::Behavior && calls.size() == 1,
          "behavior error stops policy");
    behavior.fail = false;
    auto update = service.update_command(7);
    update.entity = "Unrelated";
    calls.clear();
    auto final = service.prepare_update_command(update);
    check(final && final.value().entity == "Final" && update.entity == "Unrelated" &&
              calls[0] == "update behavior" && calls[1] == "update policy",
          "update selects repository behavior and leaves caller command intact");
    c->insert_resource(*c, service);
    auto& schema = c->make_resource<Schema>();
    c->set_schema_provider(*c, schema);
    calls.clear();
    check(bool(c->ensure_schema().wait()), "bootstrap fixture succeeds");
    check(calls.size() == 1 && calls[0] == "policy", "bootstrap only bypasses behavior");
    detail::UserContextMetadata view(std::allocator_arg, Allocator(r), *c);
    c->disable_sql_log();
    check(!view.capture_execution_metadata() && !view.capture_query_debug() &&
              !view.query_diagnostic_observer(r) && !view.mutation_diagnostic_observer(r),
          "Context metadata flags differ from base MetadataStore defaults");
    c->enable_mutation_sql_log();
    check(!view.query_diagnostic_observer(r) && view.mutation_diagnostic_observer(r),
          "mutation observer follows independent flag");
    c->enable_select_sql_log();
    auto observer = view.query_diagnostic_observer(r);
    check(bool(observer), "select observer enabled");
    rejected(
        [&] {
            g.make<ExecutionObserver>(*observer);
        },
        "Context observer cannot escape into Global");
    rejected(
        [&] {
            view.all_entities(g.memory_resource());
        },
        "metadata view pointer list cannot escape User lifetime");
    c->set_user_identifier("u");
    auto& context_service = Access::context_service(service);
    check(context_service.resolve_final_comment({}, "reason") == "[u] reason",
          "business comment gains user prefix");
    check(context_service.resolve_final_comment(query.trace_chain, "ignored") ==
              "[u] query(pending): inner",
          "trace chain takes precedence over explicit comment");
    c->set_user_identifier_option({});
    check(!context_service.resolve_final_comment({}, ""),
          "empty user and empty business comment produce None");
}

static void plans_and_scopes() {
    GlobalContext g;
    RelationLoadPlan* saved = nullptr;
    {
        auto c = g.make_user_context();
        auto r = c->memory_resource();
        auto& store = c->make<InMemoryMetadataStore>();
        EntityDescriptor root(std::allocator_arg, Allocator(r), "Root");
        root.relation("child", "Child");
        EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
        child.property("id", DataType::U64).is_id = true;
        child.relation("back", "Root");
        store.register_entity(std::move(root));
        store.register_entity(std::move(child));
        c->set_metadata(*c, store);
        auto& executor = c->make<Executor>();
        auto& service = EntityDataService<Executor>::for_executor(*c, "Root", *c, executor);
        auto& query = c->make<SelectQuery>("Root");
        SelectQuery nested(std::allocator_arg, Allocator(r), "Wrong");
        nested.project("name").limit(2);
        nested.relations.emplace_back("back");
        query.relations.emplace_back("child", nested);
        auto plans = detail::build_relation_plans_from_loads(*c, "Root", query.relations);
        check(plans && plans.value().size() == 1 && plans.value()[0].children.size() == 1,
              "explicit nested relation plans preserved");
        auto& retained = g.make<RelationLoadPlan>(plans.value()[0]);
        saved = &retained;
        check(retained.query->memory_resource() == g.memory_resource() &&
                  retained.children[0].memory_resource() == g.memory_resource(),
              "plan copy propagates nested query and children domains");
        auto flat = Access::flat_relation_plans(service, query);
        check(flat && !flat.value(),
              "missing decoder disables flat hydration without losing valid plan");
        auto& registered = c->make<InMemoryEntityGraphDecoderRegistry>();
        registered.register_entity<decoder_fixture::Item<>>();
        c->set_entity_graph_decoder_registry(registered);
        EntityDescriptor root_with_item(std::allocator_arg, Allocator(r), "Root");
        root_with_item.relation("item", "Item");
        store.register_entity(std::move(root_with_item));
        store.register_entity(decoder_fixture::Item<>::entity_descriptor(r));
        auto& supported = c->make<SelectQuery>("Root");
        supported.relations.emplace_back("item");
        auto available = Access::flat_relation_plans(service, supported);
        check(available && available.value() && available.value()->query.size() == 1 &&
                  available.value()->behavior.empty(),
              "flat hydration eligibility retains independent query and behavior plan vectors");
        auto parent = Access::query_scoped(service);
        auto a = Access::scoped(parent, "Child");
        auto b = Access::scoped(parent, "Child");
        auto inherited = [&](const auto& repo) {
            Vector<String> out(r);
            auto intent = Access::intent(repo);
            if (intent)
                intent->extend_secrets(false, out);
            return out;
        };
        auto metadata = ExecutionMetadata::unrecorded_query(r, 0);
        metadata.parameterized_query.emplace("SELECT ?", r);
        metadata.params.emplace_back("parent secret");
        metadata.sql_log.parameter_policies.push_back(SqlParameterLogPolicy::Unknown);
        Access::record(parent, metadata);
        check(
            !inherited(parent).empty() && inherited(a).empty() && inherited(b).empty() &&
                metadata.sql_log.intent_redactions.is_empty(),
            "query metadata updates private scope without modifying caller or existing sibling snapshots");
        auto descendant = Access::scoped(parent, "Child");
        check(!inherited(descendant).empty(),
              "new descendant inherits parent's current intent snapshot");
        auto later = ExecutionMetadata::unrecorded_query(r, 0);
        later.parameterized_query.emplace("SELECT ?", r);
        later.params.emplace_back("child secret");
        later.sql_log.parameter_policies.push_back(SqlParameterLogPolicy::Unknown);
        Access::record(descendant, later);
        check(inherited(parent).size() == 1 && inherited(descendant).size() == 2,
              "child provenance does not mutate parent scope");
        auto batch = ExecutionMetadata::unrecorded_query(r, 0);
        batch.statements.emplace_back(later);
        auto second = detail::clone_in(r, later);
        second.params[0] = Value(r, "second child secret");
        batch.statements.emplace_back(std::move(second));
        Access::record(parent, batch);
        check(inherited(parent).size() == 3 &&
                  batch.statements[1].sql_log.intent_redactions.is_empty(),
              "recursive statement leaves inherit earlier intent without mutating caller metadata");
        c->enable_select_sql_log();
        auto observer = [&] {
            auto temporary = Access::scoped(parent, "Child");
            return Access::observer(temporary);
        }();
        check(bool(observer), "query observer follows metadata capture flag");
        rejected(
            [&] {
                g.make<ExecutionObserver>(*observer);
            },
            "query observer cannot escape its User Context");
        (*observer)(detail::clone_in(r, later));
        check(
            inherited(parent).size() == 3,
            "observer survives temporary repository destruction and preserves parent intent isolation");
        // Close only after stack-owned PMR plan/query values have left their scope in
        // the ordinary destructor path; retained is a Global allocation.
        check(retained.target_entity == "Child", "retained plan readable");
    }
    check(saved->query->relations[0].name == "back" && saved->children[0].target_entity == "Root",
          "Global nested plan remains valid after User destruction");
}

static void sqlite_base_adapter() {
    GlobalContext g;
    auto c = g.make_user_context();
    auto r = c->memory_resource();
    auto& connection = c->make_resource<SqliteConnection>();
    auto& provider = c->make<SqliteMutationExecutor>(connection);
    auto& sql = c->make<CompiledQuery>();
    sql.sql = "CREATE TABLE items(id INTEGER PRIMARY KEY, label TEXT)";
    check(bool(provider.execute(r, sql)), "SQLite setup table");
    sql.sql = "INSERT INTO items VALUES(1, 'row through layered runtime')";
    check(bool(provider.execute(r, sql)), "SQLite setup row");
    auto& metadata = c->make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
    descriptor.table("items");
    descriptor.property("id", DataType::U64).is_id = true;
    descriptor.property("label", DataType::Text);
    metadata.register_entity(std::move(descriptor));
    c->set_metadata(*c, metadata);
    auto& dialect = c->make_resource<SqliteDialect>();
    auto& state = c->make<SqlExecutorState>();
    auto& executor =
        c->make<SqlDataServiceExecutor<MutationExecutorError>>(dialect, provider, metadata, state);
    auto& service = EntityDataService<std::remove_reference_t<decltype(executor)>>::for_executor(
        *c, "Item", *c, executor);
    auto query = service.select();
    auto& result = [&]() -> auto& {
        auto temporary = Access::scoped(service, "Item");
        auto base = Access::context_service(temporary).data_service();
        return base.template fetch_entities<decoder_fixture::Item<>>(query);
    }();
    check(
        result.wait() &&
            result.wait().value()[0].row.get("label")->try_text() == "row through layered runtime",
        "runtime base adapter outlives temporary wrappers and composes actual SQL executor, transport and SQLite provider");
}

int main() {
    try {
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        binding_and_base_queries();
        hooks_and_metadata();
        plans_and_scopes();
        sqlite_base_adapter();
        std::pmr::set_default_resource(old);
        std::cout << "Entity service planning and adapter contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
