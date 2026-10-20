#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

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
    Ref<Order> retained;
    Ref<Order> expired;
    Ref<EntityDescriptor> retained_metadata;
    const std::string label(2048, 'x');
    const std::string input_text =
        std::string("{\"id\":3,\"version\":7,\"code\":\"") + label +
        "\",\"_note\":{\"x\":1},\"lines\":[{\"id\":4,\"version\":7,\"order_id\":3,\"product_id\":5,\"product\":{\"id\":5,\"sku\":\"" +
        label + "\",\"price\":\"1.20\"}}]}";
    {
        auto user_context = global.make_user_context();
        module(*user_context).apply_to(*user_context);
        auto r = user_context->memory_resource();
        {
            auto descriptor = Line::entity_descriptor(r);
            auto& property = descriptor.properties[1];
            property.json_column_name.emplace(label, r);
            retained_metadata = user_context->make_global<EntityDescriptor>(descriptor);
            const auto& copied =
                user_context->get(retained_metadata).properties[1].json_column_name;
            if (!copied || copied->get_allocator().resource() == r ||
                copied->data() == property.json_column_name->data()) {
                throw std::logic_error("JSON input alias retained its source allocator");
            }
        }
        {
            auto input = Json::parse(r, input_text);
            auto decoded = user_context->decode_json_entity<Order>(*input);
            if (!decoded) {
                throw std::logic_error("Generated JSON graph decode failed");
            }
            expired = user_context->make_ref<Order>(std::move(decoded).value());
            retained = user_context->make_global<Order>(user_context->get(expired));
        }
    }
    auto next_user_context = global.make_user_context();
    try {
        (void)next_user_context->get(expired);
        throw std::logic_error("Deleted User JSON reference was accepted");
    } catch (const ReferenceError& error) {
        if (error.kind() != ReferenceError::Kind::Closed) {
            throw;
        }
    }
    auto rebound = next_user_context->rebind_global(retained);
    auto r = next_user_context->memory_resource();
    auto metadata_reference = next_user_context->rebind_global(retained_metadata);
    const auto& copied_alias =
        next_user_context->get(metadata_reference).properties[1].json_column_name;
    if (!copied_alias || std::string_view(*copied_alias) != label) {
        throw std::logic_error("JSON input metadata did not survive User destruction");
    }
    auto& model = next_user_context->get(rebound);
    Order copy(std::allocator_arg, Allocator(r), model);
    auto output = std::move(copy).into_json(r);
    auto expected = Json::parse(r, input_text);
    if (!(output == *expected)) {
        throw std::logic_error("Retained generated JSON graph changed its projection");
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
        auto out = name == "Person"    ? execute<Person>(*user_context, *input)
                   : name == "Order"   ? execute<Order>(*user_context, *input)
                   : name == "Line"    ? execute<Line>(*user_context, *input)
                   : name == "Product" ? execute<Product>(*user_context, *input)
                   : name == "Profile" ? execute<Profile>(*user_context, *input)
                   : name == "Company" ? execute<Company>(*user_context, *input)
                                       : execute<Platform>(*user_context, *input);
        std::cout << out.dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
