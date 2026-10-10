#include "../../tests/support/runtime_mutation_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>
using namespace teaql;
using namespace mutation_fixture;
using Access = detail::EntityDataServiceAccess;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void adapters_and_audit() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->disable_sql_log();
    auto& state = user->make<State>();
    auto& executor = user->make_resource<Executor>(state);
    auto& sink = user->make_resource<Sink>(state);
    user->set_event_sink(*user, sink);
    auto& cache = user->make_resource<Cache>(state);
    user->insert_resource(*user, static_cast<AggregationCacheBackend&>(cache));
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& cmd = user->make<UpdateCommand>("Order", std::uint64_t(7));
    cmd.value("name", "new").expected_version(3);
    cmd.old_values.emplace(r);
    cmd.old_values->fields.emplace(String("name", r), Value(r, "old"));
    cmd.old_values->fields.emplace(String("other", r), Value(r, "preserved"));
    auto& result = Access::update_internal(service, cmd).wait();
    check(result && result.value() == 1, "runtime update executes");
    check(state.notes.size() == 3 && state.notes[0] == "provider" &&
              state.notes[1] == "fixture::entity:Order" && state.notes[2] == "event",
          "cache invalidation precedes event delivery");
    check(state.events[0].old_values &&
              state.events[0].new_values->at(String("other", r)).try_text() == "preserved",
          "complete old snapshots retained and merged");
    auto& batch = user->make<BatchUpdateCommand>("Order");
    batch.batch_ids.emplace_back(std::uint64_t(7));
    batch.batch_expected_versions.emplace_back(4);
    batch.batch_values.emplace_back();
    batch.batch_values.back().fields.emplace(String("name", r), Value(r, "after"));
    batch.batch_values.back().fields.emplace(String("unknown", r), Value(r, "event only"));
    batch.update_fields.emplace_back("name");
    auto& written = Access::execute_prepared_batch_update(service, batch).wait();
    check(written && state.events.back().values.contains(std::string_view("unknown")),
          "batch event retains full input beyond SQL projection");
    check(std::get<UpdateCommand>(state.requests.back().data).values.fields.size() == 1,
          "SQL batch update filters by update_fields");
    auto& retained = global.make<RawAuditEvent>(state.events.back());
    user.reset();
    check(retained.values.contains(std::string_view("unknown")),
          "Global audit copy survives actual User destruction");
}

static void conflict_failure_and_cancel() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& state = user->make<State>();
    state.affected.push_back(0);
    auto& executor = user->make_resource<Executor>(state);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& cmd = user->make<UpdateCommand>("Order", std::uint64_t(7));
    cmd.expected_version(1);
    auto& result = Access::update_internal(service, cmd).wait();
    check(!result &&
              result.error().runtime_error().kind() == RuntimeError::Kind::OptimisticLockConflict,
          "zero version-checked rows conflict");
    check(state.events.empty(), "failed mutation emits no event");
    state.pending = true;
    auto& future = Access::update_internal(service, cmd);
    check(!future.poll(), "mutation may suspend");
    unsigned cancelled = 0;
    state.external_cancel = &cancelled;
    user.reset();
    check(cancelled == 1, "close joins pending native mutation before arena reclamation");
}

