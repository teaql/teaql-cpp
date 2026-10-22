#include "../../examples/models.hpp"
#include <teaql/runtime/detail/data_service/audited_auto_save.hpp>
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/transaction.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/id.hpp>
#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/sql/executor.hpp>
#include <sqlite3.h>
#include <iostream>
#include <array>
#include <algorithm>
#include <filesystem>
#include <cstdlib>

using namespace teaql;
using NativeExecutor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
using Error = NativeExecutor::Error;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Mirrors the locked source's Observed<E>: actual returned metadata on success,
// and actual failure metadata through the upstream observer. No traces are synthesized.
struct Capture {
    using allocator_type = Allocator;
    Vector<MutationRequest> commands;
    Vector<ExecutionMetadata> metadata;

    Capture(std::allocator_arg_t, Allocator a) : commands(a.resource()), metadata(a.resource()) {}

    Memory memory_resource() const {
        return commands.get_allocator().resource();
    }

    ExecutionObserver observe(std::optional<ExecutionObserver> upstream) {
        return ExecutionObserver(std::allocator_arg,
                                 Allocator(memory_resource()),
                                 [this, upstream = std::move(upstream)](ExecutionMetadata event) {
                                     metadata.emplace_back(event);
                                     if (upstream) {
                                         (*upstream)(std::move(event));
                                     }
                                 });
    }

    template <class T>
    struct ResultCapture {
        using allocator_type = Allocator;
        Memory resource;
        Capture& capture;
        DataServiceOperationResult<T, Error>& pending;
        WakeSignal::Forward forwarding;

        ResultCapture(std::allocator_arg_t,
                      Allocator a,
                      Capture& capture,
                      DataServiceOperationResult<T, Error>& pending)
            : resource(a.resource()), capture(capture), pending(pending) {}

        std::optional<Result<T, Error>> poll(WakeSignal& wake) {
            forwarding.attach(pending.wake_signal(), wake);
            auto* result = pending.poll();
            if (!result) {
                return {};
            }
            if (*result) {
                capture.metadata.emplace_back(result->value().metadata);
            }
            return detail::clone_in(resource, *result);
        }

        void finish_and_wait() noexcept {
            forwarding.reset();
        }

        void cancel_and_wait() noexcept {
            forwarding.reset();
            pending.cancel();
        }
    };

    template <class T>
    DataServiceOperationResult<T, Error>& record(DataServiceOperationResult<T, Error>& pending) {
        return make_operation<Result<T, Error>, ResultCapture<T>>(
            memory_resource(), *this, pending);
    }
};

struct ObservedTransaction final : DataServiceTransaction<Error> {
    using allocator_type = Allocator;
    DataServiceTransaction<Error>& inner;
    Capture& capture;

    ObservedTransaction(std::allocator_arg_t,
                        Allocator a,
                        DataServiceTransaction<Error>& inner,
                        Capture& capture)
        : DataServiceTransaction<Error>(a.resource()), inner(inner), capture(capture) {
        check(a.resource() == inner.memory_resource() && a.resource() == capture.memory_resource(),
              "transaction observer retains its Context domain");
    }

    DataServiceCapabilities capabilities() const override {
        return inner.capabilities();
    }

protected:
    TransactionResourceScope do_bind_transaction_resources(Memory r) override {
        return inner.bind_transaction_resources(r);
    }

    DataServiceOperationResult<QueryResult, Error>&
    query_transaction(Memory r, QueryRequest request) override {
        return query_transaction_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<MutationResult, Error>&
    mutate_transaction(Memory r, MutationRequest request) override {
        return mutate_transaction_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<QueryResult, Error>& query_transaction_observed(
        Memory r, QueryRequest request, std::optional<ExecutionObserver> observer) override {
        return capture.record(
            inner.query_observed(r, std::move(request), capture.observe(std::move(observer))));
    }

    DataServiceOperationResult<MutationResult, Error>& mutate_transaction_observed(
        Memory r, MutationRequest request, std::optional<ExecutionObserver> observer) override {
        capture.commands.emplace_back(request);
        return capture.record(
            inner.mutate_observed(r, std::move(request), capture.observe(std::move(observer))));
    }

    DataServiceOperationResult<std::monostate, Error>& commit_transaction(Memory r) override {
        return inner.commit(r);
    }

    DataServiceOperationResult<std::monostate, Error>& rollback_transaction(Memory r) override {
        return inner.rollback(r);
    }

    void cancel_transaction() noexcept override {
        inner.cancel();
    }
};

struct ObservedExecutor final : QueryExecutor<Error>,
                                MutationExecutor<Error>,
                                TransactionExecutor<Error> {
    using allocator_type = Allocator;
    UserContext& user_context;
    NativeExecutor& inner;
    Capture& capture;

    ObservedExecutor(std::allocator_arg_t,
                     Allocator a,
                     UserContext& user_context,
                     NativeExecutor& inner,
                     Capture& capture)
        : user_context(user_context), inner(inner), capture(capture) {
        check(a.resource() == user_context.memory_resource() &&
                  a.resource() == inner.memory_resource() &&
                  a.resource() == capture.memory_resource(),
              "executor observer retains its Context domain");
    }

    Memory memory_resource() const {
        return user_context.memory_resource();
    }

    DataServiceCapabilities capabilities() const override {
        return inner.capabilities();
    }

    DataServiceOperationResult<QueryResult, Error>& query(Memory r, QueryRequest request) override {
        return query_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<MutationResult, Error>& mutate(Memory r,
                                                              MutationRequest request) override {
        return mutate_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<QueryResult, Error>& query_observed(
        Memory r, QueryRequest request, std::optional<ExecutionObserver> observer) override {
        return capture.record(
            inner.query_observed(r, std::move(request), capture.observe(std::move(observer))));
    }

    DataServiceOperationResult<MutationResult, Error>& mutate_observed(
        Memory r, MutationRequest request, std::optional<ExecutionObserver> observer) override {
        capture.commands.emplace_back(request);
        return capture.record(
            inner.mutate_observed(r, std::move(request), capture.observe(std::move(observer))));
    }

    struct Begin {
        using allocator_type = Allocator;
        ObservedExecutor& owner;
        DataServiceOperationResult<TransactionHandle<Error>, Error>& pending;
        WakeSignal::Forward forwarding;

        Begin(std::allocator_arg_t, Allocator a, ObservedExecutor& owner)
            : owner(owner), pending(owner.inner.begin(a.resource())) {}

        std::optional<Result<TransactionHandle<Error>, Error>> poll(WakeSignal& wake) {
            auto r = owner.memory_resource();
            forwarding.attach(pending.wake_signal(), wake);
            auto* result = pending.poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return Result<TransactionHandle<Error>, Error>::err(
                    r, detail::clone_in(r, result->error()));
            }
            auto& transaction =
                owner.user_context.make<ObservedTransaction>(result->value().get(), owner.capture);
            return Result<TransactionHandle<Error>, Error>::ok(
                r, TransactionHandle<Error>(std::allocator_arg, Allocator(r), transaction));
        }

        void finish_and_wait() noexcept {
            forwarding.reset();
        }

        void cancel_and_wait() noexcept {
            forwarding.reset();
            pending.cancel();
        }
    };

    DataServiceOperationResult<TransactionHandle<Error>, Error>& begin(Memory r) override {
        check(r == memory_resource(), "begin remains in observer Context");
        return make_operation<Result<TransactionHandle<Error>, Error>, Begin>(r, *this);
    }
};

// The source derive stores comments per entity key, including before allocation.
template <class Derived>
struct AllocationModel : example::Model<Derived> {
    using Base = example::Model<Derived>;
    using Base::Base;

    void set_comment(std::string_view reason) {
        this->root.set_entity_comment(this->entity_key(), reason);
    }

    std::optional<String> get_comment() const {
        return this->root.get_entity_comment(this->entity_key());
    }

protected:
    MutationValues scalar_values() const {
        auto values = Base::scalar_values();
        auto r = this->memory_resource();
        if (auto reason = get_comment()) {
            values.fields.insert_or_assign(String("_comment", r), Value(r, *reason));
        }
        return values;
    }
};

struct AllocationRoot : AllocationModel<AllocationRoot> {
    using Model = AllocationModel<AllocationRoot>;
    static constexpr std::string_view ENTITY_NAME = "AllocationRoot";
    std::int64_t version_value = 0;

    AllocationRoot(std::allocator_arg_t, Allocator a) : Model(std::allocator_arg, a) {}

    AllocationRoot(std::allocator_arg_t, Allocator a, const AllocationRoot& other)
        : Model(std::allocator_arg, a, other), version_value(other.version_value) {}

    AllocationRoot(AllocationRoot&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.table("allocation_root_data");
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.property("version", DataType::I64).version().not_null();
        descriptor.property("name", DataType::Text).not_null();
        return descriptor;
    }

    static Result<AllocationRoot, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<AllocationRoot, EntityError>;
        AllocationRoot entity(std::allocator_arg, Allocator(r));
        auto id = example::decode_field<std::uint64_t>(r, row, ENTITY_NAME, "id");
        if (!id) {
            return R::err(r, std::move(id).error());
        }
        auto version = example::decode_field<std::int64_t>(r, row, ENTITY_NAME, "version");
        if (!version) {
            return R::err(r, std::move(version).error());
        }
        auto name = example::decode_field<String>(r, row, ENTITY_NAME, "name");
        if (!name) {
            return R::err(r, std::move(name).error());
        }
        entity.id = id.value();
        entity.version_value = version.value();
        entity.name = std::move(name).value();
        entity.root.set_original_compact_row(ENTITY_NAME, row);
        return R::ok(r, std::move(entity));
    }

    MutationValues into_values() && {
        auto values = scalar_values();
        values.fields.emplace(String("version", memory_resource()),
                              Value(memory_resource(), version_value));
        return values;
    }
};

struct AllocationChild : AllocationModel<AllocationChild> {
    using Model = AllocationModel<AllocationChild>;
    static constexpr std::string_view ENTITY_NAME = "AllocationChild";
    std::int64_t version_value = 0;
    std::uint64_t root_id = 0;

    AllocationChild(std::allocator_arg_t, Allocator a) : Model(std::allocator_arg, a) {}

    AllocationChild(std::allocator_arg_t, Allocator a, const AllocationChild& other)
        : Model(std::allocator_arg, a, other), version_value(other.version_value),
          root_id(other.root_id) {}

    AllocationChild(AllocationChild&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.table("allocation_child_data");
        descriptor.property("id", DataType::U64).id().not_null();
        descriptor.property("version", DataType::I64).version().not_null();
        descriptor.property("root_id", DataType::U64).not_null();
        descriptor.property("name", DataType::Text).not_null();
        return descriptor;
    }

    static Result<AllocationChild, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<AllocationChild, EntityError>;
        AllocationChild entity(std::allocator_arg, Allocator(r));
        auto id = example::decode_field<std::uint64_t>(r, row, ENTITY_NAME, "id");
        if (!id) {
            return R::err(r, std::move(id).error());
        }
        auto version = example::decode_field<std::int64_t>(r, row, ENTITY_NAME, "version");
        if (!version) {
            return R::err(r, std::move(version).error());
        }
        auto root_id = example::decode_field<std::uint64_t>(r, row, ENTITY_NAME, "root_id");
        if (!root_id) {
            return R::err(r, std::move(root_id).error());
        }
        auto name = example::decode_field<String>(r, row, ENTITY_NAME, "name");
        if (!name) {
            return R::err(r, std::move(name).error());
        }
        entity.root_id = root_id.value();
        entity.id = id.value();
        entity.version_value = version.value();
        entity.name = std::move(name).value();
        entity.root.set_original_compact_row(ENTITY_NAME, row);
        return R::ok(r, std::move(entity));
    }

    MutationValues into_values() && {
        auto values = scalar_values();
        values.fields.emplace(String("version", memory_resource()),
                              Value(memory_resource(), version_value));
        values.fields.emplace(String("root_id", memory_resource()),
                              Value(memory_resource(), root_id));
        return values;
    }
};

struct SafeCapture final : SafeAuditEventSink {
    using allocator_type = Allocator;
    const SqliteConnection& connection;
    mutable Vector<SafeAuditEvent> events;

