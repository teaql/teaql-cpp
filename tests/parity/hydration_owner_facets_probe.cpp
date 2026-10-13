#include <teaql/runtime/data_service.hpp>
#include <teaql/core/request.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

struct Mock : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    Json::Array requests;

    Mock(std::allocator_arg_t, Allocator a) : resource(a.resource()), requests(resource) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Ready {
        using allocator_type = Allocator;
        Result<QueryResult, String> value;

        Ready(std::allocator_arg_t, Allocator a, QueryResult result)
            : value(Result<QueryResult, String>::ok(a.resource(), std::move(result))) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal&) {
            return std::move(value);
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
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
        return make_operation<Result<QueryResult, String>, Ready>(r, std::move(result));
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
    for (auto name : {"link", "remaining"}) {
        relation(parent, name, "Child", "link", "owner");
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
    user_context.disable_sql_log();
    auto& executor = user_context.make<Mock>();
    auto& service =
        EntityDataService<Mock>::for_executor(user_context, "Parent", user_context, executor);
    SelectQuery query(std::allocator_arg, Allocator(r), "Parent");
    for (auto name : {"link"}) {
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
    detail::EntityRelationFetch<Mock> fetch(
        std::allocator_arg,
        Allocator(r),
        [&](const auto& child_service, SelectQuery query) -> auto& {
            return detail::EntityDataServiceAccess::fetch_all(child_service, query);
        });
    auto& result = detail::enhance_relation_plans(service, parents, plans.value(), fetch).wait();
    auto out = detail::model_json(r);
    detail::model_put(out, "error", result ? Json(r) : Json(r, result.error().to_string(r)));
    Json::Array values(r);
    for (const auto& row : parents) {
        auto value = detail::model_json(r);
        for (const auto& key : row.keys()) {
            detail::model_put(value, key, detail::debug_value(r, *row.get(key)));
        }
        values.emplace_back(std::move(value));
    }
    detail::model_put(out, "parents", values);
    Json::Array facets(r);
    for (const auto& parent : parents) {
        Json item(r);
        if (auto* loaded = parent.loaded_relation("link")) {
            item = detail::model_json(r);
            for (const auto& [name, list] : loaded->facets) {
                Json::Array rows(r);
                for (const auto& row : list.data) {
                    auto fields = detail::model_json(r);
                    for (const auto& key : row.keys()) {
                        detail::model_put(fields, key, detail::debug_value(r, *row.get(key)));
                    }
                    rows.emplace_back(std::move(fields));
                }
                detail::model_put(item, name, rows);
            }
        }
        facets.emplace_back(std::move(item));
    }
    detail::model_put(out, "facets", facets);
    detail::model_put(out, "requests", executor.requests);
    return out;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
