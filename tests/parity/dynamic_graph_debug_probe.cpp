#include <teaql/runtime/graph_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto parsed = Json::parse(r, line);
            if (!parsed) {
                throw std::runtime_error("invalid graph debug fixture");
            }
            const auto& input = *parsed;
            GraphNode node(std::allocator_arg, Allocator(r), "School");
            if (text(field(input, "mode")) != "none") {
                const std::array types{DynamicFieldDefinitions::Field{"title", DataType::Text}};
                auto& definitions = user_context->make<DynamicFieldDefinitions>(
                    std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", types))
                        .value());
                const DynamicFieldDefinitions* bound = &definitions;
                if (std::get<bool>(field(input, "bound").data)) {
                    bound = &user_context->make<DynamicFieldDefinitions>(
                        definitions.with_storage_binding(r, "profile", 37));
                }
                Value::Object values(r);
                if (text(field(input, "mode")) != "empty") {
                    values.emplace(String("title", r),
                                   text(field(input, "mode")) == "null"
                                       ? Value(r)
                                       : Value(r, text(field(input, "value"))));
                }
                node.dynamic_fields = detail::make_box<DynamicFieldValues>(
                    r, std::move(DynamicFieldValues::from_values(r, *bound, values)).value());
            }
            auto& copied = global.make<GraphNode>(node);
            Json::Object before(r);
            before.emplace(String("debug", r), Json(r, debug_string(r, node)));
            DebugOptions options;
            options.pretty = true;
            before.emplace(String("pretty", r), Json(r, debug_string(r, node, options)));
            before.emplace(String("copy_equal", r), Json(r, copied == node));
            if (node.dynamic_fields) {
                if (!node.dynamic_fields->assign("title", Value(r, text(field(input, "after"))))) {
                    throw std::runtime_error("invalid graph mutation fixture");
                }
            }
            Json::Object output(r);
            output.emplace(String("before", r), Json(r, before));
            output.emplace(String("equal_after", r), Json(r, copied == node));
            output.emplace(String("copy_debug", r), Json(r, debug_string(r, copied)));
            std::cout << Json(r, output).dump(r) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