    SafeCapture(std::allocator_arg_t, Allocator a, const SqliteConnection& connection)
        : connection(connection), events(a.resource()) {}

    Memory memory_resource() const {
        return events.get_allocator().resource();
    }

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory r, const UserContext&, const SafeAuditEvent& event) const override {
        if (event.kind == RawAuditEventKind::Created &&
            (event.entity == "AllocationRoot" || event.entity == "AllocationChild")) {
            check(sqlite3_get_autocommit(connection.native_handle()) != 0,
                  "safe entity creation audit is delivered after physical transaction completion");
        }
        events.emplace_back(event);
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user_context = global.make_user_context();
    SqliteConnection& connection = user_context->make_resource<SqliteConnection>();
    Capture& capture = user_context->make<Capture>();
    SafeCapture& safe = user_context->make<SafeCapture>(connection);
    ObservedExecutor* executor = nullptr;

    explicit Fixture(bool reverse = true, bool forward = false) {
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        auto root = AllocationRoot::entity_descriptor(r);
        if (reverse) {
            auto& children = root.relation("children", "AllocationChild");
            children.many = true;
            children.local_key = "id";
            children.foreign_key = "root_id";
        }
        metadata.register_entity(std::move(root));
        auto child = AllocationChild::entity_descriptor(r);
        child.property("quantity", DataType::I64);
        if (forward) {
            auto& parent = child.relation("root", "AllocationRoot");
            parent.local_key = "root_id";
            parent.foreign_key = "id";
        }
        metadata.register_entity(std::move(child));
        user_context->set_metadata(*user_context, metadata);
        auto& transport = user_context->make<SqliteMutationExecutor>(connection);
        use_sqlite_provider(*user_context, *user_context, transport);
        auto dialect = user_context->require_resource<SqliteDialect>();
        check(bool(dialect), "SQLite dialect installed");
        auto& state = user_context->make<SqlExecutorState>();
        auto& native =
            user_context->make<NativeExecutor>(dialect.value().get(), transport, metadata, state);
        executor = &user_context->make<ObservedExecutor>(*user_context, native, capture);
        user_context->register_executor(*user_context, *executor);
        auto& registry = user_context->make<InMemoryEntityRegistry>();
        registry.register_entity("AllocationRoot");
        registry.register_entity("AllocationChild");
        user_context->set_entity_registry(*user_context, registry);
        user_context->set_custom_event_sink(*user_context, safe);
        check(bool(user_context->ensure_schema().wait()), "source-shaped root schema initialized");
        safe.events.clear();
        user_context->clear_sql_logs();
    }

    MutationRequest insert(std::uint64_t id) {
        auto r = user_context->memory_resource();
        InsertCommand command(std::allocator_arg, Allocator(r), "AllocationRoot");
        command.value("id", id).value("version", std::int64_t(1)).value("name", "observer fixture");
        return MutationRequest(
            std::allocator_arg, Allocator(r), std::move(command), "verify observer delegation");
    }

    std::int64_t rows() {
        sqlite3_stmt* statement = nullptr;
        check(sqlite3_prepare_v2(connection.native_handle(),
                                 "SELECT count(*) FROM allocation_root_data",
                                 -1,
                                 &statement,
                                 nullptr) == SQLITE_OK,
              "prepare physical row count");
        int status = sqlite3_step(statement);
        auto count = status == SQLITE_ROW ? sqlite3_column_int64(statement, 0) : -1;
        sqlite3_finalize(statement);
        check(status == SQLITE_ROW, "read physical row count");
        return count;
    }
};

// Infrastructure controls only: these are not SLAT01-SLAT15 acceptance.
static void observer_delegates_real_execution() {
    Fixture fixture;
    auto r = fixture.user_context->memory_resource();
    auto& executor = *fixture.executor;
    check(bool(executor.mutate(r, fixture.insert(1)).wait()), "real insert succeeds");
    check(fixture.rows() == 1 && fixture.capture.commands.size() == 1 &&
              fixture.capture.metadata.size() == 1,
          "success records one actual command and returned metadata");
    unsigned forwarded = 0;
    ExecutionObserver upstream(std::allocator_arg, Allocator(r), [&forwarded](ExecutionMetadata) {
        ++forwarded;
    });
    check(!executor.mutate_observed(r, fixture.insert(1), std::move(upstream)).wait(),
          "duplicate ID fails in SQLite");
    check(forwarded == 1 && fixture.capture.commands.size() == 2 &&
              fixture.capture.metadata.size() == 2 && fixture.rows() == 1,
          "failure metadata captured once and forwarded without repeating writes");
    for (bool commit : {false, true}) {
        auto& opened = executor.begin(r).wait();
        check(bool(opened), "real transaction opened through observer");
        auto& transaction = opened.value().get();
        check(sqlite3_get_autocommit(fixture.connection.native_handle()) == 0,
              "SQLite transaction is active");
        check(bool(transaction.mutate(r, fixture.insert(2)).wait()), "wrapped transaction inserts");
        check(fixture.rows() == 2, "transaction sees its write");
        check(bool((commit ? transaction.commit(r) : transaction.rollback(r)).wait()),
              "terminal delegated to SQLite");
        check(sqlite3_get_autocommit(fixture.connection.native_handle()) != 0 &&
                  fixture.rows() == (commit ? 2 : 1),
              "physical commit or rollback matches requested terminal");
    }
    SelectQuery query(std::allocator_arg, Allocator(r), "AllocationRoot");
    query.comment("Read observer fixture rows");
    query.purpose("Verify actual query metadata survives executor wrapping");
    check(bool(executor.query(r, QueryRequest(std::allocator_arg, Allocator(r), query, false, true))
                   .wait()),
          "real query succeeds through observer");
    check(fixture.capture.commands.size() == 4 && fixture.capture.metadata.size() == 5,
          "transaction writes and query each retain one actual metadata result");
}

static void
assigned_root_chain(std::span<const TraceNode> chain, std::uint64_t id, std::string_view reason) {
    unsigned count = 0;
    for (const auto& node : chain) {
        if (node.kind == TraceKind::AuditReason) {
            check(node.entity_type == "AllocationRoot" && node.entity_id == id &&
                      node.comment == reason,
                  "assigned root identity and reason survive consumer boundary");
            ++count;
        }
    }
    check(count == 1, "root has exactly one ordered audit reason frame");
}

static const MutationRequest* find_insert(const MutationRequest& request, std::string_view entity) {
    if (auto* insert = std::get_if<InsertCommand>(&request.data)) {
        return insert->entity == entity ? &request : nullptr;
    }
    if (auto* batch = std::get_if<MutationRequest::Batch>(&request.data)) {
        for (const auto& child : *batch) {
            if (auto* found = find_insert(child, entity)) {
                return found;
            }
        }
    }
    return nullptr;
}

static void assert_root_sinks(Fixture& fixture,
                              std::uint64_t id,
                              std::string_view reason,
                              std::string_view safe_reason) {
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    check(id > 0 && fixture.rows() == 1, "runtime allocates root identity and persists actual row");
    const MutationRequest* insert = nullptr;
    for (const auto& command : fixture.capture.commands) {
        if (auto* found = find_insert(command, "AllocationRoot")) {
            insert = found;
            break;
        }
    }
    check(insert != nullptr, "actual lower-ledger root insert captured");
    check(std::get<InsertCommand>(insert->data).values.fields.at(String("id", r)).try_u64() == id,
          "lower-ledger command uses returned assigned root ID");
    assigned_root_chain(insert->trace_chain(), id, reason);
    unsigned inserts = 0, readbacks = 0;
    auto physical = [&](const ExecutionMetadata& record) {
        if (record.operation != DataServiceOperation::Insert) {
            return;
        }
        assigned_root_chain(record.trace_chain, id, reason);
        bool found = false;
        for (const auto& node : record.trace_chain) {
            if (node.kind == TraceKind::Entity && node.entity_type == "AllocationRoot" &&
                node.entity_id == id) {
                found = true;
            }
        }
        check(found, "physical SQL entity frame uses assigned root ID");
        ++inserts;
    };
    for (const auto& record : fixture.capture.metadata) {
        if (record.operation == DataServiceOperation::Query) {
            assigned_root_chain(record.trace_chain, id, reason);
            ++readbacks;
        }
        if (record.statements.empty()) {
            physical(record);
        } else {
            for (const auto& statement : record.statements) {
                physical(statement);
            }
        }
    }
    check(inserts > 0 && readbacks > 0,
          "actual physical insert and authoritative readback observed");
    unsigned audits = 0;
    for (const auto& event : fixture.safe.events) {
        if (event.entity != "AllocationRoot" || event.kind != RawAuditEventKind::Created) {
            continue;
        }
        assigned_root_chain(event.trace_chain, id, safe_reason);
        bool found = false;
        for (const auto& field : event.fields) {
            if (field.name == "id" && field.value &&
                std::string_view(*field.value) == std::to_string(id)) {
                found = true;
            }
        }
        check(found, "safe created event retains assigned ID field");
        ++audits;
    }
    check(audits == 1, "one committed safe root creation audit");
    unsigned safe_reads = 0;
    for (const auto& entry : user_context.sql_logs()) {
        if (entry.operation == SqlLogOperation::Select && entry.comment &&
            *entry.comment == safe_reason) {
            auto purpose = entry.purpose ? std::string_view(*entry.purpose) : std::string_view{};
            detail::trim(purpose);
            check(entry.audit_reason && *entry.audit_reason == safe_reason && !purpose.empty(),
                  "safe authoritative readback has masked reason and nonblank purpose");
            ++safe_reads;
        }
    }
    check(safe_reads > 0, "authoritative readback reaches safe SQL sink");
}

static const AllocationRoot& save_and_assert_root(Fixture& fixture,
                                                  std::string_view name,
                                                  std::string_view reason,
                                                  std::string_view safe_reason) {
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    CompactRow row(std::allocator_arg, Allocator(r));
    row.insert("id", Value(r, std::uint64_t(0)));
    row.insert("version", Value(r, std::int64_t(0)));
    row.insert("name", Value(r, name));
    auto decoded = AllocationRoot::from_compact_row(r, std::move(row));
    check(bool(decoded), "source-shaped new root decodes");
    auto root = std::move(decoded).value();
    root.mark_as_new();
    root.root.set(root.entity_key(), "name", Value(r, name));
    auto& saved =
        save_audited_ledger_entity(
            Audited<AllocationRoot>(std::allocator_arg, Allocator(r), std::move(root), reason),
            user_context)
            .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    auto id = saved.value().id;
    assert_root_sinks(fixture, id, reason, safe_reason);
    return saved.value();
}

static void root_allocated_during_ledger_planning_reaches_command_sql_and_committed_audit() {
    Fixture fixture;
    save_and_assert_root(fixture, "late root", "create late root", "create [REDACTED]");
}

static void child_chain(std::span<const TraceNode> chain, std::uint64_t child_id, bool safe) {
    unsigned index = 0;
    for (const auto& node : chain) {
        if (node.kind != TraceKind::AuditReason) {
            continue;
        }
        check(index < 2, "child has no additional audit reason frames");
        check(node.entity_type == (index == 0 ? "AllocationRoot" : "AllocationChild") &&
                  node.entity_id == (index == 0 ? 42 : child_id) &&
                  node.comment == (index == 0
                                       ? "submit root"
                                       : (safe ? "authorize [REDACTED]" : "authorize late child")),
              "child trace retains ordered parent and assigned child identities with local reason");
        ++index;
    }
    check(index == 2, "child has both parent and local audit reason frames");
}

static void child_allocated_during_ledger_planning_retains_its_own_and_parent_identity() {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    CompactRow row(std::allocator_arg, Allocator(r));
    row.insert("id", Value(r, std::uint64_t(42)));
    row.insert("version", Value(r, std::int64_t(0)));
    row.insert("name", Value(r, "before"));
    auto decoded = AllocationRoot::from_compact_row(r, std::move(row));
    check(bool(decoded), "existing root seed decodes");
    auto seed = std::move(decoded).value();
    seed.mark_as_new();
    seed.root.set(seed.entity_key(), "name", Value(r, "before"));
    auto& seeded =
        save_audited_ledger_entity(
            Audited<AllocationRoot>(std::allocator_arg, Allocator(r), std::move(seed), "seed root"),
            user_context)
            .wait();
    if (!seeded) {
        throw std::runtime_error(std::string(seeded.error().to_string(r)));
    }
    auto root = detail::clone_in(r, seeded.value());
    check(root.id == 42, "seeded root retains explicit identity");
    fixture.capture.commands.clear();
    fixture.capture.metadata.clear();
    fixture.safe.events.clear();
    user_context.clear_sql_logs();
    root.name = "after";
    root.root.set(root.entity_key(), "name", Value(r, "after"));
    CompactRow child_row(std::allocator_arg, Allocator(r));
    child_row.insert("id", Value(r, std::uint64_t(0)));
    child_row.insert("version", Value(r, std::int64_t(0)));
    child_row.insert("root_id", Value(r, std::uint64_t(42)));
    child_row.insert("name", Value(r, "late child"));
    auto child_decoded = AllocationChild::from_compact_row(r, std::move(child_row));
    check(bool(child_decoded), "new child decodes before allocation");
    auto child = std::move(child_decoded).value();
    child.mark_as_new();
    auto child_state = *child.entity_runtime_state();
    child_state.set(child.entity_key(), "root_id", Value(r, std::uint64_t(42)));
    child_state.set(child.entity_key(), "name", Value(r, "late child"));
    child.set_comment("authorize late child");
    auto comment = child.get_comment();
    check(comment && *comment == "authorize late child", "typed child stores local comment");
    auto state_comment = child_state.get_entity_comment(child.entity_key());
    check(state_comment && *state_comment == "authorize late child",
          "shared ledger stores local comment");
    EntityKey pending_key(
        std::allocator_arg, Allocator(r), "AllocationChild", Value(r, std::uint64_t(0)));
    check(child.entity_key() == pending_key, "child retains zero identity until ledger planning");
    check(bool(LedgerEntityDefaults<AllocationRoot>::compose(root, child)),
          "typed root adopts pending child ledger");
    auto composed_comment = root.entity_runtime_state()->get_entity_comment(pending_key);
    check(composed_comment && *composed_comment == "authorize late child",
          "composition retains local reason before allocation");
    auto& saved = save_audited_ledger_entity(
                      Audited<AllocationRoot>(
                          std::allocator_arg, Allocator(r), std::move(root), "submit root"),
                      user_context)
                      .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    SelectQuery query(std::allocator_arg, Allocator(r), "AllocationChild");
    query.limit(10).comment("check allocated child").purpose("verify persistent identity");
    auto& queried =
        fixture.executor
            ->query(r, QueryRequest(std::allocator_arg, Allocator(r), query, false, true))
            .wait();
    check(bool(queried) && queried.value().rows.size() == 1,
          "actual query returns one persisted child");
    const auto& persisted = queried.value().rows.front();
    auto* id_value = persisted.get("id");
    auto* root_id = persisted.get("root_id");
    check(id_value && id_value->try_u64() && *id_value->try_u64() > 0 && root_id &&
              root_id->try_u64() == 42,
          "child has assigned positive ID and existing parent foreign key");
    auto id = *id_value->try_u64();
    const MutationRequest* insert = nullptr;
    for (const auto& request : fixture.capture.commands) {
        if (auto* found = find_insert(request, "AllocationChild")) {
            insert = found;
            break;
        }
    }
    check(insert != nullptr, "actual lower-ledger child insert captured");
    check(std::get<InsertCommand>(insert->data).values.fields.at(String("id", r)).try_u64() == id,
          "child INSERT uses persisted assigned ID");
    child_chain(insert->trace_chain(), id, false);
    unsigned physical_count = 0;
    auto inspect = [&](const ExecutionMetadata& record) {
        if (record.operation != DataServiceOperation::Insert) {
            return;
        }
        bool found = false;
        for (const auto& node : record.trace_chain) {
            if (node.kind == TraceKind::Entity && node.entity_type == "AllocationChild" &&
                node.entity_id == id) {
                found = true;
            }
        }
        if (found) {
            child_chain(record.trace_chain, id, false);
            ++physical_count;
        }
    };
    for (const auto& record : fixture.capture.metadata) {
        if (record.statements.empty()) {
            inspect(record);
        } else {
            for (const auto& statement : record.statements) {
                inspect(statement);
            }
        }
    }
    check(physical_count > 0, "physical SQL retains assigned child and parent trace");
    unsigned audits = 0;
    for (const auto& event : fixture.safe.events) {
        if (event.entity != "AllocationChild" || event.kind != RawAuditEventKind::Created) {
            continue;
        }
        child_chain(event.trace_chain, id, true);
        bool found = false;
        for (const auto& field : event.fields) {
            if (field.name == "id" && field.value &&
                std::string_view(*field.value) == std::to_string(id)) {
                found = true;
            }
        }
        check(found, "committed safe child audit retains allocated ID field");
        ++audits;
    }
    check(audits == 1, "exactly one committed safe child creation audit");
}

struct ObservedDatabaseIds final : InternalIdGenerator {
    using allocator_type = Allocator;
    SqliteIdSpaceGenerator& generator;
    const SqliteConnection& connection;
    const SafeCapture& safe;
    mutable Vector<EntityKey> allocated;

