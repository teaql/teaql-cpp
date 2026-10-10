#include <teaql/runtime/language.hpp>
#include <teaql/runtime/i18n.hpp>
#include <teaql/core/detail/text.hpp>
#include <teaql/core/detail/uppercase_tables.hpp>
#include <teaql/core/detail/language_case_tables.hpp>
#include <algorithm>

namespace teaql {
std::string_view language_code(Language language) {
    constexpr std::array<std::string_view, 15> codes{"en",
                                                     "zh-CN",
                                                     "zh-TW",
                                                     "ja",
                                                     "ko",
                                                     "de",
                                                     "fr",
                                                     "es",
                                                     "pt",
                                                     "ar",
                                                     "th",
                                                     "id",
                                                     "fil",
                                                     "uk",
                                                     "vi"};
    return codes.at(std::size_t(language));
}

std::optional<Language> language_from_code(std::string_view code) {
    detail::trim(code);
    // Match the normalized code without allocating into an implicit resource.
    auto equal = [&](std::string_view value) {
        if (value.size() != code.size()) {
            return false;
        }
        for (std::size_t i = 0; i != code.size(); ++i) {
            auto c = code[i];
            if (c >= 'A' && c <= 'Z') {
                c += 'a' - 'A';
            }
            if (c == '_') {
                c = '-';
            }
            if (c != value[i]) {
                return false;
            }
        }
        return true;
    };
    for (auto value : {"zh", "zh-cn", "zh-hans", "zh-sg", "cn"}) {
        if (equal(value)) {
            return Language::Chinese;
        }
    }
    for (auto value : {"zh-tw", "zh-hant", "zh-hk", "zh-mo", "tw"}) {
        if (equal(value)) {
            return Language::TraditionalChinese;
        }
    }
    if (equal("tl")) {
        return Language::Filipino;
    }
    if (equal("zh-latn") || code.empty()) {
        return {};
    }
    auto separator = code.find_first_of("-_");
    code = code.substr(0, separator);
    for (auto language : all_languages) {
        if (language != Language::Chinese && language != Language::TraditionalChinese &&
            equal(language_code(language))) {
            return language;
        }
    }
    return {};
}

namespace {
char32_t decode(std::string_view input, std::size_t& index) {
    auto invalid = []() -> char32_t {
        throw std::invalid_argument("Invalid UTF-8 in object location");
    };
    auto first = static_cast<unsigned char>(input[index++]);
    if (first < 128) {
        return first;
    }
    unsigned count = first >= 0xc2 && first <= 0xdf   ? 1
                     : first >= 0xe0 && first <= 0xef ? 2
                     : first >= 0xf0 && first <= 0xf4 ? 3
                                                      : 0;
    if (!count || input.size() - index < count) {
        return invalid();
    }
    char32_t value = first & ((1u << (6 - count)) - 1);
    for (unsigned i = 0; i != count; ++i) {
        auto byte = static_cast<unsigned char>(input[index++]);
        if ((byte & 0xc0) != 0x80) {
            return invalid();
        }
        value = (value << 6) | (byte & 63);
    }
    if (value < (count == 1   ? 0x80u
                 : count == 2 ? 0x800u
                              : 0x10000u) ||
        value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) {
        return invalid();
    }
    return value;
}

void append(String& out, char32_t value) {
    if (value < 128) {
        out += char(value);
    } else if (value < 0x800) {
        out += char(0xc0 | (value >> 6));
        out += char(0x80 | (value & 63));
    } else if (value < 0x10000) {
        out += char(0xe0 | (value >> 12));
        out += char(0x80 | ((value >> 6) & 63));
        out += char(0x80 | (value & 63));
    } else {
        out += char(0xf0 | (value >> 18));
        out += char(0x80 | ((value >> 12) & 63));
        out += char(0x80 | ((value >> 6) & 63));
        out += char(0x80 | (value & 63));
    }
}

template <class Ranges>
char32_t map_case(char32_t value, const Ranges& ranges) {
    auto it = std::upper_bound(
        ranges.begin(), ranges.end(), value, [](char32_t value, const auto& range) {
            return value < range.first;
        });
    if (it != ranges.begin()) {
        --it;
        if (value <= it->last && (value - it->first) % it->step == 0) {
            return char32_t(std::int64_t(value) + it->delta);
        }
    }
    return value;
}

bool uppercase(char32_t value) {
    const auto& ranges = detail::language_upper_predicates;
    auto it = std::upper_bound(
        ranges.begin(), ranges.end(), value, [](char32_t value, const auto& range) {
            return value < range.first;
        });
    return it != ranges.begin() && value <= (it - 1)->last;
}

void case_append(String& out, char32_t value, bool upper) {
    if (upper) {
        const auto& expansions = detail::uppercase_expansions;
        auto it = std::lower_bound(
            expansions.begin(), expansions.end(), value, [](const auto& expansion, char32_t value) {
                return expansion.from < value;
            });
        if (it != expansions.end() && it->from == value) {
            for (auto c : it->to) {
                if (c) {
                    append(out, c);
                }
            }
            return;
        }
        append(out, map_case(value, detail::uppercase_ranges));
    } else {
        const auto& expansions = detail::language_lower_expansions;
        auto it = std::lower_bound(
            expansions.begin(), expansions.end(), value, [](const auto& expansion, char32_t value) {
                return expansion.source < value;
            });
        if (it != expansions.end() && it->source == value) {
            for (auto c : it->target) {
                if (c) {
                    append(out, c);
                }
            }
            return;
        }
        append(out, map_case(value, detail::language_lower_ranges));
    }
}
}

String translate_location(Memory r, Language, const ObjectLocation& location) {
    auto path = location.to_string(r);
    String out(r);
    std::size_t start = 0;
    for (;;) {
        auto end = path.find('.', start);
        auto part = std::string_view(path).substr(
            start, end == path.npos ? path.size() - start : end - start);
        auto bracket = part.find('[');
        auto name = part.substr(0, bracket);
        std::size_t index = 0, count = 0;
        while (index < name.size()) {
            auto c = decode(name, index);
            if (count && uppercase(c)) {
                out += ' ';
            }
            case_append(out, c, count++ == 0);
        }
        if (bracket != part.npos) {
            out += part.substr(bracket);
        }
        if (end == path.npos) {
            break;
        }
        out += '.';
        start = end + 1;
    }
    return out;
}

String translate_check_result(Memory r, Language language, const CheckResult& result) {
    return I18nCatalog::builtin(r).translate_check_result(r, language, result);
}
}
