#include <teaql/data_service/model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Json model(Memory owner, SystemTime time) {
    Json::Object result(owner);
    result.emplace("debug", Json(owner, debug_string(owner, time)));
    Json ticks(owner);
    try {
        ticks = Json(owner, std::to_string(time.time_since_epoch().count()));
    } catch (const std::overflow_error&) {
    }
    result.emplace("chrono_nanos", std::move(ticks));
    return Json(owner, std::move(result));
}

static Json evaluate(Memory owner, const Json& input) {
    const auto& encoded = field(input, "time");
    auto time =
        SystemTime::from_unix_seconds(number<std::int64_t>(field(encoded, "seconds")),
                                      std::get<std::uint64_t>(field(encoded, "nanos").data));
    const auto& elapsed = field(input, "duration");
    SystemDuration duration{
        number<std::uint64_t>(field(elapsed, "seconds")),
        static_cast<std::uint32_t>(std::get<std::uint64_t>(field(elapsed, "nanos").data))};
    Json::Object result(owner);
    result.emplace("input", model(owner, time));
    auto add = time.checked_add(duration);
    auto sub = time.checked_sub(duration);
    result.emplace("add", add ? model(owner, *add) : Json(owner));
    result.emplace("sub", sub ? model(owner, *sub) : Json(owner));
    auto copy = time;
    Json assigned_add(owner), assigned_sub(owner);
    try {
        copy += duration;
        assigned_add = model(owner, copy);
    } catch (const std::overflow_error&) {
        if (copy != time) {
            throw std::logic_error("Failed time addition changes its operand");
        }
    }
    copy = time;
    try {
        copy -= duration;
        assigned_sub = model(owner, copy);
    } catch (const std::overflow_error&) {
        if (copy != time) {
            throw std::logic_error("Failed time subtraction changes its operand");
        }
    }
    result.emplace("add_assign", std::move(assigned_add));
    result.emplace("sub_assign", std::move(assigned_sub));
    result.emplace("after", model(owner, time));
    return Json(owner, std::move(result));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid time arithmetic input");
        }
        std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                  << '\n';
    }
}
