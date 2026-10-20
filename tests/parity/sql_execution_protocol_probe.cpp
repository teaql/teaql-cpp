#define main sql_execution_historical_main
#include "sql_execution_probe.cpp"
#undef main

static std::optional<RequestIntentError> validate_protocol(Memory r, const Json& input) {
    auto comment = optional(input, "comment");
    auto intent =
        MutationIntent::from_optional(r,
                                      comment && std::holds_alternative<String>(comment->data)
                                          ? std::optional<std::string_view>(text(*comment))
                                          : std::nullopt);
    if (!intent) {
        return std::move(intent).error();
    }
    if (text(field(input, "mode")) == "batch") {
        for (const auto& child : items(input, "children")) {
            if (auto error = validate_protocol(r, child)) {
                return error;
            }
        }
    }
    return {};
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid SQL execution protocol fixture");
        }
        auto error = validate_protocol(r, field(*input, "request"));
        if (error) {
            auto output = detail::model_json(r);
            detail::model_put(output, "intent_error", error->to_string(r));
            detail::model_put(output, "calls_before_poll", 0);
            detail::model_put(output, "calls", Json(r, Json::Array(r)));
            detail::model_put(output, "events", Json(r, Json::Array(r)));
            if (auto kind = optional(*input, "observe_kind"); kind && std::get<bool>(kind->data)) {
                detail::model_put(output, "kind_calls", 0);
            }
            std::cout << output.dump(r) << '\n';
        } else {
            std::cout << evaluate(*user_context, *input).dump(r) << '\n';
        }
    }
    std::pmr::set_default_resource(previous);
}
