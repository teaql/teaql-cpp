#pragma once
#include <teaql/core/debug.hpp>

template <class T>
static teaql::Json formatted_debug(teaql::Memory owner, const T& value) {
    using namespace teaql;
    Json::Object output(owner);
    for (const auto* name : {"compact",
                             "pretty",
                             "lower_hex",
                             "upper_hex",
                             "pretty_lower_hex",
                             "pretty_upper_hex",
                             "width_precision",
                             "fancy",
                             "zero_pad"}) {
        DebugOptions options;
        auto label = std::string_view(name);
        options.pretty = label.starts_with("pretty");
        if (label.find("lower_hex") != label.npos) {
            options.numbers = DebugNumbers::LowerHex;
        } else if (label.find("upper_hex") != label.npos) {
            options.numbers = DebugNumbers::UpperHex;
        }
        if (label == "width_precision") {
            options.width = 40;
            options.precision = 2;
            options.alignment = DebugAlignment::Right;
        } else if (label == "fancy") {
            options.width = 18;
            options.precision = 3;
            options.alignment = DebugAlignment::Center;
            options.fill = U'🧊';
            options.sign_plus = true;
        } else if (label == "zero_pad") {
            options.width = 12;
            options.zero_pad = true;
            options.sign_plus = true;
        }
        output.emplace(name, Json(owner, debug_string(owner, value, options)));
    }
    return Json(owner, std::move(output));
}
