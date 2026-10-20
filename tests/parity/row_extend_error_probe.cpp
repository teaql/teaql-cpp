#include <teaql/core/debug.hpp>
#include <teaql/core/dynamic_fields_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <array>
#include <iostream>

static CompactRow row(UserContext& user_context, std::string_view name, DataType type) {
    auto owner = user_context.memory_resource();
    const std::array<DynamicPropertyDefinitions::Field, 1> fields{{{name, type}}};
    auto definitions = DynamicPropertyDefinitions::from_fields(owner, fields);
    if (!definitions) {
        throw std::runtime_error("Invalid row schema fixture");
    }
    auto& metadata = user_context.make<DynamicPropertyDefinitions>(std::move(definitions).value());
    Vector<String> names(owner);
    names.emplace_back(name);
    auto& layout = user_context.make<ColumnLayout>(std::move(names));
    const auto& shaped = layout.with_dynamic_property_definitions(metadata);
    Vector<Value> values(owner);
    values.emplace_back(nullptr);
    return CompactRow(std::allocator_arg, Allocator(owner), shaped, std::move(values));
}

static Json observe(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    if (auto* kind = optional(input, "kind"); kind && text(*kind) == "error_debug") {
        using namespace std::literals::string_view_literals;
        constexpr std::array codes{"DYNAMIC_PROPERTY_TYPE_MISMATCH"sv,
                                   "DYNAMIC_FIELD_OWNER_MISMATCH"sv,
                                   "DYNAMIC_FIELD_SCHEMA_MISMATCH"sv,
                                   ""sv,
                                   "custom\0code"sv,
                                   "code\n雪"sv,
                                   "\\\"code"sv,
                                   "_"sv};
        auto code = codes.at(std::size_t(std::get<std::uint64_t>(field(input, "code").data)));
        DynamicFieldError error(owner, code, text(field(input, "name")));
        DynamicFieldError copy(std::allocator_arg, Allocator(owner), error);
        Json::Object result(owner);
        result.emplace("debug", formatted_debug(owner, error));
        result.emplace("clone", formatted_debug(owner, copy));
        result.emplace("equal_clone", Json(owner, error == copy));
        result.emplace("display", Json(owner, error.to_string(owner)));
        return Json(owner, std::move(result));
    }
    auto name = text(field(input, "name"));
    auto left =
        row(user_context, name, DataType(std::get<std::uint64_t>(field(input, "left").data)));
    auto right =
        row(user_context, name, DataType(std::get<std::uint64_t>(field(input, "right").data)));
    CompactRow before(std::allocator_arg, Allocator(owner), left);
    Json error(owner);
    try {
        left.extend(right);
    } catch (const std::invalid_argument& failure) {
        error = Json(owner, failure.what());
    }
    Json::Object result(owner);
    result.emplace("error", std::move(error));
    result.emplace("unchanged", Json(owner, left == before));
    result.emplace("before", Json(owner, debug_string(owner, before)));
    result.emplace("after", Json(owner, debug_string(owner, left)));
    return Json(owner, std::move(result));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid row extension fixture");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