    ObservedDatabaseIds(std::allocator_arg_t,
                        Allocator a,
                        SqliteIdSpaceGenerator& generator,
                        const SqliteConnection& connection,
                        const SafeCapture& safe)
        : generator(generator), connection(connection), safe(safe), allocated(a.resource()) {}

    Memory memory_resource() const {
        return allocated.get_allocator().resource();
    }

    Result<std::uint64_t, RuntimeError> generate_id(Memory r,
                                                    std::string_view entity) const override {
        check(sqlite3_get_autocommit(connection.native_handle()) == 0,
              "database identity must allocate inside actual SQLite save transaction");
        check(safe.events.empty(), "database allocation precedes committed safe audits");
        auto result = generator.generate_id(r, entity);
        if (result) {
            allocated.emplace_back(entity, Value(r, result.value()));
        }
        return result;
    }

protected:
    Result<std::monostate, RuntimeError>
    do_ensure_floor(Memory r, std::string_view entity, std::uint64_t floor) const override {
        return static_cast<const InternalIdGenerator&>(generator).ensure_floor(r, entity, floor);
    }
};

static ObservedDatabaseIds& install_database_ids(Fixture& fixture) {
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& generator = user_context.make<SqliteIdSpaceGenerator>(fixture.executor->inner.transport);
    check(bool(generator.ensure_floor(r, "AllocationRoot", 500)) &&
              bool(generator.ensure_floor(r, "AllocationChild", 800)),
          "initialize database identity floors");
    fixture.capture.commands.clear();
    fixture.capture.metadata.clear();
    fixture.safe.events.clear();
    user_context.clear_sql_logs();
    auto& observed =
        user_context.make<ObservedDatabaseIds>(generator, fixture.connection, fixture.safe);
    user_context.set_internal_id_generator(user_context, observed);
    return observed;
}

static std::int64_t root_floor(Fixture& fixture) {
    sqlite3_stmt* statement = nullptr;
    check(sqlite3_prepare_v2(
              fixture.connection.native_handle(),
              "SELECT current_level FROM teaql_id_space WHERE type_name='allocation_root'",
              -1,
              &statement,
              nullptr) == SQLITE_OK,
          "prepare actual database root floor");
    auto status = sqlite3_step(statement);
    auto floor = status == SQLITE_ROW ? sqlite3_column_int64(statement, 0) : -1;
    sqlite3_finalize(statement);
    check(status == SQLITE_ROW, "read actual database root floor");
    return floor;
}

static void database_root_allocation_inside_transaction_reaches_all_trace_boundaries() {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& observed = install_database_ids(fixture);
    const auto& saved = save_and_assert_root(
        fixture, "database root", "allocate database root", "allocate [REDACTED]");
    check(saved.id == 501 && saved.version_value == 1, "database allocates root501/version1");
    EntityKey expected(
        std::allocator_arg, Allocator(r), "AllocationRoot", Value(r, std::uint64_t(501)));
    check(observed.allocated.size() == 1 && observed.allocated.front() == expected,
          "actual allocator records exactly root501");
    sqlite3_stmt* statement = nullptr;
    check(sqlite3_prepare_v2(
              fixture.connection.native_handle(),
              "SELECT current_level FROM teaql_id_space WHERE type_name='allocation_root'",
              -1,
              &statement,
              nullptr) == SQLITE_OK,
          "prepare actual database root floor");
    auto status = sqlite3_step(statement);
    auto floor = status == SQLITE_ROW ? sqlite3_column_int64(statement, 0) : -1;
    sqlite3_finalize(statement);
    check(status == SQLITE_ROW && floor == 501, "committed database floor is501");
    check(sqlite3_get_autocommit(fixture.connection.native_handle()) != 0,
          "save restores physical autocommit");
}

enum class ExplicitMode { Commit, Reject, Suspend };

struct ExplicitRootWork {
    using allocator_type = Allocator;
    using R = Result<AllocationRoot, RuntimeError>;
    const TransactionScope<ObservedExecutor>& scope;
    Fixture& fixture;
    ExplicitMode mode;
    AsyncOperation<R>* pending = nullptr;
    WakeSignal::Forward forwarding;

