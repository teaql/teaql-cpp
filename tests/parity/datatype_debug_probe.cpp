#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include <iostream>

using namespace teaql;

static Json evaluate(Memory owner, const Json& input) {
    const auto& fields = std::get<Json::Object>(input.data);
    auto kind = DataType(std::get<std::uint64_t>(fields.find("type")->second.data));
    Json::Object result(owner);
    result.emplace("type", formatted_debug(owner, kind));
    result.emplace("typed_null", formatted_debug(owner, Value(owner, TypedNull{kind})));
    Json::Array equal(owner);
    for (unsigned other = 0; other < 10; ++other) {
        equal.emplace_back(kind == DataType(other));
    }
    result.emplace("equal", Json(owner, std::move(equal)));
    return Json(owner, std::move(result));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::invalid_argument("Invalid DataType input");
            }
            std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
