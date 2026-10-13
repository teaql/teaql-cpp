#include "../../tests/support/dynamic_hydration_fixture.hpp"
#include <teaql/runtime/detail/data_service/audited_auto_save.hpp>
#include <teaql/runtime/transaction.hpp>
#include <teaql/runtime/graph_debug.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;
using dynamic_hydration_fixture::School;
using dynamic_hydration_fixture::Executor;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void sql(SqliteConnection& connection, const char* text) {
    check(sqlite3_exec(connection.native_handle(), text, nullptr, nullptr, nullptr) == SQLITE_OK,
          "dynamic save fixture SQL succeeds");
}

struct Sink final : RawAuditEventSink {
    using allocator_type = Allocator;
    mutable Vector<RawAuditEvent> events;
    bool fail = false;

    Sink(std::allocator_arg_t, Allocator a) : events(a.resource()) {}

    Memory memory_resource() const {
        return events.get_allocator().resource();
    }

    Result<std::monostate, RuntimeError>
    on_event(Memory r, const UserContext&, const RawAuditEvent& event) const override {
        events.emplace_back(event);
        if (fail) {
            return Result<std::monostate, RuntimeError>::err(
                r, RuntimeError::event(r, "committed sink failed"));
        }
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user_context = global.make_user_context();
    SqliteConnection* connection = nullptr;
    Executor& executor;
    const DynamicFieldDefinitions& definitions;
    Sink& sink;

    Fixture()
        : executor(dynamic_hydration_fixture::setup(*user_context, &connection, false)),
          definitions(make_definitions()), sink(user_context->make<Sink>()) {
        auto r = user_context->memory_resource();
        sql(*connection,
            "CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);INSERT INTO school_data VALUES(7,'original',3)");
        const std::array defs{&definitions};
        auto configured = DatabaseDynamicFieldsProvider<Executor>::create(r, "profile", defs);
        check(bool(configured), "trusted database provider configures");
        auto& provider = user_context->make<DatabaseDynamicFieldsProvider<Executor>>(
            std::move(configured).value());
        user_context->set_dynamic_fields_provider(*user_context, provider);
        check(bool(provider.ensure_schema(*user_context).wait()),
              "dynamic schema exists before save");
        user_context->register_executor(*user_context, executor);
        user_context->set_event_sink(*user_context, sink);
        user_context->disable_sql_log();
        DynamicFieldWrite::Changes changes(r);
        changes.emplace(String("title", r), DynamicFieldMutation::set(r, Value(r, "old")));
        changes.emplace(String("count", r),
                        DynamicFieldMutation::set(r, Value(r, std::int64_t(9))));
        Vector<DynamicFieldWrite> writes(r);
        writes.emplace_back("profile", definitions, 7, changes);
        auto& tx = executor.begin(r).wait().value().get();
        check(
            bool(tx.dynamic_field_store()
                     ->apply_writes(r, writes, MutationIntent::create(r, "seed extensions").value())
                     .wait()),
            "seed extensions on native transaction");
        check(bool(tx.commit(r).wait()), "seed transaction commits");
    }

    const DynamicFieldDefinitions& make_definitions() {
        auto r = user_context->memory_resource();
        const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                                DynamicFieldDefinitions::Field{"count", DataType::I64}};
        return user_context->make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
    }

