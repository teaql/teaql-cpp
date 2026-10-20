#include "../generated_checkers/include/cpp_checker_fixture.hpp"
#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace teaql;
using namespace cpp_checker_fixture;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class CatalogCheckLogic final : public NoteCheckerLogic {
public:
    using NoteCheckerLogic::NoteCheckerLogic;

    void checkAndFixNote(const UserContext&,
                         Note& entity,
                         CheckObjectStatus,
                         const ObjectLocation& location,
                         CheckResults& results) const override {
        requiredText(entity.name(), "name", location, results);
    }
};

int main() {
    try {
        GlobalContext global;
        Ref<I18nCatalog> retained;
        {
            auto user_context_owner = global.make_user_context();
            auto& user_context = *user_context_owner;
            auto r = user_context.memory_resource();
            auto& input = user_context.make_resource<std::ifstream>(
                TEAQL_SOURCE_DIR "/tests/generated_checkers/teaql-i18n.json");
            check(input.is_open(), "Application opens the generated catalog explicitly");
            String source(r);
            source.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
            auto parsed = I18nCatalog::from_json(user_context.memory_resource(), source);
            check(bool(parsed), "The generated vocabulary is a valid runtime catalog");
            auto catalog = user_context.make_ref<I18nCatalog>(std::move(parsed).value());
            user_context.set_i18n_catalog(catalog);
            check(bool(user_context.set_locale_code("fr")), "Select the request locale explicitly");
            check(user_context.get(catalog).vocabulary(r, Language::French, "entity.note") ==
                      "Note",
                  "An untranslated model word falls back to the generated English vocabulary");
            auto unsupported = user_context.set_locale_code("unsupported-language");
            check(!unsupported &&
                      unsupported.error().kind() == RuntimeError::Kind::UnsupportedLocale &&
                      user_context.language() == Language::French,
                  "Typed locale failure preserves the previous locale");
            auto invalid = I18nCatalog::from_json(r, "{}");
            check(!invalid && user_context.language() == Language::French,
                  "A failed parse does not install a catalog or change locale");

            auto& logic = user_context.make<CatalogCheckLogic>();
            auto& checker = user_context.make<NoteChecker>(logic);
            auto& entity = user_context.make<Note>();
            entity.updateId(7).updateVersion(0).updateName("");
            user_context.with_active_root("Tenant", 42);
            auto& fixes = user_context.make<detail::GraphFixState>(Timestamp(1234));
            CheckResults findings(r);
            {
                detail::GraphFixActivation active(user_context, fixes);
                checker.check_and_fix_typed(user_context,
                                            entity,
                                            CheckObjectStatus::Create,
                                            ObjectLocation::root(r),
                                            findings);
            }
            check(!findings.empty(), "The generated checker runs the application logic");
            user_context.translate_check_results(findings);
            for (const auto& finding : findings) {
                check(finding.message.has_value(), "The selected catalog renders Checker messages");
            }

            constexpr std::string_view customized =
                R"({"schema":"teaql.i18n/v1","defaultLocale":"en","locales":{"en":{"messages":{},"vocabulary":{}},"fr":{"messages":{"checker.required":"Application requires {location}"},"vocabulary":{}}}})";
            auto custom = I18nCatalog::from_json(r, customized);
            check(bool(custom), "Application message customization parses");
            auto custom_catalog = user_context.make_ref<I18nCatalog>(std::move(custom).value());
            user_context.set_i18n_catalog(custom_catalog);
            bool required = false;
            for (auto& finding : findings) {
                auto rule = finding.rule;
                finding.message.reset();
                CheckResults one(r);
                one.emplace_back(finding);
                user_context.translate_check_results(one);
                check(one[0].rule == rule, "Translation preserves the typed Checker rule");
                if (rule == CheckRule::Required) {
                    required = true;
                    check(one[0].message->starts_with("Application requires "),
                          "The application-selected locale overrides the built-in message");
                }
            }
            check(required, "The application checker produced a required-field finding");
            retained = user_context.make_global<I18nCatalog>(user_context.get(catalog));
        }
        auto next_user_context_owner = global.make_user_context();
        auto& next_user_context = *next_user_context_owner;
        auto rebound = next_user_context.rebind_global(retained);
        next_user_context.set_i18n_catalog(rebound);
        check(next_user_context.get(rebound).vocabulary(
                  next_user_context.memory_resource(), Language::French, "entity.note") == "Note",
              "The explicit Global copy survives actual source UserContext destruction");
        std::cout << "Generated catalog, locale and Checker integration passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
