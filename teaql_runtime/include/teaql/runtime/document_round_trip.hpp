#pragma once
#include <teaql/runtime/round_trip_reference_types.hpp>
#include <teaql/runtime/entity_reference.hpp>
#include <teaql/async/operation.hpp>
#include <teaql/core/json.hpp>
#include <set>
#include <span>

namespace teaql {
class UserContext;

enum class DocumentMutationKind { Update, CreateChild, RemoveChild };
enum class DocumentRelationCompleteness { Complete, Partial };

class DocumentRoundTripError final : public std::exception {
public:
    using allocator_type = Allocator;

    DocumentRoundTripError(std::allocator_arg_t, Allocator, const DocumentRoundTripError& other)
        : code_(other.code_) {}

    explicit DocumentRoundTripError(const RoundTripReferenceError& error) : code_(error.code()) {}

    static DocumentRoundTripError invalid() {
        return DocumentRoundTripError("ROUND_TRIP_REFERENCE_INVALID");
    }

    static DocumentRoundTripError expired() {
        return DocumentRoundTripError("ROUND_TRIP_REFERENCE_EXPIRED");
    }

    static DocumentRoundTripError scope_mismatch() {
        return DocumentRoundTripError("ROUND_TRIP_REFERENCE_SCOPE_MISMATCH");
    }

    static DocumentRoundTripError authorization_required() {
        return DocumentRoundTripError("ROUND_TRIP_REFERENCE_AUTHORIZATION_REQUIRED");
    }

    static DocumentRoundTripError configuration() {
        return DocumentRoundTripError("ROUND_TRIP_REFERENCE_CONFIGURATION_INVALID");
    }

    static DocumentRoundTripError revision_conflict() {
        return DocumentRoundTripError("DOCUMENT_REVISION_CONFLICT");
    }

    static DocumentRoundTripError projection_violation() {
        return DocumentRoundTripError("DOCUMENT_PROJECTION_VIOLATION");
    }

    static DocumentRoundTripError validation_failed() {
        return DocumentRoundTripError("DOCUMENT_VALIDATION_FAILED");
    }

    static DocumentRoundTripError not_found() {
        return DocumentRoundTripError("DOCUMENT_NOT_FOUND");
    }

    std::string_view code() const noexcept {
        return code_;
    }

    const char* what() const noexcept override {
        return code_.data();
    }

    const std::exception* source() const noexcept {
        return nullptr;
    }

    bool operator==(const DocumentRoundTripError& other) const noexcept {
        return code_ == other.code_;
    }

private:
    explicit DocumentRoundTripError(std::string_view code) : code_(code) {}

    std::string_view code_;
};

template <class T>
using DocumentResult = Result<T, DocumentRoundTripError>;
using DocumentMutationKinds = std::pmr::set<DocumentMutationKind>;
using DocumentFieldNames = std::pmr::set<String, std::less<>>;

namespace detail {
template <class T, class... Args>
DocumentResult<T> validated_document(Memory r, Args&&... args) {
    try {
        T value(std::allocator_arg, Allocator(r), std::forward<Args>(args)...);
        value.validate();
        return DocumentResult<T>::ok(r, std::move(value));
    } catch (const DocumentRoundTripError& error) {
        return DocumentResult<T>::err(r, error);
    }
}
}

struct DocumentEntitySnapshot {
    using allocator_type = Allocator;

    template <class... Args>
    static DocumentResult<DocumentEntitySnapshot> create(Memory r, Args&&... args) {
        return detail::validated_document<DocumentEntitySnapshot>(r, std::forward<Args>(args)...);
    }

    String row_key;
    ReferenceIdentity identity;
    DocumentFieldNames loaded_fields, writable_fields;
    DocumentMutationKinds allowed_mutations;
    DocumentEntitySnapshot(std::allocator_arg_t,
                           Allocator,
                           std::string_view,
                           const ReferenceIdentity&,
                           std::span<const std::string_view>,
                           std::span<const std::string_view>,
                           std::span<const DocumentMutationKind>);
    DocumentEntitySnapshot(std::allocator_arg_t, Allocator, const DocumentEntitySnapshot&);
    DocumentEntitySnapshot(const DocumentEntitySnapshot&) = delete;
    DocumentEntitySnapshot(DocumentEntitySnapshot&&) noexcept = default;

    Memory memory_resource() const {
        return row_key.get_allocator().resource();
    }

    void validate() const;
    bool operator==(const DocumentEntitySnapshot&) const = default;
};

struct DocumentRelationSnapshot {
    using allocator_type = Allocator;

    template <class... Args>
    static DocumentResult<DocumentRelationSnapshot> create(Memory r, Args&&... args) {
        return detail::validated_document<DocumentRelationSnapshot>(r, std::forward<Args>(args)...);
    }

