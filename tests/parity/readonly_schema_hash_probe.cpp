#include <teaql/runtime/context.hpp>
#include <teaql/core/debug.hpp>
#include <teaql/core/query_debug.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Vector<DynamicPropertyDefinitions::Field> fields(r);
    for (const auto& item : std::get<Json::Array>(field(input, "fields").data)) {
        const auto& pair = std::get<Json::Array>(item.data);
        fields.emplace_back(text(pair[0]), DataType(std::get<std::uint64_t>(pair[1].data)));
    }
    auto& definitions = user_context.make<DynamicPropertyDefinitions>(
        std::move(DynamicPropertyDefinitions::from_fields(r, fields)).value());
    auto& columns =
        user_context.make<ColumnLayout>(Vector<String>(r), std::span<const String>{}, &definitions);
    auto& row = user_context.make<CompactRow>(columns, Vector<Value>(r));
    SelectQuery query(std::allocator_arg, Allocator(r), "Item");
    query.dynamic_property_definitions =
        detail::make_box<DynamicPropertyDefinitions>(r, definitions);
    Json::Object output(r);
    output.emplace("hash", Json(r, std::to_string(definitions.schema_hash())));
    output.emplace("definitions", Json(r, debug_string(r, definitions)));
    output.emplace("definitions_pretty",
                   Json(r, debug_string(r, definitions, DebugOptions{.pretty = true})));
    output.emplace("layout", Json(r, debug_string(r, columns)));
    output.emplace("row", Json(r, debug_string(r, row)));
    output.emplace("query", Json(r, debug_string(r, query)));
    return Json(r, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("invalid fixture JSON");
        }
        std::cout << observe(*user_context, *input).dump(r) << '\n';
    }
}
