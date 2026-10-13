#include <teaql/runtime.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static EntityGraphBuilder::RelationFacets make_facets(Memory r, const Json& input) {
    EntityGraphBuilder::RelationFacets facets(r);
    for (const auto& [name, values] : std::get<Json::Object>(input.data)) {
        SmartList<CompactRow> list(std::allocator_arg, Allocator(r));
        for (const auto& value : std::get<Json::Array>(values.data)) {
            Record record(r);
            record.emplace(String("label", r), Value(r, std::get<String>(value.data)));
            list.data.emplace_back(record);
        }
        facets.emplace(name, std::move(list));
    }
    return facets;
}

static Json inspect(Memory r, const EntityRuntimeState& state, const Json& input) {
    auto owner = text(field(input, "owner"));
    auto id = std::get<std::uint64_t>(field(input, "id").data);
    auto relation = text(field(input, "relation"));
    Json facet(r);
    if (auto* list = state.relation_facet(owner, id, relation, text(field(input, "facet")))) {
        Json::Array rows(r);
        for (const auto& row : list->data) {
            rows.emplace_back(Json(r, *row.get("label")->try_text()));
        }
        facet = Json(r, std::move(rows));
    }
    auto out = detail::model_json(r);
    auto list = state.relation_list<CompactRow>(owner, id, relation);
    auto option = state.relation_option<CompactRow>(owner, id, "optional");
    auto state_name = [](LoadedRelation value) -> std::string_view {
        switch (value) {
        case LoadedRelation::Loaded:
            return "Loaded";
        case LoadedRelation::Empty:
            return "Empty";
        case LoadedRelation::NotLoaded:
            return "NotLoaded";
        }
        throw std::logic_error("Unknown relation state");
    };
    detail::model_put(out, "facet", facet);
    detail::model_put(out, "list_state", state_name(list.state()));
    detail::model_put(out, "option_state", state_name(option.state()));
    return out;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    auto& graph = user_context.make<EntityGraphBuilder>();
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto owner = text(field(action, "owner"));
        auto id = std::get<std::uint64_t>(field(action, "id").data);
        auto relation = text(field(action, "relation"));
        auto op = text(field(action, "op"));
        if (op == "facets") {
            graph.install_relation_facets(
                owner, id, relation, make_facets(r, field(action, "facets")));
        } else if (op == "list") {
            SmartList<CompactRow> list(std::allocator_arg, Allocator(r));
            list.facets = make_facets(r, field(action, "facets"));
            graph.install_relation_list(owner, id, relation, std::move(list));
        } else if (op == "null") {
            graph.install_relation_option<CompactRow>(owner, id, relation, std::nullopt);
        } else {
            throw std::logic_error("Unknown graph fixture operation");
        }
    }
    auto count = graph.relation_list_count();
    auto observations = [&](const EntityRuntimeState& state) {
        Json::Array result(r);
        for (const auto& lookup : std::get<Json::Array>(field(input, "lookups").data)) {
            result.emplace_back(inspect(r, state, lookup));
        }
        return result;
    };
    auto before = observations(root);
    if (!root.freeze_graph(std::move(graph))) {
        throw std::logic_error("Fixture graph freeze failed");
    }
    auto& fresh =
        user_context.make<EntityRuntimeState>(EntityRuntimeState::fresh_with_shared_graph(r, root));
    auto& first =
        user_context.make<EntityRuntimeState>(EntityRuntimeState::fresh_with_shared_graph(r, root));
    Record record(r);
    record.emplace(String("id", r), Value(r, std::uint64_t{1}));
    CompactRow row(std::allocator_arg, Allocator(r), record);
    SmartList<CompactRow> loaded(std::allocator_arg, Allocator(r));
    loaded.facets = make_facets(r, field(input, "snapshot_facets"));
    row.set_loaded_relation("reference", loaded);
    first.set_original_compact_row("Owner", row);
    first.set_comment("independent ledger");
    auto out = detail::model_json(r);
    detail::model_put(out, "before", before);
    detail::model_put(out, "after", observations(root));
    detail::model_put(out, "shared", observations(fresh));
    detail::model_put(out, "relation_count", std::uint64_t(count));
    detail::model_put(out, "first_comment", *first.get_comment());
    detail::model_put(
        out, "fresh_comment", fresh.get_comment() ? Json(r, *fresh.get_comment()) : Json(r));
    if (detail::EntityRuntimeAccess::original_row(first)->has_loaded_relations()) {
        throw std::logic_error("Query facets leaked into original mutation snapshot");
    }
    return out;
}

static void facet_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& root = user_context->make<EntityRuntimeState>();
    auto& graph = user_context->make<EntityGraphBuilder>();
    auto& holder = user_context->make<SmartList<CompactRow>>();
    auto& row = user_context->make<Record>();
    row.emplace(String("label", r), Value(r, String(4096, 'x', r)));
    holder.data.emplace_back(row);
    auto& facets = user_context->make<EntityGraphBuilder::RelationFacets>();
    facets.emplace(String("choices", r), holder);
    graph.install_relation_facets("Owner", 1, "reference", facets);
    graph.install_relation_option<CompactRow>("Owner", 1, "reference", std::nullopt);
    if (!root.freeze_graph(std::move(graph))) {
        throw std::logic_error("Lifetime graph freeze failed");
    }
    auto* original = root.relation_facet("Owner", 1, "reference", "choices");
    if (!original || !root.relation_option<CompactRow>("Owner", 1, "reference").is_empty() ||
        detail::EntityRuntimeAccess::has_mutation_context(root)) {
        throw std::logic_error(
            "Facets must coexist with null relation and pristine mutation state");
    }
    auto& retained = global.make<EntityRuntimeState>(root);
    auto* copied = retained.relation_facet("Owner", 1, "reference", "choices");
    if (!copied || copied == original || copied->memory_resource() != global.memory_resource() ||
        copied->data[0].get("label")->try_text()->data() ==
            original->data[0].get("label")->try_text()->data()) {
        throw std::logic_error("Global retention must deeply copy nested graph facet storage");
    }
    user_context->close();
    user_context.reset();
    if (copied->data[0].get("label")->try_text()->size() != 4096 ||
        copied->data[0].get("label")->try_text()->back() != 'x') {
        throw std::logic_error("Copied graph facet must survive actual UserContext destruction");
    }
    std::cout << "Graph facet lifetime contracts passed\n";
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        facet_lifetimes();
        std::pmr::set_default_resource(previous);
        return 0;
    }
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid fixture JSON");
        }
        std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
