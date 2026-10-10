#include "../../tests/support/relation_hydration_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <atomic>
#include <iostream>
#include <semaphore>
#include <thread>
using namespace hydration_fixture;
using Access = detail::ContextEntityAccess;
using ServiceAccess = detail::EntityDataServiceAccess;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
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

static CompactRow row(Memory r,
                      std::initializer_list<std::pair<std::string_view, std::uint64_t>> fields) {
    Record values(r);
    for (auto [key, value] : fields) {
        values.emplace(String(key, r), Value(r, value));
    }
    return CompactRow(std::allocator_arg, Allocator(r), values);
}

static InMemoryMetadataStore& metadata(UserContext& c) {
    auto r = c.memory_resource();
    auto& store = c.make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
    parent.table("parents");
    parent.property("id", DataType::U64).is_id = true;
    auto& children = parent.relation("children", "Child");
    children.local_key = "id";
    children.foreign_key = "parent_id";
    children.many = true;
    EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
    child.table("children");
    child.property("id", DataType::U64).is_id = true;
    child.property("parent_id", DataType::U64);
    child.property("leaf_ref", DataType::U64);
    auto& back = child.relation("parent", "Parent");
    back.local_key = "parent_id";
    back.foreign_key = "id";
    auto& product = child.relation("product", "Leaf");
    product.local_key = "leaf_ref";
    product.foreign_key = "id";
    EntityDescriptor leaf(std::allocator_arg, Allocator(r), "Leaf");
    leaf.table("leaves");
    leaf.property("id", DataType::U64).is_id = true;
    store.register_entity(std::move(parent));
    store.register_entity(std::move(child));
    store.register_entity(std::move(leaf));
    c.set_metadata(c, store);
    auto& decoders = c.make<InMemoryEntityGraphDecoderRegistry>();
    decoders.register_entity<Model<0>>();
    decoders.register_entity<Model<1>>();
    decoders.register_entity<Model<2>>();
    c.set_entity_graph_decoder_registry(decoders);
    return store;
}

static Vector<RelationLoadPlan>& plans(UserContext& c) {
    auto r = c.memory_resource();
    auto& result = c.make<Vector<RelationLoadPlan>>();
    RelationDescriptor children(std::allocator_arg, Allocator(r), "children", "Child");
    children.local_key = "id";
    children.foreign_key = "parent_id";
    children.many = true;
    result.emplace_back("Parent", children);
    result.back().query.emplace(std::allocator_arg, Allocator(r), "Child");
    result.back().query->order_asc("id");
    RelationDescriptor product(std::allocator_arg, Allocator(r), "product", "Leaf");
    product.local_key = "leaf_ref";
    product.foreign_key = "id";
    result.back().children.emplace_back("Child", product);
    return result;
}

static void check_graph(const EntityRuntimeState& root, Memory r, bool compact) {
    auto children = root.relation_list<Model<1>>("Parent", 1, "children");
    auto empty = root.relation_list<Model<1>>("Parent", 2, "children");
    check(children.state() == LoadedRelation::Loaded && children.value()->data.size() == 3,
          "SQLite children installed through list decoder");
    check(empty.state() == LoadedRelation::Empty && empty.value()->is_loaded &&
              !root.has_relation_view("Parent", 3, "children"),
          "loaded empty and not loaded remain distinct");
    auto* leaf = root.resolve_entity<Model<2>>(20);
    check(leaf && leaf->memory_resource() == r, "forward target installed in its identity table");
    const auto& a = children.value()->data[0];
    const auto& b = children.value()->data[1];
    const auto& missing = children.value()->data[2];
    check(a.state && b.state && a.state->resolve_entity<Model<2>>(20) == leaf &&
              b.state->resolve_entity<Model<2>>(20) == leaf,
          "relation model states resolve one shared target identity");
    check(a.row.memory_resource() == r && a.row.get("leaf_ref")->memory_resource() == r,
          "nested model data uses owning arena");
    check(missing.row.contains_key("product") != compact,
          "only ordinary flat path installs missing forward scalar marker");
    auto inverse = root.relation_option<Model<0>>("Child", 10, "parent");
    check(inverse.state() == (compact ? LoadedRelation::NotLoaded : LoadedRelation::Loaded),
          "ordinary and compact inverse rules remain separate");
    if (!compact) {
        check(inverse.value()->state->relation_list<Model<1>>("Parent", 1, "children").value() ==
                  children.value(),
              "inverse model follows frozen graph back to same relation view");
    }
}