    ExplicitRootWork(std::allocator_arg_t,
                     Allocator,
                     const TransactionScope<ObservedExecutor>& scope,
                     Fixture& fixture,
                     ExplicitMode mode)
        : scope(scope), fixture(fixture), mode(mode) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = scope.context().memory_resource();
        if (!pending) {
            CompactRow row(std::allocator_arg, Allocator(r));
            row.insert("id", Value(r, std::uint64_t(0)));
            row.insert("version", Value(r, std::int64_t(0)));
            row.insert("name", Value(r, "explicit database root"));
            auto decoded = AllocationRoot::from_compact_row(r, std::move(row));
            check(bool(decoded), "explicit transaction root decodes");
            auto root = std::move(decoded).value();
            root.mark_as_new();
            root.root.set(root.entity_key(), "name", Value(r, "explicit database root"));
            pending = &scope.save_audited(Audited<AllocationRoot>(
                std::allocator_arg, Allocator(r), std::move(root), "explicit root allocation"));
        }
        forwarding.attach(pending->wake_signal(), wake);
        auto* result = pending->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(r, detail::clone_in(r, result->error()));
        }
        check(result->value().id == 501, "explicit body receives database-assigned root501");
        check(fixture.safe.events.empty(), "explicit body observes no committed safe audits");
        if (mode == ExplicitMode::Suspend) {
            return {};
        }
        if (mode == ExplicitMode::Reject) {
            return R::err(r, RuntimeError::event(r, "reject explicit ID save"));
        }
        return detail::clone_in(r, *result);
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

static auto& explicit_root_work(Fixture& fixture, ExplicitMode mode) {
    auto& user_context = *fixture.user_context;
    return user_context.execute_in_transaction<ObservedExecutor, AllocationRoot>(
        [&fixture, mode](const TransactionScope<ObservedExecutor>& scope)
            -> AsyncOperation<ExplicitRootWork::R>& {
            return make_operation<ExplicitRootWork::R, ExplicitRootWork>(
                scope.context().memory_resource(), scope, fixture, mode);
        });
}

static void database_allocated_explicit_transaction_delivers_audit_only_after_commit() {
    Fixture fixture;
    install_database_ids(fixture);
    auto& result = explicit_root_work(fixture, ExplicitMode::Commit).wait();
    if (!result) {
        throw std::runtime_error(
            std::string(result.error().to_string(fixture.user_context->memory_resource())));
    }
    check(result.value().id == 501, "explicit transaction returns root501 after commit");
    assert_root_sinks(fixture, 501, "explicit root allocation", "explicit root allocation");
    check(root_floor(fixture) == 501 && fixture.rows() == 1,
          "explicit transaction commits root row and identity floor");
}

static void database_scope_failure_and_cancellation() {
    for (auto mode : {ExplicitMode::Reject, ExplicitMode::Suspend}) {
        Fixture fixture;
        install_database_ids(fixture);
        auto& work = explicit_root_work(fixture, mode);
        if (mode == ExplicitMode::Suspend) {
            check(work.poll() == nullptr && fixture.rows() == 1 && root_floor(fixture) == 501,
                  "suspended body retains uncommitted row and ID floor");
            work.cancel();
        } else {
            check(!work.wait(), "business rejection rolls back scoped ID save");
        }
        check(fixture.rows() == 0 && root_floor(fixture) == 500 && fixture.safe.events.empty(),
              "failure or cancellation rolls back rows and floors without committed audit");
        check(sqlite3_get_autocommit(fixture.connection.native_handle()) != 0,
              "failed or cancelled scope releases physical transaction");
    }
}

static AllocationRoot new_root(Memory r, std::string_view name, std::uint64_t id = 0) {
    CompactRow row(std::allocator_arg, Allocator(r));
    row.insert("id", Value(r, id));
    row.insert("version", Value(r, std::int64_t(0)));
    row.insert("name", Value(r, name));
    auto decoded = AllocationRoot::from_compact_row(r, std::move(row));
    check(bool(decoded), "new failure-path root decodes");
    auto root = std::move(decoded).value();
    root.mark_as_new();
    root.root.set(root.entity_key(), "name", Value(r, name));
    return root;
}

static void database_allocation_rolls_back_and_retry_uses_new_identity_and_intent() {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& ids = install_database_ids(fixture);
    check(sqlite3_exec(
              fixture.connection.native_handle(),
              "CREATE TRIGGER reject_allocation_root BEFORE INSERT ON allocation_root_data "
              "WHEN NEW.name = 'reject' BEGIN SELECT RAISE(ABORT, 'fixture rejects root'); END;",
              nullptr,
              nullptr,
              nullptr) == SQLITE_OK,
          "install actual SQLite rejection trigger");
    auto root = new_root(r, "reject");
    auto& failed =
        save_audited_ledger_entity(
            Audited<AllocationRoot>(std::allocator_arg, Allocator(r), root, "rejected allocation"),
            user_context)
            .wait();
    check(!failed && failed.error().to_string(r).find("fixture rejects root") != String::npos,
          "actual SQLite trigger rejects first attempt");
    EntityKey assigned(
        std::allocator_arg, Allocator(r), "AllocationRoot", Value(r, std::uint64_t(501)));
    check(ids.allocated.size() == 1 && ids.allocated.front() == assigned,
          "failed attempt really allocated root501");
    check(root_floor(fixture) == 500 && fixture.rows() == 0 && fixture.safe.events.empty(),
          "failed write rolls back allocated floor and row without safe audit");
    check(sqlite3_get_autocommit(fixture.connection.native_handle()) != 0,
          "failed write restores autocommit");
    auto state = *root.entity_runtime_state();
    EntityKey pending(
        std::allocator_arg, Allocator(r), "AllocationRoot", Value(r, std::uint64_t(0)));
    check(state.new_keys().contains(pending) && !state.current_change_set().changes().empty(),
          "failed save retains zero-ID new marker and pending ledger");
    check(bool(ids.generator.ensure_floor(r, "AllocationRoot", 700)),
          "another operation advances sequence before retry");
    root.name = "accepted";
    state.set(pending, "name", Value(r, "accepted"));
    fixture.capture.commands.clear();
    fixture.capture.metadata.clear();
    fixture.safe.events.clear();
    user_context.clear_sql_logs();
    auto& saved = save_audited_ledger_entity(Audited<AllocationRoot>(std::allocator_arg,
                                                                     Allocator(r),
                                                                     std::move(root),
                                                                     "retry reviewed allocation"),
                                             user_context)
                      .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    check(saved.value().id == 701 && root_floor(fixture) == 701,
          "retry uses new database identity701");
    assert_root_sinks(fixture, 701, "retry reviewed allocation", "retry reviewed allocation");
    check(fixture.rows() == 1 && state.current_change_set().changes().empty(),
          "retry commits one row and clears original shared ledger");
}

static void database_id_failure_cannot_fall_back_to_an_in_process_identity() {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& ids = install_database_ids(fixture);
    constexpr auto ceiling = std::numeric_limits<std::int64_t>::max();
    check(
        bool(ids.generator.ensure_floor(r, "AllocationRoot", static_cast<std::uint64_t>(ceiling))),
        "reserve exhausted signed SQLite ID space");
    auto& failed = save_audited_ledger_entity(Audited<AllocationRoot>(std::allocator_arg,
                                                                      Allocator(r),
                                                                      new_root(r, "overflow"),
                                                                      "reject exhausted ID space"),
                                              user_context)
                       .wait();
    check(!failed && failed.error().to_string(r).find("overflow") != String::npos,
          "database ID overflow propagates");
    check(ids.allocated.empty() && fixture.capture.commands.empty() && fixture.safe.events.empty(),
          "overflow cannot fall back to process IDs, dispatch writes or publish audit");
    check(root_floor(fixture) == ceiling && fixture.rows() == 0,
          "overflow preserves floor without persisting root");
    check(sqlite3_get_autocommit(fixture.connection.native_handle()) != 0,
          "allocation error releases real transaction");
}

struct ExpectedFrame {
    std::string_view entity;
    std::uint64_t id;
    std::string_view reason;
};

static void assigned_chain(std::span<const TraceNode> chain,
                           std::span<const ExpectedFrame> expected) {
    std::size_t index = 0;
    for (const auto& node : chain) {
        if (node.kind != TraceKind::AuditReason) {
            continue;
        }
        check(index < expected.size(), "trace has no additional audit reason frames");
        const auto& frame = expected[index++];
        check(node.entity_type == frame.entity && node.entity_id == frame.id &&
                  node.comment == frame.reason,
              "ordered allocated identity and reason survive consumer boundary");
    }
    check(index == expected.size(), "trace contains every expected audit reason frame");
}

static void insert_names(const MutationRequest& request, Vector<String>& names) {
    if (auto* command = std::get_if<InsertCommand>(&request.data)) {
        auto field = command->values.fields.find(String("name", names.get_allocator().resource()));
        if (field != command->values.fields.end()) {
            if (auto text = field->second.try_text()) {
                names.emplace_back(*text);
            }
        }
    } else if (auto* batch = std::get_if<MutationRequest::Batch>(&request.data)) {
        for (const auto& child : *batch) {
            insert_names(child, names);
        }
    }
}

static void
assert_insert(Fixture& fixture, std::string_view entity, std::span<const ExpectedFrame> expected) {
    auto r = fixture.user_context->memory_resource();
    const MutationRequest* request = nullptr;
    for (const auto& command : fixture.capture.commands) {
        if (auto* found = find_insert(command, entity)) {
            request = found;
            break;
        }
    }
    check(request != nullptr, "actual lower-ledger entity INSERT exists");
    const auto& command = std::get<InsertCommand>(request->data);
    auto assigned = command.values.fields.at(String("id", r)).try_u64();
    check(assigned && *assigned > 0, "actual INSERT has assigned identity");
    assigned_chain(request->trace_chain(), expected);
    unsigned physical = 0;
    auto inspect = [&](const ExecutionMetadata& record) {
        if (record.operation != DataServiceOperation::Insert) {
            return;
        }
        bool relevant = false, identified = false;
        for (const auto& node : record.trace_chain) {
            if (node.kind == TraceKind::Entity && node.entity_type == entity) {
                relevant = true;
                identified |= node.entity_id == assigned;
            }
        }
        if (relevant) {
            check(identified, "physical SQL entity frame retains assigned identity");
            assigned_chain(record.trace_chain, expected);
            ++physical;
        }
    };
    for (const auto& record : fixture.capture.metadata) {
        if (record.statements.empty()) {
            inspect(record);
        } else {
            for (const auto& statement : record.statements) {
                inspect(statement);
            }
        }
    }
    check(physical > 0, "physical SQL INSERT metadata observed");
    Vector<String> names(r);
    for (const auto& command : fixture.capture.commands) {
        insert_names(command, names);
    }
    std::sort(names.begin(), names.end(), [](const auto& a, const auto& b) {
        return a.size() > b.size();
    });
    Vector<String> safe_reasons(r);
    for (const auto& frame : expected) {
        String reason(frame.reason, r);
        for (const auto& name : names) {
            check(!name.empty(), "source fixture private names are nonempty");
            std::size_t position = 0;
            while ((position = reason.find(name, position)) != String::npos) {
                reason.replace(position, name.size(), "[REDACTED]");
                position += std::string_view("[REDACTED]").size();
            }
        }
        safe_reasons.emplace_back(std::move(reason));
    }
    Vector<ExpectedFrame> safe_expected(r);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        safe_expected.push_back({expected[i].entity, expected[i].id, safe_reasons[i]});
    }
    unsigned audits = 0;
    for (const auto& event : fixture.safe.events) {
        if (event.entity != entity || event.kind != RawAuditEventKind::Created) {
            continue;
        }
        assigned_chain(event.trace_chain, safe_expected);
        bool found = false;
        for (const auto& field : event.fields) {
            if (field.name == "id" && field.value &&
                std::string_view(*field.value) == std::to_string(*assigned)) {
                found = true;
            }
        }
        check(found, "safe committed event retains actual assigned ID field");
        ++audits;
    }
    check(audits == 1, "one committed safe creation event per entity");
}

