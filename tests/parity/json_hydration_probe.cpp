#include "../support/json_hydration_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include <iostream>

using namespace teaql;
using json_hydration_fixture::Model;

template <unsigned K>
static Json projection(Memory r, const Model<K>& model, unsigned depth = 0) {
    auto out = detail::model_json(r);
    auto values = detail::model_json(r);
    for (const auto& name : model.row.keys()) {
        detail::model_put(values, name, detail::debug_value(r, *model.row.get(name)));
    }
    detail::model_put(out, "fields", values);
    detail::model_put(out, "view", model.state ? Json(r, model.state->json_view_id()) : Json(r));
    auto loaded = detail::model_json(r);
    for (auto name : {"parent", "alternate", "children"}) {
        detail::model_put(loaded, name, model.row.is_loaded_relation(name));
    }
    detail::model_put(out, "loaded", loaded);
    auto edges = detail::model_json(r);
    if (model.state && depth < 6) {
        auto identity = model.id_value().try_u64();
        if (identity) {
            for (auto name : {"parent", "alternate"}) {
                const auto* edge = model.state->template resolve_relation_option<Model<K>>(
                    "JsonNode", *identity, name);
                if (edge) {
                    detail::model_put(
                        edges, name, *edge ? projection(r, **edge, depth + 1) : Json(r));
                }
            }
            if (const auto* list = model.state->template resolve_relation_list<Model<K>>(
                    "JsonNode", *identity, "children")) {
                Json::Array items(r);
                for (const auto& child : list->data) {
                    items.emplace_back(projection(r, child, depth + 1));
                }
                detail::model_put(edges, "children", items);
            }
        }
    }
    detail::model_put(out, "edges", edges);
    return out;
}

template <unsigned K>
static Json execute(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    const auto mode = std::get<std::uint64_t>(field(input, "metadata").data);
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    auto descriptor = json_hydration_fixture::descriptor(r);
    if (mode == 2) {
        descriptor.properties[0].nullable = false;
    }
    if (mode == 3) {
        descriptor.table_name = "other_table";
    }
    if (mode != 1) {
        metadata.register_entity(std::move(descriptor));
    }
    user_context.set_metadata(user_context, metadata);
    auto& registry = user_context.make<InMemoryEntityGraphDecoderRegistry>();
    if (std::get<bool>(field(input, "registered").data)) {
        registry.register_entity<Model<K>>();
    }
    user_context.set_entity_graph_decoder_registry(registry);
    auto out = detail::model_json(r);
    const bool array = std::get<bool>(field(input, "array").data);
    const auto& payload = field(input, "value");
    if (array) {
        auto result = user_context.decode_json_entities<Model<K>>(payload);
        if (!result) {
            detail::model_put(out, "error", result.error().to_string(r));
            return out;
        }
        Json::Array rows(r), sharing(r);
        for (const auto& model : result.value()) {
            rows.emplace_back(projection(r, model));
            Json::Array shared(r);
            for (const auto& other : result.value()) {
                shared.emplace_back(
                    Json(r, model.row.shared_layout() == other.row.shared_layout()));
            }
            sharing.emplace_back(Json(r, shared));
        }
        detail::model_put(out, "rows", rows);
        detail::model_put(out, "sharing", sharing);
    } else {
        auto result = user_context.decode_json_entity<Model<K>>(payload);
        if (!result) {
            detail::model_put(out, "error", result.error().to_string(r));
            return out;
        }
        detail::model_put(out, "row", projection(r, result.value()));
    }
    return out;
}

static void self_test(GlobalContext& global) {
    Ref<Model<2>> retained;
    Ref<Model<2>> expired;
    const std::string label(2048, 'x');
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        metadata.register_entity(json_hydration_fixture::descriptor(r));
        user_context->set_metadata(*user_context, metadata);
        auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
        registry.register_entity<Model<2>>();
        user_context->set_entity_graph_decoder_registry(registry);
        auto text = std::string("{\"id\":1,\"label\":\"") + label +
                    "\",\"children\":[{\"id\":2,\"parent_id\":1,\"parent\":{\"id\":1}}]}";
        {
            auto input = Json::parse(r, text);
            auto decoded = user_context->decode_json_entity<Model<2>>(*input);
            if (!decoded || !decoded.value().state ||
                !decoded.value().row.is_loaded_relation("children")) {
                throw std::logic_error("JSON graph hydration failed");
            }
            expired = user_context->make_ref<Model<2>>(std::move(decoded).value());
            retained = user_context->make_global<Model<2>>(user_context->get(expired));
            if (user_context->get(retained).row.get("label")->try_text()->data() ==
                user_context->get(expired).row.get("label")->try_text()->data()) {
                throw std::logic_error("JSON Global copy retained User character storage");
            }
        }
    }
    auto next_user_context = global.make_user_context();
    try {
        (void)next_user_context->get(expired);
        throw std::logic_error("Expired JSON model was accepted");
    } catch (const ReferenceError& error) {
        if (error.kind() != ReferenceError::Kind::Closed) {
            throw;
        }
    }
    auto rebound = next_user_context->rebind_global(retained);
    auto& model = next_user_context->get(rebound);
    if (*model.row.get("label")->try_text() != label) {
        throw std::logic_error("Retained JSON scalar changed");
    }
    const auto* children = model.state->resolve_relation_list<Model<2>>("JsonNode", 1, "children");
    if (!children || children->data.size() != 1 || !children->is_loaded) {
        throw std::logic_error("Retained JSON graph lost its relation");
    }
    const auto& child = children->data.front();
    const auto* parent = child.state->resolve_relation_option<Model<2>>("JsonNode", 2, "parent");
    if (!parent || !*parent || (**parent).id_value().try_u64() != 1) {
        throw std::logic_error("Retained JSON child lost its view-specific parent");
    }
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    ContextLimits limits;
    if (argc == 2 && std::string_view(argv[1]) == "--large-fixture") {
        limits.user.max_bytes = 512 * 1024 * 1024;
        limits.max_live_user_bytes = limits.user.max_bytes;
    }
    GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        self_test(global);
        std::pmr::set_default_resource(previous);
        return 0;
    }
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        auto kind = std::get<std::uint64_t>(field(*input, "kind").data);
        auto result = kind == 0   ? execute<0>(*user_context, *input)
                      : kind == 1 ? execute<1>(*user_context, *input)
                      : kind == 2 ? execute<2>(*user_context, *input)
                                  : execute<3>(*user_context, *input);
        std::cout << result.dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