static void sqlite_hydration(detail::RelationHydrationMode mode) {
    GlobalContext g;
    EntityRuntimeState* retained_graph = nullptr;
    Vector<CompactRow>* retained_rows = nullptr;
    {
        auto c = g.make_user_context();
        auto r = c->memory_resource();
        c->disable_sql_log();
        auto& store = metadata(*c);
        auto& connection = c->make_resource<SqliteConnection>();
        auto& provider = c->make<SqliteMutationExecutor>(connection);
        auto& sql = c->make<CompiledQuery>();
        for (auto text :
             {"CREATE TABLE parents(id INTEGER PRIMARY KEY)",
              "CREATE TABLE children(id INTEGER PRIMARY KEY,parent_id INTEGER,leaf_ref INTEGER)",
              "CREATE TABLE leaves(id INTEGER PRIMARY KEY)",
              "INSERT INTO parents VALUES(1),(2)",
              "INSERT INTO children VALUES(10,1,20),(11,1,20),(12,1,99)",
              "INSERT INTO leaves VALUES(20)"}) {
            sql.sql = text;
            check(bool(provider.execute(r, sql)), "create SQLite hydration fixture");
        }
        auto& dialect = c->make_resource<SqliteDialect>();
        auto& state = c->make<SqlExecutorState>();
        auto& executor =
            c->make<SqlDataServiceExecutor<MutationExecutorError>>(dialect, provider, store, state);
        using E = std::remove_reference_t<decltype(executor)>;
        auto& repo = EntityDataService<E>::for_executor(*c, "Parent", *c, executor);
        auto& children = plans(*c);
        auto query = repo.select();
        query.order_asc("id");
        auto& fetched = ServiceAccess::context_service(repo).fetch_all(query);
        check(bool(fetched.wait()), "fetch parent rows from SQLite");
        auto& parents = c->make<Vector<CompactRow>>(fetched.wait().value());
        auto& traces = c->make<Vector<TraceChain>>();
        // This fixture supplies the entity-fetch boundary. Public EntityDataService
        // fetch orchestration, pagination and aggregates have separate contracts.
        detail::EntityRelationFetch<E> fetch(
            std::allocator_arg,
            Allocator(r),
            [&](const EntityDataService<E>& child, SelectQuery q) -> auto& {
                auto prepared = ServiceAccess::prepare_select(child, q);
                check(bool(prepared), "prepare relation query through entity hooks");
                traces.emplace_back(prepared.value().trace_chain);
                return ServiceAccess::context_service(child).fetch_all(prepared.value());
            });
        auto& root = c->make<EntityRuntimeState>();
        auto& graph = c->make<EntityGraphBuilder>();
        auto& work =
            mode == detail::RelationHydrationMode::Embedded
                ? detail::enhance_relation_plans(repo, parents, children, fetch)
            : mode == detail::RelationHydrationMode::Flat
                ? detail::hydrate_flat_plans(repo, parents, children, root, graph, fetch)
                : detail::hydrate_compact_flat_plans(repo, parents, children, root, graph, fetch);
        auto& result = work.wait();
        check(bool(result), "recursive hydration from actual SQLite rows");
        check(traces.size() == (mode == detail::RelationHydrationMode::Embedded ? 4 : 2) &&
                  traces.back().size() == 2 && traces.back()[0].comment == "Parent.children" &&
                  traces.back()[1].comment == "Child.product",
              "nested fetch carries relation lineage");
        if (mode == detail::RelationHydrationMode::Embedded) {
            auto* values = std::get_if<Value::List>(&parents[0].get("children")->data);
            check(values && values->size() == 3, "embedded children attached");
            auto& first = std::get<Value::Object>((*values)[0].data);
            auto& inverse = std::get<Value::Object>(first.find("parent")->second.data);
            auto& leaf = std::get<Value::Object>(first.find("product")->second.data);
            check(!inverse.contains("children") && inverse.at(String("id", r)).try_u64() == 1 &&
                      leaf.at(String("id", r)).try_u64() == 20,
                  "embedded inverse removes current relation before recursive enhancement");
            check(std::get<Value::List>(parents[1].get("children")->data).empty(),
                  "embedded empty relation retained");
            retained_rows = &g.make<Vector<CompactRow>>(parents);
        } else {
            check(bool(Access::decode_entity_graph_compact_batch(
                      *c, "Parent", detail::clone_in(r, parents), root, graph)),
                  "root model decode remains separate from hydration");
            check(bool(root.freeze_graph(std::move(graph))), "freeze hydrated graph");
            check_graph(root, r, mode == detail::RelationHydrationMode::CompactFlat);
            retained_graph = &g.make<EntityRuntimeState>(root);
        }
    }
    if (retained_graph) {
        check_graph(*retained_graph,
                    g.memory_resource(),
                    mode == detail::RelationHydrationMode::CompactFlat);
    }
    if (retained_rows) {
        auto& first = std::get<Value::List>(retained_rows->front().get("children")->data).front();
        check(first.memory_resource() == g.memory_resource() &&
                  std::get<Value::Object>(first.data).find("product")->second.memory_resource() ==
                      g.memory_resource(),
              "explicit embedded result copy outlives native database and User arena");
    }
}

