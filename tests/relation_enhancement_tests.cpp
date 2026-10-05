#include <teaql/runtime/detail/relation_enhancement.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include "relation_hydration_fixture.hpp"
#include <atomic>
#include <iostream>
#include <semaphore>
#include <thread>
using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejected(F&& fn, const char* message) {
    bool refused = false;
    try {
        fn();
    } catch (const std::logic_error&) {
        refused = true;
    }
    check(refused, message);
}

static CompactRow row(Memory r,
                      std::initializer_list<std::pair<std::string_view, std::uint64_t>> fields) {
    Record values(r);
    for (auto [name, value] : fields) {
        values.emplace(String(name, r), Value(r, value));
    }
    return CompactRow(std::allocator_arg, Allocator(r), values);
}

static InMemoryMetadataStore& metadata(UserContext& c) {
    auto r = c.memory_resource();
    auto& store = c.make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
    parent.table("parents");
    parent.property("id", DataType::U64).is_id = true;
    parent.property("ref", DataType::U64);
    parent.property("label", DataType::Text);
    auto& relation = parent.relation("children", "Child");
    relation.local_key = "id";
    relation.foreign_key = "parent_id";
    relation.many = true;
    EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
    child.table("children");
    child.property("id", DataType::U64).is_id = true;
    child.property("parent_id", DataType::U64).column("parent_col");
    child.property("price", DataType::I64);
    child.property("label", DataType::Text);
    store.register_entity(std::move(parent));
    store.register_entity(std::move(child));
    c.set_metadata(c, store);
    return store;
}

static void sqlite_enhancements() {
    GlobalContext g;
    Vector<CompactRow>* retained = nullptr;
    {
        auto c = g.make_user_context();
        auto r = c->memory_resource();
        c->disable_sql_log();
        auto& store = metadata(*c);
        auto& connection = c->make_resource<SqliteConnection>();
        auto& provider = c->make<SqliteMutationExecutor>(connection);
        auto& sql = c->make<CompiledQuery>();
        for (
            auto text :
            {"CREATE TABLE parents(id INTEGER PRIMARY KEY,ref INTEGER,label TEXT)",
             "CREATE TABLE children(id INTEGER PRIMARY KEY,parent_col INTEGER,price INTEGER,label TEXT)",
             "INSERT INTO parents VALUES(1,10,'p1'),(2,99,'p2')",
             "INSERT INTO children VALUES(10,1,7,'first child'),(11,1,8,'second child')"}) {
            sql.sql = text;
            check(bool(provider.execute(r, sql)), "create enhancement SQLite fixture");
        }
        auto& dialect = c->make_resource<SqliteDialect>();
        auto& state = c->make<SqlExecutorState>();
        auto& executor =
            c->make<SqlDataServiceExecutor<MutationExecutorError>>(dialect, provider, store, state);
        using E = std::remove_reference_t<decltype(executor)>;
        auto& repo = EntityDataService<E>::for_executor(*c, "Parent", *c, executor);
        auto q = repo.select();
        q.order_asc("id");
        auto& initial = Access::context_service(repo).fetch_all(q);
        check(bool(initial.wait()), "SQLite parents query");
        auto& parents = c->make<Vector<CompactRow>>(initial.wait().value());
        auto& seen = c->make<Vector<SelectQuery>>();
        detail::EntityRelationFetch<E> fetch(
            std::allocator_arg,
            Allocator(r),
            [&](const EntityDataService<E>& child, SelectQuery query) -> auto& {
                auto prepared = Access::prepare_select(child, query);
                check(bool(prepared), "entity query hooks prepare enhancement");
                seen.emplace_back(prepared.value());
                return Access::context_service(child).fetch_all(prepared.value());
            });
        TraceChain trace(r);
        trace.emplace_back(TraceKind::Request, "Parent", "summary request");
        Vector<RelationAggregate> aggregates(r);
        SelectQuery count(std::allocator_arg, Allocator(r), "Ignored");
        count.project("label").order_desc("price").limit(1);
        aggregates.emplace_back("children", "count", count, true);
        SelectQuery sum(std::allocator_arg, Allocator(r), "Child");
        sum.aggregates.emplace_back(Aggregate::sum(r, "price", "sum"));
        aggregates.emplace_back("children", "prices", sum, false);
        auto& aggregate = detail::enhance_relation_aggregates(
            repo,
            parents,
            aggregates,
            AggregationCacheOptions{
                .enabled = false, .propagate = true, .propagate_cache_expired_millis = 321},
            trace,
            fetch);
        check(bool(aggregate.wait()), "relation aggregates query actual SQLite");
        check(parents[0].get("count")->try_u64() == 2 && parents[1].get("count")->try_u64() == 0,
              "count ignores relation slice and defaults empty parent to zero");
        const auto& values = std::get<Value::List>(parents[0].get("prices")->data);
        check(values.size() == 1 &&
                  std::get<Value::Object>(values[0].data).at(String("sum", r)).try_i64() == 15 &&
                  std::get<Value::List>(parents[1].get("prices")->data).empty(),
              "group rows become aggregate object list and remove matching key");
        check(seen[0].entity == "Child" && seen[0].projection.empty() && !seen[0].slice &&
                  seen[0].aggregation_cache->enabled &&
                  seen[0].aggregation_cache->cache_expired_millis == 321,
              "aggregation planning preserves source cache propagation even from disabled parent");
        check(seen[0].trace_chain.size() == 2 &&
                  seen[0].trace_chain.back().entity_type == "Child" &&
                  seen[0].trace_chain.back().comment == "count",
              "aggregate lineage records actual target and result alias");
        Vector<ObjectGroupBy> groups(r);
        SelectQuery target(std::allocator_arg, Allocator(r), "Child");
        target.project("label");
        groups.emplace_back("object", "ref", target);
        check(bool(detail::enhance_object_group_bys(repo, parents, groups, trace, fetch).wait()),
              "object-group SQLite lookup");
        auto& object = std::get<Value::Object>(parents[0].get("object")->data);
        check(object.at(String("id", r)).try_u64() == 10 &&
                  object.at(String("label", r)).try_text() == "first child" &&
                  std::holds_alternative<std::monostate>(parents[1].get("object")->data),
              "object group adds identity projection and missing target Null");
        parents[0].insert("label", Value(r, "before enhancement"));
        Vector<SelectQuery> children(r);
        children.emplace_back("Parent");
        children.back().project("label");
        check(bool(detail::enhance_child_queries(repo, parents, children, trace, fetch).wait()),
              "child query merges through prepared entity fetch");
        check(parents[0].get("label")->try_text() == "p1" &&
                  parents[0].get("count")->try_u64() == 2,
              "child merge replaces returned fields and retains prior aggregates");
        retained = &g.make<Vector<CompactRow>>(parents);
    }
    auto r = g.memory_resource();
    check(retained && retained->front().get("object")->memory_resource() == r &&
              std::get<Value::Object>(retained->front().get("object")->data)
                      .at(String("label", r))
                      .try_text() == "first child",
          "nested enhanced rows explicitly retained after database/User destruction");
}