    School loaded() {
        auto r = user_context->memory_resource();
        Record values(r);
        values.emplace(String("id", r), Value(r, std::uint64_t(7)));
        values.emplace(String("name", r), Value(r, "original"));
        values.emplace(String("version", r), Value(r, std::int64_t(3)));
        auto decoded =
            School::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), values));
        check(bool(decoded), "decode loaded carrier");
        const std::array ids{std::uint64_t(7)};
        auto selection = DynamicFieldSelection::all(r);
        auto intent =
            QueryIntent::create(r, "load starting dynamic view", "bind trusted storage provenance")
                .value();
        auto& batch = user_context->load_dynamic_fields("School", ids, selection, intent).wait();
        check(bool(batch), "read trusted starting dynamic fields");
        auto fields =
            std::move(detail::clone_in(r, batch.value())).into_values("School", ids, selection);
        check(fields && bool(detail::install_saved_dynamic_fields(
                            r, decoded.value(), &fields.value().front())),
              "install starting dynamic snapshot");
        return std::move(decoded).value();
    }

    DynamicStorageRows stored() {
        auto r = user_context->memory_resource();
        const std::array ids{std::uint64_t(7)};
        auto& result =
            executor.dynamic_field_store()
                ->load_values(
                    r,
                    "profile",
                    definitions,
                    ids,
                    DynamicFieldSelection::all(r),
                    QueryIntent::create(r, "inspect save result", "verify persisted extensions")
                        .value())
                .wait();
        check(bool(result), "read durable save result");
        return detail::clone_in(r, result.value());
    }

    std::int64_t version() {
        auto r = user_context->memory_resource();
        CompiledQuery query(std::allocator_arg, Allocator(r));
        query.sql = "SELECT version FROM school_data WHERE id=7";
        auto& result = executor.transport.fetch_all_compact_sql(r, query).wait();
        check(bool(result), "read native version");
        return *result.value()[0].get("version")->try_i64();
    }
};

enum class Mode { Set, Null, Delete, Trigger, WriteFailure, ReadFailure, AuditFailure, PlainSet };

static void automatic_save(Mode mode) {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto entity = fixture.loaded();
    auto& updates = fixture.global.make<unsigned>(0);
    check(sqlite3_create_function(
              fixture.connection->native_handle(),
              "remember_native_update",
              0,
              SQLITE_UTF8,
              &updates,
              [](sqlite3_context* context, int, sqlite3_value**) {
                  ++*static_cast<unsigned*>(sqlite3_user_data(context));
                  sqlite3_result_int(context, 1);
              },
              nullptr,
              nullptr) == SQLITE_OK,
          "register native DML observation");
    sql(*fixture.connection,
        "CREATE TRIGGER observe_native_update AFTER UPDATE ON school_data BEGIN SELECT remember_native_update(); END");
    EntityKey key(std::allocator_arg, Allocator(r), "School", Value(r, std::uint64_t(7)));
    entity.root.set_comment("save dynamic fixture");
    if (mode == Mode::Delete) {
        check(bool(entity.root.delete_dynamic_field(key, "title")),
              "delete dynamic intent records");
    } else {
        check(bool(entity.root.update_dynamic_field(
                  key, "title", mode == Mode::Null ? Value(r) : Value(r, "pending"))),
              "set dynamic intent records");
    }
    if (mode == Mode::Trigger) {
        sql(*fixture.connection,
            "CREATE TRIGGER change_extension AFTER UPDATE ON teaql_dynamic_field_storage_v1 WHEN NEW.code='title' BEGIN UPDATE teaql_dynamic_field_storage_v1 SET payload='\"triggered\"' WHERE code=NEW.code AND owner_id=NEW.owner_id; END");
    } else if (mode == Mode::WriteFailure) {
        sql(*fixture.connection,
            "CREATE TRIGGER reject_extension BEFORE INSERT ON teaql_dynamic_field_storage_v1 BEGIN SELECT RAISE(ABORT,'private driver failure'); END");
    } else if (mode == Mode::ReadFailure) {
        sql(*fixture.connection,
            "CREATE TRIGGER corrupt_extension AFTER UPDATE ON teaql_dynamic_field_storage_v1 WHEN NEW.code='title' BEGIN UPDATE teaql_dynamic_field_storage_v1 SET payload='123' WHERE code=NEW.code AND owner_id=NEW.owner_id; END");
    }
    fixture.sink.fail = mode == Mode::AuditFailure;
    auto audited =
        Audited<School>(std::allocator_arg, Allocator(r), entity, "save dynamic fixture");
    auto& operation = mode == Mode::PlainSet
                          ? AuditedSaveExt<School>::save(std::move(audited), user_context)
                          : save_audited_ledger_entity(std::move(audited), user_context);
    auto& result = operation.wait();
    bool rollback = mode == Mode::WriteFailure || mode == Mode::ReadFailure;
    check(updates == 1,
          "native optimistic update precedes dynamic persistence/readback exactly once");
    if (rollback) {
        check(!result && result.error().kind() == RuntimeError::Kind::DynamicField &&
                  fixture.version() == 3 && fixture.sink.events.empty(),
              "dynamic failure rolls back native version and suppresses audit");
        check(entity.root.has_pending_dynamic_mutations() &&
                  fixture.stored().at(7).at(String("title", r)).try_text() == "old",
              "failed save preserves original storage and retryable intent");
        return;
    }
    if (!result && !rollback && mode != Mode::AuditFailure) {
        throw std::runtime_error(std::string(result.error().to_string(r)));
    }
    check(fixture.version() == 4 && fixture.sink.events.size() == 1 &&
              !entity.root.has_pending_dynamic_mutations(),
          "extension-only save increments native version and clears committed ledger once");
    if (mode == Mode::AuditFailure) {
        check(!result && result.error().kind() == RuntimeError::Kind::AuditAfterCommit,
              "committed dynamic sink failure is explicit");
        return;
    }
    check(bool(result) && result.value().is_field_loaded("#count") &&
              result.value().dynamic_field_values()->field("count").value().value()->try_i64() == 9,
          "authoritative saved view retains previously selected untouched extension");
    auto field = result.value().dynamic_field_values()->field("title").value();
    if (mode == Mode::Delete) {
        check(field.state() == DynamicFieldState::NotLoaded &&
                  !fixture.stored().at(7).contains("title"),
              "delete removes storage rather than persisting NULL");
    } else if (mode == Mode::Null) {
        check(field.state() == DynamicFieldState::Null &&
                  fixture.stored().at(7).at(String("title", r)).is_null(),
              "explicit NULL remains a durable selected row");
    } else {
        auto expected = mode == Mode::Trigger ? "triggered" : "pending";
        check(field.value()->try_text() == expected &&
                  fixture.stored().at(7).at(String("title", r)).try_text() == expected,
              "saved view uses authoritative dynamic readback");
        auto audit =
            fixture.sink.events.front().values.at(String("#title", r)).to_json_value(r).dump(r);
        check(audit.find(expected) != audit.npos &&
                  (mode != Mode::Trigger || audit.find("pending") == audit.npos),
              "native fact contains authoritative extension operation value");
    }
    check(result.value().root.current_change_set().is_empty(),
          "returned saved carrier has a clean ledger");
    auto& retained = fixture.global.make<GraphNode>(
        detail::extract_graph_node_from_entity(user_context, detail::clone_in(r, result.value()))
            .value());
    check(retained.dynamic_fields &&
              retained.dynamic_fields->values() == result.value().dynamic_field_values()->values(),
          "explicit Global graph copy carries private dynamic values");
}

