#include "../../examples/generated/include/cpp_examples.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/debug.hpp>
#include <iostream>

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Record fields(r);
    fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    fields.emplace(String("name", r), Value(r, "original"));
    fields.emplace(String("version", r), Value(r, std::int64_t(3)));
    for (const auto& [name, item] : std::get<Json::Object>(field(input, "fields").data)) {
        fields.insert_or_assign(String(name, r), value(r, item));
    }
    auto row = CompactRow(std::allocator_arg, Allocator(r), std::move(fields));
    auto decoded = cpp_examples::Product::from_compact_row(r, std::move(row));
    if (!decoded) {
        throw std::runtime_error(std::string(decoded.error().to_string(r)));
    }
    auto& entity = user_context.make<cpp_examples::Product>(std::move(decoded).value());
    auto& alias = user_context.make<cpp_examples::Product>(entity);
    Json::Array observations(r);
    for (const auto& item : std::get<Json::Array>(field(input, "keys").data)) {
        auto key = text(item);
        Json::Object entry(r);
        entry.emplace("key", Json(r, key));
        entry.emplace("present", Json(r, entity.hasDynamicProperty(key)));
        auto* current = entity.dynamicProperty(key);
        entry.emplace("value", current ? Json(r, detail::debug_value(r, *current)) : Json(r));
        entry.emplace("clone_present", Json(r, alias.hasDynamicProperty(key)));
        auto* cloned = alias.dynamicProperty(key);
        entry.emplace("clone_value", cloned ? Json(r, detail::debug_value(r, *cloned)) : Json(r));
        observations.emplace_back(Json(r, std::move(entry)));
    }
    Json::Object result(r);
    result.emplace("supported", Json(r, cpp_examples::Product::supports_dynamic_property_load()));
    result.emplace("observations", Json(r, std::move(observations)));
    return Json(r, std::move(result));
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
        auto output = observe(*user_context, *input);
        std::cout << output.dump(r) << '\n';
    }
}
