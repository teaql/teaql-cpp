#include <teaql/runtime/data_service.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "../support/relation_hydration_fixture.hpp"
#include <iostream>
using namespace teaql;
using Parent = hydration_fixture::Model<0>;
using Child = hydration_fixture::Model<1>;
using Leaf = hydration_fixture::Model<2>;

static const Json& field(const Json& input, std::string_view name) {
    return std::get<Json::Object>(input.data).find(name)->second;
}

static bool flag(const Json& input, std::string_view name) {
    return std::get<bool>(field(input, name).data);
}

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T value;

    Ready(std::allocator_arg_t, Allocator a, T value)
        : value(detail::move_in(a.resource(), std::move(value))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(value);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Behavior : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    Memory resource;
    bool loads;
    mutable unsigned calls = 0;

    Behavior(std::allocator_arg_t, Allocator a, bool loads)
        : resource(a.resource()), loads(loads) {}

    Memory memory_resource() const {
        return resource;
    }

    RuntimeHookResult before_select(Memory r, const UserContext&, SelectQuery&) const override {
        ++calls;
        return RuntimeHookResult::ok(r, {});
    }

    Vector<String> relation_loads(Memory r, const UserContext&) const override {
        Vector<String> out(r);
        if (loads) {
            out.emplace_back("children");
        }
        return out;
    }
};

struct Mock : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    Vector<String> requests;
    String failure, fail_model;

    Mock(std::allocator_arg_t, Allocator a, std::string_view failure, std::string_view fail_model)
        : resource(a.resource()), requests(resource), failure(failure, resource),
          fail_model(fail_model, resource) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        requests.emplace_back(detail::debug_query(resource, request.query));
        using R = Result<QueryResult, String>;
        auto name = std::string_view(request.query.entity);
        if (name == failure) {
            return make_operation<R, Ready<R>>(r, R::err(r, String("provider failed", r)));
        }
        QueryResult out(
            std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, 0));
        auto add = [&](std::initializer_list<std::pair<std::string_view, std::uint64_t>> fields) {
            out.rows.emplace_back();
            for (auto [key, value] : fields) {
                out.rows.back().insert(key, Value(r, value));
            }
        };
        if (name == "Parent") {
            add({{"id", 1}, {"ref", 10}});
            add({{"id", 2}});
            out.rows.back().insert("ref", Value(r));
        } else if (name == "Child") {
            add({{"id", 10}, {"parent_id", 1}, {"leaf_ref", 100}});
            add({{"id", 11}, {"parent_id", 1}, {"leaf_ref", 100}});
        } else {
            add({{"id", 100}});
        }
        if (name == fail_model) {
            out.rows[0].insert("fail", Value(r, true));
        }
        out.metadata.result_count = out.rows.size();
        return make_operation<R, Ready<R>>(r, R::ok(r, std::move(out)));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

static InMemoryMetadataStore& metadata(UserContext& c, const Json& input) {
    auto r = c.memory_resource();
    auto& store = c.make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
    parent.property("id", DataType::U64).is_id = true;
    parent.property("ref", DataType::U64);
    auto& children = parent.relation("children", "Child");
    children.many = true;
    children.local_key = "id";
    children.foreign_key = "parent_id";
    auto& selected = parent.relation("selected", "Child");
    selected.local_key = "ref";
    selected.foreign_key = "id";
    EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
    child.property("id", DataType::U64).is_id = true;
    child.property("parent_id", DataType::U64);
    child.property("leaf_ref", DataType::U64);
    auto& inverse = child.relation("parent", "Parent");
    inverse.local_key = "parent_id";
    inverse.foreign_key = "id";
    auto& product = child.relation("product", "Leaf");
    product.local_key = "leaf_ref";
    product.foreign_key = "id";
    EntityDescriptor leaf(std::allocator_arg, Allocator(r), "Leaf");
    leaf.property("id", DataType::U64).is_id = true;
    store.register_entity(std::move(parent));
    store.register_entity(std::move(child));
    store.register_entity(std::move(leaf));
    c.set_metadata(c, store);
    auto& decoders = c.make<InMemoryEntityGraphDecoderRegistry>();
    decoders.register_entity<Parent>();
    if (flag(input, "child_decoder")) {
        decoders.register_entity<Child>();
    }
    if (flag(input, "leaf_decoder")) {
        decoders.register_entity<Leaf>();
    }
    c.set_entity_graph_decoder_registry(decoders);
    return store;
}

static Json snapshot(Memory r, const Parent& value) {
    auto out = detail::model_json(r);
    const auto& state = *value.state;
    auto id = *value.id_value().try_u64();
    auto children = state.relation_list<Child>("Parent", id, "children");
    auto row = detail::model_json(r);
    for (const auto& key : value.row.keys()) {
        detail::model_put(row, key, detail::debug_value(r, *value.row.get(key)));
    }
    detail::model_put(out, "row", row);
    detail::model_put(out,
                      "children_state",
                      children.state() == LoadedRelation::NotLoaded ? "NotLoaded"
                      : children.state() == LoadedRelation::Empty   ? "Empty"
                                                                    : "Loaded");
    Json related(r);
    if (children.value()) {
        Json::Array items(r);
        for (const auto& child : children.value()->data) {
            auto item = detail::model_json(r);
            detail::model_put(item, "id", *child.id_value().try_u64());
            detail::model_put(item, "product_embedded", child.row.get("product") != nullptr);
            detail::model_put(item, "leaf", child.state->resolve_entity<Leaf>(100) != nullptr);
            detail::model_put(
                item,
                "parent_view",
                child.state->has_relation_view("Child", *child.id_value().try_u64(), "parent"));
            items.emplace_back(std::move(item));
        }
        related = Json(r, items);
    }
    detail::model_put(out, "children", related);
    detail::model_put(out, "selected", state.resolve_entity<Child>(10) != nullptr);
    detail::model_put(out, "has_original", bool(state.original_snapshot()));
    return out;
}

