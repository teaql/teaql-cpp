#include <teaql/runtime/i18n_debug.hpp>
#include <teaql/runtime/detail/builtin_i18n_data.hpp>
#include <algorithm>

namespace teaql {
namespace {
template <class Entries>
void write_entries(DebugWriter& writer, const Entries& entries) {
    writer.sequence(
        entries,
        [](auto& out, const auto& entry) {
            out.text(entry.first);
            out.raw(": ");
            out.text(entry.second);
        },
        "{",
        "}");
}

template <class Messages, class Vocabulary>
void write_locale(DebugWriter& writer, const Messages& messages, const Vocabulary& vocabulary) {
    writer.structure("LocaleCatalog", [&](auto& fields) {
        fields.field("messages", [&](auto& out) {
            write_entries(out, messages);
        });
        fields.field("vocabulary", [&](auto& out) {
            write_entries(out, vocabulary);
        });
    });
}

void write_builtin_locales(DebugWriter& writer) {
    using Entry = detail::BuiltinI18nEntry;
    std::pmr::map<std::string_view, std::pmr::vector<const Entry*>> groups(
        writer.memory_resource());
    for (const auto& entry : detail::builtin_i18n_entries) {
        groups.try_emplace(entry.locale).first->second.push_back(&entry);
    }
    for (auto& [locale, entries] : groups) {
        std::ranges::sort(entries, [](const auto* a, const auto* b) {
            return a->key < b->key;
        });
    }
    writer.sequence(
        groups,
        [](auto& out, const auto& group) {
            out.text(group.first);
            out.raw(": ");
            out.structure("LocaleCatalog", [&](auto& fields) {
                for (bool vocabulary : {false, true}) {
                    fields.field(vocabulary ? "vocabulary" : "messages", [&](auto& inner) {
                        auto entries = group.second | std::views::filter([&](const auto* entry) {
                                           return entry->vocabulary == vocabulary;
                                       });
                        inner.sequence(
                            entries,
                            [](auto& value, const auto* entry) {
                                value.text(entry->key);
                                value.raw(": ");
                                value.text(entry->text);
                            },
                            "{",
                            "}");
                    });
                }
            });
        },
        "{",
        "}");
}
}

void write_debug(DebugWriter& writer, const I18nCatalog& catalog) {
    writer.structure("I18nCatalog", [&](auto& fields) {
        fields.field("default_locale", [&](auto& out) {
            out.text(catalog.default_locale_);
        });
        fields.field("locales", [&](auto& out) {
            if (catalog.builtin_) {
                write_builtin_locales(out);
            } else {
                out.sequence(
                    catalog.locales_,
                    [](auto& inner, const auto& locale) {
                        inner.text(locale.first);
                        inner.raw(": ");
                        write_locale(inner, locale.second.messages, locale.second.vocabulary);
                    },
                    "{",
                    "}");
            }
        });
        fields.field("fallback", [&](auto& out) {
            if (catalog.fallback_) {
                out.tuple("Some", [&](auto& inner) {
                    auto builtin = I18nCatalog::builtin(inner.memory_resource());
                    write_debug(inner, builtin);
                });
            } else {
                out.raw("None");
            }
        });
    });
}
}