static void sqlite_entry() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->disable_sql_log();
    auto& metadata = user->make<InMemoryMetadataStore>();
    EntityDescriptor d(std::allocator_arg, Allocator(r), "Order");
    d.table("orders");
    d.property("id", DataType::U64).id();
    d.property("version", DataType::I64).version();
    d.property("name", DataType::Text);
    metadata.register_entity(std::move(d));
    user->set_metadata(*user, metadata);
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    sql.sql = "CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)";
    check(bool(transport.execute(r, sql)), "SQLite setup");
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& state = user->make<SqlExecutorState>();
    auto& executor = user->make<SqlDataServiceExecutor<MutationExecutorError>>(
        dialect, transport, metadata, state);
    using E = std::remove_reference_t<decltype(executor)>;
    auto& service = EntityDataService<E>::for_executor(*user, "Order", *user, executor);
    auto& insert = user->make<InsertCommand>("Order");
    insert.value("id", std::uint64_t(7)).value("name", "first");
    auto& result = Access::insert_internal(service, insert).wait();
    check(result && result.value() == 1, "prepared runtime insert uses real SQLite");
    auto& update = user->make<UpdateCommand>("Order", std::uint64_t(7));
    update.value("name", "second").expected_version(1);
    auto& changed = Access::update_internal(service, update).wait();
    check(changed && changed.value() == 1, "real SQLite guarded update through runtime adapter");
    auto& conflict = Access::update_internal(service, update).wait();
    check(!conflict, "second update with stale version conflicts");
}

struct ClosingHook final : EntityDataServiceBehavior {
    UserContext& user;

    explicit ClosingHook(UserContext& user) : user(user) {}

    RuntimeHookResult before_insert(Memory r, const UserContext&, InsertCommand&) const override {
        user.close();
        return RuntimeHookResult::ok(r, {});
    }
};

struct ClosingSink final : RawAuditEventSink {
    UserContext& user;

    explicit ClosingSink(UserContext& user) : user(user) {}

    RuntimeHookResult on_event(Memory r, const UserContext&, const RawAuditEvent&) const override {
        user.close();
        return RuntimeHookResult::ok(r, {});
    }
};

static void callback_expiry() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& state = user->make<State>();
    auto& executor = user->make_resource<Executor>(state);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto registry = user->make_ref<InMemoryEntityDataServiceBehaviorRegistry>();
    user->set_entity_data_service_behavior_registry(*user, user->get(registry));
    user->destroy(registry);
    auto& command = user->make<InsertCommand>("Order");
    bool rejected = false;
    try {
        (void)service.prepare_insert_command(command);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "destroyed behavior registry is rejected before callbacks");
    auto replacement = user->make_ref<InMemoryEntityDataServiceBehaviorRegistry>();
    user->set_entity_data_service_behavior_registry(*user, user->get(replacement));
    auto policy = user->make_resource_ref<RequestPolicy>();
    user->set_request_policy(*user, user->get(policy));
    user->destroy(policy);
    rejected = false;
    try {
        (void)service.prepare_insert_command(command);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "destroyed request policy is rejected before virtual dispatch");
    user->clear_request_policy();
    auto sink = user->make_resource_ref<Sink>(state);
    user->set_event_sink(*user, user->get(sink));
    user->destroy(sink);
    rejected = false;
    RawAuditEvent event(std::allocator_arg, Allocator(r), RawAuditEventKind::Created, "Order");
    try {
        (void)user->send_event(event);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "destroyed raw sink is rejected before virtual dispatch");
}

int main(int argc, char** argv) {
    if (argc == 2) {
        std::set_terminate([] {
            std::_Exit(0);
        });
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        if (std::string_view(argv[1]) == "reentrant-event") {
            auto& sink = user->make_resource<ClosingSink>(*user);
            user->set_event_sink(*user, sink);
            RawAuditEvent event(
                std::allocator_arg, Allocator(r), RawAuditEventKind::Created, "Order");
            (void)user->send_event(event);
        } else {
            auto& registry = user->make<InMemoryEntityDataServiceBehaviorRegistry>();
            auto& hook = user->make_resource<ClosingHook>(*user);
            registry.register_behavior("Order", *user, hook);
            user->set_entity_data_service_behavior_registry(*user, registry);
            auto& state = user->make<State>();
            auto& executor = user->make_resource<Executor>(state);
            auto& service =
                EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
            auto& command = user->make<InsertCommand>("Order");
            (void)service.prepare_insert_command(command);
        }
        return 1;
    }
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        adapters_and_audit();
        conflict_failure_and_cancel();
        sqlite_entry();
        callback_expiry();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime mutation contracts passed\n";
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
}
