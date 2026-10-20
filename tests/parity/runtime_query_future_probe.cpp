#include <teaql/runtime.hpp>
#include <teaql/runtime/detail/context/pagination.hpp>
#include <array>
#include <iostream>
#include <type_traits>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Probe : EntityDefaults<Probe> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "FutureProbe";
    std::uint64_t id = 0;
    std::int64_t version = 0;
    String name;

    Probe(std::allocator_arg_t, Allocator a) : name(a.resource()) {}

    Probe(std::allocator_arg_t, Allocator a, const Probe& other)
        : id(other.id), version(other.version), name(other.name, a.resource()) {}

    Probe(std::allocator_arg_t, Allocator a, Probe&& other)
        : Probe(detail::move_to(a.resource(), std::move(other))) {}

    Probe(const Probe&) = delete;
    Probe(Probe&&) noexcept = default;

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.property("id", DataType::U64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        return descriptor;
    }

    static Result<Probe, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<Probe, EntityError>;
        auto* id = row.get("id");
        auto* version = row.get("version");
        auto* name = row.get("name");
        if (!id || !id->try_u64() || !version || !version->try_i64() || !name ||
            !name->try_text()) {
            return R::err(
                r,
                EntityError(std::allocator_arg, Allocator(r), ENTITY_NAME, "missing probe field"));
        }
        Probe value(std::allocator_arg, Allocator(r));
        value.id = *id->try_u64();
        value.version = *version->try_i64();
        value.name = *name->try_text();
        return R::ok(r, std::move(value));
    }

    MutationValues into_values() && {
        MutationValues result(memory_resource());
        result.fields.emplace("id", Value(memory_resource(), id));
        result.fields.emplace("version", Value(memory_resource(), version));
        result.fields.emplace("name", Value(memory_resource(), name));
        return result;
    }
};

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T result;

    Ready(std::allocator_arg_t, Allocator a, T value)
        : result(detail::move_in(a.resource(), std::move(value))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(result);
    }

    void cancel_and_wait() noexcept {}

    void finish_and_wait() noexcept {}
};

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory owner;

    Executor(std::allocator_arg_t, Allocator a) : owner(a.resource()) {}

    Memory memory_resource() const {
        return owner;
    }

    unsigned calls = 0;

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        ++calls;
        check(request.comment() == "what: measure ordinary query future", "validated query intent");
        QueryResult result(
            std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, 1));
        Record values(r);
        values.emplace("id", Value(r, std::uint64_t(1)));
        values.emplace("version", Value(r, std::int64_t(1)));
        values.emplace("name", Value(r, "probe"));
        result.rows.emplace_back(values);
        using R = Result<QueryResult, String>;
        return make_operation<R, Ready<R>>(r, R::ok(r, std::move(result)));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("read-only future regression");
    }
};

static_assert(std::same_as<runtime::EntityDataService<Executor>, EntityDataService<Executor>>);
static_assert(std::same_as<runtime::RelationLoadPlan, RelationLoadPlan>);
static_assert(std::same_as<runtime::AggregationCacheBackend, AggregationCacheBackend>);
static_assert(std::same_as<runtime::InMemoryAggregationCache, InMemoryAggregationCache>);
static_assert(std::same_as<runtime::GraphTransactionBoundary, GraphTransactionBoundary>);
static_assert(!std::same_as<detail::ContextDataService<Executor>, EntityDataService<Executor>>);
static_assert(std::derived_from<detail::UserContextMetadata, MetadataStore>);
static_assert(!std::same_as<detail::RuntimeDataService<InMemoryMetadataStore, Executor>,
                            detail::ContextDataService<Executor>>);
static_assert(std::is_trivially_copyable_v<GraphTransactionBoundary>);

int main() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    metadata.register_entity(Probe::entity_descriptor(r));
    user_context->set_metadata(*user_context, metadata);
    user_context->disable_sql_log();
    detail::ContextPaginationAccess::observe_id_set(*user_context, "UNEXECUTED", 77);
    auto& executor = user_context->make<Executor>();
    auto& service = EntityDataService<Executor>::for_executor(
        *user_context, "FutureProbe", *user_context, executor);
    constexpr std::string_view comment = "what: measure ordinary query future";
    constexpr std::string_view purpose = "why: avoid complex graph frame per ordinary query";
    detail::EntityDataServiceAccess::bind_query_intent(
        service, QueryIntent::create(r, comment, purpose).value());
    SelectQuery query(std::allocator_arg, Allocator(r), "FutureProbe");
    query.project("id").project("version").project("name").limit(1).comment(comment);
    PurposedSelectQuery wrapped(std::allocator_arg, Allocator(r), query, purpose);
    auto& operation = service.fetch_enhanced_entities<Probe>(wrapped);
    constexpr auto bytes = sizeof(detail::EnhancedEntitiesOperation<Executor, Probe>);
    std::cout << "ORDINARY_TYPED_QUERY_ARENA_FRAME_BYTES=" << bytes << '\n';
    check(bytes <= 8192, "ordinary arena state exceeds the source frame budget");
    auto plan = user_context->id_set_plan();
    check(plan && *plan == "UNEXECUTED" && user_context->id_set_count() == 77 &&
              executor.calls == 0,
          "unpolled operation must preserve observation and avoid provider execution");
    const auto& result = operation.wait();
    check(result && result.value().data.size() == 1 &&
              result.value().data.front().name == "probe" && result.value().data.front().id == 1 &&
              result.value().data.front().version == 1 && executor.calls == 1,
          "ordinary typed query result");
    plan = user_context->id_set_plan();
    check(plan && *plan == "ID_SET_DISABLED" && !user_context->id_set_count(),
          "executed ordinary ID-set observation");
    constexpr std::array values{GraphTransactionBoundary::Started,
                                GraphTransactionBoundary::AlreadyActive,
                                GraphTransactionBoundary::Unsupported};
    constexpr std::array<std::string_view, 3> names{"Started", "AlreadyActive", "Unsupported"};
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto copy = values[i];
        check(debug_string(r, copy) == names[i], "transaction boundary Debug");
        for (std::size_t j = 0; j < values.size(); ++j) {
            check((copy == values[j]) == (i == j), "transaction boundary equality");
        }
    }
    std::cout << "SERVICE_BOUNDARIES=Started,AlreadyActive,Unsupported\n";
}
