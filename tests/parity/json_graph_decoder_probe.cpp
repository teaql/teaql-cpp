#include "../support/json_graph_decoder_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include <iostream>

using namespace teaql;
using json_decoder_fixture::Model;

static std::uint64_t integer(const Json& input) {
    return std::get<std::uint64_t>(input.data);
}

template <unsigned K>
static Json fields(Memory r, const Model<K>& model) {
    auto out = detail::model_json(r);
    auto values = detail::model_json(r);
    for (const auto& name : model.row.keys()) {
        detail::model_put(values, name, detail::debug_value(r, *model.row.get(name)));
    }
    detail::model_put(out, "fields", values);
    detail::model_put(out, "view", model.state ? Json(r, model.state->json_view_id()) : Json(r));
    return out;
}

template <unsigned K>
static Json execute(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& original = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    const bool registered = std::get<bool>(field(input, "registered").data);
    if (registered) {
        original.register_entity<Model<K>>();
    }
    auto& copied = user_context.make<InMemoryEntityGraphDecoderRegistry>(original);
    original.register_entity<Model<3>>();
    auto& fresh = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    if (registered) {
        fresh.register_entity<Model<K>>();
    }
    auto& registry = std::get<bool>(field(input, "copy").data) ? copied : fresh;
    auto descriptor = json_decoder_fixture::descriptor(r);
    switch (integer(field(input, "metadata"))) {
    case 1: {
        decltype(descriptor.properties) reversed(r);
        for (auto it = descriptor.properties.rbegin(); it != descriptor.properties.rend(); ++it) {
            reversed.emplace_back(*it);
        }
        descriptor.properties.clear();
        for (const auto& item : reversed) {
            descriptor.properties.emplace_back(item);
        }
    } break;
    case 2:
        descriptor.table_name = "different_table";
        break;
    case 3:
        descriptor.name = "Missing";
        break;
    case 4:
        descriptor.properties.pop_back();
        break;
    case 5:
        descriptor.property("extra", DataType::Bool);
        break;
    case 6:
        descriptor.properties[0].nullable = false;
        break;
    case 7:
        descriptor.properties[0].column_name = "other_id";
        break;
    case 8:
        descriptor.relations[0].target_entity = "Other";
        break;
    case 9:
        descriptor.relations.pop_back();
        break;
    case 10: {
        decltype(descriptor.relations) reversed(r);
        for (auto it = descriptor.relations.rbegin(); it != descriptor.relations.rend(); ++it) {
            reversed.emplace_back(*it);
        }
        descriptor.relations.clear();
        for (const auto& item : reversed) {
            descriptor.relations.emplace_back(item);
        }
    } break;
    case 11:
        descriptor.audit_mask_fields_declared = true;
        descriptor.audit_mask_fields.emplace_back("label");
        break;
    default:
        break;
    }
    auto capability = registry.json_capability(descriptor);
    auto& root = user_context.make<EntityRuntimeState>(
        EntityRuntimeState(std::allocator_arg, Allocator(r))
            .with_json_view(integer(field(input, "parent_view"))));
    auto weak = detail::EntityRuntimeAccess::fresh_with_weak_graph(r, root).with_json_view(55);
    const bool weak_rejected =
        !weak.freeze_graph(EntityGraphBuilder(std::allocator_arg, Allocator(r)));
    auto& graph = user_context.make<EntityGraphBuilder>(
        std::get<bool>(field(input, "json").data)
            ? EntityGraphBuilder::for_json_read(r)
            : EntityGraphBuilder(std::allocator_arg, Allocator(r)));
    InMemoryEntityGraphDecoderRegistry::JsonRows rows(r);
    for (const auto& input_row : std::get<Json::Array>(field(input, "rows").data)) {
        Record values(r);
        for (const auto& [name, v] : std::get<Json::Object>(field(input_row, "fields").data)) {
            values.emplace(name, value(r, v));
        }
        rows.emplace_back(integer(field(input_row, "view")),
                          CompactRow(std::allocator_arg, Allocator(r), std::move(values)));
    }
    auto result =
        registry.decode_json_edge(text(field(input, "entity")),
                                  std::move(rows),
                                  root,
                                  graph,
                                  {"Owner", 1, "items", std::get<bool>(field(input, "many").data)});
    Json::Array counts(r);
    counts.emplace_back(Json(r, std::uint64_t(graph.entity_count())));
    counts.emplace_back(Json(r, std::uint64_t(graph.relation_list_count())));
    if (!root.freeze_graph(std::move(graph))) {
        throw std::logic_error("Graph freeze failed");
    }
    Json list(r);
    if (const auto* loaded = root.resolve_relation_list<Model<K>>("Owner", 1, "items")) {
        list = detail::model_json(r);
        Json::Array values(r);
        for (const auto& model : loaded->data) {
            values.emplace_back(fields(r, model));
        }
        detail::model_put(list, "rows", values);
        detail::model_put(list, "loaded", loaded->is_loaded);
    }
    const auto* option = root.resolve_relation_option<Model<K>>("Owner", 1, "items");
    const auto* identity = root.resolve_entity<Model<K>>(10);
    auto out = detail::model_json(r);
    detail::model_put(out, "capability", capability ? Json(r, capability.value()) : Json(r));
    detail::model_put(
        out, "capability_error", capability ? Json(r) : Json(r, capability.error().to_string(r)));
    detail::model_put(out, "decode_error", result ? Json(r) : Json(r, result.error().to_string(r)));
    detail::model_put(out, "counts", counts);
    detail::model_put(out, "list", list);
    detail::model_put(out, "option", option && *option ? fields(r, **option) : Json(r));
    detail::model_put(out, "option_present", option != nullptr);
    detail::model_put(out, "identity", identity ? fields(r, *identity) : Json(r));
    detail::model_put(out, "weak_rejected", weak_rejected);
    return out;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    switch (integer(field(input, "kind"))) {
    case 0:
        return execute<0>(user_context, input);
    case 1:
        return execute<1>(user_context, input);
    case 2:
        return execute<2>(user_context, input);
    default:
        return execute<3>(user_context, input);
    }
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
