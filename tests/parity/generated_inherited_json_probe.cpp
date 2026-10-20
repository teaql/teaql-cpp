#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace cpp_inheritance_graph;

template <Entity T>
static Json snapshot(Memory r, const T& entity) {
    auto out = detail::model_json(r);
    auto loaded = detail::model_json(r);
    auto descriptor = T::entity_descriptor(r);
    for (const auto& field : descriptor.properties) {
        detail::model_put(loaded, field.name, entity.is_field_loaded(field.name));
    }
    for (const auto& relation : descriptor.relations) {
        detail::model_put(loaded, relation.name, entity.is_field_loaded(relation.name));
    }
    detail::model_put(out, "loaded", loaded);
    T copy(std::allocator_arg, Allocator(r), entity);
    detail::model_put(out, "json", std::move(copy).into_json(r));
    const auto dirty = entity.dirty_fields();
    Json::Array fields(r);
    if (dirty) {
        for (const auto& name : *dirty) {
            fields.emplace_back(Json(r, name));
        }
    }
    detail::model_put(out, "dirty", dirty ? Json(r, fields) : Json(r));
    detail::model_put(out, "is_new", entity.is_new());
    detail::model_put(out, "deleted", entity.is_marked_as_delete());
    return out;
}

template <Entity T>
static Json execute(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto out = detail::model_json(r);
    if (std::get<bool>(field(input, "array").data)) {
        auto result = user_context.decode_json_entities<T>(field(input, "value"));
        if (!result) {
            detail::model_put(out, "error", result.error().to_string(r));
        } else {
            Json::Array rows(r);
            for (const auto& entity : result.value()) {
                rows.emplace_back(snapshot(r, entity));
            }
            detail::model_put(out, "rows", rows);
        }
    } else {
        auto result = user_context.decode_json_entity<T>(field(input, "value"));
        if (!result) {
            detail::model_put(out, "error", result.error().to_string(r));
        } else {
            detail::model_put(out, "row", snapshot(r, result.value()));
        }
    }
    return out;
}

static void self_test(GlobalContext& global) {
    Ref<ResearchAcademy> retained;
    Ref<ResearchAcademy> expired;
    const std::string label(2048, 'x');
    const auto input_text =
        std::string("{\"id\":7,\"version\":5,\"name\":\"") + label +
        "\",\"rank\":3,\"campus_code\":\"" + label +
        "\",\"budget\":\"12.50\",\"advisor_id\":2,\"advisor\":{\"id\":2,\"version\":7,\"name\":\"" +
        label +
        "\",\"active\":true,\"platform_id\":1,\"platform\":{\"id\":1,\"name\":\"tenant\"}}}";
    {
        auto user_context = global.make_user_context();
        module(*user_context).apply_to(*user_context);
        auto r = user_context->memory_resource();
        {
            auto input = Json::parse(r, input_text);
            auto decoded = user_context->decode_json_entity<ResearchAcademy>(*input);
            if (!decoded) {
                throw std::logic_error("Inherited JSON hydration failed");
            }
            expired = user_context->make_ref<ResearchAcademy>(std::move(decoded).value());
            retained = user_context->make_global<ResearchAcademy>(user_context->get(expired));
        }
    }
    auto next_user_context = global.make_user_context();
    try {
        (void)next_user_context->get(expired);
        throw std::logic_error("Expired inherited JSON object was accepted");
    } catch (const ReferenceError& error) {
        if (error.kind() != ReferenceError::Kind::Closed) {
            throw;
        }
    }
    auto r = next_user_context->memory_resource();
    auto rebound = next_user_context->rebind_global(retained);
    ResearchAcademy copy(std::allocator_arg, Allocator(r), next_user_context->get(rebound));
    auto json = std::move(copy).into_json(r);
    auto expected = Json::parse(r, input_text);
    if (!(json == *expected)) {
        throw std::logic_error("Global retained inheritance or nested relation projection changed");
    }
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        self_test(global);
        std::pmr::set_default_resource(previous);
        return 0;
    }
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        module(*user_context).apply_to(*user_context);
        auto input = Json::parse(user_context->memory_resource(), line);
        auto name = text(field(*input, "entity"));
        auto out = name == "School"            ? execute<School>(*user_context, *input)
                   : name == "Academy"         ? execute<Academy>(*user_context, *input)
                   : name == "ResearchAcademy" ? execute<ResearchAcademy>(*user_context, *input)
                   : name == "Specialist"      ? execute<Specialist>(*user_context, *input)
                   : name == "Enrollment"      ? execute<Enrollment>(*user_context, *input)
                                               : execute<Platform>(*user_context, *input);
        std::cout << out.dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
