#include <teaql/core/entity_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json observe(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto& base = user.make<BaseEntityData>();
    base.with_id(number<std::uint64_t>(field(input, "id")));
    base.with_version(number<std::int64_t>(field(input, "version")));
    for (const auto& [key, item] : std::get<Json::Object>(field(input, "dynamic").data)) {
        auto v =
            text(field(item, "kind")) == "typed_null_type"
                ? Value(owner,
                        TypedNull{DataType(std::get<std::uint64_t>(field(item, "value").data))})
                : value(owner, item);
        base.put_dynamic(key, std::move(v));
    }
    auto& error =
        user.make<EntityError>(text(field(input, "entity")), text(field(input, "message")));
    Json::Object result(owner);
    result.emplace("base", formatted_debug(owner, base));
    result.emplace("error", formatted_debug(owner, error));
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
                throw std::invalid_argument("Invalid entity Debug input");
            }
            std::cout << observe(*user, *input).dump(user->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