static AllocationChild new_child(Memory r, std::uint64_t root_id, std::string_view name) {
    CompactRow row(std::allocator_arg, Allocator(r));
    row.insert("id", Value(r, std::uint64_t(0)));
    row.insert("version", Value(r, std::int64_t(0)));
    row.insert("root_id", Value(r, root_id));
    row.insert("name", Value(r, name));
    auto decoded = AllocationChild::from_compact_row(r, std::move(row));
    check(bool(decoded), "source-shaped new child decodes");
    auto child = std::move(decoded).value();
    child.mark_as_new();
    child.root.set(child.entity_key(), "root_id", Value(r, root_id));
    child.root.set(child.entity_key(), "name", Value(r, name));
    return child;
}

static std::int64_t child_floor(Fixture& fixture) {
    sqlite3_stmt* raw = nullptr;
    auto status = sqlite3_prepare_v2(
        fixture.connection.native_handle(),
        "SELECT current_level FROM teaql_id_space WHERE type_name='allocation_child'",
        -1,
        &raw,
        nullptr);
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    check(status == SQLITE_OK && sqlite3_step(raw) == SQLITE_ROW, "read actual child ID floor");
    return sqlite3_column_int64(raw, 0);
}

static void persisted_child(Fixture& fixture, std::int64_t parent, bool check_quantity) {
    sqlite3_stmt* raw = nullptr;
    auto status =
        sqlite3_prepare_v2(fixture.connection.native_handle(),
                           "SELECT root_id,quantity FROM allocation_child_data WHERE id=801",
                           -1,
                           &raw,
                           nullptr);
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    check(status == SQLITE_OK && sqlite3_step(raw) == SQLITE_ROW, "read persisted child801");
    check(sqlite3_column_type(raw, 0) == SQLITE_INTEGER && sqlite3_column_int64(raw, 0) == parent,
          "actual child foreign key matches assigned or existing parent");
    if (check_quantity) {
        check(sqlite3_column_type(raw, 1) == SQLITE_INTEGER && sqlite3_column_int64(raw, 1) == 0,
              "ordinary numeric zero is not rebound or replaced with NULL");
    }
}