using Rows = detail::RelationRowsResult<UnusedExecutor>;

struct ReadyRows {
    using allocator_type = Allocator;
    Memory r;
    Vector<CompactRow> rows;

    ReadyRows(std::allocator_arg_t, Allocator a, Vector<CompactRow> rows)
        : r(a.resource()), rows(std::move(rows), r) {}

    std::optional<Rows> poll(WakeSignal&) {
        return Rows::ok(r, std::move(rows));
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct NativeCounts {
    bool read = false;
    unsigned stopped = 0, scopes = 0, successes = 0, failures = 0, drops = 0;
    bool dropped_before_stop = false;
};

struct NativeScope : RuntimeTelemetryScope {
    NativeCounts& counts;

    explicit NativeScope(NativeCounts& counts) : counts(counts) {
        ++counts.scopes;
    }

    ~NativeScope() {
        ++counts.drops;
        if (!counts.stopped) {
            counts.dropped_before_stop = true;
        }
    }

    void with_context(FunctionRef<void()> cb) const override {
        cb();
    }

    void success(const RuntimeAttributes&) override {
        ++counts.successes;
    }

    void failure(std::string_view) override {
        ++counts.failures;
    }
};

struct NativeTelemetry : RuntimeTelemetry {
    using allocator_type = Allocator;
    Memory r;
    NativeCounts& counts;

    NativeTelemetry(std::allocator_arg_t, Allocator a, NativeCounts& counts)
        : r(a.resource()), counts(counts) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory r, const RuntimeOperation&) const override {
        return detail::resource_arena(r).make<ContextBox<RuntimeTelemetryScope>>(
            std::in_place_type<NativeScope>, counts);
    }
};

struct NativeWork {
    using allocator_type = Allocator;
    Memory r;
    const EntityDataService<UnusedExecutor>& service;
    SelectQuery query;
    NativeCounts& counts;
    String payload;
    std::thread worker;
    std::binary_semaphore go{0};
    std::atomic<bool> done = false;
    bool released = false;

