#include <teaql/core/entity.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Value input_value(Memory r, const Json& input) {
    if (text(field(input, "kind")) == "typed_null_type") {
        return Value(r, TypedNull{DataType(std::get<std::uint64_t>(field(input, "value").data))});
    }
    return value(r, input);
}

static Json record(Memory r, const Record& values) {
    auto output = detail::model_json(r);
    for (const auto& [key, item] : values) {
        detail::model_put(output, key, detail::debug_value(r, item));
    }
    return output;
}

static Json evaluate(Memory r, const Json& input) {
    Record fields(r);
    for (const auto& [key, fixture] : std::get<Json::Object>(field(input, "values").data)) {
        fields.emplace(key, input_value(r, fixture));
    }
    auto parsed = BaseEntityData::from_values_map(r, fields);
    auto output = detail::model_json(r);
    if (parsed) {
        auto& base = parsed.value();
        detail::model_put(output, "id", std::to_string(base.id));
        detail::model_put(output, "version", std::to_string(base.version));
        detail::model_put(output, "dynamic", record(r, base.dynamic_values));
        detail::model_put(output, "values", record(r, base.to_values_map(r)));
        auto copy = BaseEntityData(std::allocator_arg, Allocator(r), base);
        detail::model_put(output, "clone_equal", base == copy);
    } else {
        detail::model_put(output, "entity", parsed.error().entity);
        detail::model_put(output, "message", parsed.error().message);
        detail::model_put(output, "display", parsed.error().to_string(r));
    }
    return output;
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::invalid_argument("Invalid BaseEntityData input");
            }
            std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
