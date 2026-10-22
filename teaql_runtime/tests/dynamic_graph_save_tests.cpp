#include "../../tests/support/dynamic_hydration_fixture.hpp"
#include <teaql/runtime/detail/data_service/audited_auto_save.hpp>
#include <teaql/runtime/transaction.hpp>
#include <teaql/runtime/graph_debug.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <sqlite3.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>

using namespace teaql;
using School = dynamic_hydration_fixture::SourceSchool;
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

struct SafeDynamicAuditSink final : SafeAuditEventSink {
    using allocator_type = Allocator;
    mutable Vector<SafeAuditEvent> events;

    SafeDynamicAuditSink(std::allocator_arg_t, Allocator a) : events(a.resource()) {}

    Memory memory_resource() const {
        return events.get_allocator().resource();
    }

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory r, const UserContext&, const SafeAuditEvent& event) const override {
        events.emplace_back(event);
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user_context = global.make_user_context();
    SqliteConnection* connection = nullptr;
    bool source_setup;
    Executor& executor;
    const DynamicFieldDefinitions& definitions;
    Sink& sink;
    SafeDynamicAuditSink* safe_sink = nullptr;

    explicit Fixture(bool source = false, bool seed = true)
        : source_setup(source),
          executor(dynamic_hydration_fixture::setup(*user_context, &connection, false, source)),
          definitions(make_definitions(source)), sink(user_context->make<Sink>()) {
        auto r = user_context->memory_resource();
        if (!source_setup) {
            sql(*connection,
                "CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);INSERT INTO school_data VALUES(7,'original',3)");
        }
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
        if (source_setup) {
            safe_sink = &user_context->make<SafeDynamicAuditSink>();
            user_context->set_custom_event_sink(*user_context, *safe_sink);
            use_sqlite_provider(*user_context, *user_context, executor.transport);
            check(bool(user_context->ensure_schema().wait()),
                  "source atomic fixture initializes modeled SQLite schema");
            sink.events.clear();
            safe_sink->events.clear();
            if (!seed) {
                return;
            }
            auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
            const std::array names{"original", "second", "third"};
            for (std::uint64_t id : {7, 8, 9}) {
                School row(std::allocator_arg, Allocator(r));
                row.id = id;
                row.name = names[id - 7];
                row.mark_as_new();
                row.root.set(row.entity_key(), "name", Value(r, row.name));
                row.load_state = eval::LoadState::fully_loaded(r)
                                     .into_indexed(pool, School::field_layout(r))
                                     .value();
                const auto& saved =
                    AuditedSaveExt<School>::save(
                        Audited<School>(
                            std::allocator_arg, Allocator(r), row, "seed ordinary root records"),
                        *user_context)
                        .wait();
                check(saved && saved.value().version() == 1,
                      "source atomic audited root starts at version one");
            }
            check(safe_sink->events.size() == 3,
                  "source atomic fixture retains three safe seed audits");
            return;
        }
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

    const DynamicFieldDefinitions& make_definitions(bool source) {
        auto r = user_context->memory_resource();
        if (source) {
            const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                                    DynamicFieldDefinitions::Field{"extra", DataType::Text},
                                    DynamicFieldDefinitions::Field{"amount", DataType::F64}};
            return user_context->make<DynamicFieldDefinitions>(
                std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
        }
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
        auto descriptor = dynamic_hydration_fixture::School::entity_descriptor(r);
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

static School read_school(Fixture& fixture, std::optional<std::uint64_t> owner = std::nullopt) {
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto service = user_context.entity_data_service<Executor>("School");
    check(bool(service), "held-view service resolves");
    SelectQuery query(std::allocator_arg, Allocator(r), "School");
    query.project("id").project("version").project("name").order_asc("id").limit(3);
    if (owner) {
        query.and_filter(Expr::eq(r, "id", Value(r, *owner)));
    }
    const std::array fields{DynamicFieldSelection::Field{"title", DataType::Text}};
    query.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, fields)).value());
    query.comment("load typed atomic graph fixture");
    const auto& result =
        service.value()
            .get()
            .fetch_enhanced_entities<School>(PurposedSelectQuery(
                std::allocator_arg, Allocator(r), query, "verify persistence boundaries"))
            .wait();
    check(result && result.value().data.size() == (fixture.source_setup && !owner ? 3 : 1),
          "held-view query reads the expected bounded persisted owners");
    return detail::clone_in(r, result.value().data.front());
}

static void replace_profile(Fixture& fixture,
                            std::string_view name,
                            const DynamicFieldDefinitions& source_definitions) {
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& definitions = user_context.make<DynamicFieldDefinitions>(source_definitions);
    const std::array configured{&definitions};
    using Provider = DatabaseDynamicFieldsProvider<Executor>;
    auto made = Provider::create(r, name, configured);
    check(bool(made), "replacement storage profile validates");
    auto& provider = user_context.make<Provider>(std::move(made).value());
    user_context.set_dynamic_fields_provider(user_context, provider);
}

static void held_view_storage_provenance(bool different_executor) {
    Fixture original(true);
    Fixture other(true);
    original.safe_sink->events.clear();
    other.safe_sink->events.clear();
    auto& user_context = *original.user_context;
    auto r = user_context.memory_resource();
    auto held = read_school(original);
    const std::string_view name =
        different_executor ? "must-not-cross-database" : "must-not-cross-profile";
    held.updateNativeName(name);
    check(bool(held.updateDynamicField(
              "title", Value(r, different_executor ? "private original value" : name))),
          "held view stages native and dynamic changes");
    auto& destination = different_executor ? other : original;
    replace_profile(
        destination, different_executor ? "profile" : "other-profile", original.definitions);
    auto& destination_context = *destination.user_context;
    auto destination_memory = destination_context.memory_resource();
    // Crossing C++ allocation domains is explicit; copied provenance must remain authoritative.
    School candidate(std::allocator_arg, Allocator(destination_memory), held);
    const auto& rejected =
        AuditedSaveExt<School>::save(
            Audited<School>(std::allocator_arg,
                            Allocator(destination_memory),
                            candidate,
                            different_executor
                                ? "reject another physical executor with identical schema"
                                : "reject a held view after storage profile switch"),
            destination_context)
            .wait();
    check(!rejected && rejected.error()
                               .to_string(destination_memory)
                               .find("DYNAMIC_FIELD_STORAGE_PROVENANCE_MISMATCH") != String::npos,
          "held view cannot follow a different storage profile or executor");
    check(read_school(original).name == "original" && read_school(other).name == "original" &&
              original.version() == 1 && other.version() == 1 &&
              original.safe_sink->events.empty() && other.safe_sink->events.empty(),
          "rejection changes neither native database and emits no audit");
    auto unchanged = original.stored();
    check((!unchanged.contains(7) || unchanged.at(7).empty()) &&
              held.root.has_pending_dynamic_mutations(),
          "rejected Save preserves empty extension storage and retry intent");
    if (!different_executor) {
        replace_profile(original, "profile", original.definitions);
    }
    const auto& retried =
        AuditedSaveExt<School>::save(
            Audited<School>(std::allocator_arg,
                            Allocator(r),
                            held,
                            different_executor ? "retry with original context and executor"
                                               : "retry in the original storage profile"),
            user_context)
            .wait();
    if (!retried) {
        throw std::runtime_error(std::string(retried.error().to_string(r)));
    }
    check(read_school(original).name == name && original.version() == 2 &&
              original.safe_sink->events.size() == 1 && read_school(other).name == "original" &&
              other.version() == 1 && other.safe_sink->events.empty(),
          "retry commits only to the original native database");
    check(original.stored().at(7).at(String("title", r)).try_text() ==
              (different_executor ? "private original value" : name),
          "retry preserves the original dynamic operand");
}

