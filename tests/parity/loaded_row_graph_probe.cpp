#include <teaql/runtime.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "../support/relation_hydration_fixture.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;
using hydration_fixture::Model;

static SmartList<CompactRow> list_in(Memory, const Json&);

static CompactRow row_in(Memory r, const Json& input) {
    Record fields(r);
    for (const auto& [name, v] : std::get<Json::Object>(field(input, "fields").data)) {
        fields.emplace(name, value(r, v));
    }
    CompactRow row(std::allocator_arg, Allocator(r), std::move(fields));
    for (const auto& [name, v] : std::get<Json::Object>(field(input, "loaded").data)) {
        row.set_loaded_relation(name, list_in(r, v));
    }
    return row;
}

static SmartList<CompactRow> list_in(Memory r, const Json& input) {
    SmartList<CompactRow> list(std::allocator_arg, Allocator(r));
    for (const auto& row : std::get<Json::Array>(field(input, "data").data)) {
        list.data.emplace_back(row_in(r, row));
    }
    if (auto* count = std::get_if<std::uint64_t>(&field(input, "total_count").data)) {
        list.total_count = *count;
    }
    list.is_loaded = std::get<bool>(field(input, "is_loaded").data);
    for (const auto& [name, v] : std::get<Json::Object>(field(input, "summary").data)) {
        list.summary.emplace(name, value(r, v));
    }
    for (const auto& [name, v] : std::get<Json::Object>(field(input, "aggregations").data)) {
        list.aggregations.emplace(name, value(r, v));
    }
    for (const auto& [name, v] : std::get<Json::Object>(field(input, "facets").data)) {
        list.facets.emplace(name, list_in(r, v));
    }
    return list;
}

static Json record_out(Memory r, const CompactRow& row) {
    auto out = detail::model_json(r);
    for (const auto& key : row.keys()) {
        detail::model_put(out, key, detail::debug_value(r, *row.get(key)));
    }
    return out;
}

template <class T>
static Json list_out(Memory r, const SmartList<T>& list, Json::Array rows) {
    auto out = detail::model_json(r);
    detail::model_put(out, "data", rows);
    detail::model_put(out, "total_count", list.total_count ? Json(r, *list.total_count) : Json(r));
    detail::model_put(out, "is_loaded", list.is_loaded);
    for (auto pair :
         {std::pair("summary", &list.summary), std::pair("aggregations", &list.aggregations)}) {
        auto fields = detail::model_json(r);
        for (const auto& [name, value] : *pair.second) {
            detail::model_put(fields, name, detail::debug_value(r, value));
        }
        detail::model_put(out, pair.first, fields);
    }
    auto facets = detail::model_json(r);
    for (const auto& [name, list] : list.facets) {
        Json::Array rows(r);
        for (const auto& row : list.data) {
            rows.emplace_back(record_out(r, row));
        }
        detail::model_put(facets, name, list_out(r, list, std::move(rows)));
    }
    detail::model_put(out, "facets", facets);
    return out;
}

template <unsigned K>
static Json inspect(Memory r,
                    const EntityRuntimeState& root,
                    std::string_view owner,
                    std::uint64_t id,
                    std::string_view relation) {
    auto out = detail::model_json(r);
    Json list(r), option(r), facet(r);
    if (auto* value = root.resolve_relation_list<Model<K>>(owner, id, relation)) {
        Json::Array rows(r);
        for (const auto& model : value->data) {
            rows.emplace_back(record_out(r, model.row));
        }
        list = list_out(r, *value, std::move(rows));
    }
    auto* optional = root.resolve_relation_option<Model<K>>(owner, id, relation);
    if (optional && *optional) {
        option = record_out(r, (**optional).row);
    }
    if (auto* value = root.relation_facet(owner, id, relation, "choices")) {
        Json::Array rows(r);
        for (const auto& row : value->data) {
            rows.emplace_back(record_out(r, row));
        }
        facet = list_out(r, *value, std::move(rows));
    }
    detail::model_put(out, "list", list);
    detail::model_put(out, "option", option);
    detail::model_put(out, "option_present", optional != nullptr);
    detail::model_put(out, "facet", facet);
    detail::model_put(out, "present", root.has_relation_view(owner, id, relation));
    return out;
}

