#include <teaql/runtime/context.hpp>
#include <teaql/runtime/error.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static CheckResult make_check(Memory r, const Json& input) {
    auto location = ObjectLocation::root(r);
    for (const auto& part : std::get<Json::Array>(field(input, "path").data)) {
        if (auto* name = std::get_if<String>(&part.data))
            location.member(*name);
        else
            location.element(std::get<std::uint64_t>(part.data));
    }
    auto constructor =
        optional(input, "constructor") ? text(field(input, "constructor")) : std::string_view{};
    auto create = [&]() {
        auto rule = CheckRule(std::get<std::uint64_t>(field(input, "rule").data));
        if (constructor == "required")
            return CheckResult::required(r, std::move(location));
        if (constructor == "min")
            return CheckResult::min(r,
                                    std::move(location),
                                    value(r, field(input, "system")),
                                    value(r, field(input, "input")));
        if (constructor == "max")
            return CheckResult::max(r,
                                    std::move(location),
                                    value(r, field(input, "system")),
                                    value(r, field(input, "input")));
        if (constructor == "min_str")
            return CheckResult::min_str(r,
                                        std::move(location),
                                        std::get<std::uint64_t>(field(input, "length").data),
                                        value(r, field(input, "input")));
        if (constructor == "max_str")
            return CheckResult::max_str(r,
                                        std::move(location),
                                        std::get<std::uint64_t>(field(input, "length").data),
                                        value(r, field(input, "input")));
        return CheckResult(std::allocator_arg, Allocator(r), rule, std::move(location));
    };
    auto result = create();
    if (auto* v = optional(input, "input"))
        result.with_input_value(value(r, *v));
    if (auto* v = optional(input, "system"))
        result.with_system_value(value(r, *v));
    if (auto* v = optional(input, "message"))
        result.with_message(text(*v));
    if (auto* v = optional(input, "entity"))
        result.with_entity_type(text(*v));
    if (auto* v = optional(input, "source"))
        result.with_source_instance_path(text(*v));
    return result;
}

static RuntimeError make_error(Memory r, const Json& input) {
    auto message = optional(input, "text") ? text(field(input, "text")) : std::string_view{};
    auto other = optional(input, "other") ? text(field(input, "other")) : std::string_view{};
    switch (std::get<std::uint64_t>(field(input, "kind").data)) {
    case 0:
        return RuntimeError::missing_entity(r, message);
    case 1: {
        SqlCompileError e(std::allocator_arg,
                          Allocator(r),
                          SqlCompileError::Code(std::get<std::uint64_t>(field(input, "sql").data)),
                          message);
        e.reason = other;
        e.identifier = other;
        e.version = -9;
        e.actual_bytes = 99;
        e.max_bytes = 64;
        return RuntimeError::sql_compile(r, std::move(e));
    }
    case 2:
        return RuntimeError::behavior(r, message);
    case 3:
        return RuntimeError::event(r, message);
    case 4:
        return RuntimeError::policy(r, message);
    case 5: {
        CheckResults checks(r);
        for (const auto& item : std::get<Json::Array>(field(input, "checks").data))
            checks.emplace_back(make_check(r, item));
        return RuntimeError::check(r, std::move(checks));
    }
    case 6:
        return RuntimeError::graph(r, message);
    case 7:
        return RuntimeError::id_generation(r, message);
    case 8:
        return RuntimeError::language(r, message);
    case 9:
        return RuntimeError::unsupported_locale(r, message);
    case 10:
        return RuntimeError::schema(r, message);
    case 11:
        return RuntimeError::transaction(r, message);
    case 12:
        return RuntimeError::missing_relation(r, message, other);
    default:
        return RuntimeError::optimistic_lock_conflict(r, message, other);
    }
}

static Json evaluate(Memory r, const Json& input) {
    auto out = detail::model_json(r);
    auto op = text(field(input, "op"));
    if (op == "check") {
        auto profile = json_field_naming_from_model_value(r, text(field(input, "profile")));
        if (!profile) {
            detail::model_put(out, "error", profile.error());
            return out;
        }
        auto result = make_check(r, input);
        auto& location = result.location;
        detail::model_put(out, "root", location.is_root());
        detail::model_put(out, "level", location.level());
        detail::model_put(out, "model", location.model_path(r));
        detail::model_put(out, "native", location.native_path(r));
        detail::model_put(out, "display", location.to_string(r));
        detail::model_put(out, "defaultInstance", location.instance_path(r));
        detail::model_put(out, "instance", location.instance_path_with(r, profile.value()));
        detail::model_put(out, "message", result.to_string(r));
        detail::model_put(out, "inputPresent", bool(result.input_value));
        detail::model_put(out, "systemPresent", bool(result.system_value));
        detail::model_put(out, "wire", result.to_wire(r, profile.value()).to_json(r));
    } else if (op == "error") {
        auto error = make_error(r, input);
        detail::model_put(out, "display", error.to_string(r));
        if (error.kind() == RuntimeError::Kind::Check) {
            Json::Array checks(r);
            for (const auto& c : error.check_results())
                checks.emplace_back(c.to_wire(r, JsonFieldNamingProfile::CamelCase).to_json(r));
            detail::model_put(out, "checks", checks);
        } else
            detail::model_put(out, "checks", Json(r));
        detail::model_put(
            out, "wrapped", DataServiceError<String>::runtime(r, std::move(error)).to_string(r));
    } else if (op == "entity_error")
        detail::model_put(out,
                          "display",
                          DataServiceError<String>::entity(r,
                                                           EntityError(std::allocator_arg,
                                                                       Allocator(r),
                                                                       text(field(input, "entity")),
                                                                       text(field(input, "text"))))
                              .to_string(r));
    else if (op == "executor_error")
        detail::model_put(
            out,
            "display",
            DataServiceError<String>::executor(r, String(text(field(input, "text")), r))
                .to_string(r));
    else
        throw std::runtime_error("unknown operation");
    return out;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input)
            throw std::runtime_error("invalid input");
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
