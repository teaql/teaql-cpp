#include <teaql/runtime/telemetry_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static RuntimeAttributeValue attribute(Memory owner, const Json& input) {
    auto kind = text(field(input, "kind"));
    auto make = [&](auto value) {
        return std::make_obj_using_allocator<RuntimeAttributeValue>(Allocator(owner), value);
    };
    if (kind == "string") {
        return make(text(field(input, "value")));
    }
    if (kind == "i64") {
        return make(number<std::int64_t>(field(input, "value")));
    }
    if (kind == "usize") {
        auto value = number<std::uint64_t>(field(input, "value"));
        if (value > std::numeric_limits<std::size_t>::max()) {
            throw std::invalid_argument("Fixture exceeds native usize");
        }
        return make(static_cast<std::size_t>(value));
    }
    if (kind == "bool") {
        return make(std::get<bool>(field(input, "value").data));
    }
    auto encoded = text(field(input, "value"));
    std::uint64_t bits = 0;
    auto [end, error] = std::from_chars(encoded.data(), encoded.data() + encoded.size(), bits, 16);
    if (error != std::errc{} || end != encoded.data() + encoded.size()) {
        throw std::invalid_argument("Invalid float fixture");
    }
    return make(std::bit_cast<double>(bits));
}

static Json evaluate(Memory owner, const Json& input) {
    auto value = attribute(owner, input);
    auto clone = std::make_obj_using_allocator<RuntimeAttributeValue>(Allocator(owner), value);
    RuntimeOperation operation(std::allocator_arg,
                               Allocator(owner),
                               text(field(input, "family")),
                               text(field(input, "name")));
    operation.attribute(text(field(input, "key")), value);
    auto copied = std::make_obj_using_allocator<RuntimeOperation>(Allocator(owner), operation);
    Json::Object output(owner);
    output.emplace("value", formatted_debug(owner, value));
    output.emplace("clone", formatted_debug(owner, clone));
    output.emplace("value_equal", Json(owner, value == clone));
    output.emplace("operation", formatted_debug(owner, operation));
    output.emplace("operation_clone", formatted_debug(owner, copied));
    output.emplace("operation_equal", Json(owner, operation == copied));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid telemetry-model input");
        }
        std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                  << '\n';
    }
}