struct SaveBody {
    using allocator_type = Allocator;
    const TransactionScope<Executor>& scope;
    const EntityRuntimeState& pending_root;
    School entity;
    bool abort;
    AsyncOperation<Result<School, RuntimeError>>* pending = nullptr;
    WakeSignal::Forward forwarding;

    SaveBody(std::allocator_arg_t,
             Allocator a,
             const TransactionScope<Executor>& scope,
             const School& entity,
             bool abort)
        : scope(scope), pending_root(entity.root), entity(std::allocator_arg, a, entity),
          abort(abort) {}

    std::optional<Result<School, RuntimeError>> poll(WakeSignal& wake) {
        auto r = scope.context().memory_resource();
        if (!pending) {
            pending = &scope.save_audited(Audited<School>(
                std::allocator_arg, Allocator(r), std::move(entity), "manual dynamic save"));
        }
        forwarding.attach(pending->wake_signal(), wake);
        auto* result = pending->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return Result<School, RuntimeError>::err(r, detail::clone_in(r, result->error()));
        }
        check(pending_root.has_pending_dynamic_mutations() &&
                  detail::TransactionScopeAccess::pending_ledgers(scope) == 1 &&
                  result->value().root.current_change_set().is_empty(),
              "scope retains pending original ledger while the returned view is freshly hydrated");
        if (abort) {
            return Result<School, RuntimeError>::err(r, RuntimeError::event(r, "outer body abort"));
        }
        return Result<School, RuntimeError>::ok(r, detail::clone_in(r, result->value()));
    }

    void finish_and_wait() noexcept {
        forwarding.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding.reset();
        if (pending) {
            pending->cancel();
        }
    }
};

