#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <chrono>
#include <iostream>

static Json evaluate(Memory owner, const Json& input) {
    Json::Object output(owner);
    auto kind = text(field(input, "kind"));
    if (kind == "wire") {
        auto parsed = Json::parse(owner, text(field(input, "wire")));
        auto decoded = parsed ? Timestamp::from_json_value(*parsed) : std::nullopt;
        output.emplace("accepted", Json(owner, bool(decoded)));
        if (decoded) {
            output.emplace("millis", Json(owner, std::to_string(decoded->as_millis())));
            output.emplace("serialized", Json(owner, decoded->to_json_value(owner).dump(owner)));
        }
        return Json(owner, std::move(output));
    }
    auto timestamp = kind == "unsigned"
                         ? Timestamp::from_u64(number<std::uint64_t>(field(input, "value")))
                         : Timestamp(number<std::int64_t>(field(input, "value")));
    if (kind == "field") {
        auto& millis = timestamp.millis;
        millis = number<std::int64_t>(field(input, "replacement"));
    }
    auto copied = timestamp;
    auto cloned = Timestamp(timestamp);
    auto datetime = timestamp.to_datetime();
    auto millis_now = [] {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    };
    auto before = millis_now();
    auto now = Timestamp::now();
    auto after = millis_now();
    Json::Array components(owner);
    for (auto component : {datetime.date.year(),
                           static_cast<std::int32_t>(datetime.date.month()),
                           static_cast<std::int32_t>(datetime.date.day()),
                           static_cast<std::int32_t>(datetime.hour),
                           static_cast<std::int32_t>(datetime.minute),
                           static_cast<std::int32_t>(datetime.second),
                           static_cast<std::int32_t>(datetime.millisecond)}) {
        components.emplace_back(component);
    }
    output.emplace("millis", Json(owner, std::to_string(timestamp.as_millis())));
    output.emplace("default", Json(owner, Timestamp{}.as_millis()));
    output.emplace("copy_equal", Json(owner, copied == timestamp));
    output.emplace("clone_equal", Json(owner, cloned == timestamp));
    output.emplace("datetime", Json(owner, std::move(components)));
    output.emplace("serialized", Json(owner, timestamp.to_json_value(owner).dump(owner)));
    output.emplace("debug", formatted_debug(owner, timestamp));
    output.emplace("now_in_bounds",
                   Json(owner, before <= now.as_millis() && now.as_millis() <= after));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid Timestamp fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
