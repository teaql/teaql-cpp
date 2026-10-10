#pragma once
#include <teaql/memory/clone.hpp>
#include <teaql/memory/scoped.hpp>
#include <teaql/core/time.hpp>

namespace teaql {
enum class FixEvidenceSource { Clock, Context };
std::string_view fix_evidence_source_name(FixEvidenceSource);

struct FixEvidence {
    using allocator_type = Allocator;
    String entity_type, model_path;
    FixEvidenceSource source;
    String source_label;
    FixEvidence(std::allocator_arg_t,
                Allocator,
                std::string_view entity_type,
                std::string_view model_path,
                FixEvidenceSource,
                std::string_view source_label);
    FixEvidence(std::allocator_arg_t, Allocator, const FixEvidence&);

    FixEvidence(std::allocator_arg_t, Allocator a, FixEvidence&& other)
        : FixEvidence(detail::move_to(a.resource(), std::move(other))) {}

    FixEvidence(const FixEvidence&) = delete;
    FixEvidence(FixEvidence&&) noexcept = default;

    FixEvidence& operator=(FixEvidence&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return entity_type.get_allocator().resource();
    }

    bool operator==(const FixEvidence&) const = default;
};
}
