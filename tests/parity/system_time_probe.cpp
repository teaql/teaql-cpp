#include <teaql/data_service/model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static SystemTime time_value(const Json& input) {
    return SystemTime::from_unix_seconds(number<std::int64_t>(field(input, "seconds")),
                                         std::get<std::uint64_t>(field(input, "nanos").data));
}

static Json model(Memory owner, SystemTime time) {
    Json::Object output(owner);
    output.emplace("debug", Json(owner, debug_string(owner, time)));
    Json chrono(owner);
    try {
        auto count = time.time_since_epoch().count();
        char digits[24];
        auto end = std::to_chars(digits, digits + sizeof(digits), count).ptr;
        chrono = Json(owner, std::string_view(digits, end - digits));
    } catch (const std::overflow_error&) {
    }
    output.emplace("chrono_nanos", std::move(chrono));
    return Json(owner, std::move(output));
}

static Json evaluate(Memory owner, const Json& input) {
    auto start = time_value(field(input, "start"));
    auto end = time_value(field(input, "end"));
    auto delta = number<std::uint64_t>(field(input, "delta"));
    auto metadata = ExecutionMetadata::unrecorded_query(owner, 1);
    metadata.started_at = start;
    metadata.ended_at = end;
    Json::Object output(owner);
    output.emplace("valid", Json(owner, true));
    output.emplace("start", model(owner, start));
    output.emplace("end", model(owner, end));
    output.emplace("metadata", formatted_debug(owner, metadata));
    Json elapsed(owner);
    if (auto duration = end.duration_since(start)) {
        Json::Object fields(owner);
        char digits[24];
        auto last = std::to_chars(digits, digits + sizeof(digits), duration->seconds).ptr;
        fields.emplace("seconds", Json(owner, std::string_view(digits, last - digits)));
        fields.emplace("nanos", Json(owner, duration->nanos));
        elapsed = Json(owner, std::move(fields));
    }
    output.emplace("elapsed", std::move(elapsed));
    auto add = start.checked_add_seconds(delta);
    auto sub = start.checked_sub_seconds(delta);
    output.emplace("add", add ? model(owner, *add) : Json(owner));
    output.emplace("sub", sub ? model(owner, *sub) : Json(owner));
    output.emplace("order", Json(owner, start < end ? -1 : start > end ? 1 : 0));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid system-time fixture");
        }
        std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                  << '\n';
    }
}
