#include <teaql/runtime/data_service.hpp>
#include <teaql/core/request.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include "fixture_value.hpp"
#include "../support/relation_hydration_fixture.hpp"
#include <iostream>
using namespace teaql;
using hydration_fixture::Model;
using RelationInstallSignature = EntityGraphDecodeResult (*)(const UserContext&,
                                                             std::span<CompactRow>,
                                                             const RelationLoadPlan&,
                                                             Vector<CompactRow>,
                                                             const EntityRuntimeState&,
                                                             EntityGraphBuilder&);
static_assert(
    std::same_as<decltype(static_cast<RelationInstallSignature>(&detail::install_flat_relation)),
                 RelationInstallSignature>);
static_assert(std::same_as<decltype(static_cast<RelationInstallSignature>(
                               &detail::install_compact_flat_relation)),
                           RelationInstallSignature>);

struct Mock : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    Json::Array requests;
    std::uint64_t pending = 0, fail_at = 0, facet_calls = 0, drops = 0;
    std::uint64_t* external_stops = nullptr;

    Mock(std::allocator_arg_t, Allocator a) : resource(a.resource()), requests(resource) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Ready {
        using allocator_type = Allocator;
        Result<QueryResult, String> value;
        Mock& owner;
        std::uint64_t remaining;
        bool facet, fail, stopped = false;
        String host_buffer;

        Ready(std::allocator_arg_t,
              Allocator a,
              QueryResult result,
              Mock& owner,
              bool facet,
              bool fail)
            : value(Result<QueryResult, String>::ok(a.resource(), std::move(result))), owner(owner),
              remaining(facet ? owner.pending : 0), facet(facet), fail(fail),
              host_buffer(32768, 'h', a.resource()) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal& wake) {
            if (remaining) {
                --remaining;
                wake.notify();
                return {};
            }
            if (fail) {
                return Result<QueryResult, String>::err(
                    value.memory_resource(), String("facet fetch failed", value.memory_resource()));
            }
            return std::move(value);
        }

        void finish_and_wait() noexcept {
            stop();
        }

        void cancel_and_wait() noexcept {
            stop();
        }

        void stop() noexcept {
            if (!stopped) {
                stopped = true;
                if (facet) {
                    if (host_buffer.size() != 32768 || host_buffer.back() != 'h') {
                        std::terminate();
                    }
                    ++owner.drops;
                    if (owner.external_stops) {
                        ++*owner.external_stops;
                    }
                }
            }
        }
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        auto snapshot = detail::model_json(resource);
        detail::model_put(snapshot, "entity", request.query.entity);
        detail::model_put(
            snapshot,
            "filter",
            request.query.predicate
                ? Json(resource, detail::debug_expr(resource, *request.query.predicate))
                : Json(resource));
        requests.emplace_back(std::move(snapshot));
        QueryResult result(
            std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, 2));
        if (request.query.entity != "Parent") {
            for (std::uint64_t i : {1, 2}) {
                Record values(r);
                values.emplace(String("id", r), Value(r, i * 10));
                values.emplace(String("owner", r), Value(r, i));
                values.emplace(String("link", r), Value(r, i));
                values.emplace(String("label", r), Value(r, i == 1 ? "child1" : "child2"));
                result.rows.emplace_back(values);
            }
        }
        bool facet = request.query.entity == "Leaf";
        auto index = facet ? ++facet_calls : 0;
        return make_operation<Result<QueryResult, String>, Ready>(
            r, std::move(result), *this, facet, facet && index == fail_at);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("Unexpected mutation");
    }
};

static EntityDescriptor descriptor(Memory r, std::string_view name) {
    EntityDescriptor value(std::allocator_arg, Allocator(r), name);
    for (auto field : {"id", "owner", "link"}) {
        value.property(field, DataType::U64).is_id = std::string_view(field) == "id";
    }
    value.property("label", DataType::Text);
    return value;
}

