#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/runtime/i18n_debug.hpp>
#include <iostream>
#include <thread>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class Error, class F>
static void rejected(F&& action, const char* message) {
    bool failed = false;
    try {
        action();
    } catch (const Error&) {
        failed = true;
    }
    check(failed, message);
}

static constexpr std::string_view source =
    R"({"schema":"teaql.i18n/v1","defaultLocale":"en","locales":{"en":{"messages":{"checker.required":"Custom {location}","long":"Long translated text to force owned character allocation"},"vocabulary":{"long":"Long vocabulary to force owned character allocation"}},"fr":{"messages":{"checker.required":"French {location}"},"vocabulary":{}}}})";

static CheckResults results(Memory r) {
    CheckResults out(r);
    out.emplace_back(CheckResult::required(r, ObjectLocation::hash_root(r, "firstName")));
    out.emplace_back(CheckResult::required(r, ObjectLocation::root(r)).with_message(""));
    out.emplace_back(CheckResult::required(r, ObjectLocation::root(r)).with_message("Keep me"));
    return out;
}

static void catalog_copies_and_output_owners() {
    GlobalContext global;
    auto user = global.make_user_context();
    Ref<I18nCatalog> kept;
    Ref<RuntimeError> retained_error;
    auto& retained = global.make<CheckResults>(results(global.memory_resource()));
    {
        auto parsed = I18nCatalog::from_json(user->memory_resource(), source);
        check(bool(parsed), "custom catalog parses");
        auto original = user->make_ref<I18nCatalog>(std::move(parsed).value());
        kept = user->make_global<I18nCatalog>(user->get(original));
        auto error = I18nCatalog::from_json(user->memory_resource(), R"({"a":3})");
        check(!error, "invalid catalog returns Language error");
        retained_error = user->make_global<RuntimeError>(error.error());
        user->with_i18n_catalog(original).with_language(Language::French);
        user->set_i18n_catalog(*user, user->get(original));
        auto& owned = user->make<CheckResults>(results(user->memory_resource()));
        user->translate_check_results(owned);
        check(*owned[0].message == "French First name" && owned[1].message->empty() &&
                  *owned[2].message == "Keep me",
              "Context fills only missing messages and preserves existing empty messages");
        check(owned[0].message->get_allocator().resource() == user->memory_resource(),
              "User messages own their characters in the requesting domain");

        // An explicitly Global result owns copies of translated text, not catalog views.
        user->translate_check_results(retained);
        check(retained[0].message->get_allocator().resource() == global.memory_resource(),
              "translation respects each result's explicit destination owner");
    }
    user.reset();
    auto next = global.make_user_context();
    next->with_i18n_catalog(next->rebind_global(kept));
    next->set_i18n_catalog(global, next->get(next->rebind_global(kept)));
    auto& copied = next->get(next->rebind_global(kept));
    auto catalog_debug = debug_string(next->memory_resource(), copied);
    check(
        catalog_debug.find("I18nCatalog { default_locale: \"en\"") != String::npos &&
            catalog_debug.find("Long vocabulary to force owned character allocation") !=
                String::npos &&
            catalog_debug.find("fallback: Some(I18nCatalog") != String::npos &&
            catalog_debug.get_allocator().resource() == next->memory_resource(),
        "catalog diagnostics retain source private maps after User deletion in an explicit destination");
    check(debug_string(next->memory_resource(), Language{}) == "English" &&
              debug_string(next->memory_resource(), BuiltinTranslator(Language::French)) ==
                  "BuiltinTranslator { language: French }",
          "language and translator diagnostics preserve source derives");
    auto text = copied.message(next->memory_resource(), Language::Japanese, "long");
    auto vocabulary = copied.vocabulary(next->memory_resource(), Language::Japanese, "long");
    check(text == "Long translated text to force owned character allocation" &&
              vocabulary == "Long vocabulary to force owned character allocation" &&
              text.get_allocator().resource() == next->memory_resource(),
          "deep-copied catalog and fallback survive source User reclamation");
    auto& diagnostic = next->get(next->rebind_global(retained_error));
    check(diagnostic.kind() == RuntimeError::Kind::Language &&
              diagnostic.to_string(next->memory_resource()).find("root must contain only") !=
                  String::npos &&
              *retained[0].message == "French First name",
          "explicitly retained diagnostics and messages survive source close");
    auto& requests = next->make<CheckResults>(results(next->memory_resource()));
    next->translate_check_results(requests);
    check(*requests[0].message == "Custom First name", "new User chooses its own default language");
    next->destroy(next->rebind_global(kept));
    // Source checks do not access the catalog when every message is already set.
    next->translate_check_results(requests);
    requests[0].message.reset();
    rejected<ReferenceError>(
        [&] {
            next->translate_check_results(requests);
        },
        "early-destroyed catalog is rejected before reading it");
}

