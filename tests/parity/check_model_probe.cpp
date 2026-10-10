#include <teaql/runtime.hpp>
#include <teaql/runtime/checker_model_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <iostream>

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto output = detail::model_json(owner);
    auto put = [&](std::string_view name, auto&& value) {
        detail::model_put(output, name, std::forward<decltype(value)>(value));
    };
    auto parsed = json_field_naming_from_model_value(owner, text(field(input, "profile")));
    if (!parsed) {
        put("error", parsed.error());
        return output;
    }
    auto profile = parsed.value();
    auto location = ObjectLocation::root(owner);
    for (const auto& part : detail::model_array(field(input, "path"), "path")) {
        if (std::holds_alternative<String>(part.data)) {
            location.member(text(part));
        } else {
            location.element(detail::model_integer<std::size_t>(part, "path"));
        }
    }
    auto rule = CheckRule(detail::model_integer<unsigned>(field(input, "rule"), "rule"));
    CheckResult result(std::allocator_arg, Allocator(owner), rule, location);
    if (const auto* item = optional(input, "input")) {
        result.with_input_value(value(owner, *item));
    }
    if (const auto* item = optional(input, "system")) {
        result.with_system_value(value(owner, *item));
    }
    if (const auto* item = optional(input, "message")) {
        result.with_message(text(*item));
    }
    if (const auto* item = optional(input, "entity")) {
        result.with_entity_type(text(*item));
    }
    if (const auto* item = optional(input, "source")) {
        result.with_source_instance_path(text(*item));
    }
    auto clone = detail::clone_in(owner, result);
    auto changed = detail::clone_in(owner, result);
    changed.message.emplace("changed", owner);
    auto wire = result.to_wire(owner, profile);
    auto wire_clone = detail::clone_in(owner, wire);
    auto wire_changed = detail::clone_in(owner, wire);
    wire_changed.instance_path += '!';
    put("rule", formatted_debug(owner, rule));
    put("profile", formatted_debug(owner, profile));
    put("default_profile", formatted_debug(owner, JsonFieldNamingProfile{}));
    put("status_create", formatted_debug(owner, CheckObjectStatus::Create));
    put("status_update", formatted_debug(owner, CheckObjectStatus::Update));
    put("status_unknown", formatted_debug(owner, CheckObjectStatus::Unknown));
    put("location", formatted_debug(owner, location));
    put("location_clone", formatted_debug(owner, detail::clone_in(owner, location)));
    put("model", location.model_path(owner));
    put("native", location.native_path(owner));
    put("instance", location.instance_path_with(owner, profile));
    put("display", location.to_string(owner));
    put("rule_id", wire_id(rule));
    put("check", formatted_debug(owner, result));
    put("check_clone", formatted_debug(owner, clone));
    put("check_clone_equal", result == clone);
    put("check_changed_equal", result == changed);
    put("check_display", result.to_string(owner));
    put("wire", formatted_debug(owner, wire));
    put("wire_clone", formatted_debug(owner, wire_clone));
    put("wire_clone_equal", wire == wire_clone);
    put("wire_changed_equal", wire == wire_changed);
    put("json", wire.to_json(owner));
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid checker model fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