using E = hydration_fixture::UnusedExecutor;
using Rows = detail::RelationRowsResult<E>;

struct Counts {
    bool read = false;
    unsigned stops = 0;
};

struct NativeWork {
    using allocator_type = Allocator;
    Memory r;
    const EntityDataService<E>& repo;
    SelectQuery query;
    Counts& counts;
    std::thread worker;
    std::binary_semaphore go{0};
    std::atomic<bool> done = false;
    bool released = false;

    NativeWork(std::allocator_arg_t,
               Allocator a,
               const EntityDataService<E>& repo,
               SelectQuery q,
               Counts& counts)
        : r(a.resource()), repo(repo), query(std::allocator_arg, a, std::move(q)), counts(counts) {}

    ~NativeWork() {
        if (worker.joinable()) {
            std::terminate();
        }
    }

    std::optional<Rows> poll(WakeSignal& wake) {
        if (!worker.joinable() && !done.load()) {
            worker = std::thread([this, &wake] {
                go.acquire();
                auto trace = Access::trace(repo);
                counts.read = repo.entity() == "Child" && query.entity == "Child" &&
                              !trace.empty() && trace.front().comment == "original trace";
                done.store(true);
                wake.notify();
            });
            return {};
        }
        if (!done.load()) {
            return {};
        }
        Vector<CompactRow> rows(r);
        rows.emplace_back(row(r, {{"id", 1}, {"parent_id", 1}, {"count", 5}}));
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
        ++counts.stops;
    }

    void finish_and_wait() noexcept {
        stop();
    }

    void cancel_and_wait() noexcept {
        stop();
    }
};

