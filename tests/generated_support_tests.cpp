#include <teaql/runtime.hpp>
#include <teaql/sql.hpp>
#include <teaql/core.hpp>
#include "relation_hydration_fixture.hpp"
#include "parity/generated_support_fixture.hpp"
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Runtime final : TeaqlRuntime {
    using allocator_type = Allocator;
    const UserContext& context;
    mutable Vector<SelectQuery> queries;
    mutable Vector<TraceChain> traces;
    bool pending = false;
    unsigned* cancelled;

    Runtime(std::allocator_arg_t, Allocator a, const UserContext& context, unsigned& cancelled)
        : context(context), queries(a.resource()), traces(a.resource()), cancelled(&cancelled) {}

    const UserContext& user_context() const override {
        return context;
    }

    struct Work {
        Runtime const& runtime;
        std::string_view entity;
        const PurposedSelectQuery& query;
        std::span<const TraceNode> trace;
        std::size_t call;

        std::optional<Result<SmartList<CompactRow>, RuntimeError>> poll(WakeSignal&) {
            if (runtime.pending) {
                return {};
            }
            check(entity == query.as_query().entity && trace.back().entity_type == entity,
                  "entity, purposed query and trace borrows survive suspended facet execution");
            auto r = query.memory_resource();
            SmartList<CompactRow> rows(std::allocator_arg, Allocator(r));
            rows.total_count = call;
            return Result<SmartList<CompactRow>, RuntimeError>::ok(r, std::move(rows));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            ++*runtime.cancelled;
        }
    };

    AsyncOperation<Result<SmartList<CompactRow>, RuntimeError>>&
    fetch_facet_smart_list(std::string_view entity,
                           const PurposedSelectQuery& query,
                           std::span<const RelationAggregate>,
                           std::span<const TraceNode> trace) const override {
        queries.emplace_back(query.as_query());
        traces.emplace_back(trace.begin(), trace.end());
        return make_operation<Result<SmartList<CompactRow>, RuntimeError>, Work>(
            context.memory_resource(), Work{*this, entity, query, trace, queries.size()});
    }
};

static void ordered_facets_and_shutdown() {
    unsigned cancelled = 0;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& runtime = user->make_resource<Runtime>(*user, cancelled);
    {
        SelectQuery outer(std::allocator_arg, Allocator(r), "Parent");
        outer.trace_chain.emplace_back(TraceKind::Purpose, "Parent", "outer request");
        request::QueryOptions options(std::allocator_arg, Allocator(r));
        SelectQuery facet(std::allocator_arg, Allocator(r), String(256, 'c', r));
        options.facets.emplace_back("same", "ignored", facet, true);
        options.facets.emplace_back("same", "ignored", facet, true);
        auto& operation = execute_facets(runtime, outer, options);
        check(runtime.queries.empty(), "facet execution is lazy");
        runtime.pending = true;
        check(!operation.poll() && runtime.queries.size() == 1,
              "facets execute sequentially while the first child is pending");
        options.facets.clear();
        outer.trace_chain.clear();
        facet.entity.clear();
        runtime.pending = false;
        const auto& result = operation.wait();
        check(result && result.value().size() == 1 &&
                  result.value().begin()->second.total_count == 2,
              "duplicate names preserve source last-result-wins semantics");
        check(runtime.queries.size() == 2 && runtime.traces[0].size() == 2 &&
                  runtime.traces[0][0].comment == "outer request" &&
                  runtime.traces[0][1].kind == TraceKind::Relation &&
                  runtime.traces[0][1].comment == "same" &&
                  runtime.queries[0].trace_chain.back().kind == TraceKind::Purpose &&
                  runtime.queries[0].trace_chain.back().comment == "Calculate facet same",
              "outer provenance, relation trace and query purpose remain distinct");
        options.facets.emplace_back("pending", "ignored", runtime.queries[0], true);
        runtime.pending = true;
        auto& abandoned = execute_facets(runtime, outer, options);
        check(!abandoned.poll(), "shutdown fixture starts a pending child");
    }
    user.reset();
    check(cancelled == 1 && global.live_user_bytes() == 0,
          "Context close cancels the facet child before reclaiming its borrowed inputs");
}

