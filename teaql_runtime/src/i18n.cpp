#include <teaql/runtime/i18n.hpp>
#include <teaql/runtime/detail/builtin_i18n_data.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <charconv>

namespace teaql {
namespace {
std::optional<std::string_view>
builtin_entry(std::string_view locale, bool vocabulary, std::string_view key) {
    for (const auto& entry : detail::builtin_i18n_entries) {
        if (entry.locale == locale && entry.vocabulary == vocabulary && entry.key == key) {
            return entry.text;
        }
    }
    return {};
}

const Json* field(const Json::Object& object, std::string_view key) {
    auto found = object.find(key);
    return found == object.end() ? nullptr : &found->second;
}

const String* text(const Json* value) {
    return value ? std::get_if<String>(&value->data) : nullptr;
}

const Json::Object* object(const Json* value) {
    return value ? std::get_if<Json::Object>(&value->data) : nullptr;
}

std::optional<String> template_error(Memory r, std::string_view value) {
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '}') {
            return String("unmatched template closing brace", r);
        }
        if (value[i] != '{') {
            continue;
        }
        auto first = ++i;
        while (i < value.size() && value[i] != '}') {
            if (value[i] == '{') {
                return String("nested template opening brace", r);
            }
            ++i;
        }
        if (i == value.size()) {
            return String("unclosed template placeholder", r);
        }
        auto name = value.substr(first, i - first);
        if (name != "location" && name != "system" && name != "input" && name != "input_len") {
            String out("unknown template placeholder: ", r);
            out += name;
            return out;
        }
    }
    return {};
}

template <class T>
String integer_text(Memory r, T value) {
    char buffer[32];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return String(buffer, result.ptr, r);
}

String format_value(Memory r, const Value& value) {
    return std::visit(
        [&](const auto& data) -> String {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::same_as<T, std::monostate> || std::same_as<T, TypedNull>) {
                return String("null", r);
            } else if constexpr (std::same_as<T, bool>) {
                return String(data ? "true" : "false", r);
            } else if constexpr (std::integral<T>) {
                return integer_text(r, data);
            } else if constexpr (std::same_as<T, double>) {
                String out(r);
                detail::append_float(out, data, false);
                return out;
            } else if constexpr (std::same_as<T, String>) {
                return String(data, r);
            } else if constexpr (std::same_as<T, Json>) {
                return data.dump(r);
            } else if constexpr (std::same_as<T, Timestamp>) {
                return integer_text(r, data.as_millis());
            } else if constexpr (std::same_as<T, Date> || std::same_as<T, Decimal>) {
                return data.to_string(r);
            } else if constexpr (std::same_as<T, Value::Object>) {
                return String("<object>", r);
            } else {
                return String("<list>", r);
            }
        },
        value.data);
}

// Rust String::replace scans each original pass, then the next argument sees
// the previous pass's replacements. It does not recursively replace one key.
String replace(Memory r, std::string_view source, std::string_view key, std::string_view value) {
    String result(r);
    std::size_t position = 0;
    for (;;) {
        auto found = source.find(key, position);
        if (found == source.npos) {
            result += source.substr(position);
            return result;
        }
        result += source.substr(position, found - position);
        result += value;
        position = found + key.size();
    }
}
}

I18nCatalog::I18nCatalog(std::allocator_arg_t, Allocator a)
    : default_locale_(a.resource()), locales_(a.resource()) {}

I18nCatalog::I18nCatalog(std::allocator_arg_t, Allocator a, const I18nCatalog& other)
    : default_locale_(other.default_locale_, a.resource()), locales_(other.locales_, a.resource()),
      builtin_(other.builtin_), fallback_(other.fallback_) {}

I18nCatalog I18nCatalog::builtin(Memory r) {
    I18nCatalog out(std::allocator_arg, Allocator(r));
    out.default_locale_ = "en";
    out.builtin_ = true;
    return out;
}