static void relation(EntityDescriptor& entity,
                     std::string_view name,
                     std::string_view target,
                     std::string_view local,
                     std::string_view foreign) {
    auto& value = entity.relation(name, target);
    value.local_key = local;
    value.foreign_key = foreign;
    value.many = true;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto flag = [&](std::string_view name) {
        return std::get<bool>(field(input, name).data);
    };
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    auto parent = descriptor(r, "Parent");
    for (auto name : {"children"}) {
        relation(parent, name, "Child", flag("reverse") ? "id" : "link", "owner");
        parent.relations.back().many = flag("many");
    }
    auto child = descriptor(r, "Child");
    for (auto name : {"link", "remaining"}) {
        relation(child, name, "Leaf", "link", "owner");
    }
    relation(child, "parent", "Parent", "owner", "link");
    relation(child, "tags", "Leaf", "id", "owner");
    metadata.register_entity(std::move(parent));
    metadata.register_entity(std::move(child));
    metadata.register_entity(descriptor(r, "Leaf"));
    user_context.set_metadata(user_context, metadata);
    auto& decoders = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    decoders.register_entity<Model<0>>();
    decoders.register_entity<Model<1>>();
    decoders.register_entity<Model<2>>();
    user_context.set_entity_graph_decoder_registry(decoders);
    user_context.disable_sql_log();
    auto& executor = user_context.make<Mock>();
    executor.pending = std::get<std::uint64_t>(field(input, "pending").data);
    executor.fail_at = std::get<std::uint64_t>(field(input, "fail_at").data);
    auto& service =
        EntityDataService<Mock>::for_executor(user_context, "Parent", user_context, executor);
    SelectQuery query(std::allocator_arg, Allocator(r), "Parent");
    for (auto name : {"children"}) {
        SelectQuery target(std::allocator_arg, Allocator(r), "Child");
        target.comment("Inspect sibling relation keys");
        target.purpose_text.emplace("Verify independent hydration", r);
        if (flag("sliced")) {
            target.offset(1).limit(1).order_desc("id");
        }
        if (flag("facets")) {
            SelectQuery facet(std::allocator_arg, Allocator(r), "Leaf");
            facet.comment("Observe relation facet values");
            target.facets.emplace_back("tagsFacet", "tags", facet, flag("include_all"));
        }

        if (false) {
            for (auto leaf_name : {flag("collision") ? "link" : "remaining", "remaining"}) {
                SelectQuery leaf(std::allocator_arg, Allocator(r), "Leaf");
                leaf.comment("Inspect descendant relation keys")
                    .purpose("Verify descendant hydration");
                target.relations.emplace_back(leaf_name, leaf);
            }
        }
        if (false) {
            SelectQuery inverse(std::allocator_arg, Allocator(r), "Parent");
            inverse.comment("Inspect explicit inverse target")
                .purpose("Preserve filtered inverse projection");
            target.relations.emplace_back("parent", inverse);
        }
        query.relations.emplace_back(name, target);
    }
    auto plans = detail::build_relation_plans_from_loads(user_context, "Parent", query.relations);
    if (!plans) {
        throw std::runtime_error("Invalid fixture plans");
    }
    Vector<CompactRow> parents(r);
    for (std::uint64_t i : {1, 2}) {
        Record values(r);
        values.emplace(String("id", r), Value(r, i));
        auto key_kind = std::get<std::uint64_t>(field(input, "keys").data);
        if (key_kind != 2) {
            values.emplace(String("link", r),
                           key_kind == 0 ? Value(r, i) : Value(r, TypedNull{DataType::U64}));
        }
        parents.emplace_back(values);
    }
    if (flag("missing_id")) {
        parents[1].remove("id");
    }
    auto& root = user_context.make<EntityRuntimeState>();
    auto& graph = user_context.make<EntityGraphBuilder>();
    detail::EntityRelationFetch<Mock> fetch(
        std::allocator_arg,
        Allocator(r),
        [&](const auto& child_service, SelectQuery query) -> auto& {
            return detail::EntityDataServiceAccess::fetch_all(child_service, query);
        });
    auto& work =
        flag("compact")
            ? detail::hydrate_compact_flat_plans(
                  service, parents, plans.value(), root, graph, fetch)
            : detail::hydrate_flat_plans(service, parents, plans.value(), root, graph, fetch);
    const detail::RelationHydrationResult<Mock>* result = nullptr;
    auto polls = std::get<std::uint64_t>(field(input, "polls").data);
    for (std::uint64_t i = 0; i < polls; ++i) {
        result = work.poll();
        if (result) {
            break;
        }
    }
    if (!result) {
        work.cancel();
    }
    auto out = detail::model_json(r);
    detail::model_put(
        out, "error", result && !*result ? Json(r, result->error().to_string(r)) : Json(r));
    detail::model_put(out, "cancelled", result == nullptr);
    detail::model_put(out, "drops", executor.drops);
    Json::Array values(r);
    for (const auto& row : parents) {
        auto value = detail::model_json(r);
        for (const auto& key : row.keys()) {
            detail::model_put(value, key, detail::debug_value(r, *row.get(key)));
        }
        values.emplace_back(std::move(value));
    }
    detail::model_put(out, "parents", values);
    auto entity_count = graph.entity_count();
    auto relation_count = graph.relation_list_count();
    if (!root.freeze_graph(std::move(graph))) {
        throw std::logic_error("Fixture graph freeze failed");
    }
    auto row_out = [&](const CompactRow& row) {
        auto fields = detail::model_json(r);
        for (const auto& key : row.keys()) {
            detail::model_put(fields, key, detail::debug_value(r, *row.get(key)));
        }
        return fields;
    };
    auto state_name = [](LoadedRelation state) -> std::string_view {
        switch (state) {
        case LoadedRelation::Loaded:
            return "Loaded";
        case LoadedRelation::Empty:
            return "Empty";
        case LoadedRelation::NotLoaded:
            return "NotLoaded";
        }
        throw std::logic_error("Unknown relation state");
    };
    Json::Array facets(r), states(r), lists(r), tables(r);
    for (std::uint64_t id : {1, 2}) {
        Json facet(r);
        if (auto* loaded = root.relation_facet("Parent", id, "children", "tagsFacet")) {
            Json::Array rows(r);
            for (const auto& row : loaded->data) {
                rows.emplace_back(row_out(row));
            }
            facet = Json(r, std::move(rows));
        }
        facets.emplace_back(std::move(facet));
        auto state = flag("many")
                         ? root.relation_list<Model<1>>("Parent", id, "children").state()
                         : root.relation_option<Model<1>>("Parent", id, "children").state();
        states.emplace_back(Json(r, state_name(state)));
        Json list(r);
        if (flag("many")) {
            if (auto* loaded = root.resolve_relation_list<Model<1>>("Parent", id, "children")) {
                Json::Array rows(r);
                for (const auto& model : loaded->data) {
                    rows.emplace_back(row_out(model.row));
                }
                list = Json(r, std::move(rows));
            }
        }
        lists.emplace_back(std::move(list));
    }
    for (std::uint64_t id : {10, 20}) {
        auto* model = root.resolve_entity<Model<1>>(id);
        tables.emplace_back(model ? row_out(model->row) : Json(r));
    }
    Json::Array markers(r);
    for (const auto& parent : parents) {
        markers.emplace_back(Json(r, parent.is_loaded_relation("children")));
    }
    detail::model_put(out, "markers", markers);
    detail::model_put(out, "facets", facets);
    detail::model_put(out, "states", states);
    detail::model_put(out, "lists", lists);
    detail::model_put(out, "tables", tables);
    detail::model_put(out, "entity_count", std::uint64_t(entity_count));
    detail::model_put(out, "relation_count", std::uint64_t(relation_count));
    detail::model_put(out, "requests", executor.requests);
    return out;
}