static void domain_rejection_and_terminal_close() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto other = global.make_user_context();
    {
        auto temporary = I18nCatalog::builtin(user->memory_resource());
        rejected<std::invalid_argument>(
            [&] {
                user->set_i18n_catalog(*user, temporary);
            },
            "matching allocator does not make a stack catalog a managed object");
    }
    auto local = user->make_ref<I18nCatalog>(I18nCatalog::builtin(user->memory_resource()));
    rejected<std::invalid_argument>(
        [&] {
            other->set_i18n_catalog(*user, user->get(local));
        },
        "catalog cannot borrow a sibling User's arena");
    ResourceBorrow<const I18nCatalog> borrow(std::allocator_arg,
                                             Allocator(user->memory_resource()),
                                             user->get(local),
                                             detail::resource_arena(user->memory_resource()));
    rejected<std::invalid_argument>(
        [&] {
            (void)global.make<ResourceBorrow<const I18nCatalog>>(borrow);
        },
        "Global cannot retain a User catalog borrow");
    user->set_i18n_catalog(local);
    user->destroy(local);
    auto& checks = user->make<CheckResults>(results(user->memory_resource()));
    rejected<ReferenceError>(
        [&] {
            user->translate_check_results(checks);
        },
        "local catalog early destruction expires Context's borrow");
    auto replacement = user->make_ref<I18nCatalog>(I18nCatalog::builtin(user->memory_resource()));
    user->set_i18n_catalog(replacement);
    user->translate_check_results(checks);
    check(*checks[0].message == "The First name is required",
          "catalog replacement restores translation");
    bool wrong_thread = false;
    std::thread worker([&] {
        try {
            user->set_language(Language::German);
        } catch (const std::logic_error&) {
            wrong_thread = true;
        }
    });
    worker.join();
    check(wrong_thread && user->language() == Language::English,
          "language mutation enforces the Context owner thread");
    CheckResults empty(other->memory_resource());
    user->close();
    rejected<ReferenceError>(
        [&] {
            user->translate_check_results(empty);
        },
        "closed Context rejects translation");
    rejected<ReferenceError>(
        [&] {
            user->set_language(Language::German);
        },
        "closed Context rejects language mutation");
}

static void module_and_translator() {
    GlobalContext global;
    auto user = global.make_user_context();
    user->set_language(Language::French);
    auto& module = global.make<RuntimeModule>();
    module.apply_to(*user);
    check(user->language() == Language::French,
          "unspecified module language preserves Context preference");
    module.language(Language::German);
    auto& copy = global.make<RuntimeModule>(module);
    auto from_module = copy.into_context(global);
    check(from_module->language() == Language::German,
          "copied module applies language during Context creation");
    auto rejected_code = from_module->set_locale_code(" tl-PH ");
    check(!rejected_code && from_module->language() == Language::German &&
              rejected_code.error().to_string(from_module->memory_resource()).find(" tl-PH ") !=
                  String::npos,
          "unsupported locale retains original input and does not change preference");
    auto& translator = user->make<BuiltinTranslator>(Language::English);
    MessageTranslator& abstract = translator;
    auto result = CheckResult::required(user->memory_resource(),
                                        ObjectLocation::root(user->memory_resource()));
    check(abstract.language() == Language::English &&
              abstract.translate_check_result(user->memory_resource(), result) ==
                  "The $ is required",
          "scoped translator preserves abstract interface and root display path");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        catalog_copies_and_output_owners();
        domain_rejection_and_terminal_close();
        module_and_translator();
        std::pmr::set_default_resource(previous);
        std::cout << "Language/catalog Context ownership contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
