#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static Json evaluate(UserContext& context, const Json& input) {
    auto r = context.memory_resource();
    Json::Array outputs(r);
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto op = text(field(action, "op"));
        auto out = detail::model_json(r);
        auto name = optional(action, "name") ? text(field(action, "name")) : std::string_view{};
        auto type = optional(action, "type") ? text(field(action, "type")) : std::string_view{};
        if (op == "typed_insert" || op == "named_insert") {
            if (type == "text") {
                auto& object = context.make<String>(text(field(action, "value")));
                if (op == "typed_insert")
                    context.insert_resource(context, object);
                else
                    context.insert_named_resource(name, context, object);
            } else {
                auto& object = context.make<std::int64_t>(
                    (std::holds_alternative<std::int64_t>(field(action, "value").data)
                         ? std::get<std::int64_t>(field(action, "value").data)
                         : std::int64_t(std::get<std::uint64_t>(field(action, "value").data))));
                if (op == "typed_insert")
                    context.insert_resource(context, object);
                else
                    context.insert_named_resource(name, context, object);
            }
            detail::model_put(out, "ok", true);
        } else if (op == "typed_get" || op == "named_get") {
            if (type == "text") {
                auto* found = op == "typed_get" ? context.get_resource<String>()
                                                : context.get_named_resource<String>(name);
                if (found)
                    detail::model_put(out, "value", *found);
                else
                    detail::model_put(out, "value", Json(r));
            } else {
                auto* found = op == "typed_get" ? context.get_resource<std::int64_t>()
                                                : context.get_named_resource<std::int64_t>(name);
                if (found)
                    detail::model_put(out, "value", *found);
                else
                    detail::model_put(out, "value", Json(r));
            }
        } else if (op == "named_require") {
            if (type == "text") {
                auto result = context.require_named_resource<String>(name);
                if (result)
                    detail::model_put(out, "value", result.value().get());
                else
                    detail::model_put(out, "error", result.error().to_string(r));
            } else {
                auto result = context.require_named_resource<std::int64_t>(name);
                if (result)
                    detail::model_put(out, "value", result.value().get());
                else
                    detail::model_put(out, "error", result.error().to_string(r));
            }
        } else if (op == "put_local") {
            context.put_local(name, value(r, field(action, "value")));
            detail::model_put(out, "ok", true);
        } else if (op == "local") {
            auto* found = context.local(name);
            detail::model_put(out, "present", found != nullptr);
            detail::model_put(out, "value", found ? found->to_json_value(r) : Json(r));
        } else if (op == "remove_local") {
            auto found = context.remove_local(name);
            detail::model_put(out, "present", bool(found));
            detail::model_put(out, "value", found ? found->to_json_value(r) : Json(r));
        } else if (op == "error") {
            auto kind = type == "typed"   ? ContextError::Kind::MissingTypedResource
                        : type == "named" ? ContextError::Kind::MissingResource
                                          : ContextError::Kind::MissingEntityDataService;
            ContextError error(std::allocator_arg, Allocator(r), kind, name);
            detail::model_put(out, "error", error.to_string(r));
        } else
            throw std::runtime_error("unknown fixture operation");
        outputs.emplace_back(std::move(out));
    }
    return Json(r, outputs);
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
        std::cout << evaluate(*user, *input).dump(r) << '\n';
    }
}
