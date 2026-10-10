#pragma once
#include <teaql/runtime/check_result.hpp>
#include <array>

namespace teaql {
enum class Language {
    English,
    Chinese,
    TraditionalChinese,
    Japanese,
    Korean,
    German,
    French,
    Spanish,
    Portuguese,
    Arabic,
    Thai,
    Indonesian,
    Filipino,
    Ukrainian,
    Vietnamese
};
using Locale = Language;
inline constexpr std::array<Language, 15> all_languages{Language::English,
                                                        Language::Chinese,
                                                        Language::TraditionalChinese,
                                                        Language::Japanese,
                                                        Language::Korean,
                                                        Language::German,
                                                        Language::French,
                                                        Language::Spanish,
                                                        Language::Portuguese,
                                                        Language::Arabic,
                                                        Language::Thai,
                                                        Language::Indonesian,
                                                        Language::Filipino,
                                                        Language::Ukrainian,
                                                        Language::Vietnamese};
std::string_view language_code(Language);
std::optional<Language> language_from_code(std::string_view);
String translate_location(Memory, Language, const ObjectLocation&);
String translate_check_result(Memory, Language, const CheckResult&);

class MessageTranslator {
public:
    virtual ~MessageTranslator() = default;
    virtual Language language() const = 0;
    virtual String translate_check_result(Memory, const CheckResult&) const = 0;
};

class BuiltinTranslator final : public MessageTranslator {
public:
    using allocator_type = Allocator;

    explicit BuiltinTranslator(Language language) : language_(language) {}

    BuiltinTranslator(std::allocator_arg_t, Allocator, Language language) : language_(language) {}

    BuiltinTranslator(std::allocator_arg_t, Allocator, const BuiltinTranslator& other)
        : language_(other.language_) {}

    Language language() const override {
        return language_;
    }

    String translate_check_result(Memory r, const CheckResult& result) const override {
        return teaql::translate_check_result(r, language_, result);
    }

private:
    Language language_;
};
}
