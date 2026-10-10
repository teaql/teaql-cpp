#include <teaql/runtime/data_service.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include "../../tests/support/relation_hydration_fixture.hpp"
#include <iostream>

using namespace teaql;
using Parent = hydration_fixture::Model<0>;
using Child = hydration_fixture::Model<1>;
using Leaf = hydration_fixture::Model<2>;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Loads : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    Memory resource;
    unsigned* calls;

    Loads(std::allocator_arg_t, Allocator a, unsigned& calls)
        : resource(a.resource()), calls(&calls) {}

    Memory memory_resource() const {
        return resource;
    }

    RuntimeHookResult before_select(Memory r, const UserContext&, SelectQuery&) const override {
        ++*calls;
        return RuntimeHookResult::ok(r, {});
    }

    Vector<String> relation_loads(Memory r, const UserContext&) const override {
        Vector<String> result(r);
        result.emplace_back("children");
        return result;
    }
};

using SqlE = SqlDataServiceExecutor<MutationExecutorError>;

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory resource = user->memory_resource();
    SqliteConnection& connection = user->make_resource<SqliteConnection>();
    SqliteMutationExecutor& transport = user->make<SqliteMutationExecutor>(connection);
    InMemoryMetadataStore& metadata = user->make<InMemoryMetadataStore>();
    InMemoryEntityGraphDecoderRegistry& decoders = user->make<InMemoryEntityGraphDecoderRegistry>();
    SqliteDialect& dialect = user->make_resource<SqliteDialect>();
    SqlExecutorState& state = user->make<SqlExecutorState>();
    SqlE& executor = user->make<SqlE>(dialect, transport, metadata, state);
    EntityDataService<SqlE>& repo =
        EntityDataService<SqlE>::for_executor(*user, "Parent", *user, executor);

    Fixture(bool child_decoder = true, bool leaf_decoder = true) {
        user->disable_sql_log();
        auto& sql = user->make<CompiledQuery>();
        for (
            auto text :
            {"CREATE TABLE parents(id INTEGER PRIMARY KEY,ref INTEGER,label TEXT)",
             "CREATE TABLE children(id INTEGER PRIMARY KEY,parent_id INTEGER,leaf_ref INTEGER,label TEXT)",
             "CREATE TABLE leaves(id INTEGER PRIMARY KEY,label TEXT)",
             "INSERT INTO parents VALUES(1,10,'one'),(2,20,'two'),(3,NULL,'three')",
             "INSERT INTO children VALUES(10,1,100,'first'),(11,1,100,'second'),(20,2,200,'third')",
             "INSERT INTO leaves VALUES(100,'shared'),(200,'other')"}) {
            sql.sql = text;
            check(bool(transport.execute(resource, sql)), "set up enhanced SQLite fixture");
        }
        EntityDescriptor parent(std::allocator_arg, Allocator(resource), "Parent");
        parent.table("parents");
        parent.property("id", DataType::U64).is_id = true;
        parent.property("ref", DataType::U64);
        parent.property("label", DataType::Text);
        auto& children = parent.relation("children", "Child");
        children.many = true;
        children.local_key = "id";
        children.foreign_key = "parent_id";
        auto& selected = parent.relation("selected", "Child");
        selected.local_key = "ref";
        selected.foreign_key = "id";
        EntityDescriptor child(std::allocator_arg, Allocator(resource), "Child");
        child.table("children");
        child.property("id", DataType::U64).is_id = true;
        child.property("parent_id", DataType::U64);
        child.property("leaf_ref", DataType::U64);
        child.property("label", DataType::Text);
        auto& inverse = child.relation("parent", "Parent");
        inverse.local_key = "parent_id";
        inverse.foreign_key = "id";
        auto& product = child.relation("product", "Leaf");
        product.local_key = "leaf_ref";
        product.foreign_key = "id";
        EntityDescriptor leaf(std::allocator_arg, Allocator(resource), "Leaf");
        leaf.table("leaves");
        leaf.property("id", DataType::U64).is_id = true;
        leaf.property("label", DataType::Text);
        metadata.register_entity(std::move(parent));
        metadata.register_entity(std::move(child));
        metadata.register_entity(std::move(leaf));
        user->set_metadata(*user, metadata);
        decoders.register_entity<Parent>();
        if (child_decoder) {
            decoders.register_entity<Child>();
        }
        if (leaf_decoder) {
            decoders.register_entity<Leaf>();
        }
        user->set_entity_graph_decoder_registry(decoders);
    }

    SelectQuery query() {
        auto result = repo.select();
        result.order_asc("id");
        return result;
    }
};