static void close_pending_owner_facet(bool compact) {
    GlobalContext global;
    std::uint64_t stops = 0;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    auto parent = descriptor(r, "Parent");
    relation(parent, "children", "Child", "id", "owner");
    metadata.register_entity(std::move(parent));
    auto child = descriptor(r, "Child");
    relation(child, "tags", "Leaf", "id", "owner");
    metadata.register_entity(std::move(child));
    metadata.register_entity(descriptor(r, "Leaf"));
    user_context->set_metadata(*user_context, metadata);
    user_context->disable_sql_log();
    auto& executor = user_context->make<Mock>();
    executor.pending = 100;
    executor.external_stops = &stops;
    auto& service =
        EntityDataService<Mock>::for_executor(*user_context, "Parent", *user_context, executor);
    auto& relation = user_context->make<RelationDescriptor>("children", "Child");
    relation.foreign_key = "owner";
    auto& plan = user_context->make<RelationLoadPlan>("Parent", relation);
    auto& outer = user_context->make<SelectQuery>("Child");
    outer.comment("Inspect pending owner facet").purpose("Verify Context cleanup");
    auto& facet = user_context->make<SelectQuery>("Leaf");
    facet.comment("Inspect pending facet child");
    outer.facets.emplace_back("tagsFacet", "tags", facet, false);
    plan.query.emplace(std::allocator_arg, Allocator(r), outer);
    auto& parents = user_context->make<Vector<CompactRow>>();
    auto& values = user_context->make<Record>();
    values.emplace(String("id", r), Value(r, std::uint64_t{1}));
    parents.emplace_back(values);
    auto& plans = user_context->make<Vector<RelationLoadPlan>>();
    plans.emplace_back(plan);
    auto& fetch = user_context->make<detail::EntityRelationFetch<Mock>>(
        [&](const auto& child_service, SelectQuery query) -> auto& {
            return detail::EntityDataServiceAccess::fetch_all(child_service, query);
        });
    auto& root = user_context->make<EntityRuntimeState>();
    auto& graph = user_context->make<EntityGraphBuilder>();
    auto& work =
        compact ? detail::hydrate_compact_flat_plans(service, parents, plans, root, graph, fetch)
                : detail::hydrate_flat_plans(service, parents, plans, root, graph, fetch);
    if (work.poll() || executor.facet_calls != 1 || stops != 0) {
        throw std::runtime_error("Owner facet must suspend with live host storage");
    }
    user_context->close();
    if (stops != 1 || user_context->allocation_stats().live_blocks != 0) {
        throw std::runtime_error(
            "Context close must stop the pending facet once before reclaiming blocks");
    }
    std::cout << "Pending owner facet Context cleanup passed\n";
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        close_pending_owner_facet(false);
        close_pending_owner_facet(true);
        std::pmr::set_default_resource(previous);
        return 0;
    }
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
