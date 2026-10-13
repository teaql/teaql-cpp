#include <teaql/runtime/context.hpp>
#include <teaql/runtime/round_trip_reference.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>

namespace teaql::detail {
template <class T, class Request>
class DocumentServiceOperation {
public:
    using allocator_type = Allocator;

    DocumentServiceOperation(std::allocator_arg_t,
                             Allocator a,
                             const UserContext& user_context,
                             Request request)
        : user_context_(user_context), request_(std::allocator_arg, a, request) {}

    std::optional<DocumentResult<T>> poll(WakeSignal& wake) {
        auto r = request_.memory_resource();
        if (!pending_) {
            auto* service = user_context_.get_resource<ContextBoundDocumentService>();
            if (!service) {
                return DocumentResult<T>::err(r, DocumentRoundTripError::configuration());
            }
            auto* principal = user_context_.get_resource<TrustedReferencePrincipal>();
            if (!principal) {
                return DocumentResult<T>::err(r, DocumentRoundTripError::authorization_required());
            }
            if constexpr (std::same_as<T, ContextualDocument>) {
                pending_ = &service->open_document(
                    user_context_, *principal, detail::clone_in(r, request_));
            } else {
                pending_ = &service->accept_document(
                    user_context_, *principal, detail::clone_in(r, request_));
            }
            if (pending_->memory_resource() != r) {
                pending_->cancel();
                pending_ = nullptr;
                throw std::invalid_argument(
                    "Document service operation belongs to another Context");
            }
            forward_.attach(pending_->wake_signal(), wake);
        }
        if (!pending_->poll()) {
            return std::nullopt;
        }
        return pending_->take_result();
    }

    void cancel_and_wait() noexcept {
        forward_.reset();
        if (pending_) {
            pending_->cancel();
            pending_ = nullptr;
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

private:
    const UserContext& user_context_;
    Request request_;
    AsyncOperation<DocumentResult<T>>* pending_ = nullptr;
    WakeSignal::Forward forward_;
};
}

namespace teaql {
AsyncOperation<DocumentResult<ContextualDocument>>&
UserContext::open_document(DocumentOpenRequest request) const {
    check_public();
    return make_operation<
        DocumentResult<ContextualDocument>,
        detail::DocumentServiceOperation<ContextualDocument, DocumentOpenRequest>>(
        memory_resource(), *this, std::move(request));
}

AsyncOperation<DocumentResult<AcceptedContextualDocument>>&
UserContext::accept_document(DocumentAcceptRequest request) const {
    check_public();
    return make_operation<
        DocumentResult<AcceptedContextualDocument>,
        detail::DocumentServiceOperation<AcceptedContextualDocument, DocumentAcceptRequest>>(
        memory_resource(), *this, std::move(request));
}

DocumentResult<String> UserContext::issue_document_snapshot(const DocumentSnapshot& snapshot,
                                                            ReferenceLifetime lifetime) const {
    check_public();
    auto r = memory_resource();
    detail::ReferenceCallGuard guard(r, r);
    auto* runtime = get_resource<ContextBoundReferenceRuntime>();
    if (!runtime) {
        return DocumentResult<String>::err(r, DocumentRoundTripError::configuration());
    }
    auto* principal = get_resource<TrustedReferencePrincipal>();
    if (!principal) {
        return DocumentResult<String>::err(r, DocumentRoundTripError::authorization_required());
    }
    return runtime->issue_document_snapshot(r, *principal, snapshot, lifetime);
}

DocumentResult<VerifiedDocumentSnapshot>
UserContext::consume_document_snapshot(std::string_view token,
                                       std::string_view expected_type,
                                       std::string_view expected_purpose) const {
    check_public();
    auto r = memory_resource();
    detail::ReferenceCallGuard guard(r, r);
    auto* runtime = get_resource<ContextBoundReferenceRuntime>();
    if (!runtime) {
        return DocumentResult<VerifiedDocumentSnapshot>::err(
            r, DocumentRoundTripError::configuration());
    }
    auto* principal = get_resource<TrustedReferencePrincipal>();
    if (!principal) {
        return DocumentResult<VerifiedDocumentSnapshot>::err(
            r, DocumentRoundTripError::authorization_required());
    }
    return runtime->consume_document_snapshot(
        r, *principal, token, expected_type, expected_purpose);
}
}
