// Shared handwritten fixtures for direct pinned runtime-root contracts.
#pragma once
#include "../../../examples/models.hpp"
#include <teaql/sql/dialect.hpp>
#include <array>
#include <bit>
#include <charconv>
#include <iostream>

namespace original_runtime_root {
using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct TestDialect final : SqlDialect {
    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory owner, std::string_view name) const override {
        return quote_identifier_if_needed(owner, name, '"');
    }

    String placeholder(Memory owner, std::size_t index) const override {
        char digits[32];
        auto end = std::to_chars(digits, digits + sizeof(digits), index).ptr;
        String result("?", owner);
        result.append(digits, end);
        return result;
    }
};

static EntityDescriptor entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Order");
    result.table("orders");
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version().not_null();
    result.property("name", DataType::Text);
    result.relation("lines", "OrderLine").local("id").foreign("order_id").to_many();
    return result;
}

static EntityDescriptor line_entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "OrderLine");
    result.table("orderline");
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version();
    result.property("order_id", DataType::U64).not_null();
    result.property("name", DataType::Text);
    result.property("product_id", DataType::U64).not_null();
    result.relation("product", "Product").local("product_id").foreign("id");
    return result;
}

static EntityDescriptor product_entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Product");
    result.table("product");
    result.property("id", DataType::U64).id().not_null();
    result.property("name", DataType::Text);
    return result;
}

struct StubError {
    const char* what() const noexcept {
        return "stub error";
    }
};

template <class T>
struct Ready {
    using allocator_type = Allocator;
    Result<T, StubError> result;

    Ready(std::allocator_arg_t, Allocator a, T value)
        : result(Result<T, StubError>::ok(a.resource(), std::move(value))) {}