static void manual_save(bool abort) {
    Fixture fixture;
    auto r = fixture.user_context->memory_resource();
    auto entity = fixture.loaded();
    EntityKey key(std::allocator_arg, Allocator(r), "School", Value(r, std::uint64_t(7)));
    check(bool(entity.root.update_dynamic_field(key, "title", Value(r, "manual"))),
          "manual dynamic intent records");
    auto& result = fixture.user_context
                       ->execute_in_transaction<Executor, School>(
                           [&](const TransactionScope<Executor>& scope)
                               -> AsyncOperation<Result<School, RuntimeError>>& {
                               return make_operation<Result<School, RuntimeError>, SaveBody>(
                                   r, scope, entity, abort);
                           })
                       .wait();
    check(bool(result) == !abort && fixture.version() == (abort ? 3 : 4) &&
              fixture.sink.events.size() == (abort ? 0 : 1),
          "manual dynamic save follows enclosing commit and audit result");
    check(entity.root.has_pending_dynamic_mutations() == abort,
          "manual rollback retains original pending ledger");
    check(fixture.stored().at(7).at(String("title", r)).try_text() == (abort ? "old" : "manual"),
          "manual storage shares native transaction");
}

static void allocated_graph_and_retention() {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    GraphNode* retained = nullptr;
    {
        auto& metadata = const_cast<InMemoryMetadataStore&>(
            dynamic_cast<const InMemoryMetadataStore&>(fixture.executor.schema_provider));
        auto descriptor = School::entity_descriptor(r);
        RelationDescriptor relation(std::allocator_arg, Allocator(r), "children", "Child");
        relation.many = true;
        relation.local_key = "id";
        relation.foreign_key = "school_id";
        descriptor.relation(relation);
        metadata.register_entity(std::move(descriptor));
        EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
        child.table("child_data");
        child.property("id", DataType::U64).id();
        child.property("school_id", DataType::U64);
        child.property("name", DataType::Text);
        child.property("version", DataType::I64).version();
        metadata.register_entity(std::move(child));
        sql(*fixture.connection,
            "PRAGMA foreign_keys=ON;CREATE TABLE child_data(id INTEGER PRIMARY KEY,school_id INTEGER REFERENCES school_data(id),name TEXT,version INTEGER)");
        auto& generator = user_context.make<AtomicCounterIdGenerator>(100);
        user_context.set_internal_id_generator(user_context, generator);
        auto loaded = fixture.loaded();
        auto& root = user_context.make<EntityRuntimeState>();
        Value::Object empty(r);
        auto fields =
            DynamicFieldValues::from_values(r, loaded.dynamic_field_values()->definitions(), empty);
        root.install_loaded_dynamic_fields(fields.value());
        EntityKey parent_key(
            std::allocator_arg, Allocator(r), "School", Value(r, std::uint64_t(0)));
        EntityKey child_key(std::allocator_arg, Allocator(r), "Child", Value(r, std::uint64_t(0)));
        root.mark_as_new(parent_key);
        root.mark_as_new(child_key);
        root.set(parent_key, "name", Value(r, "allocated parent"));
        root.set(child_key, "school_id", Value(r, std::uint64_t(0)));
        root.set(child_key, "name", Value(r, "allocated child"));
        check(bool(root.update_dynamic_field(parent_key, "title", Value(r, "allocated extension"))),
              "new graph records dynamic intent");
        GraphNode node(std::allocator_arg, Allocator(r), "School");
        node.value("id", std::uint64_t(0))
            .value("name", "allocated parent")
            .with_comment("allocate graph with extensions");
        auto saver = user_context.require_resource<detail::DynGraphSaver>();
        auto& result =
            saver.value().get().save_ledger_dyn(user_context, std::move(node), root).wait();
        if (!result) {
            throw std::runtime_error(std::string(result.error().to_string(r)));
        }
        auto parent_id = result.value().values.fields.at(String("id", r)).try_u64();
        check(parent_id && *parent_id >= 100 && result.value().dynamic_fields &&
                  result.value().dynamic_fields->values().at(String("title", r)).try_text() ==
                      "allocated extension",
              "dynamic storage owner uses allocated native identity");
        CompiledQuery query(std::allocator_arg, Allocator(r));
        query.sql = "SELECT school_id FROM child_data";
        auto& children = fixture.executor.transport.fetch_all_compact_sql(r, query).wait();
        check(children && children.value().size() == 1 &&
                  children.value()[0].get("school_id")->try_u64() == parent_id,
              "new graph native foreign key is rebound before first insert");
        check(fixture.sink.events.size() == 2 && root.current_change_set().is_empty(),
              "new graph commits both native facts and dynamic ledger");
        retained = &fixture.global.make<GraphNode>(result.value());
    }
    fixture.user_context.reset();
    check(retained->dynamic_fields &&
              retained->dynamic_fields->memory_resource() == fixture.global.memory_resource() &&
              retained->dynamic_fields->definitions().memory_resource() ==
                  fixture.global.memory_resource() &&
              retained->dynamic_fields->values()
                      .at(String("title", fixture.global.memory_resource()))
                      .try_text() == "allocated extension",
          "explicit Global saved graph copy survives actual UserContext deletion");
    auto& managed = fixture.global.make<DynamicFieldValues>(*retained->dynamic_fields);
    check(managed.field("title").value().value()->try_text() == "allocated extension",
          "a checked field loan requires the explicit managed Context carrier");
}

