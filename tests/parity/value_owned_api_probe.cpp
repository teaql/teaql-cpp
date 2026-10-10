#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Json evaluate(Memory owner, const Json& input) {
    bool transferred = false;
    auto kind = text(field(input, "kind"));
    auto owned = [&] {
        if (kind == "text") {
            String source(text(field(input, "text")), owner);
            auto* pointer = source.data();
            Value result(owner, std::move(source));
            transferred = result.try_text()->data() == pointer;
            return result;
        }
        if (kind == "list") {
            Value::List source(owner);
            for (const auto& item : std::get<Json::Array>(field(input, "values").data)) {
                source.emplace_back(value(owner, item));
            }
            auto* pointer = source.data();
            Value result(owner, std::move(source));
            transferred = result.as_list()->data() == pointer;
            return result;
        }
        if (kind == "object") {
            Value::Object source(owner);
            for (const auto& [key, item] : std::get<Json::Object>(field(input, "values").data)) {
                source.emplace(key, value(owner, item));
            }
            auto* pointer = source.empty() ? nullptr : &source.begin()->second;
            Value result(owner, std::move(source));
            auto* retained =
                result.as_object()->empty() ? nullptr : &result.as_object()->begin()->second;
            transferred = pointer == retained;
            return result;
        }
        throw std::invalid_argument("Unknown owned Value constructor");
    }();
    auto result = detail::model_json(owner);
    detail::model_put(result, "debug", detail::debug_value(owner, owned));
    detail::model_put(result, "json", owned.to_json_value(owner));
    auto copy = Value(owner, owned);
    detail::model_put(result, "clone_equal", owned == copy);
    detail::model_put(result, "transferred", transferred);
    return result;
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::invalid_argument("Invalid owned Value input");
            }
            std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
