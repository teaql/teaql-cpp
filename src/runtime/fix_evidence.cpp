#include <teaql/runtime/fix_evidence.hpp>
#include <teaql/core/detail/text.hpp>
#include <array>

namespace teaql {
namespace {
bool contains_ascii_folded(std::string_view text, std::string_view word) {
    if (text.size() < word.size()) {
        return false;
    }
    for (std::size_t start = 0; start <= text.size() - word.size(); ++start) {
        bool match = true;
        for (std::size_t i = 0; i < word.size(); ++i) {
            auto c = text[start + i];
            if (c >= 'A' && c <= 'Z') {
                c += 'a' - 'A';
            }
            if (c != word[i]) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

void nonblank(std::string_view value, const char* message) {
    detail::trim(value);
    if (value.empty()) {
        throw std::invalid_argument(message);
    }
}
}

std::string_view fix_evidence_source_name(FixEvidenceSource source) {
    constexpr std::array<std::string_view, 2> names{"Clock", "Context"};
    return names.at(std::size_t(source));
}

FixEvidence::FixEvidence(std::allocator_arg_t,
                         Allocator a,
                         std::string_view entity,
                         std::string_view path,
                         FixEvidenceSource source,
                         std::string_view label)
    : entity_type(a.resource()), model_path(a.resource()), source(source),
      source_label(a.resource()) {
    nonblank(entity, "entity_type must not be blank");
    nonblank(path, "model_path must not be blank");
    nonblank(label, "source_label must not be blank");
    if (contains_ascii_folded(label, "authorization") || contains_ascii_folded(label, "cookie") ||
        contains_ascii_folded(label, "token=")) {
        throw std::invalid_argument("source_label must be a safe framework label");
    }
    entity_type = entity;
    model_path = path;
    source_label = label;
}

FixEvidence::FixEvidence(std::allocator_arg_t, Allocator a, const FixEvidence& other)
    : entity_type(other.entity_type, a.resource()), model_path(other.model_path, a.resource()),
      source(other.source), source_label(other.source_label, a.resource()) {}
}
