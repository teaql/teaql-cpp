#include <teaql/runtime/document_round_trip.hpp>
#include <teaql/core/detail/text.hpp>
#include <algorithm>

namespace teaql {
namespace {
void require_text(std::string_view text) {
    auto trimmed = text;
    detail::trim(trimmed);
    if (trimmed.empty() || text.size() > 16384) {
        throw DocumentRoundTripError::invalid();
    }
}
}

DocumentEntitySnapshot::DocumentEntitySnapshot(std::allocator_arg_t,
                                               Allocator a,
                                               std::string_view key,
                                               const ReferenceIdentity& value,
                                               std::span<const std::string_view> loaded,
                                               std::span<const std::string_view> writable,
                                               std::span<const DocumentMutationKind> mutations)
    : row_key(key, a.resource()), identity(std::allocator_arg, a, value),
      loaded_fields(loaded.begin(), loaded.end(), a.resource()),
      writable_fields(writable.begin(), writable.end(), a.resource()),
      allowed_mutations(mutations.begin(), mutations.end(), a.resource()) {}

DocumentEntitySnapshot::DocumentEntitySnapshot(std::allocator_arg_t,
                                               Allocator a,
                                               const DocumentEntitySnapshot& value)
    : row_key(value.row_key, a.resource()), identity(std::allocator_arg, a, value.identity),
      loaded_fields(value.loaded_fields, a.resource()),
      writable_fields(value.writable_fields, a.resource()),
      allowed_mutations(value.allowed_mutations, a.resource()) {}

void DocumentEntitySnapshot::validate() const {
    require_text(row_key);
    require_text(identity.entity_type);
    if (identity.id == 0 || identity.version < 0 || loaded_fields.size() > 1024 ||
        writable_fields.size() > 1024 || allowed_mutations.empty() ||
        !std::includes(loaded_fields.begin(),
                       loaded_fields.end(),
                       writable_fields.begin(),
                       writable_fields.end())) {
        throw DocumentRoundTripError::projection_violation();
    }
    for (const auto& field : loaded_fields) {
        require_text(field);
    }
    for (const auto& field : writable_fields) {
        require_text(field);
    }
}

DocumentRelationSnapshot::DocumentRelationSnapshot(std::allocator_arg_t,
                                                   Allocator a,
                                                   std::string_view name,
                                                   std::span<const std::string_view> rows,
                                                   DocumentRelationCompleteness completeness,
                                                   bool writable,
                                                   std::span<const DocumentMutationKind> mutations)
    : relation_name(name, a.resource()), row_keys(rows.begin(), rows.end(), a.resource()),
      completeness(completeness), writable(writable),
      allowed_mutations(mutations.begin(), mutations.end(), a.resource()) {}

DocumentRelationSnapshot::DocumentRelationSnapshot(std::allocator_arg_t,
                                                   Allocator a,
                                                   const DocumentRelationSnapshot& value)
    : relation_name(value.relation_name, a.resource()), row_keys(value.row_keys, a.resource()),
      completeness(value.completeness), writable(value.writable),
      allowed_mutations(value.allowed_mutations, a.resource()) {}

void DocumentRelationSnapshot::validate() const {
    require_text(relation_name);
    if (row_keys.size() > 10000 || (!writable && !allowed_mutations.empty())) {
        throw DocumentRoundTripError::projection_violation();
    }
    DocumentFieldNames unique(memory_resource());
    for (const auto& key : row_keys) {
        require_text(key);
        if (!unique.insert(key).second) {
            throw DocumentRoundTripError::projection_violation();
        }
    }
}

DocumentSnapshot::DocumentSnapshot(std::allocator_arg_t,
                                   Allocator a,
                                   std::string_view fingerprint,
                                   std::string_view business,
                                   const ReferenceDocumentScope& scope,
                                   const ReferenceIdentity& aggregate,
                                   std::span<const DocumentEntitySnapshot> entities,
                                   std::span<const DocumentRelationSnapshot> relations)
    : model_fingerprint(fingerprint, a.resource()), business_id(business, a.resource()),
      scope(std::allocator_arg, a, scope), aggregate(std::allocator_arg, a, aggregate),
      entities(entities.begin(), entities.end(), a.resource()),
      relations(relations.begin(), relations.end(), a.resource()) {}

DocumentSnapshot::DocumentSnapshot(std::allocator_arg_t, Allocator a, const DocumentSnapshot& value)
    : DocumentSnapshot(std::allocator_arg,
                       a,
                       value.model_fingerprint,
                       value.business_id,
                       value.scope,
                       value.aggregate,
                       value.entities,
                       value.relations) {}

void DocumentSnapshot::validate() const {
    for (std::string_view text : {std::string_view(model_fingerprint),
                                  std::string_view(business_id),
                                  std::string_view(scope.document_id),
                                  std::string_view(scope.purpose),
                                  std::string_view(scope.aggregate_type)}) {
        require_text(text);
    }
    if (entities.size() > 10000 || relations.size() > 1024 ||
        aggregate.entity_type != scope.aggregate_type || aggregate.id != scope.aggregate_id ||
        aggregate.version != scope.aggregate_revision) {
        throw DocumentRoundTripError::projection_violation();
    }
    DocumentFieldNames rows(memory_resource());
    for (const auto& value : entities) {
        value.validate();
        if (!rows.insert(value.row_key).second) {
            throw DocumentRoundTripError::projection_violation();
        }
    }
    for (const auto& value : relations) {
        value.validate();
        for (const auto& key : value.row_keys) {
            if (!rows.contains(key)) {
                throw DocumentRoundTripError::projection_violation();
            }
        }
    }
}

const DocumentEntitySnapshot* DocumentSnapshot::entity(std::string_view key) const {
    auto found = std::ranges::find(entities, key, &DocumentEntitySnapshot::row_key);
    return found == entities.end() ? nullptr : &*found;
}

const DocumentRelationSnapshot* DocumentSnapshot::relation(std::string_view name) const {
    auto found = std::ranges::find(relations, name, &DocumentRelationSnapshot::relation_name);
    return found == relations.end() ? nullptr : &*found;
}

void DocumentOpenRequest::validate() const {
    require_text(aggregate_type);
    require_text(business_id);
    require_text(purpose);
    if (lifetime.seconds == 0 && lifetime.nanoseconds == 0) {
        throw DocumentRoundTripError::invalid();
    }
}

void DocumentAcceptRequest::validate() const {
    require_text(expected_aggregate_type);
    require_text(purpose);
    require_text(submitted.business_id);
    require_text(submitted.document_token);
    require_text(submitted.command_id);
}
}
