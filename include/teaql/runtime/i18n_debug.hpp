#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/runtime/i18n.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, Language language) {
    constexpr std::string_view names[]{"English",
                                       "Chinese",
                                       "TraditionalChinese",
                                       "Japanese",
                                       "Korean",
                                       "German",
                                       "French",
                                       "Spanish",
                                       "Portuguese",
                                       "Arabic",
                                       "Thai",
                                       "Indonesian",
                                       "Filipino",
                                       "Ukrainian",
                                       "Vietnamese"};
    auto index = static_cast<std::size_t>(language);
    if (index >= std::size(names)) {
        throw std::invalid_argument("Unknown language diagnostic variant");
    }
    writer.raw(names[index]);
}

inline void write_debug(DebugWriter& writer, const BuiltinTranslator& translator) {
    writer.structure("BuiltinTranslator", [&](auto& fields) {
        fields.field("language", [&](auto& out) {
            write_debug(out, translator.language());
        });
    });
}

void write_debug(DebugWriter&, const I18nCatalog&);
}
