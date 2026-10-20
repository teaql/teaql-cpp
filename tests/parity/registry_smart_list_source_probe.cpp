#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "../support/entity_decoder_fixture.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;
using namespace decoder_fixture;

static CompactRow row(Memory r, const Json& input) {
    Record values(r);
    for (const auto& [key, item] : std::get<Json::Object>(input.data)) {
        values.emplace(String(key, r), value(r, item));
    }
    return CompactRow(std::allocator_arg, Allocator(r), std::move(values));
}

static SmartList<CompactRow> metadata(Memory r, const Json& input) {
    SmartList<CompactRow> list(std::allocator_arg, Allocator(r));
    for (const auto& input_row : std::get<Json::Array>(field(input, "rows").data)) {
        list.data.emplace_back(row(r, input_row));
    }
    if (auto count = std::get_if<std::uint64_t>(&field(input, "total").data)) {
        list.total_count = *count;
    }
    list.is_loaded = std::get<bool>(field(input, "loaded").data);
    for (const auto& [key, input_value] :
         std::get<Json::Object>(field(input, "aggregations").data)) {
        list.aggregations.emplace(String(key, r), value(r, input_value));
    }
    for (const auto& [key, input_value] : std::get<Json::Object>(field(input, "summary").data)) {
        list.summary.emplace(String(key, r), value(r, input_value));
    }
    for (const auto& [key, input_list] : std::get<Json::Object>(field(input, "facets").data)) {
        list.facets.emplace(String(key, r), metadata(r, input_list));
    }
    return list;
}

static Json fields(Memory r, const Record& record) {
    auto output = detail::model_json(r);
    for (const auto& [key, item] : record) {
        detail::model_put(output, key, detail::debug_value(r, item));
    }
    return output;
}

template <class T>
static Json describe(Memory r, const SmartList<T>& list) {
    auto output = detail::model_json(r);
    Json::Array rows(r);
    for (const auto& entity : list.data) {
        if constexpr (std::same_as<T, CompactRow>) {
            rows.emplace_back(fields(r, entity.into_map(r)));
        } else {
            rows.emplace_back(fields(r, entity.row.into_map(r)));
        }
    }
    detail::model_put(output, "rows", rows);
    detail::model_put(output, "loaded", list.is_loaded);
    detail::model_put(output, "total", list.total_count ? Json(r, *list.total_count) : Json(r));
    detail::model_put(output, "aggregations", fields(r, list.aggregations));
    detail::model_put(output, "summary", fields(r, list.summary));
    auto facets = detail::model_json(r);
    for (const auto& [name, facet] : list.facets) {
        detail::model_put(facets, name, describe(r, facet));
    }
    detail::model_put(output, "facets", facets);
    return output;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    auto& registry = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    if (std::get<bool>(field(input, "seed").data)) {
        registry.register_entity<Item<0>>();
        Vector<CompactRow> seed(r);
        Record previous(r);
        previous.emplace(String("id", r), Value(r, std::uint64_t{99}));
        previous.emplace(String("label", r), Value(r, "previous"));
        seed.emplace_back(CompactRow(std::allocator_arg, Allocator(r), std::move(previous)));
        auto result =
            registry.decode_compact_list("Item", std::move(seed), root, graph, "Owner", 7, "items");
        if (!result) {
            throw std::runtime_error("Seed relation failed");
        }
    }
    auto& selected = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    auto registration = std::get<std::uint64_t>(field(input, "registration").data);
    if (registration != 0) {
        selected.register_entity<Item<0>>();
    }
    auto& copied = user_context.make<InMemoryEntityGraphDecoderRegistry>(selected);
    selected.register_entity<Item<1>>();
    auto& direct = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    if (registration == 1) {
        direct.register_entity<Item<0>>();
    }
    const auto& active = registration == 2 ? copied : direct;
    auto list = metadata(r, field(input, "list"));
    auto outcome = text(field(input, "mode")) == "smart"
                       ? active.decode_compact_smart_list(
                             "Item", std::move(list), root, graph, "Owner", 7, "items")
                       : active.decode_compact_list(
                             "Item", std::move(list.data), root, graph, "Owner", 7, "items");
    auto output = detail::model_json(r);
    Json::Array counts(r);
    counts.emplace_back(graph.entity_count());
    counts.emplace_back(graph.relation_list_count());
    detail::model_put(output, "counts", counts);
    detail::model_put(output, "error", outcome ? Json(r) : Json(r, outcome.error().to_string(r)));
    auto frozen = root.freeze_graph(std::move(graph));
    if (!frozen) {
        throw std::runtime_error("Freeze failed");
    }
    const auto* decoded = root.resolve_relation_list<Item<0>>("Owner", 7, "items");
    detail::model_put(output, "list", decoded ? describe(r, *decoded) : Json(r));
    return output;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            return 2;
        }
        std::cout << evaluate(*user_context, input.value()).dump(r) << '\n';
    }
}
