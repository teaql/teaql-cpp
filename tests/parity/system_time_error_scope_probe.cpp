#include <teaql/core/system_time_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

template <class T>
static Json formats(Memory owner, const T& item) {
    auto output = formatted_debug(owner, item);
    auto& fields = std::get<Json::Object>(output.data);
    for (const auto& [name, precision] :
         {std::pair{"precision_zero", 0u}, {"precision_one", 1u}, {"precision_twelve", 12u}}) {
        DebugOptions options;
        options.precision = precision;
        fields.emplace(name, Json(owner, debug_string(owner, item, options)));
    }
    return output;
}

static SystemTime instant(const Json& input) {
    return SystemTime::from_unix_seconds(number<std::int64_t>(field(input, "seconds")),
                                         std::get<std::uint64_t>(field(input, "nanos").data));
}

static Json evaluate(Memory owner, const Json& input) {
    auto start = instant(field(input, "start"));
    auto end = instant(field(input, "end"));
    auto result = end.duration_since(start);
    Json::Object output(owner);
    output.emplace("start_debug", Json(owner, debug_string(owner, start)));
    output.emplace("end_debug", Json(owner, debug_string(owner, end)));
    output.emplace("ok", Json(owner, result.is_ok()));
    output.emplace("result", formats(owner, result));
    auto clone = result;
    output.emplace("clone_result", formats(owner, clone));
    auto duration = result.is_ok() ? result.value() : result.error().duration();
    if (result.is_err()) {
        const auto& error = result.error();
        output.emplace("error", formats(owner, error));
        auto clone_error = error;
        output.emplace("clone_error", formats(owner, clone_error));
        Json::Array display(owner);
        for (unsigned i = 0; i < 4; ++i) {
            display.emplace_back(error.message());
        }
        output.emplace("display", Json(owner, std::move(display)));
        output.emplace("source_none", Json(owner, error.source() == nullptr));
    }
    Json::Object duration_fields(owner);
    duration_fields.emplace("seconds", Json(owner, std::to_string(duration.seconds)));
    duration_fields.emplace("nanos", Json(owner, duration.nanos));
    duration_fields.emplace("debug", formats(owner, duration));
    output.emplace("duration", Json(owner, std::move(duration_fields)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid time-error input");
        }
        std::cout << evaluate(user->memory_resource(), *input).dump(user->memory_resource())
                  << '\n';
    }
}