    String relation_name;
    Vector<String> row_keys;
    DocumentRelationCompleteness completeness;
    bool writable;
    DocumentMutationKinds allowed_mutations;
    DocumentRelationSnapshot(std::allocator_arg_t,
                             Allocator,
                             std::string_view,
                             std::span<const std::string_view>,
                             DocumentRelationCompleteness,
                             bool,
                             std::span<const DocumentMutationKind>);
    DocumentRelationSnapshot(std::allocator_arg_t, Allocator, const DocumentRelationSnapshot&);
    DocumentRelationSnapshot(const DocumentRelationSnapshot&) = delete;
    DocumentRelationSnapshot(DocumentRelationSnapshot&&) noexcept = default;

    Memory memory_resource() const {
        return relation_name.get_allocator().resource();
    }

    void validate() const;
    bool operator==(const DocumentRelationSnapshot&) const = default;
};

struct DocumentSnapshot {
    using allocator_type = Allocator;

    template <class... Args>
    static DocumentResult<DocumentSnapshot> create(Memory r, Args&&... args) {
        return detail::validated_document<DocumentSnapshot>(r, std::forward<Args>(args)...);
    }

    String model_fingerprint, business_id;
    ReferenceDocumentScope scope;
    ReferenceIdentity aggregate;
    Vector<DocumentEntitySnapshot> entities;
    Vector<DocumentRelationSnapshot> relations;
    DocumentSnapshot(std::allocator_arg_t,
                     Allocator,
                     std::string_view,
                     std::string_view,
                     const ReferenceDocumentScope&,
                     const ReferenceIdentity&,
                     std::span<const DocumentEntitySnapshot>,
                     std::span<const DocumentRelationSnapshot>);
    DocumentSnapshot(std::allocator_arg_t, Allocator, const DocumentSnapshot&);
    DocumentSnapshot(const DocumentSnapshot&) = delete;
    DocumentSnapshot(DocumentSnapshot&&) noexcept = default;

    Memory memory_resource() const {
        return model_fingerprint.get_allocator().resource();
    }

    void validate() const;
    const DocumentEntitySnapshot* entity(std::string_view) const;
    const DocumentRelationSnapshot* relation(std::string_view) const;
    bool operator==(const DocumentSnapshot&) const = default;
};

struct ContextualDocument {
    using allocator_type = Allocator;
    String business_id, document_token;
    Json body;

    ContextualDocument(std::allocator_arg_t,
                       Allocator a,
                       std::string_view id,
                       std::string_view token,
                       const Json& value)
        : business_id(id, a.resource()), document_token(token, a.resource()),
          body(a.resource(), value) {}

    ContextualDocument(std::allocator_arg_t, Allocator a, const ContextualDocument& value)
        : ContextualDocument(
              std::allocator_arg, a, value.business_id, value.document_token, value.body) {}

    ContextualDocument(const ContextualDocument&) = delete;
    ContextualDocument(ContextualDocument&&) noexcept = default;

    Memory memory_resource() const {
        return business_id.get_allocator().resource();
    }

    bool operator==(const ContextualDocument&) const = default;
};

struct SubmittedContextualDocument {
    using allocator_type = Allocator;
    String business_id, document_token, command_id;
    Json body;

    SubmittedContextualDocument(std::allocator_arg_t,
                                Allocator a,
                                std::string_view id,
                                std::string_view token,
                                std::string_view command,
                                const Json& value)
        : business_id(id, a.resource()), document_token(token, a.resource()),
          command_id(command, a.resource()), body(a.resource(), value) {}

    SubmittedContextualDocument(std::allocator_arg_t,
                                Allocator a,
                                const SubmittedContextualDocument& value)
        : SubmittedContextualDocument(std::allocator_arg,
                                      a,
                                      value.business_id,
                                      value.document_token,
                                      value.command_id,
                                      value.body) {}

    SubmittedContextualDocument(const SubmittedContextualDocument&) = delete;
    SubmittedContextualDocument(SubmittedContextualDocument&&) noexcept = default;

    Memory memory_resource() const {
        return business_id.get_allocator().resource();
    }

    bool operator==(const SubmittedContextualDocument&) const = default;
};

struct DocumentOpenRequest {
    using allocator_type = Allocator;

    template <class... Args>
    static DocumentResult<DocumentOpenRequest> create(Memory r, Args&&... args) {
        return detail::validated_document<DocumentOpenRequest>(r, std::forward<Args>(args)...);
    }

    String aggregate_type, business_id, purpose;
    ReferenceLifetime lifetime;

    DocumentOpenRequest(std::allocator_arg_t,
                        Allocator a,
                        std::string_view type,
                        std::string_view id,
                        std::string_view reason,
                        ReferenceLifetime duration)
        : aggregate_type(type, a.resource()), business_id(id, a.resource()),
          purpose(reason, a.resource()), lifetime(duration) {}