static void setup(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Parent");
    auto& relation = parent.relation("children", "Child");
    relation.local_key = text(field(input, "local"));
    relation.foreign_key = text(field(input, "foreign"));
    relation.many = std::get<bool>(field(input, "many").data);
    EntityDescriptor child(std::allocator_arg, Allocator(r), "Child");
    auto& leaves = child.relation("leaves", "Leaf");
    leaves.local_key = "id";
    leaves.foreign_key = "owner";
    leaves.many = true;
    metadata.register_entity(std::move(parent));
    metadata.register_entity(std::move(child));
    metadata.register_entity(EntityDescriptor(std::allocator_arg, Allocator(r), "Leaf"));
    user_context.set_metadata(user_context, metadata);
    auto& registry = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    if (std::get<bool>(field(input, "registered").data)) {
        registry.register_entity<Model<1>>();
        registry.register_entity<Model<2>>();
    }
    user_context.set_entity_graph_decoder_registry(registry);
    user_context.disable_sql_log();
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    setup(user_context, input);
    Vector<CompactRow> rows(r);
    for (const auto& row : std::get<Json::Array>(field(input, "rows").data)) {
        rows.emplace_back(row_in(r, row));
    }
    auto result = detail::attach_flat_relation_graph(user_context, "Parent", rows);
    auto out = detail::model_json(r);
    detail::model_put(out, "error", result ? Json(r) : Json(r, result.error().to_string(r)));
    Json::Array records(r), markers(r), loaded(r);
    for (const auto& row : rows) {
        records.emplace_back(record_out(r, row));
        markers.emplace_back(Json(r, row.is_loaded_relation("children")));
        loaded.emplace_back(Json(r, row.loaded_relation("children") != nullptr));
    }
    detail::model_put(out, "rows", records);
    detail::model_put(out, "markers", markers);
    detail::model_put(out, "loaded_remaining", loaded);
    Json graph(r);
    if (result) {
        auto& root = result.value();
        graph = detail::model_json(r);
        Json::Array owners(r);
        owners.emplace_back(inspect<1>(r, root, "Parent", 1, "children"));
        owners.emplace_back(inspect<1>(r, root, "Parent", 2, "children"));
        detail::model_put(graph, "owners", owners);
        detail::model_put(graph, "nested", inspect<2>(r, root, "Child", 10, "leaves"));
        auto* model = root.resolve_entity<Model<1>>(10);
        detail::model_put(graph, "table", model ? record_out(r, model->row) : Json(r));
        detail::model_put(graph,
                          "table_marker",
                          model ? Json(r, model->row.is_loaded_relation("leaves")) : Json(r));
    }
    detail::model_put(out, "graph", graph);
    return out;
}

static void lifetime_contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    EntityRuntimeState* retained = nullptr;
    std::optional<RelationHandle<SmartList<Model<1>>>> borrow;
    {
        auto input = Json::parse(
            r,
            R"({"many":true,"local":"id","foreign":"owner","registered":true,"rows":[{"fields":{"id":{"kind":"u64","value":"1"},"children":{"kind":"list","value":[]}},"loaded":{"children":{"data":[{"fields":{"id":{"kind":"u64","value":"10"},"owner":{"kind":"u64","value":"1"}},"loaded":{}}],"total_count":91,"is_loaded":false,"summary":{"discarded":{"kind":"text","value":"relation summary is rebuilt"}},"aggregations":{},"facets":{"choices":{"data":[{"fields":{"label":{"kind":"text","value":"long facet payload retained through explicit Global copy after actual Context destruction"}},"loaded":{}}],"total_count":123,"is_loaded":false,"summary":{"reason":{"kind":"text","value":"facet summary remains intact"}},"aggregations":{},"facets":{}}}}}}]})");
        if (!input) {
            throw std::logic_error("Invalid lifetime fixture");
        }
        setup(*user_context, *input);
        Vector<CompactRow> rows(r);
        rows.emplace_back(row_in(r, std::get<Json::Array>(field(*input, "rows").data).front()));
        auto result = detail::attach_flat_relation_graph(*user_context, "Parent", rows);
        if (!result) {
            throw std::logic_error("Lifetime reconstruction failed");
        }
        auto& root = result.value();
        borrow.emplace(root.relation_list<Model<1>>("Parent", 1, "children"));
        auto* source = root.relation_facet("Parent", 1, "children", "choices");
        auto* list = root.resolve_relation_list<Model<1>>("Parent", 1, "children");
        if (!source || !list || list->total_count || !list->summary.empty() || !list->is_loaded ||
            detail::EntityRuntimeAccess::has_mutation_context(root)) {
            throw std::logic_error(
                "Reconstruction must preserve facets and rebuild read metadata without a ledger");
        }
        retained = &global.make<EntityRuntimeState>(root);
        auto* copied = retained->relation_facet("Parent", 1, "children", "choices");
        if (!copied || copied == source ||
            copied->data.front().get("label")->try_text()->data() ==
                source->data.front().get("label")->try_text()->data()) {
            throw std::logic_error("Nested facet strings must be independently copied into Global");
        }
    }
    user_context->close();
    user_context.reset();
    bool rejected = false;
    try {
        (void)borrow->value();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Closed;
    }
    auto* facet = retained->relation_facet("Parent", 1, "children", "choices");
    auto* list = retained->resolve_relation_list<Model<1>>("Parent", 1, "children");
    if (!rejected || !facet || !list || facet->memory_resource() != global.memory_resource() ||
        facet->total_count != 123 || facet->is_loaded || facet->summary.size() != 1 ||
        facet->data.front().get("label")->try_text()->size() < 40 || list->facets.size() != 1) {
        throw std::logic_error(
            "Global graph/facet metadata must survive close while User borrows expire");
    }
    std::cout << "Loaded row graph lifetime contracts passed\n";
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        lifetime_contracts();
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