static void child_dynamic_union_save() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto service = user_context.entity_data_service<Executor>("School");
    check(bool(service), "union fixture service resolves");
    const std::array title_selection{DynamicFieldSelection::Field{"title", DataType::Text}};
    const std::array extra_selection{DynamicFieldSelection::Field{"extra", DataType::Text}};
    SelectQuery seed_query(std::allocator_arg, Allocator(r), "School");
    seed_query.project("id").project("version").project("name").order_asc("id").limit(3);
    seed_query.select_dynamic_fields(
        std::move(DynamicFieldSelection::fields(r, title_selection)).value());
    seed_query.comment("load typed atomic graph fixture");
    const auto& seed_rows =
        service.value()
            .get()
            .fetch_enhanced_entities<School>(PurposedSelectQuery(
                std::allocator_arg, Allocator(r), seed_query, "verify persistence boundaries"))
            .wait();
    check(seed_rows && seed_rows.value().data.size() == 3, "three seed owners load from SQLite");
    for (std::size_t index = 0; index < 2; ++index) {
        auto row = detail::clone_in(r, seed_rows.value().data[index]);
        check(bool(row.updateDynamicField("title",
                                          index == 0 ? Value(r, "enhanced-private") : Value(r))) &&
                  bool(row.updateDynamicField("extra",
                                              index == 0 ? Value(r, "root-private") : Value(r))),
              "seed native-bound dynamic Value and NULL changes");
        const auto& saved =
            AuditedSaveExt<School>::save(
                Audited<School>(std::allocator_arg,
                                Allocator(r),
                                row,
                                "seed extension merge controls through audited mutation"),
                user_context)
                .wait();
        if (!saved) {
            throw std::runtime_error(std::string(saved.error().to_string(r)));
        }
    }
    SelectQuery child(std::allocator_arg, Allocator(r), "School");
    child.project("id").project("version").project("name").limit(3);
    child.comment("load selected dynamic enhancement");
    child.select_dynamic_fields(
        std::move(DynamicFieldSelection::fields(r, title_selection)).value());
    SelectQuery root(std::allocator_arg, Allocator(r), "School");
    root.project("id").project("version").project("name").order_asc("id").limit(3);
    root.comment("load root plus enhanced extension views");
    root.child_enhancement(child);
    root.select_dynamic_fields(
        std::move(DynamicFieldSelection::fields(r, extra_selection)).value());
    const auto& loaded =
        service.value()
            .get()
            .fetch_enhanced_entities<School>(PurposedSelectQuery(
                std::allocator_arg,
                Allocator(r),
                root,
                "verify private fields and shared actual selection through graph save"))
            .wait();
    check(loaded && loaded.value().data.size() == 3, "root and child selections load three owners");
    const auto& rows = loaded.value().data;
    const std::array states{
        DynamicFieldState::Value, DynamicFieldState::Null, DynamicFieldState::NotLoaded};
    for (std::size_t index = 0; index < rows.size(); ++index) {
        auto* fields = rows[index].dynamic_field_values();
        check(fields && fields->field("title").value().state() == states[index] &&
                  fields->field("extra").value().state() == states[index] &&
                  !rows[index].has_pending_dynamic_mutations(),
              "child union preserves Value/NULL/missing without creating mutation intent");
    }
    check(rows[0].loaded_state_snapshot()->identity() ==
                  rows[1].loaded_state_snapshot()->identity() &&
              rows[0].loaded_state_snapshot()->identity() !=
                  rows[2].loaded_state_snapshot()->identity(),
          "actual dynamic availability determines shared indexed snapshots");
    auto changed = detail::clone_in(r, rows[0]);
    changed.updateNativeName("enhanced-native-save");
    fixture.safe_sink->events.clear();
    const auto& saved =
        AuditedSaveExt<School>::save(
            Audited<School>(std::allocator_arg,
                            Allocator(r),
                            changed,
                            "save only native name while retaining enhanced dynamic view"),
            user_context)
            .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    check(saved.value().name == "enhanced-native-save" && saved.value().version() == 3 &&
              fixture.version() == 3 && fixture.safe_sink->events.size() == 1,
          "native-only Save commits one versioned change");
    auto copy = detail::clone_in(r, saved.value());
    auto json = std::move(copy).into_json(r);
    const auto& object = std::get<Json::Object>(json.data);
    check(object.at(String("#title", r)).dump(r) == "\"enhanced-private\"" &&
              object.at(String("#extra", r)).dump(r) == "\"root-private\"",
          "saved JSON retains both child and root dynamic values");
    const auto& after =
        service.value()
            .get()
            .fetch_enhanced_entities<School>(PurposedSelectQuery(
                std::allocator_arg, Allocator(r), seed_query, "verify persistence boundaries"))
            .wait();
    check(after && after.value().data.size() == 3 && after.value().data[1].name == "second",
          "native-only Save leaves the second owner unchanged");
}

static void selected_dynamic_fluent_save() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto row = read_school(fixture);
    check(bool(row.updateDynamicField("title", Value(r, "fluent"))) &&
              bool(row.updateDynamicField("extra", Value(r, "keep"))),
          "fluent Save stages both dynamic fields");
    const auto& first = AuditedSaveExt<School>::save(
                            Audited<School>(std::allocator_arg,
                                            Allocator(r),
                                            row,
                                            "persist extensions through normal fluent save"),
                            user_context)
                            .wait();
    if (!first) {
        throw std::runtime_error(std::string(first.error().to_string(r)));
    }
    check(first.value().version() == 2 && fixture.version() == 2 &&
              first.value().dynamic_field_values()->field("extra").value().value()->try_text() ==
                  "keep",
          "newly modified sibling is included in authoritative Save result");
    auto selected = read_school(fixture);
    check(selected.dynamic_field_values()->field("extra").value().state() ==
              DynamicFieldState::NotLoaded,
          "selected query omits the persisted sibling");
    check(bool(selected.updateDynamicField("title", Value(r, "changed"))),
          "selected-only update stages");
    const auto& second =
        AuditedSaveExt<School>::save(
            Audited<School>(
                std::allocator_arg, Allocator(r), selected, "change only selected extension"),
            user_context)
            .wait();
    if (!second) {
        throw std::runtime_error(std::string(second.error().to_string(r)));
    }
    check(second.value().version() == 3 && fixture.version() == 3 &&
              second.value().dynamic_field_values()->field("extra").value().state() ==
                  DynamicFieldState::NotLoaded,
          "selected-only Save increments version without pretending sibling was loaded");
    const auto& unchanged =
        AuditedSaveExt<School>::save(Audited<School>(std::allocator_arg,
                                                     Allocator(r),
                                                     second.value(),
                                                     "retain readonly selected extension view"),
                                     user_context)
            .wait();
    if (!unchanged) {
        throw std::runtime_error(std::string(unchanged.error().to_string(r)));
    }
    check(unchanged.value().dynamic_field_values()->field("title").value().value()->try_text() ==
              "changed",
          "unchanged fluent Save retains the selected read-only value");
    auto& copied_fields =
        user_context.make<DynamicFieldValues>(*unchanged.value().dynamic_field_values());
    auto service = user_context.entity_data_service<Executor>("School");
    check(bool(service), "full dynamic read service resolves");
    SelectQuery query(std::allocator_arg, Allocator(r), "School");
    query.project("id").project("version").project("name").order_asc("id").limit(1);
    query.select_dynamic_fields(DynamicFieldSelection::all(r));
    query.comment("read preserved extension siblings");
    const auto& full =
        service.value()
            .get()
            .fetch_enhanced_entities<School>(PurposedSelectQuery(
                std::allocator_arg, Allocator(r), query, "verify NotLoaded did not erase data"))
            .wait();
    check(full && full.value().data.size() == 1 &&
              full.value()
                      .data[0]
                      .dynamic_field_values()
                      ->field("extra")
                      .value()
                      .value()
                      ->try_text() == "keep",
          "bounded typed query proves unselected sibling remains durable");
    check(bool(copied_fields.erase("title")), "copied field container permits local deletion");
    check(unchanged.value().dynamic_field_values()->field("title").value().value()->try_text() ==
              "changed",
          "mutating copied dynamic values cannot alter the original entity view");
}

struct AlternateExecutor : Executor {
    using Executor::Executor;
};

static void separate_provider_transaction_binding() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& transport = user_context.make<SqliteMutationExecutor>(*fixture.connection);
    auto& state = user_context.make<SqlExecutorState>();
    auto& other = user_context.make<AlternateExecutor>(
        fixture.executor.dialect, transport, fixture.executor.schema_provider, state);
    user_context.insert_resource(user_context, other);
    const std::array definitions{&fixture.definitions};
    using Provider = DatabaseDynamicFieldsProvider<AlternateExecutor>;
    auto made = Provider::create(r, "profile", definitions);
    check(bool(made), "separate executor provider configures");
    auto& provider = user_context.make<Provider>(std::move(made).value());
    user_context.set_dynamic_fields_provider(user_context, provider);
    auto row = read_school(fixture);
    row.updateNativeName("must-not-write");
    check(bool(row.updateDynamicField("title", Value(r, "must-not-write"))),
          "separate-provider held view stages native and dynamic changes");
    const auto& rejected =
        AuditedSaveExt<School>::save(
            Audited<School>(
                std::allocator_arg, Allocator(r), row, "reject wrong operation binding"),
            user_context)
            .wait();
    check(!rejected && rejected.error().to_string(r).find(
                           "DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED") != String::npos,
          "separate provider cannot borrow the native transaction");
    check(read_school(fixture).name == "original" && fixture.version() == 1 &&
              read_school(fixture).version() == 1 && row.has_pending_dynamic_mutations() &&
              fixture.safe_sink->events.size() == 3,
          "wrong transaction binding performs no native write or audit and retains intent");
}