static void database_child_allocation_preserves_existing_parent_and_local_reason() {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& seeded = save_audited_ledger_entity(Audited<AllocationRoot>(std::allocator_arg,
                                                                      Allocator(r),
                                                                      new_root(r, "parent", 42),
                                                                      "seed parent"),
                                              user_context)
                       .wait();
    check(bool(seeded), "existing root42 seed persists");
    auto root = detail::clone_in(r, seeded.value());
    auto& ids = install_database_ids(fixture);
    auto child = new_child(r, 42, "database child");
    child.set_comment("allocate database child");
    check(bool(LedgerEntityDefaults<AllocationRoot>::compose(root, child)),
          "attach pending database child");
    auto& saved = save_audited_ledger_entity(Audited<AllocationRoot>(std::allocator_arg,
                                                                     Allocator(r),
                                                                     std::move(root),
                                                                     "attach database child"),
                                             user_context)
                      .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    EntityKey expected(
        std::allocator_arg, Allocator(r), "AllocationChild", Value(r, std::uint64_t(801)));
    check(saved.value().id == 42 && ids.allocated.size() == 1 && ids.allocated.front() == expected,
          "only child is allocated and existing parent remains42");
    check(child_floor(fixture) == 801 && root_floor(fixture) == 500,
          "child allocation leaves reserved root floor untouched");
    const std::array frames{ExpectedFrame{"AllocationRoot", 42, "attach database child"},
                            ExpectedFrame{"AllocationChild", 801, "allocate database child"}};
    assert_insert(fixture, "AllocationChild", frames);
    persisted_child(fixture, 42, false);
}

static void assert_safe_child_insert_log(Fixture& fixture,
                                         std::string_view reason,
                                         SqlExecutionOutcome outcome) {
    unsigned logs = 0;
    for (const auto& log : fixture.user_context->sql_logs()) {
        if (log.operation != SqlLogOperation::Insert) {
            continue;
        }
        bool child = false;
        for (const auto& node : log.trace_path) {
            if (node.kind == TraceKind::Entity && node.entity_type == "AllocationChild") {
                child = true;
            }
        }
        if (!child) {
            continue;
        }
        check(log.audit_reason && *log.audit_reason == reason &&
                  log.log_context.execution_outcome == outcome,
              "actual safe child INSERT log retains owned root reason and expected outcome");
        check(!log.trace_path.empty() && log.trace_path.front().entity_type == "AllocationRoot" &&
                  log.trace_path.back().kind == TraceKind::Sql,
              "safe SQL path keeps root-to-SQL provenance");
        for (const auto& node : log.trace_path) {
            check(node.kind != TraceKind::Comment && node.kind != TraceKind::Purpose &&
                      node.kind != TraceKind::AuditReason,
                  "safe SQL path excludes raw natural-language reason frames");
        }
        ++logs;
    }
    check(logs == 1, "one actual safe child INSERT log");
}

static void assert_new_database_graph(bool reverse, bool forward) {
    Fixture fixture(reverse, forward);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& ids = install_database_ids(fixture);
    auto root = new_root(r, "new graph");
    auto child = new_child(r, 0, "new child");
    child.set_comment("allocate new graph child");
    child.root.set(child.entity_key(), "quantity", Value(r, std::int64_t(0)));
    check(bool(LedgerEntityDefaults<AllocationRoot>::compose(root, child)),
          "compose unassigned database graph");
    auto& saved = save_audited_ledger_entity(
                      Audited<AllocationRoot>(
                          std::allocator_arg, Allocator(r), std::move(root), "allocate new graph"),
                      user_context)
                      .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    EntityKey root_key(
        std::allocator_arg, Allocator(r), "AllocationRoot", Value(r, std::uint64_t(501)));
    EntityKey child_key(
        std::allocator_arg, Allocator(r), "AllocationChild", Value(r, std::uint64_t(801)));
    check(saved.value().id == 501 && ids.allocated.size() == 2 && ids.allocated[0] == root_key &&
              ids.allocated[1] == child_key,
          "actual allocator assigns root501 then child801");
    check(root_floor(fixture) == 501 && child_floor(fixture) == 801,
          "both sequence floors commit with graph");
    const std::array root_frames{ExpectedFrame{"AllocationRoot", 501, "allocate new graph"}};
    const std::array child_frames{
        ExpectedFrame{"AllocationRoot", 501, "allocate new graph"},
        ExpectedFrame{"AllocationChild", 801, "allocate new graph child"}};
    assert_insert(fixture, "AllocationRoot", root_frames);
    assert_insert(fixture, "AllocationChild", child_frames);
    persisted_child(fixture, 501, true);
    check(fixture.safe.events.size() == 2,
          "both relation directions never duplicate committed audits");
    assert_safe_child_insert_log(fixture, "allocate [REDACTED]", SqlExecutionOutcome::Success);
}

