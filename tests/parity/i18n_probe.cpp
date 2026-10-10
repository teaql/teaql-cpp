#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;

static Language language(const Json& input) {
    auto* code = optional(input, "language");
    return *language_from_code(code ? text(*code) : "en");
}

static ObjectLocation location(Memory r, const Json& input) {
    auto out = ObjectLocation::root(r);
    for (const auto& part : std::get<Json::Array>(field(input, "path").data)) {
        if (auto* name = std::get_if<String>(&part.data)) {
            out.member(*name);
        } else {
            out.element(std::get<std::uint64_t>(part.data));
        }
    }
    return out;
}

static CheckResult check(Memory r, const Json& input) {
    CheckResult out(std::allocator_arg,
                    Allocator(r),
                    CheckRule(std::get<std::uint64_t>(field(input, "rule").data)),
                    location(r, input));
    if (auto* v = optional(input, "input")) {
        out.with_input_value(value(r, *v));
    }
    if (auto* v = optional(input, "system")) {
        out.with_system_value(value(r, *v));
    }
    if (auto* v = optional(input, "message")) {
        out.with_message(text(*v));
    }
    return out;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    auto out = detail::model_json(r);
    auto op = text(field(input, "op"));
    if (op == "language") {
        auto parsed = language_from_code(text(field(input, "code")));
        detail::model_put(out, "code", parsed ? Json(r, language_code(*parsed)) : Json(r));
        Json::Array all(r);
        for (auto lang : all_languages) {
            all.emplace_back(language_code(lang));
        }
        detail::model_put(out, "all", all);
        return out;
    }
    if (op == "path") {
        detail::model_put(out, "text", translate_location(r, language(input), location(r, input)));
        return out;
    }
    auto catalog = I18nCatalog::builtin(r);
    std::optional<Result<I18nCatalog, RuntimeError>> custom;
    if (auto* source = optional(input, "source")) {
        custom.emplace(I18nCatalog::from_json(r, text(*source)));
        if (!*custom) {
            detail::model_put(out, "error", custom->error().to_string(r));
            return out;
        }
    }
    auto& selected = custom ? custom->value() : catalog;
    if (op == "catalog") {
        for (auto name : {"messages", "vocabulary"}) {
            Json::Array values(r);
            for (const auto& key : std::get<Json::Array>(field(input, name).data)) {
                values.emplace_back(name == std::string_view("messages")
                                        ? selected.message(r, language(input), text(key))
                                        : selected.vocabulary(r, language(input), text(key)));
            }
            detail::model_put(out, name, values);
        }
    } else if (op == "translate") {
        auto result = check(r, input);
        BuiltinTranslator translator(language(input));
        detail::model_put(out, "text", selected.translate_check_result(r, language(input), result));
        detail::model_put(out, "builtin", translator.translate_check_result(r, result));
        detail::model_put(out, "language", language_code(translator.language()));
    } else if (op == "context") {
        user.with_language(language(input));
        auto ref = user.make_ref<I18nCatalog>(selected);
        user.with_i18n_catalog(ref);
        Json error(r);
        if (auto* code = optional(input, "set")) {
            auto* alias = optional(input, "alias");
            auto result = alias && std::get<bool>(alias->data)
                              ? user.set_locale_code(text(*code))
                              : user.set_language_code(text(*code));
            if (!result) {
                error = Json(r, result.error().to_string(r));
            }
        }
        if (auto* code = optional(input, "module")) {
            auto& module = user.make<RuntimeModule>();
            if (!text(*code).empty()) {
                module.language(*language_from_code(text(*code)));
            }
            module.apply_to(user);
        }
        CheckResults checks(r);
        for (const auto& item : std::get<Json::Array>(field(input, "checks").data)) {
            checks.emplace_back(check(r, item));
        }
        user.translate_check_results(checks);
        user.translate_check_results(checks);
        Json::Array messages(r);
        for (const auto& result : checks) {
            messages.emplace_back(result.message ? Json(r, *result.message) : Json(r));
        }
        detail::model_put(out, "messages", messages);
        detail::model_put(out, "language", language_code(user.language()));
        detail::model_put(out, "error", std::move(error));
    }
    return out;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            return 2;
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