static void invalid_readback_rollback_retry() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    sql(*fixture.connection,
        "CREATE TRIGGER corrupt_dynamic AFTER INSERT ON teaql_dynamic_field_storage_v1 "
        "BEGIN UPDATE teaql_dynamic_field_storage_v1 SET payload='not-json' WHERE namespace=NEW.namespace AND owner_type=NEW.owner_type AND owner_id=NEW.owner_id AND code=NEW.code; END");
    auto row = read_school(fixture);
    check(row.dynamic_field_values()->field("title").value().state() ==
              DynamicFieldState::NotLoaded,
          "rollback fixture starts without the selected extension");
    row.updateNativeName("must-roll-back");
    check(bool(row.updateDynamicField("title", Value(r, "retry"))),
          "readback failure fixture stages both writes");
    const auto& rejected =
        AuditedSaveExt<School>::save(
            Audited<School>(
                std::allocator_arg, Allocator(r), row, "reject malformed authoritative readback"),
            user_context)
            .wait();
    check(!rejected &&
              rejected.error().to_string(r).find("DYNAMIC_FIELD_INVALID_STORAGE") != String::npos,
          "malformed authoritative readback reports exact storage error");
    auto loaded = read_school(fixture);
    check(loaded.name == "original" && loaded.version() == 1 && fixture.version() == 1 &&
              read_school(fixture).version() == 1 &&
              loaded.dynamic_field_values()->field("title").value().state() ==
                  DynamicFieldState::NotLoaded &&
              fixture.safe_sink->events.size() == 3 && row.has_pending_dynamic_mutations(),
          "readback failure rolls back both stores without audit and keeps original intent");
    sql(*fixture.connection, "DROP TRIGGER corrupt_dynamic");
    const auto& retried =
        AuditedSaveExt<School>::save(
            Audited<School>(
                std::allocator_arg, Allocator(r), row, "retry after correcting storage behavior"),
            user_context)
            .wait();
    if (!retried) {
        throw std::runtime_error(std::string(retried.error().to_string(r)));
    }
    auto persisted = read_school(fixture);
    check(retried.value().version() == 2 && persisted.version() == 2 && fixture.version() == 2 &&
              persisted.name == "must-roll-back" &&
              persisted.dynamic_field_values()->field("title").value().value()->try_text() ==
                  "retry" &&
              fixture.safe_sink->events.size() == 4,
          "corrected storage commits retained native and dynamic intent once");
}

static void invalid_dynamic_preflight() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                            DynamicFieldDefinitions::Field{"extra", DataType::Text},
                            DynamicFieldDefinitions::Field{"amount", DataType::F64}};
    auto& writes = fixture.global.make<unsigned>(0);
    check(sqlite3_create_function(
              fixture.connection->native_handle(),
              "observe_invalid_update",
              0,
              SQLITE_UTF8,
              &writes,
              [](sqlite3_context* context, int, sqlite3_value**) {
                  ++*static_cast<unsigned*>(sqlite3_user_data(context));
                  sqlite3_result_int(context, 1);
              },
              nullptr,
              nullptr) == SQLITE_OK,
          "register native write observer");
    sql(*fixture.connection,
        "CREATE TRIGGER observe_invalid_update AFTER UPDATE ON school_data BEGIN SELECT observe_invalid_update(); END");
    auto row = read_school(fixture);
    row.updateNativeName("must-not-write");
    check(
        bool(row.updateDynamicField("amount", Value(r, std::numeric_limits<double>::quiet_NaN()))),
        "nonfinite value is staged for governed Save validation");
    const auto& nonfinite = AuditedSaveExt<School>::save(
                                Audited<School>(std::allocator_arg,
                                                Allocator(r),
                                                row,
                                                "reject nonfinite extension batch before writes"),
                                user_context)
                                .wait();
    check(!nonfinite &&
              nonfinite.error().to_string(r).find("DYNAMIC_FIELD_NONFINITE_NUMBER") !=
                  String::npos &&
              read_school(fixture).name == "original" && writes == 0,
          "nonfinite extension rejects before native DML");
    auto newer = DynamicFieldDefinitions::from_fields(r, "School", "v2", fields);
    check(bool(newer), "newer definition revision validates");
    replace_profile(fixture, "profile", newer.value());
    const auto& stale =
        AuditedSaveExt<School>::save(
            Audited<School>(
                std::allocator_arg, Allocator(r), row, "reject stale extension definition view"),
            user_context)
            .wait();
    check(!stale &&
              stale.error().to_string(r).find("DYNAMIC_FIELD_DEFINITION_MISMATCH") !=
                  String::npos &&
              read_school(fixture).name == "original" && fixture.version() == 1 &&
              read_school(fixture).version() == 1 && fixture.safe_sink->events.size() == 3 &&
              writes == 0,
          "stale definitions take precedence over pending NaN without writes or audit");
}

struct RequireLoadedName final : TypedChecker<School> {
    void check_and_fix_typed(const UserContext& user_context,
                             School& entity,
                             CheckObjectStatus,
                             const ObjectLocation& location,
                             CheckResults& results) const override {
        if (!entity.is_field_loaded("name")) {
            auto r = user_context.memory_resource();
            results.emplace_back(
                CheckResult::required(r, detail::clone_in(r, location).member("name")));
        }
    }
};

static void incomplete_projection_dynamic_save() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& typed = user_context.make_resource<RequireLoadedName>();
    auto& checker =
        user_context.make<TypedEntityChecker<School, RequireLoadedName>>(user_context, typed);
    auto& registry = user_context.make<InMemoryCheckerRegistry>();
    registry.register_checker(user_context, checker);
    user_context.set_checker_registry(user_context, registry);
    auto service = user_context.entity_data_service<Executor>("School");
    check(bool(service), "incomplete projection service resolves");
    SelectQuery query(std::allocator_arg, Allocator(r), "School");
    query.project("id").project("version").order_asc("id").limit(1);
    const std::array fields{DynamicFieldSelection::Field{"title", DataType::Text}};
    query.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, fields)).value());
    query.comment("load deliberately incomplete native projection");
    const auto& loaded = service.value()
                             .get()
                             .fetch_enhanced_entities<School>(
                                 PurposedSelectQuery(std::allocator_arg,
                                                     Allocator(r),
                                                     query,
                                                     "verify dynamic edits do not weaken checker"))
                             .wait();
    check(loaded && loaded.value().data.size() == 1, "incomplete typed projection loads");
    auto row = detail::clone_in(r, loaded.value().data.front());
    check(!row.is_field_loaded("name"), "projection retains explicit missing native field");
    check(bool(row.updateDynamicField("title", Value(r, "blocked"))),
          "dynamic-only mutation can be staged on the partial view");
    const auto& rejected =
        AuditedSaveExt<School>::save(
            Audited<School>(
                std::allocator_arg, Allocator(r), row, "reject edit without complete object"),
            user_context)
            .wait();
    check(!rejected && rejected.error().kind() == RuntimeError::Kind::Check &&
              row.has_pending_dynamic_mutations() && fixture.version() == 1 &&
              read_school(fixture).version() == 1 && fixture.safe_sink->events.size() == 3,
          "dynamic-only Save cannot bypass full-object Checker requirements");
}

static void incompatible_dynamic_composition() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto target = read_school(fixture);
    auto source = read_school(fixture);
    check(bool(target.updateDynamicField("title", Value(r, "target"))),
          "receiving typed row stages its original intent");
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                            DynamicFieldDefinitions::Field{"extra", DataType::Text},
                            DynamicFieldDefinitions::Field{"amount", DataType::F64}};
    auto newer = DynamicFieldDefinitions::from_fields(r, "School", "other-revision", fields);
    check(bool(newer), "incompatible source definitions are individually valid");
    Value::Object empty(r);
    auto& definitions = user_context.make<DynamicFieldDefinitions>(newer.value());
    auto values = DynamicFieldValues::from_values(r, definitions, empty);
    check(bool(values), "construct empty source payload with the newer revision");
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    auto snapshot = LoadedSnapshot::with_dynamic_fields(
        pool, source.loaded_state_snapshot().value(), values.value());
    check(snapshot && bool(source.install_loaded_dynamic_fields(values.value(), snapshot.value())),
          "install source revision on the actual typed query result");
    source.updateNativeName("must-not-import");
    check(bool(source.updateDynamicField("title", Value(r, "source"))),
          "source stages native and dynamic operands");
    auto before = target.root.current_change_set();
    auto composed = LedgerEntityDefaults<School>::compose(target, source);
    check(!composed &&
              composed.error().kind ==
                  LedgerCompositionError::Kind::ConflictingDynamicDefinitions &&
              target.root.current_change_set() == before,
          "definition conflict rejects composition without partially importing either operand");
    check(read_school(fixture).name == "original" && fixture.version() == 1 &&
              read_school(fixture).version() == 1 && fixture.safe_sink->events.size() == 3,
          "rejected composition performs no persistence or audit delivery");
}

