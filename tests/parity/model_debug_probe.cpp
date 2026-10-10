#include <teaql/core/debug.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

template <class T>
static Json formatted(Memory owner, const T& value) {
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

static Json observe(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    Vector<String> names(owner);
    Vector<Value> row_values(owner), values(owner);
    const auto& row_input = field(input, "row");
    for (const auto& name : std::get<Json::Array>(field(row_input, "names").data)) {
        names.emplace_back(text(name));
    }
    for (const auto& item : std::get<Json::Array>(field(row_input, "values").data)) {
        row_values.emplace_back(value(owner, item));
    }
    auto& layout = user.make<ColumnLayout>(std::move(names));
    auto& row = user.make<CompactRow>(layout, std::move(row_values));
    for (const auto& item : std::get<Json::Array>(field(input, "values").data)) {
        values.emplace_back(value(owner, item));
    }
    auto& list = user.make<SmartList<Value>>(std::move(values));
    list.is_loaded = std::get<bool>(field(input, "loaded").data);
    if (std::holds_alternative<String>(field(input, "total").data)) {
        list.total_count = number<std::uint64_t>(field(input, "total"));
    }
    for (const auto& [name, item] : std::get<Json::Object>(field(input, "summary").data)) {
        list.with_summary(name, value(owner, item));
    }
    for (const auto& [name, item] : std::get<Json::Object>(field(input, "aggregations").data)) {
        list.with_aggregation(name, value(owner, item));
    }
    auto& facet = user.make<SmartList<CompactRow>>();
    facet.emplace_back(row);
    facet.with_total_count(31);
    list.with_facet("rows", facet);
    Json::Object result(owner);
    result.emplace("row", formatted(owner, row));
    result.emplace("list", formatted(owner, list));
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
                throw std::runtime_error("Invalid model debug fixture");
            }
            std::cout << observe(*user, *input).dump(user->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
