#include <teaql/runtime/i18n_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto language = all_languages.at(std::get<std::uint64_t>(field(input, "language").data));
    BuiltinTranslator translator(language);
    auto catalog = [&] {
        if (auto* source = std::get_if<String>(&field(input, "source").data)) {
            auto parsed = I18nCatalog::from_json(owner, *source);
            if (!parsed) {
                throw std::invalid_argument("Invalid catalog-debug fixture");
            }
            return std::move(parsed).value();
        }
        return I18nCatalog::builtin(owner);
    }();
    auto clone = std::make_obj_using_allocator<I18nCatalog>(Allocator(owner), catalog);
    auto translator_clone = translator;
    Json::Object output(owner);
    output.emplace("language", formatted_debug(owner, language));
    output.emplace("default", formatted_debug(owner, Language{}));
    output.emplace("translator", formatted_debug(owner, translator));
    output.emplace("translator_clone", formatted_debug(owner, translator_clone));
    output.emplace("catalog", formatted_debug(owner, catalog));
    output.emplace("catalog_clone", formatted_debug(owner, clone));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid catalog-debug input");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