static void composed_dynamic_graph_rollback() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto load = [&]() {
        auto service = user_context.entity_data_service<Executor>("School");
        check(bool(service), "composed graph service resolves");
        SelectQuery query(std::allocator_arg, Allocator(r), "School");
        query.project("id").project("version").project("name").order_asc("id").limit(3);
        const std::array fields{DynamicFieldSelection::Field{"title", DataType::Text}};
        query.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, fields)).value());
        query.comment("load explicit graph boundary");
        const auto& loaded =
            service.value()
                .get()
                .fetch_enhanced_entities<School>(PurposedSelectQuery(
                    std::allocator_arg,
                    Allocator(r),
                    query,
                    "verify atomic graph persistence and unrelated intent isolation"))
                .wait();
        check(loaded && loaded.value().data.size() == 3, "three typed owners load");
        Vector<School> rows(r);
        for (const auto& row : loaded.value().data) {
            rows.emplace_back(row);
        }
        return rows;
    };
    auto rows = load();
    auto& parent = rows[0];
    auto& child = rows[1];
    auto& unrelated = rows[2];
    parent.updateNativeName("parent-pending");
    child.updateNativeName("child-pending");
    check(bool(parent.updateDynamicField("title", Value(r, "parent-extension"))) &&
              bool(child.updateDynamicField("title", Value(r, "child-extension"))) &&
              bool(unrelated.updateDynamicField("title", Value(r, "unrelated"))),
          "three independently loaded owners stage dynamic intent");
    check(bool(LedgerEntityDefaults<School>::compose(parent, child)),
          "public ledger protocol explicitly composes the child");
    sql(*fixture.connection,
        "CREATE TRIGGER fail_second_dynamic BEFORE INSERT ON teaql_dynamic_field_storage_v1 "
        "WHEN NEW.owner_id='8' BEGIN SELECT RAISE(ABORT,'reject second entity'); END");
    const auto& rejected =
        AuditedSaveExt<School>::save(
            Audited<School>(
                std::allocator_arg, Allocator(r), parent, "rollback composed multi-entity graph"),
            user_context)
            .wait();
    check(!rejected, "second owner failure rejects the composed Save");
    auto rolled_back = load();
    for (const auto& row : rolled_back) {
        check(row.version() == 1 && row.root.get_original_version(row.entity_key()) == 1 &&
                  row.dynamic_field_values()->field("title").value().state() ==
                      DynamicFieldState::NotLoaded,
              "both native and dynamic writes roll back for every owner");
    }
    check(rolled_back[0].name == "original" && rolled_back[1].name == "second" &&
              fixture.safe_sink->events.size() == 3 && parent.has_pending_dynamic_mutations() &&
              unrelated.has_pending_dynamic_mutations(),
          "rollback preserves native names and pending intent without delivering audits");
    sql(*fixture.connection, "DROP TRIGGER fail_second_dynamic");
    const auto& saved =
        AuditedSaveExt<School>::save(
            Audited<School>(
                std::allocator_arg, Allocator(r), parent, "retry the explicit graph boundary only"),
            user_context)
            .wait();
    check(saved && saved.value().version() == 2 &&
              saved.value().root.get_original_version(saved.value().entity_key()) == 2,
          "unchanged composed intent can retry");
    auto persisted = load();
    check(persisted[0].version() == 2 &&
              persisted[0].root.get_original_version(persisted[0].entity_key()) == 2 &&
              persisted[1].version() == 2 &&
              persisted[1].root.get_original_version(persisted[1].entity_key()) == 2 &&
              persisted[2].version() == 1 &&
              persisted[2].root.get_original_version(persisted[2].entity_key()) == 1 &&
              persisted[0].name == "parent-pending" && persisted[1].name == "child-pending" &&
              persisted[2].name == "third" &&
              persisted[0].dynamic_field_values()->field("title").value().value()->try_text() ==
                  "parent-extension" &&
              persisted[1].dynamic_field_values()->field("title").value().value()->try_text() ==
                  "child-extension" &&
              persisted[2].dynamic_field_values()->field("title").value().state() ==
                  DynamicFieldState::NotLoaded &&
              unrelated.has_pending_dynamic_mutations() && fixture.safe_sink->events.size() == 5,
          "retry persists exactly the explicit graph and leaves unrelated Context intent pending");
}

static void dynamic_dml_failure_retry() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto row = read_school(fixture);
    row.updateNativeName("native-pending");
    check(bool(row.updateDynamicField("title", Value(r, "private-pending"))),
          "DML failure fixture stages native and dynamic intent");
    sql(*fixture.connection,
        "CREATE TRIGGER fail_dynamic_write BEFORE INSERT ON teaql_dynamic_field_storage_v1 "
        "BEGIN SELECT RAISE(ABORT,'PRIVATE-DRIVER-CANARY'); END");
    const auto& failed =
        save_audited_ledger_entity(
            Audited<School>(std::allocator_arg, Allocator(r), row, "rollback both parts"),
            user_context)
            .wait();
    check(!failed, "dynamic write failure rejects ledger Save");
    auto diagnostic = failed.error().to_string(r);
    check(diagnostic.find("DYNAMIC_FIELD_STORAGE_FAILED") != String::npos &&
              diagnostic.find("PRIVATE-DRIVER-CANARY") == String::npos &&
              !row.root.current_change_set().is_empty(),
          "storage error masks driver text and preserves pending intent");
    auto loaded = read_school(fixture);
    check(loaded.name == "original" && loaded.version() == 1 &&
              loaded.root.get_original_version(loaded.entity_key()) == 1 &&
              loaded.dynamic_field_values()->field("title").value().state() ==
                  DynamicFieldState::NotLoaded &&
              fixture.safe_sink->events.size() == 3,
          "failed extension DML rolls back native changes and audit delivery");
    sql(*fixture.connection, "DROP TRIGGER fail_dynamic_write");
    const auto& saved =
        save_audited_ledger_entity(
            Audited<School>(std::allocator_arg, Allocator(r), row, "retry original intents"),
            user_context)
            .wait();
    check(saved && saved.value().name == "native-pending" && saved.value().version() == 2 &&
              saved.value().root.get_original_version(saved.value().entity_key()) == 2 &&
              saved.value().dynamic_field_values()->field("title").value().value()->try_text() ==
                  "private-pending" &&
              fixture.safe_sink->events.size() == 4,
          "retry commits the original native and dynamic operands once");
}

static void dynamic_stale_version_isolation() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto current = read_school(fixture, 7);
    auto stale = read_school(fixture, 7);
    auto unrelated = read_school(fixture, 8);
    check(bool(unrelated.updateDynamicField("title", Value(r, "unrelated-pending"))) &&
              bool(current.updateDynamicField("title", Value(r, "current"))),
          "independent owners stage separate extension-only intents");
    const auto& saved =
        save_audited_ledger_entity(
            Audited<School>(
                std::allocator_arg, Allocator(r), current, "advance native optimistic version"),
            user_context)
            .wait();
    check(saved && saved.value().version() == 2 &&
              saved.value().root.get_original_version(saved.value().entity_key()) == 2,
          "extension-only Save advances the native optimistic version");
    check(bool(stale.updateDynamicField("title", Value(r, "stale"))),
          "stale loaded view can stage intent before authoritative validation");
    const auto& rejected =
        save_audited_ledger_entity(
            Audited<School>(
                std::allocator_arg, Allocator(r), stale, "reject stale extension-only write"),
            user_context)
            .wait();
    check(!rejected, "stale extension-only Save rejects");
    auto persisted = read_school(fixture, 7);
    auto untouched = read_school(fixture, 8);
    check(persisted.dynamic_field_values()->field("title").value().value()->try_text() ==
                  "current" &&
              persisted.version() == 2 &&
              persisted.root.get_original_version(persisted.entity_key()) == 2 &&
              untouched.version() == 1 &&
              untouched.root.get_original_version(untouched.entity_key()) == 1 &&
              untouched.dynamic_field_values()->field("title").value().state() ==
                  DynamicFieldState::NotLoaded &&
              stale.has_pending_dynamic_mutations() && unrelated.has_pending_dynamic_mutations() &&
              fixture.safe_sink->events.size() == 4,
          "stale rejection preserves the current value and excludes unrelated Context intent");
}