static void direct_and_id_set() {
    Fixture f;
    auto query = f.query();
    query.project("label").page(1, 1).optimize_pagination_with_id_set_config("pages", 600, 3);
    PurposedSelectQuery purpose(
        std::allocator_arg, Allocator(f.resource), query, "Inspect enhanced pages");
    auto& result = f.repo.fetch_enhanced_entities<Parent>(purpose).wait();
    check(
        result && result.value().is_loaded && result.value().total_count == 3 &&
            result.value().data.size() == 1 &&
            result.value().data[0].row.get("id")->try_u64() == 2 && result.value().data[0].state &&
            f.user->id_set_plan() == "ID_SET_BUILD",
        "public enhanced entry executes ID enumeration/page slicing and context-aware model decoding");
    auto& again = f.repo.fetch_enhanced_entities<Parent>(purpose).wait();
    check(again && f.user->id_set_plan() == "ID_SET_HIT" && again.value().total_count == 3,
          "the enhanced public caller reuses the retained total and IDs");
    EntityGraphBuilder empty(std::allocator_arg, Allocator(f.resource));
    check(bool(result.value().data[0].state->freeze_graph(std::move(empty))),
          "direct enhanced decoding does not freeze an empty identity graph");
}

static void flat_graphs() {
    for (bool ordinary : {false, true}) {
        Fixture f;
        auto query = f.query();
        SelectQuery children(std::allocator_arg, Allocator(f.resource), "Child");
        children.projects({"id", "parent_id", "leaf_ref", "label"});
        children.relation("product");
        query.relation_query("children", children).relation("selected");
        if (ordinary) {
            query.optimize_for_continuous_page_fetch().page(0, 3);
        }
        PurposedSelectQuery purpose(
            std::allocator_arg, Allocator(f.resource), query, "Inspect relation identity");
        auto& result = f.repo.fetch_enhanced_entities<Parent>(purpose).wait();
        check(result && result.value().data.size() == 3,
              "public flat hydration produces root entities");
        const auto& first = result.value().data[0];
        auto related = first.state->relation_list<Child>("Parent", 1, "children");
        auto missing = first.state->relation_list<Child>("Parent", 3, "children");
        check(related.is_loaded() && related.value()->data.size() == 2 && missing.is_empty(),
              "both compact and ordinary flat modes publish loaded and empty relation views");
        auto* leaf = first.state->resolve_entity<Leaf>(100);
        const auto& a = related.value()->data[0];
        const auto& b = related.value()->data[1];
        check(
            leaf && a.state->resolve_entity<Leaf>(100) == leaf &&
                b.state->resolve_entity<Leaf>(100) == leaf &&
                a.state->relation_option<Parent>("Child", 10, "parent").is_loaded() == ordinary,
            "recursive hydration preserves shared identity and source-specific inverse installation");
        check(first.state->resolve_entity<Child>(10) && !first.row.get("children"),
              "forward targets resolve through the frozen graph and embedded rows are removed");
        EntityGraphBuilder duplicate(std::allocator_arg, Allocator(f.resource));
        check(!first.state->freeze_graph(std::move(duplicate)),
              "complex flat paths freeze the graph once");
    }
}