    DocumentOpenRequest(std::allocator_arg_t, Allocator a, const DocumentOpenRequest& value)
        : DocumentOpenRequest(std::allocator_arg,
                              a,
                              value.aggregate_type,
                              value.business_id,
                              value.purpose,
                              value.lifetime) {}

    DocumentOpenRequest(const DocumentOpenRequest&) = delete;
    DocumentOpenRequest(DocumentOpenRequest&&) noexcept = default;

    Memory memory_resource() const {
        return aggregate_type.get_allocator().resource();
    }

    void validate() const;
    bool operator==(const DocumentOpenRequest&) const = default;
};

struct DocumentAcceptRequest {
    using allocator_type = Allocator;

    template <class... Args>
    static DocumentResult<DocumentAcceptRequest> create(Memory r, Args&&... args) {
        return detail::validated_document<DocumentAcceptRequest>(r, std::forward<Args>(args)...);
    }

    String expected_aggregate_type, purpose;
    SubmittedContextualDocument submitted;

    DocumentAcceptRequest(std::allocator_arg_t,
                          Allocator a,
                          std::string_view type,
                          std::string_view reason,
                          const SubmittedContextualDocument& value)
        : expected_aggregate_type(type, a.resource()), purpose(reason, a.resource()),
          submitted(std::allocator_arg, a, value) {}

    DocumentAcceptRequest(std::allocator_arg_t, Allocator a, const DocumentAcceptRequest& value)
        : DocumentAcceptRequest(std::allocator_arg,
                                a,
                                value.expected_aggregate_type,
                                value.purpose,
                                value.submitted) {}

    DocumentAcceptRequest(const DocumentAcceptRequest&) = delete;
    DocumentAcceptRequest(DocumentAcceptRequest&&) noexcept = default;

    Memory memory_resource() const {
        return expected_aggregate_type.get_allocator().resource();
    }

    void validate() const;
    bool operator==(const DocumentAcceptRequest&) const = default;
};

struct AcceptedContextualDocument {
    using allocator_type = Allocator;
    ContextualDocument document;
    std::int64_t previous_revision, new_revision;
    Vector<ReferenceIdentity> changed_entities;

    AcceptedContextualDocument(std::allocator_arg_t,
                               Allocator a,
                               const ContextualDocument& value,
                               std::int64_t previous,
                               std::int64_t next,
                               std::span<const ReferenceIdentity> identities)
        : document(std::allocator_arg, a, value), previous_revision(previous), new_revision(next),
          changed_entities(identities.begin(), identities.end(), a.resource()) {}

    AcceptedContextualDocument(std::allocator_arg_t,
                               Allocator a,
                               const AcceptedContextualDocument& value)
        : AcceptedContextualDocument(std::allocator_arg,
                                     a,
                                     value.document,
                                     value.previous_revision,
                                     value.new_revision,
                                     value.changed_entities) {}

    AcceptedContextualDocument(const AcceptedContextualDocument&) = delete;
    AcceptedContextualDocument(AcceptedContextualDocument&&) noexcept = default;

    Memory memory_resource() const {
        return document.memory_resource();
    }

    bool operator==(const AcceptedContextualDocument&) const = default;
};

struct VerifiedDocumentSnapshot {
    using allocator_type = Allocator;
    DocumentSnapshot snapshot;
    String key_id;
    UtcDateTime issued_at, expires_at;

    VerifiedDocumentSnapshot(std::allocator_arg_t,
                             Allocator a,
                             const DocumentSnapshot& value,
                             std::string_view key,
                             UtcDateTime issued,
                             UtcDateTime expires)
        : snapshot(std::allocator_arg, a, value), key_id(key, a.resource()), issued_at(issued),
          expires_at(expires) {}

    VerifiedDocumentSnapshot(std::allocator_arg_t,
                             Allocator a,
                             const VerifiedDocumentSnapshot& value)
        : VerifiedDocumentSnapshot(std::allocator_arg,
                                   a,
                                   value.snapshot,
                                   value.key_id,
                                   value.issued_at,
                                   value.expires_at) {}

    VerifiedDocumentSnapshot(const VerifiedDocumentSnapshot&) = delete;
    VerifiedDocumentSnapshot(VerifiedDocumentSnapshot&&) noexcept = default;

    Memory memory_resource() const {
        return snapshot.memory_resource();
    }

    bool operator==(const VerifiedDocumentSnapshot&) const = default;
};

class ContextBoundDocumentService {
public:
    virtual ~ContextBoundDocumentService() = default;
    virtual AsyncOperation<DocumentResult<ContextualDocument>>&
    open_document(const UserContext& user_context,
                  const TrustedReferencePrincipal&,
                  DocumentOpenRequest) const = 0;
    virtual AsyncOperation<DocumentResult<AcceptedContextualDocument>>&
    accept_document(const UserContext& user_context,
                    const TrustedReferencePrincipal&,
                    DocumentAcceptRequest) const = 0;
};
}