    std::optional<Result<T, StubError>> poll(WakeSignal&) {
        return detail::clone_in(result.memory_resource(), result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct EmptyStream {
    using allocator_type = Allocator;

    EmptyStream(std::allocator_arg_t, Allocator) {}

    std::optional<std::optional<Result<StreamChunk, StubError>>> poll_next(WakeSignal&) {
        return std::optional<Result<StreamChunk, StubError>>{};
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

// Controlled original provider fixture. It does not implement service behavior.
struct StubExecutor final : QueryExecutor<StubError>,
                            MutationExecutor<StubError>,
                            StreamQueryExecutor<StubError> {
    using allocator_type = Allocator;
    Memory owner;
    std::uint64_t affected;
    Vector<Record> rows;
    unsigned queries = 0, mutations = 0, streams = 0;

    StubExecutor(std::allocator_arg_t, Allocator a, std::uint64_t affected = 0)
        : owner(a.resource()), affected(affected), rows(owner) {}

    Memory memory_resource() const noexcept {
        return owner;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, StubError>& query(Memory target,
                                                              QueryRequest) override {
        ++queries;
        const auto now = std::chrono::system_clock::now();
        ExecutionMetadata metadata(
            std::allocator_arg, Allocator(target), DataServiceOperation::Query, now, now);
        metadata.backend = "stub";
        metadata.result_count = rows.size();
        QueryResult result(std::allocator_arg, Allocator(target), std::move(metadata));
        for (const auto& row : rows) {
            result.rows.emplace_back(row);
        }
        return make_operation<Result<QueryResult, StubError>, Ready<QueryResult>>(
            target, std::move(result));
    }

    DataServiceOperationResult<MutationResult, StubError>& mutate(Memory target,
                                                                  MutationRequest) override {
        ++mutations;
        const auto now = std::chrono::system_clock::now();
        ExecutionMetadata metadata(
            std::allocator_arg, Allocator(target), DataServiceOperation::Update, now, now);
        metadata.backend = "stub";
        metadata.affected_rows = affected;
        MutationResult result(std::allocator_arg, Allocator(target), affected, std::move(metadata));
        return make_operation<Result<MutationResult, StubError>, Ready<MutationResult>>(
            target, std::move(result));
    }

    QueryStream<StubError>& query_stream(Memory target, QueryRequest, std::size_t) override {
        ++streams;
        return make_stream<Result<StreamChunk, StubError>, EmptyStream>(target);
    }
};

struct OrderBehavior final : EntityDataServiceBehavior {
    RuntimeHookResult
    before_select(Memory owner, const UserContext&, SelectQuery& query) const override {
        query.filter(Expr::eq(owner, "version", std::int64_t(1)));
        return RuntimeHookResult::ok(owner, {});
    }

    RuntimeHookResult
    before_insert(Memory owner, const UserContext&, InsertCommand& command) const override {
        command.values.fields.try_emplace(String("version", owner), Value(owner, std::int64_t(1)));
        return RuntimeHookResult::ok(owner, {});
    }

    Vector<String> relation_loads(Memory owner, const UserContext&) const override {
        Vector<String> result(owner);
        result.emplace_back("lines");
        return result;
    }
};

struct TenantRequestPolicy final : RequestPolicy {
    RuntimeHookResult
    enforce_select(Memory owner, const UserContext& user, SelectQuery& query) const override {
        if (query.entity == "Order") {
            auto* tenant = user.get_named_resource<std::uint64_t>("tenant_id");
            if (!tenant) {
                return RuntimeHookResult::err(owner,
                                              RuntimeError::policy(owner, "missing tenant_id"));
            }
            auto predicate = Expr::eq(owner, "id", *tenant);
            query.filter(query.predicate ? query.predicate->and_expr(predicate)
                                         : std::move(predicate));
        }
        return RuntimeHookResult::ok(owner, {});
    }

    RuntimeHookResult
    enforce_insert(Memory owner, const UserContext& user, InsertCommand& command) const override {
        if (command.entity == "Order") {
            auto* tenant = user.get_named_resource<std::uint64_t>("tenant_id");
            if (!tenant) {
                return RuntimeHookResult::err(owner,
                                              RuntimeError::policy(owner, "missing tenant_id"));
            }
            command.value("version", std::bit_cast<std::int64_t>(*tenant));
        }
        return RuntimeHookResult::ok(owner, {});
    }
};

struct DenySelectRequestPolicy final : RequestPolicy {
    RuntimeHookResult
    enforce_select(Memory owner, const UserContext&, SelectQuery&) const override {
        return RuntimeHookResult::err(owner, RuntimeError::policy(owner, "QUERY_POLICY_PROBE"));
    }
};

struct ContextAwareOrderBehavior final : EntityDataServiceBehavior {
    RuntimeHookResult
    before_insert(Memory owner, const UserContext& user, InsertCommand& command) const override {
        auto* tenant = user.get_named_resource<String>("tenant");
        auto* version = user.get_named_resource<std::int64_t>("initial_version");
        auto* trace = user.local("trace_id");
        if (!tenant) {
            return RuntimeHookResult::err(owner,
                                          RuntimeError::behavior(owner, "missing tenant resource"));
        }
        if (!version) {
            return RuntimeHookResult::err(
                owner, RuntimeError::behavior(owner, "missing initial_version resource"));
        }
        if (!trace || !std::holds_alternative<String>(trace->data)) {
            String message("missing trace_id local, got ", owner);
            if (trace) {
                message += "Some(";
                message += detail::debug_value(owner, *trace);
                message += ')';
            } else {
                message += "None";
            }
            return RuntimeHookResult::err(owner, RuntimeError::behavior(owner, message));
        }
        String name(*tenant, owner);
        name += ':';
        name += *trace->try_text();
        command.values.fields.try_emplace(String("name", owner), Value(owner, name));
        command.values.fields.try_emplace(String("version", owner), Value(owner, *version));
        return RuntimeHookResult::ok(owner, {});
    }
};

struct FixedIdGenerator final : InternalIdGenerator {
    std::uint64_t id;

    explicit FixedIdGenerator(std::uint64_t id) : id(id) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory owner, std::string_view) const override {
        return Result<std::uint64_t, RuntimeError>::ok(owner, id);
    }
};

struct RecordingScope final : RuntimeTelemetryScope {
    using allocator_type = Allocator;
    Vector<String>& events;

    RecordingScope(std::allocator_arg_t, Allocator, Vector<String>& events) : events(events) {}

    void success(const RuntimeAttributes&) override {
        events.emplace_back("success");
    }

    void failure(std::string_view) override {
        events.emplace_back("failure");
    }
};

struct RecordingTelemetry final : RuntimeTelemetry {
    using allocator_type = Allocator;
    Memory owner;
    Vector<String>& events;

    RecordingTelemetry(std::allocator_arg_t, Allocator a, Vector<String>& events)
        : owner(a.resource()), events(events) {}

    Memory memory_resource() const noexcept override {
        return owner;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory target,
                                             const RuntimeOperation& operation) const override {
        String event("start:", owner);
        event += operation.family;
        events.emplace_back(std::move(event));
        return detail::resource_arena(target).make<ContextBox<RuntimeTelemetryScope>>(
            std::in_place_type<RecordingScope>, events);
    }
};

// Original private Order mapping, independent of the example's table/relation layout.
struct Order : example::Model<Order> {
    static constexpr std::string_view ENTITY_NAME = "Order";
    std::int64_t revision = 0;

    Order(std::allocator_arg_t, Allocator a) : Model(std::allocator_arg, a) {}

    Order(std::allocator_arg_t, Allocator a, const Order& other)
        : Model(std::allocator_arg, a, other), revision(other.revision) {}

    Order(const Order&) = delete;
    Order(Order&&) noexcept = default;

    static EntityDescriptor entity_descriptor(Memory owner) {
        EntityDescriptor result(std::allocator_arg, Allocator(owner), ENTITY_NAME);
        result.table("orders");
        result.property("id", DataType::U64).id().not_null();
        result.property("version", DataType::I64).version().not_null();
        result.property("name", DataType::Text).not_null();
        return result;
    }

    static Result<Order, EntityError> from_compact_row(Memory owner, CompactRow row) {
        using Output = Result<Order, EntityError>;
        auto id = example::decode_field<std::uint64_t>(owner, row, ENTITY_NAME, "id");
        auto version = example::decode_field<std::int64_t>(owner, row, ENTITY_NAME, "version");
        auto name = example::decode_field<String>(owner, row, ENTITY_NAME, "name");
        if (!id) {
            return Output::err(owner, std::move(id).error());
        }
        if (!version) {
            return Output::err(owner, std::move(version).error());
        }
        if (!name) {
            return Output::err(owner, std::move(name).error());
        }
        Order result(std::allocator_arg, Allocator(owner));
        result.id = id.value();
        result.revision = version.value();
        result.name = std::move(name).value();
        return Output::ok(owner, std::move(result));
    }

    std::int64_t version() const noexcept {
        return revision;
    }

    MutationValues into_values() && {
        auto result = scalar_values();
        result.fields.emplace("version", Value(memory_resource(), revision));
        return result;
    }
};

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory owner = user->memory_resource();
    InMemoryMetadataStore& store = user->make<InMemoryMetadataStore>();
    StubExecutor& executor;

    Fixture(std::uint64_t affected = 1, bool related = false)
        : executor(user->make<StubExecutor>(affected)) {
        store.with_entity(entity(owner));
        if (related) {
            store.with_entity(line_entity(owner)).with_entity(product_entity(owner));
        }
        user->with_metadata(*user, store);
        auto& registry = user->make<InMemoryEntityRegistry>();
        registry.with_entity("Order");
        user->with_entity_registry(*user, registry);
        auto& dialect = user->make_resource<TestDialect>();
        user->insert_resource(*user, dialect);
        user->insert_resource(*user, executor);
    }

    EntityDataService<StubExecutor>& service() {
        auto bound = user->entity_data_service<StubExecutor>("Order");
        check(bool(bound), "original Context binds entity service");
        return bound.value().get();
    }

    template <class T>
    void behavior() {
        auto& registry = user->make<InMemoryEntityDataServiceBehaviorRegistry>();
        auto& hook = user->make_resource<T>();
        registry.register_behavior("Order", *user, hook);
        user->with_entity_data_service_behavior_registry(*user, registry);
    }

    void generator(std::uint64_t id) {
        auto& generator = user->make_resource<FixedIdGenerator>(id);
        user->with_internal_id_generator(*user, generator);
    }
};

}