static void embedded_fallback_and_aggregates() {
    Fixture f(true, false);
    unsigned hooks = 0;
    auto& behavior = f.user->make<Loads>(hooks);
    auto& behaviors = f.user->make<InMemoryEntityDataServiceBehaviorRegistry>();
    behaviors.register_behavior("Parent", *f.user, behavior);
    f.user->set_entity_data_service_behavior_registry(*f.user, behaviors);
    auto query = f.query();
    SelectQuery child(std::allocator_arg, Allocator(f.resource), "Child");
    child.relation("product");
    query.relation_query("children", child);
    Vector<RelationAggregate> aggregates(f.resource);
    SelectQuery count(std::allocator_arg, Allocator(f.resource), "Child");
    aggregates.emplace_back("children", "child_count", count, true);
    PurposedSelectQuery purpose(
        std::allocator_arg, Allocator(f.resource), query, "Inspect fallback summaries");
    auto& result =
        f.repo.fetch_enhanced_entities_with_relation_aggregates<Parent>(purpose, aggregates).wait();
    check(result && hooks == 1 && result.value().data[0].row.get("child_count")->try_u64() == 2,
          "outer aggregates remain separate and root preparation hooks execute once");
    const auto& root = result.value().data[0];
    auto related = root.state->relation_list<Child>("Parent", 1, "children");
    check(related.is_loaded() && related.value()->data.size() == 2 && !root.row.get("children"),
          "fallback relation expansion installs supported reverse collections");
    // Behavior loads replace the requested children, exactly as the source does.
    check(!related.value()->data[0].row.get("product"),
          "behavior expansion retains its own plan responsibility");
}

struct PendingExecutor : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    unsigned& stops;
    WakeSignal* child_wake = nullptr;

    PendingExecutor(std::allocator_arg_t, Allocator a, unsigned& stops)
        : resource(a.resource()), stops(stops) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Work {
        using allocator_type = Allocator;
        PendingExecutor& owner;

        Work(std::allocator_arg_t, Allocator, PendingExecutor& owner) : owner(owner) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal& wake) {
            owner.child_wake = &wake;
            return {};
        }

        void finish_and_wait() noexcept {
            owner.child_wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            ++owner.stops;
            owner.child_wake = nullptr;
        }
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory r, QueryRequest) override {
        return make_operation<Result<QueryResult, String>, Work>(r, *this);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

static void public_lifetimes() {
    {
        GlobalContext global;
        unsigned stops = 0;
        auto user = global.make_user_context();
        auto& executor = user->make<PendingExecutor>(stops);
        auto& repo =
            EntityDataService<PendingExecutor>::for_executor(*user, "Parent", *user, executor);
        auto& query = user->make<SelectQuery>("Parent");
        auto& operation = [&]() -> auto& {
            PurposedSelectQuery transient(std::allocator_arg,
                                          Allocator(user->memory_resource()),
                                          query,
                                          "Inspect pending enhanced work");
            return repo.fetch_enhanced_entities<Parent>(transient);
        }();
        check(!operation.poll(),
              "enhanced execution survives the temporary purpose wrapper and forwards Pending");
        auto ticket = operation.wake_signal().ticket();
        executor.child_wake->notify();
        check(operation.wake_signal().ticket() != ticket,
              "enhanced parent forwards the native wake signal");
        user->close();
        check(stops == 1, "User close cancels actual enhanced provider work exactly once");
    }
    Fixture f;
    SmartList<Parent>* saved = nullptr;
    {
        auto query = f.query();
        SelectQuery children(std::allocator_arg, Allocator(f.resource), "Child");
        children.projects({"id", "parent_id", "leaf_ref"}).relation("product");
        query.relation_query("children", children).relation("selected");
        PurposedSelectQuery purpose(
            std::allocator_arg, Allocator(f.resource), query, "Retain an enhanced graph");
        auto& result = f.repo.fetch_enhanced_entities<Parent>(purpose).wait();
        check(bool(result), "construct the public enhanced graph before explicit retention");
        saved = &f.global.make<SmartList<Parent>>(result.value());
    }
    f.user->close();
    f.user.reset();
    const auto& first = saved->data[0];
    auto related = first.state->relation_list<Child>("Parent", 1, "children");
    auto* leaf = first.state->resolve_entity<Leaf>(100);
    check(
        related.is_loaded() && leaf &&
            related.value()->data[0].state->resolve_entity<Leaf>(100) == leaf &&
            leaf->memory_resource() == f.global.memory_resource() &&
            first.row.memory_resource() == f.global.memory_resource(),
        "explicit Global copying preserves graph identity after the User and SQLite resources are destroyed");
}

int main() {
    try {
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        direct_and_id_set();
        flat_graphs();
        embedded_fallback_and_aggregates();
        public_lifetimes();
        std::pmr::set_default_resource(old);
        std::cout << "Public enhanced entity contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
