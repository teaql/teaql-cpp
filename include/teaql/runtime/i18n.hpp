#pragma once
#include <teaql/runtime/language.hpp>
#include <teaql/runtime/error.hpp>

namespace teaql {
class DebugWriter;

// Catalogs are immutable after parsing; only explicitly scoped copies retain data.
class I18nCatalog {
public:
    using allocator_type = Allocator;
    I18nCatalog(std::allocator_arg_t, Allocator);
    I18nCatalog(std::allocator_arg_t, Allocator, const I18nCatalog&);

    I18nCatalog(std::allocator_arg_t, Allocator a, I18nCatalog&& other)
        : I18nCatalog(detail::move_to(a.resource(), std::move(other))) {}

    I18nCatalog(const I18nCatalog&) = delete;
    I18nCatalog(I18nCatalog&&) noexcept = default;
    static Result<I18nCatalog, RuntimeError> from_json(Memory, std::string_view);
    static I18nCatalog builtin(Memory);
    String message(Memory, Language, std::string_view key) const;
    String vocabulary(Memory, Language, std::string_view key) const;
    String translate_check_result(Memory, Language, const CheckResult&) const;

    Memory memory_resource() const noexcept {
        return default_locale_.get_allocator().resource();
    }

private:
    friend void write_debug(DebugWriter&, const I18nCatalog&);

    struct LocaleCatalog {
        using allocator_type = Allocator;
        std::pmr::map<String, String, std::less<>> messages, vocabulary;

        LocaleCatalog(std::allocator_arg_t, Allocator a)
            : messages(a.resource()), vocabulary(a.resource()) {}

        LocaleCatalog(std::allocator_arg_t, Allocator a, const LocaleCatalog& other)
            : messages(other.messages, a.resource()), vocabulary(other.vocabulary, a.resource()) {}
    };

    String default_locale_;
    std::pmr::map<String, LocaleCatalog, std::less<>> locales_;
    bool builtin_ = false, fallback_ = false;
    std::optional<std::string_view>
    lookup_exact(std::string_view locale, bool vocabulary, std::string_view key) const;
    String lookup(Memory, Language, bool vocabulary, std::string_view key) const;
};
}