template <class E>
static Json run(UserContext&, const Json&, const EntityDataService<E>&, const Behavior&);

static Json evaluate(UserContext& c, const Json& input) {
    auto r = c.memory_resource();
    c.set_user_identifier("fixture");
    if (!flag(input, "capture")) {
        c.disable_sql_log();
    }
    auto& store = metadata(c, input);
    auto& behavior = c.make<Behavior>(flag(input, "behavior"));
    auto& registry = c.make<InMemoryEntityDataServiceBehaviorRegistry>();
    registry.register_behavior("Parent", c, behavior);
    c.set_entity_data_service_behavior_registry(c, registry);
    auto& cache = c.make<InMemoryAggregationCache>("enhanced");
    c.insert_resource(c, cache);
    const auto& fields = std::get<Json::Object>(input.data);
    if (auto sqlite = fields.find("sqlite");
        sqlite != fields.end() && std::get<bool>(sqlite->second.data)) {
        auto& connection = c.make_resource<SqliteConnection>();
        auto& transport = c.make<SqliteMutationExecutor>(connection);
        auto& sql = c.make<CompiledQuery>();
        for (auto text :
             {"CREATE TABLE parent_data(id INTEGER PRIMARY KEY,ref INTEGER)",
              "CREATE TABLE child_data(id INTEGER PRIMARY KEY,parent_id INTEGER,leaf_ref INTEGER)",
              "CREATE TABLE leaf_data(id INTEGER PRIMARY KEY)",
              "INSERT INTO parent_data VALUES(1,10),(2,NULL)",
              "INSERT INTO child_data VALUES(10,1,100),(11,1,100)",
              "INSERT INTO leaf_data VALUES(100)"}) {
            sql.sql = text;
            if (!transport.execute(r, sql)) {
                throw std::runtime_error("SQLite fixture setup failed");
            }
        }
        auto& dialect = c.make_resource<SqliteDialect>();
        auto& state = c.make<SqlExecutorState>();
        using E = SqlDataServiceExecutor<MutationExecutorError>;
        auto& executor = c.make<E>(dialect, transport, store, state);
        auto& repo = EntityDataService<E>::for_executor(c, "Parent", c, executor);
        return run(c, input, repo, behavior);
    }
    auto& executor = c.make<Mock>(std::get<String>(field(input, "failure").data),
                                  std::get<String>(field(input, "fail_model").data));
    auto& repo = EntityDataService<Mock>::for_executor(c, "Parent", c, executor);
    auto output = run(c, input, repo, behavior);
    Json::Array requests(r);
    for (const auto& q : executor.requests) {
        requests.emplace_back(q);
    }
    detail::model_put(output, "requests", requests);
    return output;
}

template <class E>
static Json
run(UserContext& c, const Json& input, const EntityDataService<E>& repo, const Behavior& behavior) {
    auto r = c.memory_resource();
    auto query = repo.select();
    query.order_asc("id");
    if (flag(input, "cache")) {
        query.enable_aggregation_cache();
    }
    if (flag(input, "idset")) {
        query.page(0, 1).optimize_pagination_with_id_set_config("fixture", 600, 2);
    }
    if (flag(input, "continuous")) {
        query.page(0, 1).optimize_for_continuous_page_fetch();
    }
    if (flag(input, "relations")) {
        SelectQuery child(std::allocator_arg, Allocator(r), "Child");
        const auto& fields = std::get<Json::Object>(input.data);
        if (auto explicit_projection = fields.find("explicit_child_projection");
            explicit_projection != fields.end() &&
            std::get<bool>(explicit_projection->second.data)) {
            child.projects({"id", "parent_id", "leaf_ref"});
        }
        if (flag(input, "nested")) {
            child.relation("product");
        }
        query.relation_query("children", child).relation("selected");
    }
    if (flag(input, "enhancement")) {
        SelectQuery extra(std::allocator_arg, Allocator(r), "Parent");
        query.child_enhancement(extra);
    }
    PurposedSelectQuery purpose(
        std::allocator_arg, Allocator(r), query, "Inspect enhanced entities");
    auto& work = flag(input, "owned")
                     ? repo.template fetch_enhanced_entities_with_relation_aggregates_owned<Parent>(
                           std::move(purpose), {})
                     : repo.template fetch_enhanced_entities<Parent>(purpose);
    auto& result = work.wait();
    auto output = detail::model_json(r);
    if (!result) {
        detail::model_put(output, "error", result.error().to_string(r));
    } else {
        Json::Array items(r);
        for (const auto& value : result.value().data) {
            items.emplace_back(snapshot(r, value));
        }
        detail::model_put(output, "loaded", result.value().is_loaded);
        detail::model_put(output,
                          "total",
                          result.value().total_count ? Json(r, *result.value().total_count)
                                                     : Json(r));
        detail::model_put(output, "data", items);
    }
    detail::model_put(output, "hooks", behavior.calls);
    auto plan = c.id_set_plan();
    detail::model_put(output, "id_plan", plan ? Json(r, *plan) : Json(r));
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto c = global.make_user_context();
        auto input = Json::parse(c->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("invalid fixture JSON");
        }
        std::cout << evaluate(*c, *input).dump(c->memory_resource()) << '\n';
    }
}