Result<I18nCatalog, RuntimeError> I18nCatalog::from_json(Memory r, std::string_view source) {
    using Output = Result<I18nCatalog, RuntimeError>;
    auto fail = [&](std::string_view message) {
        return Output::err(r, RuntimeError::language(r, message));
    };
    auto parsed = Json::parse_result(r, source);
    if (!parsed) {
        String error("invalid i18n catalog: ", r);
        error += parsed.error().to_string(r);
        return fail(error);
    }
    const auto* root = object(&parsed.value());
    if (!root) {
        return fail("i18n catalog root must be an object");
    }
    if (root->size() != 3 || !root->contains("schema") || !root->contains("defaultLocale") ||
        !root->contains("locales")) {
        return fail("i18n catalog root must contain only schema, defaultLocale and locales");
    }
    auto* schema = text(field(*root, "schema"));
    if (!schema || *schema != "teaql.i18n/v1") {
        return fail("unsupported i18n catalog schema");
    }
    auto* default_locale = text(field(*root, "defaultLocale"));
    if (!default_locale) {
        return fail("missing i18n defaultLocale");
    }
    auto default_language = language_from_code(*default_locale);
    if (!default_language) {
        String error("unsupported i18n defaultLocale: ", r);
        error += *default_locale;
        return fail(error);
    }
    auto* locales = object(field(*root, "locales"));
    if (!locales) {
        return fail("missing i18n locales");
    }
    I18nCatalog out(std::allocator_arg, Allocator(r));
    out.default_locale_ = language_code(*default_language);
    for (const auto& [code, value] : *locales) {
        auto language = language_from_code(code);
        if (!language) {
            String error("unsupported i18n locale: ", r);
            error += code;
            return fail(error);
        }
        if (code != language_code(*language)) {
            String error("i18n locale keys must be canonical: ", r);
            error += code;
            return fail(error);
        }
        String prefix("i18n locale ", r);
        prefix += code;
        auto* entry = object(&value);
        if (!entry) {
            prefix += " must be an object";
            return fail(prefix);
        }
        if (entry->size() != 2 || !entry->contains("messages") || !entry->contains("vocabulary")) {
            prefix += " must contain only messages and vocabulary";
            return fail(prefix);
        }
        auto& target = out.locales_.try_emplace(code).first->second;
        for (bool vocabulary : {false, true}) {
            std::string_view name = vocabulary ? "vocabulary" : "messages";
            auto* entries = object(field(*entry, name));
            String path(code, r);
            path += '.';
            path += name;
            if (!entries) {
                String error("i18n ", r);
                error += path;
                error += " must be an object";
                return fail(error);
            }
            auto& destination = vocabulary ? target.vocabulary : target.messages;
            for (const auto& [key, translation] : *entries) {
                String key_path(path, r);
                key_path += '.';
                key_path += key;
                auto* content = text(&translation);
                if (!content || content->empty()) {
                    String error("empty i18n translation: ", r);
                    error += key_path;
                    return fail(error);
                }
                if (!vocabulary) {
                    if (auto error = template_error(r, *content)) {
                        key_path += ": ";
                        key_path += *error;
                        return fail(key_path);
                    }
                }
                destination.emplace(key, *content);
            }
        }
    }
    if (!out.locales_.contains(out.default_locale_)) {
        return fail("i18n catalog does not contain its default locale");
    }
    out.fallback_ = true;
    return Output::ok(r, std::move(out));
}

std::optional<std::string_view>
I18nCatalog::lookup_exact(std::string_view locale, bool vocabulary, std::string_view key) const {
    if (builtin_) {
        return builtin_entry(locale, vocabulary, key);
    }
    auto catalog = locales_.find(locale);
    if (catalog == locales_.end()) {
        return {};
    }
    const auto& entries = vocabulary ? catalog->second.vocabulary : catalog->second.messages;
    auto entry = entries.find(key);
    return entry == entries.end() ? std::optional<std::string_view>{} : entry->second;
}

String
I18nCatalog::lookup(Memory r, Language language, bool vocabulary, std::string_view key) const {
    auto locale = language_code(language);
    auto result = lookup_exact(locale, vocabulary, key);
    if (!result && fallback_) {
        result = builtin_entry(locale, vocabulary, key);
    }
    if (!result) {
        result = lookup_exact(default_locale_, vocabulary, key);
    }
    if (!result && fallback_) {
        result = builtin_entry("en", vocabulary, key);
    }
    return String(result.value_or(key), r);
}

String I18nCatalog::message(Memory r, Language language, std::string_view key) const {
    return lookup(r, language, false, key);
}

String I18nCatalog::vocabulary(Memory r, Language language, std::string_view key) const {
    return lookup(r, language, true, key);
}

String
I18nCatalog::translate_check_result(Memory r, Language language, const CheckResult& result) const {
    constexpr std::array<std::string_view, 8> keys{"checker.required",
                                                   "checker.invalidType",
                                                   "checker.min",
                                                   "checker.max",
                                                   "checker.minLength",
                                                   "checker.maxLength",
                                                   "checker.contextRootMissing",
                                                   "checker.contextRootMismatch"};
    auto output = message(r, language, keys.at(std::size_t(result.rule)));
    auto location = translate_location(r, language, result.location);
    auto system = result.system_value ? format_value(r, *result.system_value) : String("-", r);
    auto input = result.input_value ? format_value(r, *result.input_value) : String("-", r);
    std::size_t length = 0;
    if (result.input_value) {
        if (auto* value = std::get_if<String>(&result.input_value->data)) {
            for (unsigned char byte : *value) {
                if ((byte & 0xc0) != 0x80) {
                    ++length;
                }
            }
        }
    }
    auto input_len = integer_text(r, length);
    output = replace(r, output, "{location}", location);
    output = replace(r, output, "{system}", system);
    output = replace(r, output, "{input}", input);
    output = replace(r, output, "{input_len}", input_len);
    return output;
}
}
