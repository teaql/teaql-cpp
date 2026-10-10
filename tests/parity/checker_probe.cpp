#include "../support/checker_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/debug.hpp>
#include <iostream>
using namespace teaql;
using namespace checker_fixture;

static EntityValues values(Memory r, const Json& input) {
    EntityValues out(std::allocator_arg, Allocator(r));
    for (const auto& [name, v] : std::get<Json::Object>(input.data)) {
        out.fields.emplace(String(name, r), value(r, v));
    }
    return out;
}

static Json describe(Memory r, const EntityValues& values) {
    auto out = detail::model_json(r);
    for (const auto& [name, value] : values.fields) {
        detail::model_put(out, name, detail::debug_value(r, value));
    }
    return out;
}

static ObjectLocation location(Memory r, const Json& input) {
    auto out = ObjectLocation::root(r);
    if (auto* path = optional(input, "path")) {
        for (const auto& part : std::get<Json::Array>(path->data)) {
            if (auto* text = std::get_if<String>(&part.data)) {
                out.member(*text);
            } else {
                out.element(std::get<std::uint64_t>(part.data));
            }
        }
    }
    return out;
}

static Json checks_json(Memory r, const CheckResults& checks) {
    Json::Array out(r);
    for (const auto& check : checks) {
        out.emplace_back(check.to_wire(r, JsonFieldNamingProfile::CamelCase).to_json(r));
    }
    return Json(r, out);
}

static Json evaluate(GlobalContext& global, UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    auto out = detail::model_json(r);
    auto data = values(r, field(input, "values"));
    auto op = text(field(input, "op"));
    if (op == "status") {
        detail::model_put(out, "before", check_object_status_name(check_object_status(data)));
        if (auto* status = optional(input, "mark")) {
            mark_entity_status(data, CheckObjectStatus(std::get<std::uint64_t>(status->data)));
        }
        detail::model_put(out, "marked", describe(r, data));
        detail::model_put(out, "after", check_object_status_name(check_object_status(data)));
        clear_entity_status(data);
        detail::model_put(out, "cleared", describe(r, data));
        return out;
    }
    auto action =
        optional(input, "action") ? text(field(input, "action")) : std::string_view("none");
    auto& plain = global.make<Plain>(action);
    if (op == "helpers") {
        CheckResults checks(r);
        auto path = location(r, input);
        auto name = text(field(input, "field"));
        plain.required(data, name, path, checks);
        plain.min_string_length(
            data, name, std::get<std::uint64_t>(field(input, "min").data), path, checks);
        plain.max_string_length(
            data, name, std::get<std::uint64_t>(field(input, "max").data), path, checks);
        detail::model_put(out, "checks", checks_json(r, checks));
        return out;
    }
    if (auto* code = optional(input, "language")) {
        (void)user.set_language_code(text(*code));
    }
    auto& fix = global.make<Fix>(action);
    auto& typed = global.make<TypedEntityChecker<Item, Fix>>(global, fix);
    bool is_typed = optional(input, "typed") && std::get<bool>(field(input, "typed").data);
    if (op == "typed") {
        CheckResults checks(r);
        typed.check_and_fix(user, data, location(r, input), checks);
        detail::model_put(out, "checks", checks_json(r, checks));
    } else {
        auto& registry = global.make<InMemoryCheckerRegistry>();
        auto& module = global.make<RuntimeModule>();
        if (auto* metadata = optional(input, "metadata");
            metadata && std::get<bool>(metadata->data)) {
            module.descriptor(Item::entity_descriptor(global.memory_resource()));
        }
        auto path = location(r, input);
        if (op == "module") {
            if (is_typed) {
                module.checker(global, typed);
            } else {
                module.checker(global, plain);
            }
            if (auto* replace = optional(input, "replace")) {
                auto& replacement = global.make<Plain>(text(*replace));
                module.checker(global, replacement);
            }
            auto& copied = global.make<RuntimeModule>(module);
            copied.apply_to(user);
        } else {
            if (optional(input, "registered") && std::get<bool>(field(input, "registered").data)) {
                if (is_typed) {
                    registry.register_checker(global, typed);
                } else {
                    registry.with_checker(global, plain);
                }
            }
            user.with_checker_registry(global, registry);
            user.set_metadata(global, module.metadata);
        }
        auto entity = optional(input, "entity") ? text(field(input, "entity")) : "Item";
        detail::model_put(out, "has", user.has_checker(entity));
        auto result = user.check_and_fix_values_at(entity, data, path);
        detail::model_put(out, "error", result ? Json(r) : Json(r, result.error().to_string(r)));
        detail::model_put(out,
                          "checks",
                          result ? Json(r, Json::Array(r))
                                 : checks_json(r, result.error().check_results()));
    }
    detail::model_put(out, "values", describe(r, data));
    Json::Array seen(fix.calls, r);
    detail::model_put(out, "seen", seen);
    detail::model_put(out, "plain_calls", plain.calls);
    return out;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            return 2;
        }
        std::cout << evaluate(global, *user, *input).dump(user->memory_resource()) << '\n';
    }
}