static void new_dynamic_entity_provenance(bool bound) {
    Fixture fixture(true, false);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& generator = user_context.make<AtomicCounterIdGenerator>(100);
    user_context.set_internal_id_generator(user_context, generator);
    const DynamicFieldDefinitions* definitions = &fixture.definitions;
    if (bound) {
        const std::array configured{definitions};
        using Provider = DatabaseDynamicFieldsProvider<Executor>;
        auto made = Provider::create(r, "profile", configured);
        check(bool(made), "fresh provider configuration validates");
        auto& provider = user_context.make<Provider>(std::move(made).value());
        auto resolved = provider.definitions_for_context(user_context, "School");
        check(bool(resolved), "new entity definitions bind to the actual Context storage");
        definitions = &resolved.value().get();
        sql(*fixture.connection,
            "CREATE TRIGGER normalize_dynamic AFTER INSERT ON teaql_dynamic_field_storage_v1 "
            "BEGIN UPDATE teaql_dynamic_field_storage_v1 SET payload=UPPER(NEW.payload) "
            "WHERE namespace=NEW.namespace AND owner_type=NEW.owner_type AND owner_id=NEW.owner_id AND code=NEW.code; END");
    }
    School row(std::allocator_arg, Allocator(r));
    row.name = "school-0";
    row.mark_as_new();
    row.root.set(row.entity_key(), "name", Value(r, row.name));
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    row.load_state =
        eval::LoadState::fully_loaded(r).into_indexed(pool, School::field_layout(r)).value();
    Value::Object empty(r);
    auto fields = DynamicFieldValues::from_values(r, *definitions, empty);
    check(bool(fields), "new entity accepts individually valid dynamic definitions");
    auto snapshot = LoadedSnapshot::with_dynamic_fields(
        pool, row.loaded_state_snapshot().value(), fields.value());
    check(snapshot && bool(row.install_loaded_dynamic_fields(fields.value(), snapshot.value())),
          "install the new entity dynamic snapshot");
    check(bool(row.updateDynamicField("title", Value(r, bound ? "canonicalize" : "unbound value"))),
          "new entity stages dynamic intent before ID allocation");
    if (!bound) {
        const auto& rejected =
            AuditedSaveExt<School>::save(
                Audited<School>(
                    std::allocator_arg, Allocator(r), row, "reject unbound definition source"),
                user_context)
                .wait();
        check(!rejected && rejected.error().to_string(r).find(
                               "DYNAMIC_FIELD_STORAGE_PROVENANCE_MISMATCH") != String::npos,
              "unbound definitions are never silently assigned to current storage");
        CompiledQuery query(std::allocator_arg, Allocator(r));
        query.sql = "SELECT id FROM school_data";
        const auto& loaded = fixture.executor.transport.fetch_all_compact_sql(r, query).wait();
        check(loaded && loaded.value().empty() && row.has_pending_dynamic_mutations() &&
                  fixture.safe_sink->events.empty(),
              "unbound rejection leaves storage empty, intent pending and audit absent");
        auto service = user_context.entity_data_service<Executor>("School");
        check(bool(service), "unbound rejection retains the ordinary typed service");
        SelectQuery typed_query(std::allocator_arg, Allocator(r), "School");
        typed_query.project("id").project("version").project("name").order_asc("id").limit(3);
        const std::array selected{DynamicFieldSelection::Field{"title", DataType::Text}};
        typed_query.select_dynamic_fields(
            std::move(DynamicFieldSelection::fields(r, selected)).value());
        typed_query.comment("read roots after rejecting unbound new definitions");
        const auto& typed_rows =
            service.value()
                .get()
                .fetch_enhanced_entities<School>(PurposedSelectQuery(
                    std::allocator_arg,
                    Allocator(r),
                    typed_query,
                    "verify rejected creation leaves the typed root result empty"))
                .wait();
        check(typed_rows && typed_rows.value().data.empty(),
              "unbound new-entity rejection leaves the enhanced typed query empty");
        return;
    }
    const auto& saved =
        save_audited_ledger_entity(Audited<School>(std::allocator_arg,
                                                   Allocator(r),
                                                   row,
                                                   "create typed entity and extensions together"),
                                   user_context)
            .wait();
    check(saved && saved.value().id >= 100 && saved.value().version() == 1 &&
              saved.value().root.get_original_version(saved.value().entity_key()) == 1 &&
              saved.value().dynamic_field_values()->field("title").value().value()->try_text() ==
                  "CANONICALIZE",
          "allocated typed entity returns the authoritative normalized extension");
    auto persisted = read_school(fixture, saved.value().id);
    check(persisted.dynamic_field_values()->field("title").value().value()->try_text() ==
                  "CANONICALIZE" &&
              fixture.safe_sink->events.size() == 1,
          "allocated identity owns the persisted extension and commits one audit");
}

static void dynamic_null_delete_safe_audit() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& sink = *fixture.safe_sink;
    auto no_private_audit = [&]() {
        for (const auto& event : sink.events) {
            if (debug_string(r, event).find("PRIVATE-DYNAMIC-CANARY") != String::npos) {
                return false;
            }
        }
        return true;
    };
    auto row = read_school(fixture);
    row.updateNativeName("native-updated");
    check(row.name == "native-updated" && !row.root.current_change_set().is_empty(),
          "native helper updates the typed field and ledger before Save");
    check(bool(row.updateDynamicField("title", Value(r, "PRIVATE-DYNAMIC-CANARY"))),
          "stage native and sensitive dynamic values");
    const auto& saved =
        save_audited_ledger_entity(
            Audited<School>(
                std::allocator_arg, Allocator(r), row, "save PRIVATE-DYNAMIC-CANARY graph"),
            user_context)
            .wait();
    check(saved && saved.value().name == "native-updated" && saved.value().version() == 2 &&
              saved.value().root.get_original_version(saved.value().entity_key()) == 2 &&
              saved.value().dynamic_field_values()->field("title").value().value()->try_text() ==
                  "PRIVATE-DYNAMIC-CANARY" &&
              saved.value().root.current_change_set().is_empty() && sink.events.size() == 4,
          "ledger Save commits the value and clears pending intent before safe audit delivery");
    bool masked = false;
    for (const auto& field : sink.events[3].fields) {
        if (field.name == "#title" && field.masked) {
            masked = true;
        }
    }
    check(masked && no_private_audit(),
          "safe audit masks dynamic values and sensitive reason text");
    auto cleared = detail::clone_in(r, saved.value());
    auto selected = cleared.loaded_state_snapshot().value();
    check(bool(cleared.updateDynamicField("title", Value(r))) &&
              cleared.loaded_state_snapshot()->identity() == selected.identity(),
          "NULL mutation preserves the loaded selection snapshot");
    const auto& null_saved =
        save_audited_ledger_entity(Audited<School>(std::allocator_arg,
                                                   Allocator(r),
                                                   cleared,
                                                   "clear prior PRIVATE-DYNAMIC-CANARY value"),
                                   user_context)
            .wait();
    check(null_saved && null_saved.value().version() == 3 &&
              null_saved.value().root.get_original_version(null_saved.value().entity_key()) == 3 &&
              null_saved.value().dynamic_field_values()->field("title").value().state() ==
                  DynamicFieldState::Null &&
              sink.events.size() == 5 && no_private_audit(),
          "NULL persists distinctly while prior sensitive values remain absent from safe audit");
    auto removed = detail::clone_in(r, null_saved.value());
    check(bool(removed.deleteDynamicField("title")), "stage explicit extension deletion");
    const auto& deleted =
        save_audited_ledger_entity(Audited<School>(std::allocator_arg,
                                                   Allocator(r),
                                                   removed,
                                                   "delete extension rather than persist NULL"),
                                   user_context)
            .wait();
    check(
        deleted && deleted.value().version() == 4 &&
            deleted.value().root.get_original_version(deleted.value().entity_key()) == 4 &&
            deleted.value().dynamic_field_values()->field("title").value().state() ==
                DynamicFieldState::NotLoaded &&
            read_school(fixture).dynamic_field_values()->field("title").value().state() ==
                DynamicFieldState::NotLoaded &&
            sink.events.size() == 6 && no_private_audit(),
        "Delete removes the stored extension instead of persisting NULL and retains safe audit privacy");
}

static void audited_roots_shared_dynamic_masks() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& executor = dynamic_hydration_fixture::setup(*user_context, nullptr, false, true);
    use_sqlite_provider(*user_context, *user_context, executor.transport);
    check(bool(user_context->ensure_schema().wait()), "modeled SQLite root schema initializes");
    user_context->register_executor(*user_context, executor);
    user_context->disable_sql_log();
    auto& sink = user_context->make<SafeDynamicAuditSink>();
    user_context->set_custom_event_sink(*user_context, sink);
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    for (std::uint64_t id : {1, 2, 3}) {
        School row(std::allocator_arg, Allocator(r));
        row.id = id;
        row.name = "school-";
        row.name += std::to_string(id);
        row.mark_as_new();
        row.root.set(row.entity_key(), "name", Value(r, row.name));
        row.load_state =
            eval::LoadState::fully_loaded(r).into_indexed(pool, School::field_layout(r)).value();
        const auto& saved = AuditedSaveExt<School>::save(
                                Audited<School>(std::allocator_arg,
                                                Allocator(r),
                                                row,
                                                "seed bounded dynamic-field query fixture"),
                                *user_context)
                                .wait();
        check(saved && saved.value().version() == 1 &&
                  saved.value().root.get_original_version(saved.value().entity_key()) == 1,
              "audited root seed starts at version one");
    }
    check(sink.events.size() == 3, "three root inserts deliver three safe audits");
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text}};
    auto& definitions = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "extension-v1", fields))
            .value());
    DynamicStorageRows values(r);
    values.try_emplace(1).first->second.emplace(String("title", r), Value(r, "private-note"));
    values.try_emplace(2).first->second.emplace(String("title", r), Value(r));
    const std::array owners{InMemoryDynamicFieldsProvider::Owner{&definitions, &values}};
    auto made = InMemoryDynamicFieldsProvider::from_owners(r, owners);
    check(bool(made), "in-memory extension provider configures after native root seeding");
    auto& provider = user_context->make<InMemoryDynamicFieldsProvider>(std::move(made).value());
    user_context->set_dynamic_fields_provider(*user_context, provider);
    SelectQuery query(std::allocator_arg, Allocator(r), "School");
    query.project("id").project("version").project("name").order_asc("id").limit(3);
    const std::array selected{DynamicFieldSelection::Field{"title", DataType::Text}};
    query.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, selected)).value());
    query.comment("load stored roots and dynamic availability");
    auto service = user_context->entity_data_service<Executor>("School");
    check(bool(service), "seeded root service resolves");
    const auto& loaded = service.value()
                             .get()
                             .fetch_enhanced_entities<School>(PurposedSelectQuery(
                                 std::allocator_arg,
                                 Allocator(r),
                                 query,
                                 "verify SQLite query carrier and shared masks"))
                             .wait();
    check(loaded && loaded.value().data.size() == 3, "typed query returns all three audited roots");
    const auto& rows = loaded.value().data;
    check(rows[0].loaded_state_snapshot()->identity() ==
                  rows[1].loaded_state_snapshot()->identity() &&
              rows[1].loaded_state_snapshot()->identity() !=
                  rows[2].loaded_state_snapshot()->identity(),
          "Value and NULL share a selected mask while missing uses a distinct mask");
    check(rows[0].dynamic_field_values()->field("title").value().value()->try_text() ==
                  "private-note" &&
              rows[1].dynamic_field_values()->field("title").value().state() ==
                  DynamicFieldState::Null &&
              rows[2].dynamic_field_values()->field("title").value().state() ==
                  DynamicFieldState::NotLoaded &&
              rows[0].name == "school-1",
          "native SQLite roots retain row-owned Value, NULL and NotLoaded extension states");
    for (const auto& row : rows) {
        check(row.version() == 1 && row.root.get_original_version(row.entity_key()) == 1 &&
                  !row.dirty_fields(),
              "enhanced audited roots retain version one without introducing dirty fields");
    }
}

