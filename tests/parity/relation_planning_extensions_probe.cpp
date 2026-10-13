#include <teaql/runtime/detail/data_service/relation_execution.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/request.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static std::uint64_t integer(const Json& value) {
    return std::get<std::uint64_t>(value.data);
}

static Json snapshot(Memory r, const SelectQuery& query) {
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", query.entity);
    auto strings = [r](std::span<const String> values) {
        Json::Array result(r);
        for (const auto& value : values) {
            result.emplace_back(value);
        }
        return Json(r, result);
    };
    detail::model_put(out, "projection", strings(query.projection));
    detail::model_put(out, "groups", strings(query.group_by_fields));
    Json::Array aggregates(r), orders(r);
    for (const auto& aggregate : query.aggregates) {
        Json::Array entry(r);
        entry.emplace_back(unsigned(aggregate.function));
        entry.emplace_back(aggregate.field);
        entry.emplace_back(aggregate.alias);
        aggregates.emplace_back(entry);
    }
    for (const auto& order : query.order_by) {
        Json::Array entry(r);
        entry.emplace_back(order.field);
        entry.emplace_back(unsigned(order.direction));
        entry.emplace_back(order.expr.has_value());
        orders.emplace_back(entry);
    }
    detail::model_put(out, "aggregates", aggregates);
    detail::model_put(out, "order", orders);
    detail::model_put(out, "facets", std::uint64_t(query.facets.size()));
    detail::model_put(out, "relations", std::uint64_t(query.relations.size()));
    detail::model_put(
        out, "partition", query.partition_field ? Json(r, *query.partition_field) : Json(r));
    detail::model_put(out, "filtered", query.predicate.has_value());
    Json slice(r);
    if (query.slice) {
        Json::Array values(r);
        values.emplace_back(query.slice->offset);
        values.emplace_back(query.slice->limit ? Json(r, *query.slice->limit) : Json(r));
        slice = Json(r, values);
    }
    detail::model_put(out, "slice", slice);
    return out;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto flag = [&](std::string_view key) {
        return std::get<bool>(field(input, key).data);
    };
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Child");
    if (flag("identity")) {
        descriptor.property("child_id", DataType::U64).is_id = true;
    }
    metadata.register_entity(std::move(descriptor));
    user_context.set_metadata(user_context, metadata);
    RelationDescriptor relation(std::allocator_arg, Allocator(r), "children", "Child");
    relation.local_key = "id";
    relation.foreign_key = "owner";
    relation.many = true;
    RelationLoadPlan plan(std::allocator_arg, Allocator(r), "Root", relation);
    SelectQuery query(std::allocator_arg, Allocator(r), "OriginalTarget");
    query.project("score");
    for (const auto& value : std::get<Json::Array>(field(input, "groups").data)) {
        query.group_by(text(value));
    }
    if (flag("aggregate")) {
        query.count("total");
    }
    if (flag("slice")) {
        query.offset(1).limit(2);
    }
    query.partition_field.emplace("original_partition", r);
    switch (integer(field(input, "order"))) {
    case 1:
        query.order_desc("owner");
        break;
    case 2:
        query.order_expr_asc(Expr::column(r, "owner"));
        break;
    case 3:
        query.order_desc("child_id");
        break;
    case 4:
        query.order_desc("score");
        break;
    case 5:
        query.order_desc("owner").order_desc("score");
        break;
    default:
        break;
    }
    query.relations.emplace_back("nested");
    if (flag("facets")) {
        SelectQuery facet(std::allocator_arg, Allocator(r), "FacetTarget");
        query.facets.emplace_back("facet", "nested", facet, false);
    }
    plan.query.emplace(std::move(query));
    Vector<CompactRow> parents(r);
    auto parent_kind = integer(field(input, "parents"));
    if (parent_kind == 1) {
        Record record(r);
        record.emplace(String("id", r), Value(r, std::uint64_t{7}));
        parents.emplace_back(record);
    }
    if (parent_kind != 0) {
        for (bool typed : {false, true}) {
            Record record(r);
            record.emplace(String("id", r), typed ? Value(r, TypedNull{DataType::U64}) : Value(r));
            parents.emplace_back(record);
        }
    }
    auto out = detail::model_json(r);
    detail::model_put(
        out,
        "normal",
        snapshot(r, detail::query_for_relation_plan(user_context, plan, parents, false)));
    detail::model_put(
        out,
        "compact",
        snapshot(r, detail::query_for_relation_plan(user_context, plan, parents, true)));
    detail::model_put(out, "base", snapshot(r, detail::base_relation_query(user_context, plan)));
    detail::model_put(out, "caller", snapshot(r, *plan.query));
    return out;
}

static void self_test() {
    GlobalContext global;
    Json* retained = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(
            r,
            R"({"identity":true,"groups":["owner","score"],"aggregate":true,"slice":true,"order":2,"facets":true,"parents":1})");
        auto output = evaluate(*user_context, *input);
        for (auto name : {"normal", "compact", "base"}) {
            const auto& result = field(output, name);
            if (integer(field(result, "facets")) != 0 || integer(field(result, "relations")) != 0) {
                throw std::runtime_error(
                    "Planner must consume facet and nested relation ownership");
            }
            const auto& orders = std::get<Json::Array>(field(result, "order").data);
            if (orders.size() != 3 || text(std::get<Json::Array>(orders[1].data)[0]) != "owner" ||
                text(std::get<Json::Array>(orders[2].data)[0]) != "score") {
                throw std::runtime_error(
                    "Grouped planning must append missing group fields after expression orders");
            }
        }
        if (integer(field(field(output, "caller"), "facets")) != 1) {
            throw std::runtime_error(
                "Planning must preserve the caller's reusable facet selection");
        }
        retained = &global.make<Json>(output);
    }
    if (retained->memory_resource() != global.memory_resource() ||
        integer(field(field(*retained, "caller"), "facets")) != 1) {
        throw std::runtime_error("Explicit Global copy must survive UserContext destruction");
    }
    std::cout << "Relation planning extension ownership contracts passed\n";
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        self_test();
    } else {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid fixture JSON");
            }
            std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    }
    std::pmr::set_default_resource(previous);
}