    NativeWork(std::allocator_arg_t,
               Allocator a,
               const EntityDataService<UnusedExecutor>& service,
               SelectQuery q,
               NativeCounts& counts)
        : r(a.resource()), service(service), query(std::allocator_arg, a, std::move(q)),
          counts(counts), payload(32768, 'h', r) {}

    ~NativeWork() {
        if (worker.joinable()) {
            std::terminate();
        }
    }

    std::optional<Rows> poll(WakeSignal& wake) {
        if (!worker.joinable() && !done.load()) {
            worker = std::thread([this, &wake] {
                go.acquire();
                auto trace = ServiceAccess::trace(service);
                counts.read = service.entity() == "Leaf" && query.entity == "Leaf" &&
                              trace.size() == 2 && trace.back().comment == "Child.product" &&
                              payload.back() == 'h';
                done.store(true);
                wake.notify();
            });
            return {};
        }
        if (!done.load()) {
            return {};
        }
        Vector<CompactRow> rows(r);
        rows.emplace_back(row(r, {{"id", 20}}));
        return Rows::ok(r, std::move(rows));
    }

    void release() {
        if (!released) {
            released = true;
            go.release();
        }
    }

    void stop() noexcept {
        release();
        if (worker.joinable()) {
            worker.join();
        }
        ++counts.stopped;
    }

    void finish_and_wait() noexcept {
        stop();
    }