static void database_graph_failure_retains_assigned_trace_but_rolls_back_rows_and_retryable_ids() {
    Fixture fixture;
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    auto& ids = install_database_ids(fixture);
    check(
        sqlite3_exec(fixture.connection.native_handle(),
                     "CREATE TRIGGER reject_allocated_child BEFORE INSERT ON allocation_child_data "
                     "BEGIN SELECT RAISE(ABORT, 'fixture rejects child after root insert'); END;",
                     nullptr,
                     nullptr,
                     nullptr) == SQLITE_OK,
        "install actual child failure trigger");
    auto root = new_root(r, "retryable graph");
    auto child = new_child(r, 0, "retryable child");
    child.set_comment("authorize retryable child");
    check(bool(LedgerEntityDefaults<AllocationRoot>::compose(root, child)),
          "compose retryable pending graph");
    auto& failed =
        save_audited_ledger_entity(
            Audited<AllocationRoot>(std::allocator_arg, Allocator(r), root, "first graph attempt"),
            user_context)
            .wait();
    check(!failed && failed.error().to_string(r).find("fixture rejects child after root insert") !=
                         String::npos,
          "actual child INSERT fails after root INSERT");
    EntityKey root_key(
        std::allocator_arg, Allocator(r), "AllocationRoot", Value(r, std::uint64_t(501)));
    EntityKey child_key(
        std::allocator_arg, Allocator(r), "AllocationChild", Value(r, std::uint64_t(801)));
    check(ids.allocated.size() == 2 && ids.allocated[0] == root_key &&
              ids.allocated[1] == child_key,
          "failed graph allocated both database identities in order");
    auto child_rows = [&] {
        sqlite3_stmt* raw = nullptr;
        auto status = sqlite3_prepare_v2(fixture.connection.native_handle(),
                                         "SELECT count(*) FROM allocation_child_data",
                                         -1,
                                         &raw,
                                         nullptr);
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
        check(status == SQLITE_OK && sqlite3_step(raw) == SQLITE_ROW,
              "read actual child row count");
        return sqlite3_column_int64(raw, 0);
    };
    check(fixture.safe.events.empty() && fixture.rows() == 0 && child_rows() == 0,
          "partial graph failure rolls back both tables without committed safe audit");
    check(root_floor(fixture) == 500 && child_floor(fixture) == 800,
          "partial graph failure restores both sequence floors");
    check(sqlite3_get_autocommit(fixture.connection.native_handle()) != 0,
          "partial graph failure releases actual transaction");
    assert_safe_child_insert_log(fixture, "first graph attempt", SqlExecutionOutcome::Failure);
    Vector<const ExecutionMetadata*> statements(r);
    auto collect = [&](const ExecutionMetadata& record) {
        if (record.operation == DataServiceOperation::Insert) {
            statements.push_back(&record);
        }
    };
    for (const auto& record : fixture.capture.metadata) {
        if (record.statements.empty()) {
            collect(record);
        } else {
            for (const auto& statement : record.statements) {
                collect(statement);
            }
        }
    }
    check(statements.size() == 2, "exactly two actual physical INSERT outcomes");
    check(statements[0]->sql_log.execution_outcome == SqlExecutionOutcome::Success &&
              statements[1]->sql_log.execution_outcome == SqlExecutionOutcome::Failure,
          "diagnostics retain successful root and failed child outcomes after rollback");
    const std::array root_frames{ExpectedFrame{"AllocationRoot", 501, "first graph attempt"}};
    const std::array child_frames{
        ExpectedFrame{"AllocationRoot", 501, "first graph attempt"},
        ExpectedFrame{"AllocationChild", 801, "authorize retryable child"}};
    assigned_chain(statements[0]->trace_chain, root_frames);
    assigned_chain(statements[1]->trace_chain, child_frames);
    check(sqlite3_exec(fixture.connection.native_handle(),
                       "DROP TRIGGER reject_allocated_child",
                       nullptr,
                       nullptr,
                       nullptr) == SQLITE_OK,
          "remove rejection trigger for retry");
    check(bool(ids.generator.ensure_floor(r, "AllocationRoot", 900)) &&
              bool(ids.generator.ensure_floor(r, "AllocationChild", 1200)),
          "advance actual database floors before retry");
    fixture.capture.commands.clear();
    fixture.capture.metadata.clear();
    fixture.safe.events.clear();
    user_context.clear_sql_logs();
    auto& saved = save_audited_ledger_entity(Audited<AllocationRoot>(std::allocator_arg,
                                                                     Allocator(r),
                                                                     std::move(root),
                                                                     "reviewed graph retry"),
                                             user_context)
                      .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    check(saved.value().id == 901, "retry receives fresh root identity");
    const std::array retry_root{ExpectedFrame{"AllocationRoot", 901, "reviewed graph retry"}};
    const std::array retry_child{
        ExpectedFrame{"AllocationRoot", 901, "reviewed graph retry"},
        ExpectedFrame{"AllocationChild", 1201, "authorize retryable child"}};
    assert_insert(fixture, "AllocationRoot", retry_root);
    assert_insert(fixture, "AllocationChild", retry_child);
    check(root_floor(fixture) == 901 && child_floor(fixture) == 1201 && fixture.rows() == 1 &&
              child_rows() == 1,
          "retry commits both new sequence floors and exactly one complete graph");
}

// Own the file directory before either connection is constructed, and destroy it
// after both connections and the Context have closed, including exception paths.
struct OwnedDatabaseDirectory {
    std::filesystem::path path;

    OwnedDatabaseDirectory() {
        auto pattern =
            (std::filesystem::temp_directory_path() / "teaql_blank_local_XXXXXX").string();
        auto* created = ::mkdtemp(pattern.data());
        check(created != nullptr, "create exclusively owned SQLite probe directory");
        path = created;
    }

    ~OwnedDatabaseDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

struct BlankLocalCommitProbe final : SafeAuditEventSink {
    using allocator_type = Allocator;
    SqliteConnection& independent;
    mutable Vector<SafeAuditEvent> events;

    BlankLocalCommitProbe(std::allocator_arg_t, Allocator a, SqliteConnection& independent)
        : independent(independent), events(a.resource()) {}