static void restriction_errors_and_runtime_expiry() {
    unsigned cancelled = 0;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto runtime_ref = user->make_resource_ref<Runtime>(*user, cancelled);
    auto& runtime = user->get(runtime_ref);
    SelectQuery outer(std::allocator_arg, Allocator(r), "Parent");
    SelectQuery facet(std::allocator_arg, Allocator(r), "Child");
    auto missing = restrict_facet_to_outer_query(
        runtime, request::QuerySelection(std::allocator_arg, Allocator(r), facet), outer, "kids");
    check(!missing && missing.error().kind() == RuntimeError::Kind::Graph,
          "missing outer descriptor preserves the source Graph error variant");
    auto& metadata = user->make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
    parent.property("id", DataType::U64).id();
    parent.relation("kids", "Child").local("id").foreign("parent_id").to_many();
    metadata.register_entity(std::move(parent));
    user->set_metadata(*user, metadata);
    auto wrong = restrict_facet_to_outer_query(
        runtime, request::QuerySelection(std::allocator_arg, Allocator(r), facet), outer, "other");
    check(!wrong && wrong.error().kind() == RuntimeError::Kind::MissingRelation,
          "missing relation preserves its separate typed error");
    outer.projection.emplace_back("name");
    outer.order_by.emplace_back(OrderBy::desc(r, "name"));
    outer.slice = Slice{3, 2};
    outer.hard_limit(99);
    outer.comment_text = String("keep", r);
    auto restricted = restrict_facet_to_outer_query(
        runtime, request::QuerySelection(std::allocator_arg, Allocator(r), facet), outer, "kids");
    check(restricted && restricted.value().query.predicate &&
              restricted.value().query.predicate->kind == Expr::Kind::SubQuery,
          "facet restriction uses the original IN-subquery abstraction");
    const auto& subquery = restricted.value().query.predicate->subquery_data->query;
    check(subquery.projection.size() == 1 && subquery.projection[0] == "id" &&
              subquery.order_by.empty() && !subquery.slice && subquery.aggregates.empty() &&
              subquery.group_by_fields.empty() && subquery.relations.empty() &&
              subquery.hard_limit_value == 99 && subquery.comment_text == "keep" &&
              outer.projection.size() == 1 && outer.order_by.size() == 1 && outer.slice.has_value(),
          "restriction clears only the source fields and leaves the outer query unchanged");
    request::QueryOptions options(std::allocator_arg, Allocator(r));
    options.facets.emplace_back("expire", "ignored", facet, true);
    auto& queued = execute_facets(runtime, outer, options);
    user->destroy(runtime_ref);
    bool dead = false;
    try {
        queued.wait();
    } catch (const ReferenceError& error) {
        dead = error.kind() == ReferenceError::Kind::Dead;
    }
    check(dead && cancelled == 0, "expired runtime is rejected before virtual facet dispatch");
}

static void retained_facet_results() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    Ref<FacetResults> retained;
    {
        auto& runtime = user->make_resource<support_fixture::Runtime>(*user);
        SelectQuery outer(std::allocator_arg, Allocator(r), "Parent");
        SelectQuery query(std::allocator_arg, Allocator(r), String(256, 'c', r));
        request::QueryOptions options(std::allocator_arg, Allocator(r));
        options.facets.emplace_back("retained", "ignored", query, true);
        const auto& result = execute_facets(runtime, outer, options).wait();
        check(result.is_ok(), "retention fixture completes the public facet entry");
        retained = user->make_global<FacetResults>(result.value());
        const auto& copy = user->get(retained).begin()->second;
        check(copy.memory_resource() == global.memory_resource() &&
                  copy.data[0].get("payload")->as_list()->get_allocator().resource() ==
                      global.memory_resource() &&
                  copy.data[0].get("payload")->as_list()->front().memory_resource() ==
                      global.memory_resource() &&
                  copy.facets.begin()->second.memory_resource() == global.memory_resource(),
              "facet rows, nested values and nested facets copy into explicit Global ownership");
    }
    user.reset();
    check(global.live_user_bytes() == 0, "retained facet map owns no original User blocks");
    auto next = global.make_user_context();
    const auto& copy = next->get(next->rebind_global(retained)).begin()->second;
    check(copy.data[0].get("payload")->as_list()->front().try_text() == "retained" &&
              copy.summary.find("name")->second.try_text()->size() == 256 &&
              copy.aggregations.find("count")->second.try_u64() == 1 &&
              copy.facets.begin()->second.is_loaded,
          "complete facet contents survive actual destruction through explicit rebinding");
}

struct MoveOnly {
    using allocator_type = Allocator;
    Memory resource;
    int value;

    MoveOnly(std::allocator_arg_t, Allocator a, int value) : resource(a.resource()), value(value) {}

    MoveOnly(std::allocator_arg_t, Allocator a, MoveOnly&& other)
        : resource(a.resource()), value(std::exchange(other.value, 0)) {}

    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) noexcept = default;
};

static void generic_purpose_ownership() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto value = user->make_ref<PurposedQuery<MoveOnly>>(
        MoveOnly(std::allocator_arg, Allocator(user->memory_resource()), 42),
        String(256, 'p', user->memory_resource()));
    auto retained = user->make_global<PurposedQuery<MoveOnly>>(std::move(user->get(value)));
    check(user->get(retained).inner.resource == global.memory_resource() &&
              user->get(retained).memory_resource() == global.memory_resource(),
          "generic purpose construction consumes move-only allocator-aware inputs");
    user.reset();
    auto next = global.make_user_context();
    auto& copy = next->get(next->rebind_global(retained));
    check(copy.inner.value == 42 && copy.purpose.size() == 256,
          "allocator-aware generic move retains no shorter-domain purpose characters");
}