struct OwnedDynamicDatabase {
    std::filesystem::path directory;

    OwnedDynamicDatabase() {
        auto pattern =
            (std::filesystem::temp_directory_path() / "teaql_dynamic_reopen_XXXXXX").string();
        auto* created = mkdtemp(pattern.data());
        if (!created) {
            throw std::runtime_error("cannot create owned dynamic database directory");
        }
        directory = created;
    }

    ~OwnedDynamicDatabase() {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }
};

static void durable_dynamic_reopen() {
    OwnedDynamicDatabase temporary;
    const auto filename = (temporary.directory / "database.sqlite").string();
    GlobalContext global;
    auto configure = [&](UserContext& user_context) -> Executor& {
        auto r = user_context.memory_resource();
        auto& metadata = user_context.make<InMemoryMetadataStore>();
        metadata.register_entity(School::entity_descriptor(r));
        auto& registry = user_context.make<InMemoryEntityRegistry>();
        registry.register_entity("School");
        user_context.set_metadata(user_context, metadata);
        user_context.set_entity_registry(user_context, registry);
        auto& connection = user_context.make_resource<SqliteConnection>(filename);
        auto& transport = user_context.make<SqliteMutationExecutor>(connection);
        use_sqlite_provider(user_context, user_context, transport);
        auto& dialect = user_context.make_resource<SqliteDialect>();
        auto& state = user_context.make<SqlExecutorState>();
        auto& executor = user_context.make<Executor>(dialect, transport, metadata, state);
        user_context.register_executor(user_context, executor);
        const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text}};
        auto& definitions = user_context.make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(r, "School", "extension-v1", fields))
                .value());
        const std::array configured{&definitions};
        using Provider = DatabaseDynamicFieldsProvider<Executor>;
        auto made = Provider::create(r, "school-demo", configured);
        check(bool(made), "durable provider configures");
        auto& provider = user_context.make<Provider>(std::move(made).value());
        user_context.set_dynamic_fields_provider(user_context, provider);
        user_context.disable_sql_log();
        return executor;
    };
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& executor = configure(*user_context);
        check(bool(user_context->ensure_schema().wait()) &&
                  bool(user_context->ensure_schema().wait()),
              "durable native and extension schema setup is repeatable");
        auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
        for (std::uint64_t id : {1, 2, 3}) {
            School row(std::allocator_arg, Allocator(r));
            row.id = id;
            row.name = "school-";
            row.name += std::to_string(id);
            row.mark_as_new();
            row.root.set(row.entity_key(), "name", Value(r, row.name));
            row.load_state = eval::LoadState::fully_loaded(r)
                                 .into_indexed(pool, School::field_layout(r))
                                 .value();
            const auto& saved =
                AuditedSaveExt<School>::save(
                    Audited<School>(std::allocator_arg,
                                    Allocator(r),
                                    row,
                                    "seed root through ordinary audited graph save"),
                    *user_context)
                    .wait();
            check(saved && saved.value().version() == 1 &&
                      saved.value().root.get_original_version(saved.value().entity_key()) == 1,
                  "durable root is seeded by audited Save");
        }
        const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text}};
        auto& definitions = user_context->make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(r, "School", "extension-v1", fields))
                .value());
        Vector<DynamicFieldWrite> writes(r);
        for (std::uint64_t id : {1, 2}) {
            DynamicFieldWrite::Changes changes(r);
            changes.emplace(
                String("title", r),
                DynamicFieldMutation::set(r, id == 1 ? Value(r, "stored-value") : Value(r)));
            writes.emplace_back("school-demo", definitions, id, changes);
        }
        auto& transaction = executor.begin(r).wait().value().get();
        check(
            bool(transaction.dynamic_field_store()
                     ->apply_writes(
                         r,
                         writes,
                         MutationIntent::create(r, "prepare persistent extension fixture").value())
                     .wait()),
            "transaction-owned SPI seeds persistent extension fixture");
        check(bool(transaction.commit(r).wait()), "persistent extension transaction commits");
    }
    School* retained = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        configure(*user_context);
        check(bool(user_context->ensure_schema().wait()),
              "fresh Context reopens existing native and extension schema");
        SelectQuery query(std::allocator_arg, Allocator(r), "School");
        query.project("id").project("version").project("name").order_asc("id").limit(3);
        const std::array fields{DynamicFieldSelection::Field{"title", DataType::Text}};
        query.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, fields)).value());
        query.comment("read reopened database and stored extensions");
        auto service = user_context->entity_data_service<Executor>("School");
        check(bool(service), "reopened service resolves");
        const auto& loaded = service.value()
                                 .get()
                                 .fetch_enhanced_entities<School>(PurposedSelectQuery(
                                     std::allocator_arg,
                                     Allocator(r),
                                     query,
                                     "verify durable values and shared load-state geometry"))
                                 .wait();
        check(loaded && loaded.value().data.size() == 3, "reopened database retains three roots");
        const auto& rows = loaded.value().data;
        check(rows[0].dynamic_field_values()->field("title").value().value()->try_text() ==
                      "stored-value" &&
                  rows[1].dynamic_field_values()->field("title").value().state() ==
                      DynamicFieldState::Null &&
                  rows[2].dynamic_field_values()->field("title").value().state() ==
                      DynamicFieldState::NotLoaded,
              "durable extension Value, NULL and missing survive reopening");
        check(rows[0].loaded_state_snapshot()->identity() ==
                      rows[1].loaded_state_snapshot()->identity() &&
                  rows[1].loaded_state_snapshot()->identity() !=
                      rows[2].loaded_state_snapshot()->identity() &&
                  &rows[0].dynamic_field_values()->definitions() ==
                      &rows[2].dynamic_field_values()->definitions(),
              "reopened typed rows share selection geometry and definition identity");
        for (const auto& row : rows) {
            check(std::string_view(row.name) ==
                          std::string_view("school-" + std::to_string(row.id)) &&
                      row.version() == 1 && row.root.get_original_version(row.entity_key()) == 1,
                  "reopened native names and versions retain audited seed values");
        }
        retained = &global.make<School>(rows[0]);
        check(retained->dynamic_field_values()->memory_resource() == global.memory_resource() &&
                  retained->dynamic_field_values()->definitions().memory_resource() ==
                      global.memory_resource(),
              "retained typed row explicitly copies nested data into GlobalContext");
    }
    check(retained->dynamic_field_values()->field("title").value().value()->try_text() ==
              "stored-value",
          "explicit Global copy remains valid after reopened UserContext destruction");
}

static void source_layout_contract() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const auto& source = School::field_layout(r);
    const auto& frozen = dynamic_hydration_fixture::School::field_layout(r);
    check(source.field_count() == 3 && source.index("id") == 0 && source.index("version") == 1 &&
              source.index("name") == 2 && source.revision() == "dynamic-school-sqlite-v1" &&
              !source.index("platform"),
          "source carrier retains exactly the original indexed field geometry");
    check(frozen.field_count() == 11 && frozen.index("name") == 3 &&
              frozen.index("version") == 10 && &source != &frozen,
          "source layout does not replace the frozen example carrier or its Context registration");
}

