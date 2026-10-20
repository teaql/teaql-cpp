#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static const DynamicPropertyDefinitions& definitions(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    Vector<DynamicPropertyDefinitions::Field> fields(owner);
    for (const auto& entry : std::get<Json::Array>(input.data)) {
        const auto& pair = std::get<Json::Array>(entry.data);
        fields.emplace_back(text(pair[0]), DataType(std::get<std::uint64_t>(pair[1].data)));
    }
    auto value = DynamicPropertyDefinitions::from_fields(owner, fields);
    if (!value) {
        throw std::runtime_error("Invalid schema fixture");
    }
    return user_context.make<DynamicPropertyDefinitions>(std::move(value).value());
}

static Json observe(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    const auto& first_schema = definitions(user_context, field(input, "first"));
    const auto& second_schema = definitions(user_context, field(input, "second"));
    const auto& equal_schema = definitions(user_context, field(input, "first"));
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    SelectQuery query(std::allocator_arg, Allocator(owner), std::string_view("Order\0雪", 9));
    if (flags & 1) {
        query.project("id");
    }
    if (flags & 2) {
        query.comment(std::string_view("preserved\0comment", 17));
    }
    if (flags & 4) {
        query.purpose(std::string_view("preserved\0purpose", 17));
    }
    Json::Object output(owner);
    output.emplace("before", formatted_debug(owner, query));
    query.with_dynamic_property_definitions(first_schema);
    auto copy = query.clone(owner);
    output.emplace("first", formatted_debug(owner, query));
    output.emplace("first_identity",
                   Json(owner, query.dynamic_property_definitions.get() == &first_schema));
    output.emplace("clone_identity",
                   Json(owner, copy.dynamic_property_definitions.get() == &first_schema));
    query.with_dynamic_property_definitions(second_schema);
    output.emplace("second", formatted_debug(owner, query));
    output.emplace("second_identity",
                   Json(owner, query.dynamic_property_definitions.get() == &second_schema));
    output.emplace("second_equals_first", Json(owner, query == copy));
    query.with_dynamic_property_definitions(equal_schema);
    output.emplace("equal_identity",
                   Json(owner, query.dynamic_property_definitions.get() == &equal_schema));
    output.emplace("distinct_equal",
                   Json(owner, query.dynamic_property_definitions.get() != &first_schema));
    query.with_dynamic_property_definitions(*query.dynamic_property_definitions);
    auto prepared = query.prepare_for_list();
    output.emplace("prepare_identity",
                   Json(owner,
                        prepared.dynamic_property_definitions.get() ==
                            query.dynamic_property_definitions.get()));
    output.emplace("restored_equals_first", Json(owner, query == copy));
    output.emplace("after_input_release", formatted_debug(owner, query));
    output.emplace("original_clone", formatted_debug(owner, copy));
    output.emplace("prepared", formatted_debug(owner, prepared));
    return Json(owner, std::move(output));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid schema-builder input");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