struct RecordingDenyPolicy final : MutationPolicy {
    using allocator_type = Allocator;
    mutable Vector<MutationPlan> plans;

    RecordingDenyPolicy(std::allocator_arg_t, Allocator a) : plans(a.resource()) {}

    Memory memory_resource() const {
        return plans.get_allocator().resource();
    }

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "pending-dynamic", "5.0.6", "fixture");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        plans.emplace_back(plan);
        MutationDecision result(std::allocator_arg, Allocator(r));
        result.verdict = MutationVerdict::Deny;
        result.code.emplace("DYNAMIC-DENIED", r);
        result.message.emplace("extension denied", r);
        result.field_paths.emplace_back("#title");
        return result;
    }
};

static void dynamic_policy_denial() {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto entity = fixture.loaded();
    EntityKey key(std::allocator_arg, Allocator(r), "School", Value(r, std::uint64_t(7)));
    check(bool(entity.root.update_dynamic_field(key, "title", Value(r, "policy operand"))),
          "record dynamic-only policy operand");
    auto& policy = user_context.make_resource<RecordingDenyPolicy>();
    auto& registry = user_context.make<FunctionMutationPolicyRegistry>(
        [&](Memory r, std::string_view request) -> std::optional<MutationPolicyBorrow> {
            check(request == "School.saveGraph", "dynamic save retains canonical request key");
            return MutationPolicyBorrow(
                std::allocator_arg, Allocator(r), policy, detail::resource_arena(r));
        });
    user_context.set_mutation_policy_registry(user_context, registry);
    auto& result = save_audited_ledger_entity(
                       Audited<School>(
                           std::allocator_arg, Allocator(r), entity, "review extension-only write"),
                       user_context)
                       .wait();
    check(!result && policy.plans.size() == 1 && policy.plans.front().operations.size() == 1,
          "policy reviews and can deny an extension-only operation");
    const auto& operation = policy.plans.front().operations.front();
    check(operation.kind == MutationOperationKind::Update && operation.original_version == 3 &&
              operation.changed_values.at(String("#title", r)).to_json_value(r).dump(r) ==
                  "{\"operation\":\"SET\",\"value\":\"policy operand\"}",
          "policy receives source-shaped dynamic intent and loaded original version");
    check(fixture.version() == 3 &&
              fixture.stored().at(7).at(String("title", r)).try_text() == "old" &&
              fixture.sink.events.empty() && entity.root.has_pending_dynamic_mutations() &&
              sqlite3_get_autocommit(fixture.connection->native_handle()) != 0,
          "policy denial leaves native/dynamic storage and pending ledger untouched");
}

int main() {
    for (auto mode : {Mode::Set,
                      Mode::Null,
                      Mode::Delete,
                      Mode::Trigger,
                      Mode::WriteFailure,
                      Mode::ReadFailure,
                      Mode::AuditFailure,
                      Mode::PlainSet}) {
        automatic_save(mode);
    }
    manual_save(false);
    manual_save(true);
    allocated_graph_and_retention();
    dynamic_policy_denial();
    std::cout << "Dynamic graph save contracts passed\n";
}