static void source_decode_validation_contract() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array fields{DynamicPropertyDefinitions::Field{"_metric", DataType::I64}};
    auto& definitions = user_context->make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, fields)).value());
    for (unsigned mode = 0; mode < 5; ++mode) {
        Record values(r);
        values.emplace(String("id", r),
                       mode == 2 || mode == 3 ? Value(r, "invalid-native")
                                              : Value(r, std::uint64_t(7)));
        values.emplace(String("version", r), Value(r, std::int64_t(1)));
        values.emplace(String("name", r), Value(r, "source-name"));
        values.emplace(String("_metric", r),
                       mode == 1 || mode == 2 ? Value(r, "PRIVATE-DECODE-CANARY")
                       : mode == 4            ? Value(r)
                                              : Value(r, std::int64_t(9)));
        std::array rows{CompactRow(std::allocator_arg, Allocator(r), values)};
        CompactRow::attach_dynamic_property_definitions(rows, definitions);
        auto direct = School::from_compact_row(r, detail::clone_in(r, rows[0]));
        auto& root = user_context->make<EntityRuntimeState>();
        Vector<CompactRow> batch(r);
        batch.emplace_back(rows[0]);
        auto hydrated = decode_compact_rows_with_read_metadata<School>(r, std::move(batch), root);
        const bool expected_success = mode == 0 || mode == 4;
        check(bool(direct) == expected_success && bool(hydrated) == expected_success,
              "direct and service hydration obey declared dynamic property types");
        if (expected_success) {
            check(direct.value().id == 7 && direct.value().version() == 1 &&
                      hydrated.value().size() == 1 && hydrated.value()[0].name == "source-name",
                  "valid and NULL declared properties do not block native scalar hydration");
        } else if (mode != 3) {
            check(direct.error().message == "invalid dynamic property type: _metric" &&
                      hydrated.error().message == direct.error().message &&
                      direct.error().message.find("PRIVATE-DECODE-CANARY") == String::npos,
                  "metadata rejection precedes native conversion and does not expose payload");
        } else {
            check(direct.error().message.find("_metric") == String::npos &&
                      hydrated.error().message == direct.error().message,
                  "valid metadata still permits the native scalar conversion error");
        }
    }
}

static void source_dynamic_mutator_contract() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto row = read_school(fixture);
    auto initial = row.loaded_state_snapshot()->identity();
    check(!row.is_field_loaded("#title") &&
              bool(row.updateDynamicField("title", Value(r, "selected"))) &&
              row.is_field_loaded("#title") && row.loaded_state_snapshot()->identity() != initial,
          "first dynamic selection refreshes the indexed carrier snapshot");
    auto selected = row.loaded_state_snapshot()->identity();
    check(bool(row.updateDynamicField("title", Value(r))) &&
              row.loaded_state_snapshot()->identity() == selected &&
              row.dynamic_field_values()->field("title").value().state() == DynamicFieldState::Null,
          "updating an already selected field to NULL preserves snapshot identity");
    check(bool(row.deleteDynamicField("title")) && !row.is_field_loaded("#title") &&
              row.loaded_state_snapshot()->identity() != selected &&
              row.dynamic_field_values()->field("title").value().state() ==
                  DynamicFieldState::NotLoaded,
          "deleting a selected field refreshes availability without turning it into NULL");
    auto removed = row.loaded_state_snapshot()->identity();
    check(bool(row.deleteDynamicField("title")) &&
              row.loaded_state_snapshot()->identity() == removed,
          "deleting an already missing field preserves snapshot identity");
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    for (bool wrong_layout : {false, true}) {
        auto candidate = read_school(fixture);
        check(bool(candidate.updateDynamicField("title", Value(r, "pending"))),
              "invalid-state control starts with a valid pending mutation");
        candidate.load_state =
            wrong_layout
                ? eval::LoadState::fully_loaded(r)
                      .into_indexed(pool, dynamic_hydration_fixture::School::field_layout(r))
                      .value()
                : eval::LoadState::fully_loaded(r);
        auto before = candidate.root.current_change_set();
        auto snapshot = candidate.loaded_state_snapshot();
        const auto expected =
            wrong_layout ? "DYNAMIC_FIELD_LAYOUT_MISMATCH" : "DYNAMIC_FIELD_INDEXED_STATE_MISSING";
        for (bool erase : {false, true}) {
            auto result = erase ? candidate.deleteDynamicField("title")
                                : candidate.updateDynamicField("title", Value(r, "rejected"));
            check(
                !result && result.error().code == expected &&
                    candidate.root.current_change_set() == before &&
                    candidate.dynamic_field_values()->field("title").value().value()->try_text() ==
                        "pending" &&
                    candidate.loaded_state_snapshot().has_value() == snapshot.has_value() &&
                    (!snapshot ||
                     candidate.loaded_state_snapshot()->identity() == snapshot->identity()),
                "invalid indexed state rejects update and delete before changing payload or ledger");
        }
    }
}

static void source_descriptor_contract() {
    Fixture fixture(true, false);
    auto r = fixture.user_context->memory_resource();
    auto descriptor = School::entity_descriptor(r);
    const std::array<std::string_view, 3> names{"id", "version", "name"};
    const std::array types{DataType::U64, DataType::I64, DataType::Text};
    check(descriptor.properties.size() == 3, "source descriptor has three scalar fields");
    for (std::size_t index = 0; index < names.size(); ++index) {
        const auto& property = descriptor.properties[index];
        check(property.name == names[index] && property.column_name == names[index] &&
                  property.data_type == types[index] && !property.nullable &&
                  property.is_id == (index == 0) && property.is_version == (index == 1),
              "source descriptor preserves derive order, types, flags and non-null metadata");
    }
    auto frozen = dynamic_hydration_fixture::School::entity_descriptor(r);
    check(frozen.properties[1].name == "name" && frozen.properties[1].nullable &&
              frozen.properties[2].name == "version" && frozen.properties[2].nullable,
          "frozen metadata remains separate from source qualification");
    sqlite3_stmt* raw = nullptr;
    check(sqlite3_prepare_v2(fixture.connection->native_handle(),
                             "PRAGMA table_info(school_data)",
                             -1,
                             &raw,
                             nullptr) == SQLITE_OK,
          "inspect the schema created through the modeled provider");
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    unsigned columns = 0;
    int result = SQLITE_ROW;
    while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
        auto name = std::string_view(
            reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1)));
        check(name == "id" || name == "version" || name == "name",
              "source SQLite schema has no unexpected scalar columns");
        check(name == "id" ? sqlite3_column_int(statement.get(), 5) != 0
                           : sqlite3_column_int(statement.get(), 3) == 1,
              "actual modeled schema preserves primary key and required scalar constraints");
        ++columns;
    }
    check(result == SQLITE_DONE && columns == 3,
          "source modeled schema inspection completes with exactly three fields");
}

static void source_dynamic_installation_contract() {
    Fixture fixture(true);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto row = read_school(fixture);
    const std::array types{DynamicFieldDefinitions::Field{"title", DataType::Text}};
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "guard-v1", types)).value());
    Value::Object values(r);
    values.emplace(String("title", r), Value(r, "original"));
    auto fields = DynamicFieldValues::from_values(r, definitions, values).value();
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    auto state =
        LoadedSnapshot::with_dynamic_fields(pool, row.loaded_state_snapshot().value(), fields)
            .value();
    check(bool(row.install_loaded_dynamic_fields(fields, state)),
          "source carrier accepts the matching owner and indexed layout");
    check(bool(row.updateDynamicField("title", Value(r, "pending"))),
          "installation rejection control retains existing pending intent");
    const auto before = row.root.current_change_set();
    auto original_snapshot = row.loaded_state_snapshot()->identity();
    auto& wrong_owner = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "Other", "guard-v1", types)).value());
    values.at(String("title", r)) = Value(r, "replacement");
    auto wrong_fields = DynamicFieldValues::from_values(r, wrong_owner, values).value();
    auto replacement = DynamicFieldValues::from_values(r, definitions, values).value();
    auto foreign_layout =
        eval::LoadState::fully_loaded(r)
            .into_indexed(pool, dynamic_hydration_fixture::School::field_layout(r))
            .value();
    for (bool wrong_layout : {false, true}) {
        auto rejected = row.install_loaded_dynamic_fields(
            wrong_layout ? replacement : wrong_fields,
            wrong_layout ? *foreign_layout.indexed_snapshot() : state);
        check(!rejected &&
                  rejected.error().message == "incompatible dynamic-field owner or loaded layout",
              "source carrier rejects owner and layout mismatches with the original error");
        check(row.loaded_state_snapshot()->identity() == original_snapshot &&
                  row.dynamic_field_values()->definitions().owner_type() == "School" &&
                  row.dynamic_field_values()->field("title").value().value()->try_text() ==
                      "pending" &&
                  row.root.current_change_set() == before,
              "rejected installation preserves snapshot, loaded payload and pending ledger");
    }
}