static void native_lifetime(unsigned mode, unsigned action) {
    Counts counts;
    GlobalContext g;
    auto c = g.make_user_context();
    auto r = c->memory_resource();
    metadata(*c);
    auto& executor = c->make<E>();
    auto& repo = EntityDataService<E>::for_executor(*c, "Parent", *c, executor);
    auto& parents = c->make<Vector<CompactRow>>();
    parents.emplace_back(row(r, {{"id", 1}, {"ref", 1}}));
    auto& trace = c->make<TraceChain>();
    trace.emplace_back(TraceKind::Request, "Parent", "original trace");
    auto& aggregates = c->make<Vector<RelationAggregate>>();
    auto& groups = c->make<Vector<ObjectGroupBy>>();
    auto& queries = c->make<Vector<SelectQuery>>();
    SelectQuery q(std::allocator_arg, Allocator(r), "Child");
    aggregates.emplace_back("children", "stats", q, true);
    groups.emplace_back("object", "ref", q);
    queries.emplace_back(q);
    NativeWork* native = nullptr;
    detail::EntityRelationFetch<E> fetch(
        std::allocator_arg, Allocator(r), [&](const auto& child, SelectQuery query) -> auto& {
            native = &c->make_resource<NativeWork>(child, std::move(query), counts);
            return c->make<AsyncOperation<Rows>>(*native);
        });
    auto& work =
        mode == 0 ? detail::enhance_relation_aggregates(repo, parents, aggregates, {}, trace, fetch)
        : mode == 1 ? detail::enhance_object_group_bys(repo, parents, groups, trace, fetch)
                    : detail::enhance_child_queries(repo, parents, queries, trace, fetch);
    trace.clear();
    aggregates.clear();
    groups.clear();
    queries.clear();
    check(!work.poll() && native, "input snapshots survive mutation of caller containers");
    check(!parents.front().contains_key("stats") && !parents.front().contains_key("object") &&
              !parents.front().contains_key("count"),
          "pending fetch has not partially attached a result");
    if (action == 0) {
        native->release();
        check(bool(work.wait()), "native wake reaches enhancement operation");
    } else if (action == 1) {
        work.cancel();
        check(parents.front().len() == 2, "cancel leaves current enhancement untouched");
    }
    c->close();
    check(counts.read && counts.stops == 1 && c->allocation_stats().live_blocks == 0,
          "Context closes after all borrowed repository/query accesses stop");
}

struct Ready {
    using allocator_type = Allocator;
    Memory r;

    Ready(std::allocator_arg_t, Allocator a) : r(a.resource()) {}

    std::optional<Rows> poll(WakeSignal&) {
        return Rows::ok(r, Vector<CompactRow>(r));
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

static void domains_and_aliases() {
    GlobalContext g;
    auto c = g.make_user_context();
    auto sibling = g.make_user_context();
    auto r = c->memory_resource();
    metadata(*c);
    auto& executor = c->make<E>();
    auto& repo = EntityDataService<E>::for_executor(*c, "Parent", *c, executor);
    auto& rows = c->make<Vector<CompactRow>>();
    rows.emplace_back(row(r, {{"id", 1}, {"ref", 1}}));
    rows.emplace_back(row(r, {{"id", 2}, {"ref", 1}}));
    SelectQuery q(std::allocator_arg, Allocator(r), "Child");
    ObjectGroupBy group(std::allocator_arg, Allocator(r), "object", "ref", q);
    detail::attach_object_group_rows(rows, group, rows);
    check(!std::get<Value::Object>(rows.back().get("object")->data).contains("object"),
          "returned-row index snapshots isolate overlapping source and destination rows");
    auto& foreign = sibling->make<Vector<CompactRow>>();
    foreign.emplace_back(row(sibling->memory_resource(), {{"id", 1}}));
    auto& unrelated = make_operation<Rows, Ready>(*sibling);
    unsigned calls = 0;
    detail::EntityRelationFetch<E> fetch(
        std::allocator_arg, Allocator(r), [&](const auto&, SelectQuery) -> auto& {
            ++calls;
            return unrelated;
        });
    Vector<RelationAggregate> aggregates(r);
    aggregates.emplace_back("children", "stats", q, true);
    Vector<ObjectGroupBy> groups(r);
    groups.emplace_back(group);
    Vector<SelectQuery> queries(r);
    queries.emplace_back(q);
    rejected(
        [&] {
            detail::enhance_relation_aggregates(repo, foreign, aggregates, {}, {}, fetch);
        },
        "aggregate refuses cross-domain parent borrow");
    rejected(
        [&] {
            detail::enhance_object_group_bys(repo, foreign, groups, {}, fetch);
        },
        "object group refuses cross-domain parent borrow");
    rejected(
        [&] {
            detail::enhance_child_queries(repo, foreign, queries, {}, fetch);
        },
        "child enhancement refuses cross-domain parent borrow");
    check(calls == 0, "domain rejection precedes I/O");
    for (unsigned mode = 0; mode < 3; ++mode) {
        auto& work =
            mode == 0   ? detail::enhance_relation_aggregates(repo, rows, aggregates, {}, {}, fetch)
            : mode == 1 ? detail::enhance_object_group_bys(repo, rows, groups, {}, fetch)
                        : detail::enhance_child_queries(repo, rows, queries, {}, fetch);
        rejected(
            [&] {
                work.poll();
            },
            "all enhancement paths reject foreign child operations");
        check(unrelated.status() == AsyncStatus::Pending,
              "domain rejection preserves foreign owner's operation");
    }
    unrelated.cancel();
}

int main() {
    try {
        setenv("TEAQL_LOG_ENDPOINT", "off", 1);
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        sqlite_enhancements();
        for (unsigned mode = 0; mode < 3; ++mode) {
            for (unsigned action = 0; action < 3; ++action) {
                native_lifetime(mode, action);
            }
        }
        domains_and_aliases();
        std::pmr::set_default_resource(old);
        std::cout << "Relation enhancement, SQLite and lifetime contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