static void purposed_query_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& query = user->make<SelectQuery>(String(256, 'e', owner));
    query.projection.emplace_back(256, 'p');
    query.trace_chain.emplace_back(TraceKind::Purpose, query.entity, "existing purpose");
    auto& selected = user->make<PurposedSelectQuery>(query, String(512, 'r', owner));
    auto retained = user->make_global<PurposedSelectQuery>(selected);
    {
        const auto& copy = user->get(retained);
        auto original_debug = debug_string(owner, selected);
        auto retained_debug = debug_string(owner, copy);
        check(original_debug == retained_debug &&
                  original_debug.starts_with("PurposedSelectQuery { query: SelectQuery {"),
              "the purpose wrapper preserves its original structural Debug");
        check(copy.as_query().trace_chain.size() == 2,
              "copying the private query does not append another purpose trace");
    }
    user.reset();
    check(global.live_user_bytes() == 0, "the original purpose query User is actually deleted");
    auto next = global.make_user_context();
    auto rebound = next->rebind_global(retained);
    const auto& copy = next->get(rebound);
    auto destination = copy.memory_resource();
    check(destination != owner && copy.as_query().memory_resource() == destination &&
              copy.as_query().entity.get_allocator().resource() == destination &&
              copy.as_query().projection[0].get_allocator().resource() == destination &&
              copy.as_query().trace_chain.back().comment.get_allocator().resource() == destination,
          "the retained private query and nested strings all use the Global allocator");
    auto cloned = detail::clone_in(next->memory_resource(), copy);
    auto extracted = std::move(cloned).into_query();
    check(extracted.trace_chain.size() == 2 && extracted.trace_chain.back().comment.size() == 512 &&
              extracted.entity.size() == 256 && extracted.projection[0].size() == 256,
          "Clone and consuming extraction preserve the full query after source User deletion");
}

static void foreign_future_is_not_cancelled() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto peer = global.make_user_context();
    unsigned cancelled = 0;
    using Output = Result<SmartList<CompactRow>, RuntimeError>;

    struct Pending {
        unsigned& cancelled;

        std::optional<Output> poll(WakeSignal&) {
            return {};
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            ++cancelled;
        }
    };

    auto& foreign = make_operation<Output, Pending>(*peer, Pending{cancelled});

    struct ForeignRuntime final : TeaqlRuntime {
        UserContext& context;
        AsyncOperation<Output>& operation;

        ForeignRuntime(UserContext& context, AsyncOperation<Output>& operation)
            : context(context), operation(operation) {}

        const UserContext& user_context() const override {
            return context;
        }

        AsyncOperation<Output>& fetch_facet_smart_list(std::string_view,
                                                       const PurposedSelectQuery&,
                                                       std::span<const RelationAggregate>,
                                                       std::span<const TraceNode>) const override {
            return operation;
        }
    };

    auto& runtime = user->make_resource<ForeignRuntime>(*user, foreign);
    {
        auto r = user->memory_resource();
        SelectQuery outer(std::allocator_arg, Allocator(r), "Parent");
        SelectQuery child(std::allocator_arg, Allocator(r), "Child");
        request::QueryOptions options(std::allocator_arg, Allocator(r));
        options.facets.emplace_back("foreign", "ignored", child, true);
        bool rejected = false;
        try {
            execute_facets(runtime, outer, options).wait();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected && foreign.status() == AsyncStatus::Pending && cancelled == 0,
              "foreign facet future is rejected without taking its cancellation authority");
    }
    user.reset();
    check(foreign.status() == AsyncStatus::Pending && cancelled == 0,
          "closing the invoking User still leaves foreign work owned by its peer");
    peer.reset();
    check(cancelled == 1, "only the foreign future's owner cancels it on close");
}

struct Streaming : QueryExecutor<String>, MutationExecutor<String>, StreamQueryExecutor<String> {};

struct NonStreaming : QueryExecutor<String>, MutationExecutor<String> {};

static_assert(TeaqlQueryDataService<EntityDataService<Streaming>>);
static_assert(!TeaqlQueryDataService<EntityDataService<NonStreaming>>);
static_assert(TeaqlEntityDataService<EntityDataService<Streaming>, hydration_fixture::Model<0>>);
static_assert(
    std::same_as<TeaqlDataServiceError<EntityDataService<Streaming>>, DataServiceError<String>>);

struct HiddenSaveContract {
    using Error = RuntimeError;
    AsyncOperation<Result<GraphNode, RuntimeError>>& save(const TeaqlRuntime&) &&;
};

static_assert(AuditedSave<HiddenSaveContract, TeaqlRuntime>);
static_assert(!AuditedSave<MoveOnly, TeaqlRuntime>);

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        ordered_facets_and_shutdown();
        restriction_errors_and_runtime_expiry();
        retained_facet_results();
        generic_purpose_ownership();
        purposed_query_retention();
        foreign_future_is_not_cancelled();
        std::cout << "Generated-support runtime contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
