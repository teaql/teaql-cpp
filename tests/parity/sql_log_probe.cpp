#include <teaql/data_service/sql_log.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <charconv>
#include <iostream>
using namespace teaql;
#include "fixture_value.hpp"

static SqlIntentRedactions binding(Memory r, const Json& input) {
    Vector<Value> params(r);
    for (const auto& item : std::get<Json::Array>(field(input, "params").data))
        params.emplace_back(value(r, item));
    SqlLogContext source(r);
    source.generated_sql = std::get<bool>(field(input, "generated").data);
    if (auto* ancestor = optional(input, "ancestor"))
        source.intent_redactions = binding(r, *ancestor);
    for (const auto& policy : std::get<Json::Array>(field(input, "policies").data))
        source.parameter_policies.emplace_back(
            SqlParameterLogPolicy(std::get<std::uint64_t>(policy.data)));
    return SqlIntentRedactions::from_bindings(r, source, params, text(field(input, "sql")));
}

static Json evaluate(Memory r, const Json& input) {
    auto collected = binding(r, input);
    if (auto* extra = optional(input, "extend"))
        for (const auto& item : std::get<Json::Array>(extra->data))
            collected.extend(binding(r, item));
    if (auto* ids = optional(input, "ids"))
        for (const auto& item : std::get<Json::Array>(ids->data))
            collected.capture_target_id(value(r, item));
    Vector<String> safe(r), debug(r);
    collected.extend_secrets(false, safe);
    collected.extend_secrets(true, debug);
    Json::Array safe_json(r), debug_json(r);
    for (const auto& s : safe)
        safe_json.emplace_back(s);
    for (const auto& s : debug)
        debug_json.emplace_back(s);
    auto result = detail::model_json(r);
    detail::model_put(result, "safe", safe_json);
    detail::model_put(result, "debug", debug_json);
    detail::model_put(result, "empty", collected.is_empty());
    detail::model_put(result, "description", collected.debug(r));
    Json::Array names(r);
    for (const auto& name : std::get<Json::Array>(field(input, "names").data))
        names.emplace_back(is_credential_log_name(text(name)));
    detail::model_put(result, "names", names);
    return result;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input)
            throw std::runtime_error("invalid fixture JSON");
        std::cout << evaluate(r, *input).dump(r) << '\n';
    }
}
