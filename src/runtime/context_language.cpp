#include <teaql/runtime/context.hpp>

namespace teaql {
RuntimeHookResult UserContext::set_language_code(std::string_view code) {
    check_public();
    ensure_mutable();
    auto parsed = language_from_code(code);
    if (!parsed) {
        return RuntimeHookResult::err(memory_resource(),
                                      RuntimeError::unsupported_locale(memory_resource(), code));
    }
    language_ = *parsed;
    return RuntimeHookResult::ok(memory_resource(), std::monostate{});
}

void UserContext::translate_check_results(CheckResults& results) const {
    check_public();
    // Missing messages alone consult the borrowed catalog. The destination is
    // each result's explicit owner, so no catalog views escape into its message.
    for (auto& result : results) {
        if (!result.message) {
            auto r = result.memory_resource();
            auto translated =
                i18n_catalog_ ? i18n_catalog_->get().translate_check_result(r, language_, result)
                              : teaql::translate_check_result(r, language_, result);
            result.message.emplace(std::move(translated));
        }
    }
}
}
