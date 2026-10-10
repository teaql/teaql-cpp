#include <teaql/core/chrono_value.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Json evaluate(Memory owner, const Json& input) {
    using namespace std::chrono;
    auto ticks = number<std::int64_t>(field(input, "ticks"));
    auto unit = text(field(input, "unit"));
    auto converted = [&] {
        if (unit == "seconds") {
            return value_from_utc(owner, sys_time<seconds>(seconds(ticks)));
        }
        if (unit == "milliseconds") {
            return value_from_utc(owner, sys_time<milliseconds>(milliseconds(ticks)));
        }
        if (unit == "microseconds") {
            return value_from_utc(owner, sys_time<microseconds>(microseconds(ticks)));
        }
        if (unit == "nanoseconds") {
            return value_from_utc(owner, sys_time<nanoseconds>(nanoseconds(ticks)));
        }
        throw std::invalid_argument("Unknown UTC duration unit");
    }();
    auto result = detail::model_json(owner);
    detail::model_put(result, "debug", detail::debug_value(owner, converted));
    detail::model_put(result, "millis", std::to_string(converted.try_timestamp()->as_millis()));
    detail::model_put(result, "json", converted.to_json_value(owner));
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
                throw std::invalid_argument("Invalid UTC Value input");
            }
            std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