    void cancel_and_wait() noexcept {
        stop();
    }
};

static void native_lifetime(detail::RelationHydrationMode mode, unsigned action) {
    NativeCounts counts;
    GlobalContext g;
    auto& telemetry = g.make_resource<NativeTelemetry>(counts);
    auto c = g.make_user_context();
    auto r = c->memory_resource();
    metadata(*c);
    c->set_runtime_telemetry(g, telemetry);
    auto& executor = c->make<UnusedExecutor>();
    auto& repo = EntityDataService<UnusedExecutor>::for_executor(*c, "Parent", *c, executor);
    auto& plan = plans(*c);
    auto& parents = c->make<Vector<CompactRow>>();
    parents.emplace_back(row(r, {{"id", 1}}));
    auto& root = c->make<EntityRuntimeState>();
    auto& graph = c->make<EntityGraphBuilder>();
    NativeWork* native = nullptr;
    detail::EntityRelationFetch<UnusedExecutor> fetch(
        std::allocator_arg,
        Allocator(r),
        [&](const auto& child, SelectQuery q) -> AsyncOperation<Rows>& {
            if (q.entity == "Child") {
                Vector<CompactRow> rows(r);
                rows.emplace_back(row(r, {{"id", 10}, {"parent_id", 1}, {"leaf_ref", 20}}));
                return make_operation<Rows, ReadyRows>(r, std::move(rows));
            }
            native = &c->make_resource<NativeWork>(child, std::move(q), counts);
            return c->make<AsyncOperation<Rows>>(*native);
        });
    auto& work = mode == detail::RelationHydrationMode::Embedded
                     ? detail::enhance_relation_plans(repo, parents, plan, fetch)
                 : mode == detail::RelationHydrationMode::Flat
                     ? detail::hydrate_flat_plans(repo, parents, plan, root, graph, fetch)
                     : detail::hydrate_compact_flat_plans(repo, parents, plan, root, graph, fetch);
    plan.front().children.clear();
    plan.front().relation_name = "changed";
    check(!work.poll() && native, "hydration owns original plan tree and suspends in a descendant");
    if (action == 0) {
        native->release();
        check(bool(work.wait()), "native completion wakes recursive operation tree");
    } else if (action == 1) {
        work.cancel();
        if (mode == detail::RelationHydrationMode::Embedded) {
            auto& child = std::get<Value::List>(parents.front().get("children")->data).front();
            check(std::get<Value::Object>(child.data).empty(),
                  "cancel preserves source mem::take partial-state rule");
        }
    }
    c->close();
    check(counts.read && counts.stopped == 1 && c->allocation_stats().live_blocks == 0,
          "all descendant host accesses join before arena reclamation");
    check(!counts.dropped_before_stop && counts.drops == counts.scopes && counts.failures == 0,
          "native child stops before relation telemetry destruction");
    check(counts.scopes == (mode == detail::RelationHydrationMode::Embedded ? 2 : 0) &&
              counts.successes ==
                  (mode == detail::RelationHydrationMode::Embedded && action == 0 ? 2 : 0),
          "only completed embedded relation scopes report success");
}

static void domain_contracts() {
    GlobalContext g;
    auto c = g.make_user_context();
    auto sibling = g.make_user_context();
    auto r = c->memory_resource();
    metadata(*c);
    auto& executor = c->make<UnusedExecutor>();
    auto& repo = EntityDataService<UnusedExecutor>::for_executor(*c, "Parent", *c, executor);
    auto& plan = plans(*c);
    auto& rows = c->make<Vector<CompactRow>>();
    rows.emplace_back(row(r, {{"id", 1}}));
    auto& foreign = sibling->make<Vector<CompactRow>>();
    foreign.emplace_back(row(sibling->memory_resource(), {{"id", 1}}));
    auto& root = c->make<EntityRuntimeState>();
    auto& graph = c->make<EntityGraphBuilder>();
    auto& foreign_root = sibling->make<EntityRuntimeState>();
    auto& foreign_graph = sibling->make<EntityGraphBuilder>();
    unsigned calls = 0;
    detail::EntityRelationFetch<UnusedExecutor> fetch(
        std::allocator_arg, Allocator(r), [&](const auto&, SelectQuery) -> AsyncOperation<Rows>& {
            ++calls;
            return make_operation<Rows, ReadyRows>(r, Vector<CompactRow>(r));
        });
    rejected(
        [&] {
            detail::enhance_relation_plans(repo, foreign, plan, fetch);
        },
        "embedded parent rows require explicit copy into query domain");
    rejected(
        [&] {
            detail::hydrate_flat_plans(repo, rows, plan, foreign_root, graph, fetch);
        },
        "flat graph refuses foreign root");
    rejected(
        [&] {
            detail::hydrate_compact_flat_plans(repo, rows, plan, root, foreign_graph, fetch);
        },
        "compact graph refuses foreign builder");
    rejected(
        [&] {
            detail::hydrate_compact_flat_plans(repo, foreign, plan, root, graph, fetch);
        },
        "readonly compact parent borrow cannot hide foreign lifetime");
    check(calls == 0 && graph.entity_count() == 0,
          "domain validation precedes all fetches and graph mutation");
    auto& unrelated =
        make_operation<Rows, ReadyRows>(*sibling, Vector<CompactRow>(sibling->memory_resource()));
    detail::EntityRelationFetch<UnusedExecutor> bad(
        std::allocator_arg, Allocator(r), [&](const auto&, SelectQuery) -> auto& {
            return unrelated;
        });
    auto& invalid = detail::hydrate_flat_plans(repo, rows, plan, root, graph, bad);
    rejected(
        [&] {
            invalid.poll();
        },
        "recursive fetch refuses a child operation from another Context");
    check(unrelated.status() == AsyncStatus::Cancelled,
          "recursive fetch cancels its rejected callback result");
    unrelated.cancel();
}

int main() {
    try {
        setenv("TEAQL_LOG_ENDPOINT", "off", 1);
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        for (auto mode : {detail::RelationHydrationMode::Embedded,
                          detail::RelationHydrationMode::Flat,
                          detail::RelationHydrationMode::CompactFlat}) {
            sqlite_hydration(mode);
            for (unsigned action = 0; action < 3; ++action) {
                native_lifetime(mode, action);
            }
        }
        domain_contracts();
        std::pmr::set_default_resource(old);
        std::cout << "Recursive relation, SQLite and lifetime contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