static void source_scalar_projection_contract() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    for (unsigned mode = 0; mode < 5; ++mode) {
        Record input(r);
        input.emplace(String("id", r), Value(r, std::uint64_t(7)));
        if (mode != 1) {
            input.emplace(String("name", r), Value(r, "source-name"));
        }
        if (mode != 2) {
            input.emplace(String("version", r),
                          mode == 3   ? Value(r)
                          : mode == 4 ? Value(r, "invalid")
                                      : Value(r, std::int64_t(9)));
        }
        auto decoded =
            School::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), input));
        if (mode == 4) {
            check(!decoded, "source carrier rejects an invalid present version type");
            continue;
        }
        check(decoded && decoded.value().version() == (mode >= 2 ? 0 : 9),
              "source version scalar decodes and defaults exactly at missing/NULL boundaries");
        auto copy = detail::clone_in(r, decoded.value());
        auto values = std::move(copy).into_values();
        check(
            values.fields.contains("name") == (mode != 1) &&
                values.fields.contains("version") == (mode != 2),
            "mutation scalar surface omits NotLoaded fields but retains explicit loaded defaults");
        auto json_copy = detail::clone_in(r, decoded.value());
        auto json = std::move(json_copy).into_json(r);
        const auto& object = std::get<Json::Object>(json.data);
        check(object.size() == (mode == 1 || mode == 2 ? 2 : 3) &&
                  object.contains("name") == (mode != 1) &&
                  object.contains("version") == (mode != 2),
              "source JSON retains exactly the loaded scalar projection");
        if (mode != 2) {
            check(object.at(String("version", r)).dump(r) == (mode == 3 ? "0" : "9"),
                  "source JSON exposes the decoded version scalar");
        }
        auto& retained = global.make<School>(decoded.value());
        check(retained.version() == decoded.value().version() &&
                  retained.is_field_loaded("version") == (mode != 2),
              "explicit destination copy retains version and loaded-state semantics");
    }
}

struct GenericScopeNotes {
    using allocator_type = Allocator;
    std::optional<Timestamp> time;
    unsigned polls = 0, cancels = 0;
    bool cancel_scoped = false, cancel_time_matches = false;

    GenericScopeNotes(std::allocator_arg_t, Allocator) {}
};

struct GenericPausedSave {
    using allocator_type = Allocator;
    const UserContext& user_context;
    GenericScopeNotes& notes;

    GenericPausedSave(std::allocator_arg_t,
                      Allocator,
                      const UserContext& user_context,
                      GenericScopeNotes& notes)
        : user_context(user_context), notes(notes) {}

    std::optional<detail::GraphSaveResult> poll(WakeSignal&) {
        check(detail::GraphFixActivation::current(user_context) != nullptr,
              "generic dynamic continuation runs inside its Fix scope");
        ++notes.polls;
        if (!notes.time) {
            notes.time = user_context.fix_time();
        }
        check(*notes.time == user_context.fix_time(), "generic Fix clock remains stable");
        user_context.record_fix_evidence(FixEvidence(std::allocator_arg,
                                                     Allocator(user_context.memory_resource()),
                                                     "School",
                                                     "name",
                                                     FixEvidenceSource::Context,
                                                     "pending operation"));
        return {};
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        ++notes.cancels;
        notes.cancel_scoped = detail::GraphFixActivation::current(user_context) != nullptr;
        notes.cancel_time_matches = notes.time && *notes.time == user_context.fix_time();
    }
};

struct GenericPausedSaver final : detail::DynGraphSaver {
    using allocator_type = Allocator;
    GenericScopeNotes& notes;

    GenericPausedSaver(std::allocator_arg_t, Allocator, GenericScopeNotes& notes) : notes(notes) {}

    AsyncOperation<detail::GraphSaveResult>& save_graph_dyn(const UserContext& user_context,
                                                            GraphNode) const override {
        return make_operation<detail::GraphSaveResult, GenericPausedSave>(
            user_context.memory_resource(), user_context, notes);
    }

    AsyncOperation<detail::GraphSaveResult>& save_ledger_dyn(
        const UserContext& user_context, GraphNode node, const EntityRuntimeState&) const override {
        return save_graph_dyn(user_context, std::move(node));
    }
};

static void generic_fix_scope_cancellation() {
    using Model = dynamic_hydration_fixture::School;
    GlobalContext global;
    for (bool close_context : {false, true}) {
        auto& notes = global.make<GenericScopeNotes>();
        auto user_context = global.make_user_context();
        AsyncOperation<Result<Model, RuntimeError>>* pending = nullptr;
        {
            auto r = user_context->memory_resource();
            auto& metadata = user_context->make<InMemoryMetadataStore>();
            metadata.register_entity(Model::entity_descriptor(r));
            user_context->set_metadata(*user_context, metadata);
            auto& saver = user_context->make_resource<GenericPausedSaver>(notes);
            user_context->insert_resource(*user_context,
                                          static_cast<detail::DynGraphSaver&>(saver));
            Vector<FixEvidence> prior(r);
            prior.emplace_back("School", "name", FixEvidenceSource::Context, "previous");
            detail::ContextFixAccess::replace_last(*user_context, prior);
            Record values(r);
            values.emplace(String("id", r), Value(r, std::uint64_t{7}));
            values.emplace(String("version", r), Value(r, std::int64_t{1}));
            values.emplace(String("name", r), Value(r, "loaded"));
            auto model =
                Model::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), values));
            check(bool(model), "generic cancellation model decodes");
            auto definitions = DynamicFieldDefinitions::from_fields(r, "School", "1", {});
            auto& owned = user_context->make<DynamicFieldDefinitions>(definitions.value());
            DynamicFieldValues::Values empty(r);
            auto fields = DynamicFieldValues::from_values(r, owned, empty);
            check(bool(detail::install_saved_dynamic_fields(r, model.value(), &fields.value())),
                  "generic cancellation model has a dynamic carrier");
            auto& operation =
                AuditedSaveExt<Model>::save(Audited<Model>(std::allocator_arg,
                                                           Allocator(r),
                                                           std::move(model).value(),
                                                           "verify generic dynamic cancellation"),
                                            *user_context);
            check(!operation.poll() && notes.polls == 1, "generic dynamic save suspends");
            check(user_context->last_fix_evidence().front().source_label == "previous",
                  "pending scope has not published evidence");
            pending = &operation;
        }
        if (close_context) {
            user_context.reset();
        } else {
            pending->cancel();
            check(user_context->last_fix_evidence().size() == 1 &&
                      user_context->last_fix_evidence().front().source_label == "previous",
                  "cancelled scope does not publish incomplete evidence");
            user_context.reset();
        }
        check(notes.cancels == 1 && notes.cancel_scoped && notes.cancel_time_matches,
              "generic dynamic cancellation runs once under the original Fix clock");
    }
}

static void generic_fix_scope_publication() {
    using Model = dynamic_hydration_fixture::School;
    GlobalContext global;
    for (bool dynamic : {false, true}) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        metadata.register_entity(Model::entity_descriptor(r));
        user_context->set_metadata(*user_context, metadata);
        Vector<FixEvidence> prior(r);
        prior.emplace_back("School", "name", FixEvidenceSource::Context, "previous");
        detail::ContextFixAccess::replace_last(*user_context, prior);
        Record values(r);
        values.emplace(String("id", r), Value(r, std::uint64_t{7}));
        values.emplace(String("version", r), Value(r, std::int64_t{1}));
        values.emplace(String("name", r), Value(r, "loaded"));
        auto model =
            Model::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), values));
        check(bool(model), "generic scope fixture decodes");
        if (dynamic) {
            auto definitions = DynamicFieldDefinitions::from_fields(r, "School", "1", {});
            auto& owned = user_context->make<DynamicFieldDefinitions>(definitions.value());
            DynamicFieldValues::Values empty(r);
            auto fields = DynamicFieldValues::from_values(r, owned, empty);
            check(bool(detail::install_saved_dynamic_fields(r, model.value(), &fields.value())),
                  "generic scope fixture has a present dynamic carrier");
        }
        const auto& saved =
            AuditedSaveExt<Model>::save(Audited<Model>(std::allocator_arg,
                                                       Allocator(r),
                                                       std::move(model).value(),
                                                       "verify generic dynamic scope"),
                                        *user_context)
                .wait();
        check(!saved &&
                  saved.error().to_string(r).find("no DynGraphSaver registered") != String::npos,
              "generic save reaches the missing-saver boundary");
        check(user_context->last_fix_evidence().size() == (dynamic ? 0u : 1u),
              "only generic dynamic save publishes its completed Fix scope");
    }
}

int main() {
    generic_fix_scope_publication();
    generic_fix_scope_cancellation();
    source_layout_contract();
    source_scalar_projection_contract();
    source_dynamic_installation_contract();
    source_descriptor_contract();
    source_dynamic_mutator_contract();
    source_decode_validation_contract();
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
    held_view_storage_provenance(false);
    held_view_storage_provenance(true);
    child_dynamic_union_save();
    selected_dynamic_fluent_save();
    separate_provider_transaction_binding();
    invalid_readback_rollback_retry();
    invalid_dynamic_preflight();
    incomplete_projection_dynamic_save();
    incompatible_dynamic_composition();
    composed_dynamic_graph_rollback();
    dynamic_dml_failure_retry();
    dynamic_stale_version_isolation();
    new_dynamic_entity_provenance(false);
    new_dynamic_entity_provenance(true);
    dynamic_null_delete_safe_audit();
    audited_roots_shared_dynamic_masks();
    durable_dynamic_reopen();
    std::cout << "Dynamic graph save contracts passed\n";
}