    Memory memory_resource() const {
        return events.get_allocator().resource();
    }

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory r, const UserContext&, const SafeAuditEvent& event) const override {
        std::string_view table;
        if (event.entity == "AllocationRoot") {
            table = "allocation_root_data";
        } else if (event.entity == "Payment") {
            table = "payment_data";
        } else if (event.entity == "PaymentAttempt") {
            table = "payment_attempt_data";
        } else if (event.entity == "Shipment") {
            table = "shipment_data";
        } else {
            throw std::runtime_error("unexpected committed audit target");
        }
        check(event.entity_id.has_value(), "committed audit has target ID");
        check(sqlite3_get_autocommit(independent.native_handle()) != 0,
              "independent audit connection has no open transaction");
        String sql("SELECT id,version FROM ", r);
        sql += table;
        sql += " WHERE id=?";
        sqlite3_stmt* raw = nullptr;
        auto status =
            sqlite3_prepare_v2(independent.native_handle(), sql.c_str(), -1, &raw, nullptr);
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
        check(status == SQLITE_OK, "independent committed-row query prepares");
        check(
            sqlite3_bind_int64(raw, 1, static_cast<sqlite3_int64>(*event.entity_id)) == SQLITE_OK &&
                sqlite3_step(raw) == SQLITE_ROW &&
                sqlite3_column_int64(raw, 0) == static_cast<sqlite3_int64>(*event.entity_id) &&
                sqlite3_column_int64(raw, 1) == 1,
            "separate connection sees committed target identity and version before audit capture");
        events.emplace_back(event);
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

struct BlankLocalFixture {
    OwnedDatabaseDirectory directory;
    GlobalContext global;
    std::unique_ptr<UserContext> user_context = global.make_user_context();
    SqliteConnection& connection =
        user_context->make_resource<SqliteConnection>((directory.path / "trace.sqlite").string());
    SqliteConnection& independent =
        user_context->make_resource<SqliteConnection>((directory.path / "trace.sqlite").string());
    Capture& capture = user_context->make<Capture>();
    BlankLocalCommitProbe& safe = user_context->make<BlankLocalCommitProbe>(independent);
    ObservedExecutor* executor = nullptr;

    explicit BlankLocalFixture(bool logging) {
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        auto root = AllocationRoot::entity_descriptor(r);
        auto relation = [](EntityDescriptor& entity,
                           std::string_view name,
                           std::string_view target,
                           std::string_view foreign) {
            auto& edge = entity.relation(name, target);
            edge.many = true;
            edge.local_key = "id";
            edge.foreign_key = foreign;
        };
        relation(root, "payments", "Payment", "customer_order");
        relation(root, "shipments", "Shipment", "customer_order");
        metadata.register_entity(std::move(root));
        auto parent = [&](std::string_view name, std::string_view table, std::string_view foreign) {
            EntityDescriptor entity(std::allocator_arg, Allocator(r), name);
            entity.table(table);
            entity.property("id", DataType::U64).id();
            entity.property("version", DataType::I64).version();
            entity.property("name", DataType::Text);
            entity.property(foreign, DataType::U64);
            return entity;
        };
        auto payment = parent("Payment", "payment_data", "customer_order");
        relation(payment, "attempts", "PaymentAttempt", "payment");
        metadata.register_entity(std::move(payment));
        metadata.register_entity(parent("PaymentAttempt", "payment_attempt_data", "payment"));
        metadata.register_entity(parent("Shipment", "shipment_data", "customer_order"));
        user_context->set_metadata(*user_context, metadata);
        auto& transport = user_context->make<SqliteMutationExecutor>(connection);
        use_sqlite_provider(*user_context, *user_context, transport);
        auto dialect = user_context->require_resource<SqliteDialect>();
        check(bool(dialect), "blank-local SQLite dialect installed");
        auto& state = user_context->make<SqlExecutorState>();
        auto& native =
            user_context->make<NativeExecutor>(dialect.value().get(), transport, metadata, state);
        executor = &user_context->make<ObservedExecutor>(*user_context, native, capture);
        user_context->register_executor(*user_context, *executor);
        auto& registry = user_context->make<InMemoryEntityRegistry>();
        for (auto name : {"AllocationRoot", "Payment", "PaymentAttempt", "Shipment"}) {
            registry.register_entity(name);
        }
        user_context->set_entity_registry(*user_context, registry);
        check(bool(user_context->ensure_schema().wait()), "blank-local schema initialized");
        user_context->set_custom_event_sink(*user_context, safe);
        if (!logging) {
            user_context->disable_sql_log();
        }
        capture.commands.clear();
        capture.metadata.clear();
        user_context->clear_sql_logs();
    }
};

static void run_blank_local_execution(bool logging,
                                      std::span<const std::optional<std::string_view>> reasons,
                                      bool inherit) {
    BlankLocalFixture fixture(logging);
    auto& user_context = *fixture.user_context;
    auto r = user_context.memory_resource();
    if (inherit) {
        for (auto reason : reasons) {
            auto invalid_intent = MutationIntent::create(r, reason.value_or(""));
            check(!invalid_intent, "original blank vector fails intent validation");
            bool rejected = false;
            try {
                Audited<AllocationRoot> invalid(std::allocator_arg,
                                                Allocator(r),
                                                new_root(r, "invalid root", 9999),
                                                reason.value_or(""));
            } catch (const std::invalid_argument& error) {
                rejected = std::string_view(error.what()) == invalid_intent.error().to_string(r);
            }
            check(rejected, "fluent audited constructor rejects with exact intent message");
            GraphNode node(std::allocator_arg, Allocator(r), "AllocationRoot");
            node.with_operation(GraphOperation::Create).value("id", std::uint64_t(9999));
            if (reason) {
                node.with_comment(*reason);
            }
            auto& service = EntityDataService<ObservedExecutor>::for_executor(
                user_context, "AllocationRoot", user_context, *fixture.executor);
            auto& result = service.plan_graph(std::move(node)).wait();
            check(!result &&
                      result.error().runtime_error().kind() == RuntimeError::Kind::RequestIntent,
                  "graph planning returns structured request intent error");
            const auto& error = result.error().runtime_error().request_intent_error();
            check(error.code() == "REQUEST_COMMENT_REQUIRED" && error.field == "comment" &&
                      error.request_kind == RequestKind::Mutation,
                  "exact graph intent error fields");
            check(fixture.capture.commands.empty() && fixture.capture.metadata.empty() &&
                      fixture.safe.events.empty() && user_context.sql_logs().empty(),
                  "invalid owned intent fails before all execution sinks even with logs disabled");
        }
    }
    auto root = new_root(r, "native root", 100);
    auto ledger = *root.entity_runtime_state();
    auto add = [&](std::string_view entity,
                   std::uint64_t id,
                   std::string_view foreign,
                   std::uint64_t parent,
                   std::optional<std::string_view> reason) {
        EntityKey key(std::allocator_arg, Allocator(r), entity, Value(r, id));
        ledger.mark_as_new(key);
        ledger.set(key, foreign, Value(r, parent));
        String name("native ", r);
        name += entity;
        name += " ";
        name += std::to_string(id);
        ledger.set(key, "name", Value(r, name));
        if (reason) {
            ledger.set_entity_comment(key, *reason);
        }
    };
    add("Payment", 201, "customer_order", 100, "authorize payment");
    add("Shipment", 301, "customer_order", 100, "prepare shipment");
    for (std::size_t i = 0; i < reasons.size(); ++i) {
        add("PaymentAttempt", 400 + i, "payment", 201, reasons[i]);
    }
    auto& saved = save_audited_ledger_entity(
                      Audited<AllocationRoot>(
                          std::allocator_arg, Allocator(r), std::move(root), "submit order"),
                      user_context)
                      .wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    check(saved.value().id == 100 && saved.value().version_value == 1,
          "authoritative root100 version1");
    Vector<const MutationRequest*> commands(r);
    auto flatten = [&](auto&& self, const MutationRequest& request) -> void {
        if (auto* batch = std::get_if<MutationRequest::Batch>(&request.data)) {
            for (const auto& item : *batch) {
                self(self, item);
            }
        } else {
            commands.push_back(&request);
        }
    };
    for (const auto& request : fixture.capture.commands) {
        flatten(flatten, request);
    }
    Vector<const ExecutionMetadata*> physical(r);
    for (const auto& record : fixture.capture.metadata) {
        if (record.statements.empty()) {
            physical.push_back(&record);
        } else {
            for (const auto& statement : record.statements) {
                physical.push_back(&statement);
            }
        }
    }
    check(commands.size() == reasons.size() + 3 && fixture.safe.events.size() == commands.size(),
          "all ledger members produce exactly one safe committed audit");
    std::size_t inserts = 0;
    bool readback = false;
    for (const auto* record : physical) {
        inserts += record->operation == DataServiceOperation::Insert;
        readback |= record->operation == DataServiceOperation::Query;
    }
    check(inserts == commands.size() && readback,
          "one physical INSERT each plus authoritative readback");
    for (const auto* request : commands) {
        auto* command = std::get_if<InsertCommand>(&request->data);
        check(command != nullptr, "create fixture has only INSERT commands");
        auto id = command->values.fields.at(String("id", r)).try_u64();
        check(id.has_value(), "command has assigned id");
        Vector<ExpectedFrame> expected(r);
        expected.push_back({"AllocationRoot", 100, "submit order"});
        if (command->entity == "Payment" || command->entity == "PaymentAttempt") {
            expected.push_back({"Payment", 201, "authorize payment"});
            if (command->entity == "PaymentAttempt" && !inherit) {
                check(*id >= 400 && *id - 400 < reasons.size(), "attempt ID indexes source reason");
                expected.push_back({"PaymentAttempt", *id, *reasons[*id - 400]});
            }
        } else if (command->entity == "Shipment") {
            expected.push_back({"Shipment", 301, "prepare shipment"});
        }
        check(request->comment() == "submit order",
              "request-owned reason never becomes local reason");
        assigned_chain(request->trace_chain(), expected);
        unsigned audits = 0;
        for (const auto& event : fixture.safe.events) {
            if (event.entity == command->entity && event.entity_id == id) {
                assigned_chain(event.trace_chain, expected);
                ++audits;
            }
        }
        check(audits == 1, "one exact committed audit per entity and id");
        unsigned statements = 0;
        for (const auto* record : physical) {
            if (record->operation != DataServiceOperation::Insert) {
                continue;
            }
            bool matches = false;
            for (const auto& node : record->trace_chain) {
                matches |= node.kind == TraceKind::Entity && node.entity_type == command->entity &&
                           node.entity_id == id;
            }
            if (!matches) {
                continue;
            }
            assigned_chain(record->trace_chain, expected);
            check(record->comment == "submit order" && record->affected_rows == 1 &&
                      record->sql_log.execution_outcome == SqlExecutionOutcome::Success,
                  "physical metadata keeps owned reason, one affected row and success");
            ++statements;
        }
        check(statements == 1, "one matching physical INSERT per entity and id");
    }
    auto logs = user_context.sql_logs();
    if (logging) {
        check(logs.size() == physical.size(), "one actual safe log for each physical statement");
        for (const auto& log : logs) {
            check(log.audit_reason == "submit order" && !log.trace_path.empty() &&
                      log.trace_path.front().entity_type == "AllocationRoot" &&
                      log.trace_path.back().kind == TraceKind::Sql,
                  "safe log retains owned intent and root-to-SQL path");
        }
    } else {
        check(logs.empty(), "SQL logging disabled while all other contracts remain active");
    }
    check(sqlite3_get_autocommit(fixture.connection.native_handle()) != 0,
          "save transaction is closed");
}

static void blank_local_execution(bool logging) {
    const std::array<std::optional<std::string_view>, 6> reasons{
        std::nullopt, "", " \t\r\n", "\u0085", "\u00a0", "\u2003"};
    run_blank_local_execution(logging, reasons, true);
}

static void non_white_space_local_execution(bool logging) {
    const std::array<std::optional<std::string_view>, 4> reasons{"\x1c", "\x1d", "\x1e", "\x1f"};
    run_blank_local_execution(logging, reasons, false);
}

int main(int argc, char** argv) {
    try {
        check(argc <= 2, "expected at most one source case selector");
        bool ran = false;
        auto run = [&](std::string_view selector, auto function) {
            if (argc == 1 || std::string_view(argv[1]) == selector) {
                function();
                std::cout << selector << " passed\n";
                ran = true;
            }
        };
        run("--observer", observer_delegates_real_execution);
        run("--slat01",
            root_allocated_during_ledger_planning_reaches_command_sql_and_committed_audit);
        run("--slat02", child_allocated_during_ledger_planning_retains_its_own_and_parent_identity);
        run("--slat03", database_root_allocation_inside_transaction_reaches_all_trace_boundaries);
        run("--slat04", database_child_allocation_preserves_existing_parent_and_local_reason);
        run("--slat05", [] {
            assert_new_database_graph(true, false);
        });
        run("--slat06", [] {
            assert_new_database_graph(false, true);
        });
        run("--slat07", [] {
            assert_new_database_graph(true, true);
        });
        run("--slat08", database_allocation_rolls_back_and_retry_uses_new_identity_and_intent);
        run("--slat09", database_id_failure_cannot_fall_back_to_an_in_process_identity);
        run("--slat10",
            database_graph_failure_retains_assigned_trace_but_rolls_back_rows_and_retryable_ids);
        run("--slat11", database_allocated_explicit_transaction_delivers_audit_only_after_commit);
        run("--slat12", [] {
            blank_local_execution(true);
        });
        run("--slat13", [] {
            blank_local_execution(false);
        });
        run("--slat14", [] {
            non_white_space_local_execution(true);
        });
        run("--slat15", [] {
            non_white_space_local_execution(false);
        });
        run("--scope-cleanup", database_scope_failure_and_cancellation);
        check(ran, "unknown source case selector");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
