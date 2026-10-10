#include "../entity_registration_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static Json evaluate(Memory r, const Json& input) {
    auto& object = std::get<Json::Object>(input.data);
    Store store(r, std::get<std::uint64_t>(object.find("limit")->second.data));
    bool completed = true;
    try {
        switch (std::get<std::uint64_t>(object.find("scenario")->second.data)) {
        case 0:
            register_entities<DefaultEntity>(store);
            break;
        case 1:
            register_entities<CustomEntity>(store);
            break;
        case 2:
            register_entities<SilentEntity>(store);
            break;
        case 3:
            register_entities<DefaultEntity, CustomEntity, SilentEntity, DefaultEntity>(store);
            break;
        case 4:
            register_entities<CustomEntity, DefaultEntity, CustomEntity>(store);
            break;
        case 5:
            register_entities<SilentEntity, DefaultEntity, DefaultEntity>(store);
            break;
        default:
            throw std::invalid_argument("Unknown registration scenario");
        }
    } catch (const std::runtime_error&) {
        completed = false;
    }
    auto result = detail::model_json(r);
    Json::Array names(r);
    for (const auto& name : store.names) {
        names.emplace_back(name);
    }
    detail::model_put(result, "names", names);
    detail::model_put(result, "attempted", store.attempted);
    detail::model_put(result, "completed", completed);
    return result;
}

int main() {
    try {
        std::string line;
        GlobalContext global;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::invalid_argument("Invalid registration input");
            }
            std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
