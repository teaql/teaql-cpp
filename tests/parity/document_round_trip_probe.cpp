#include <teaql/runtime/round_trip_reference.hpp>
#include <teaql/runtime/context.hpp>
#include "../support/entity_reference_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/detail/reference_crypto.hpp>
#include <teaql/core/detail/hmac_sha256.hpp>
#include <iostream>

using namespace teaql;

static std::int64_t number(const Json& input, std::string_view key) {
    const auto& value = field(input, key);
    if (auto* result = std::get_if<std::int64_t>(&value.data)) {
        return *result;
    }
    return static_cast<std::int64_t>(std::get<std::uint64_t>(value.data));
}

static std::string_view
setting(const Json& input, std::string_view name, std::string_view fallback) {
    const auto& object = std::get<Json::Object>(input.data);
    auto found = object.find(name);
    return found == object.end() ? fallback : text(found->second);
}

// Test-only re-authentication creates inputs for the unchanged public decoder.
static String authenticated_fixture(Memory r, std::string_view token, const Json& input) {
    using namespace detail;
    auto separator = token.find('.', 5);
    auto id = token.substr(5, separator - 5);
    auto service = setting(input, "issue_service", "order-service");
    auto environment = setting(input, "issue_environment", "test");
    CryptoBytes info(r);
    for (auto piece :
         {std::string_view("document-snapshot-encryption"), service, environment, id}) {
        info.push_back(static_cast<std::uint8_t>(piece.size() >> 8));
        info.push_back(static_cast<std::uint8_t>(piece.size()));
        info.insert(info.end(), piece.begin(), piece.end());
    }
    CryptoKey master{};
    master.fill(id == "k1" ? 0x42 : 0x24);
    auto key = hkdf_sha256_32(reference_bytes("teaql.round-trip-reference.v1"), master, info);
    auto envelope = reference_unbase64(r, token.substr(separator + 1));
    if (!envelope) {
        throw std::runtime_error("fixture token did not decode");
    }
    CryptoNonce nonce{};
    std::copy_n(envelope->begin(), 12, nonce.begin());
    CryptoBytes aad(r);
    auto purpose = text(field(field(field(input, "snapshot"), "scope"), "purpose"));
    for (auto piece : {std::string_view("tqd1"), id, service, environment, purpose}) {
        auto length = piece.size();
        for (unsigned i = 4; i > 0; --i) {
            aad.push_back(static_cast<std::uint8_t>(length >> ((i - 1) * 8)));
        }
        aad.insert(aad.end(), piece.begin(), piece.end());
    }
    auto plaintext = aes256_gcm_decrypt(r, key, nonce, CryptoView(*envelope).subspan(12), aad);
    if (!plaintext) {
        throw std::runtime_error("fixture authentication failed");
    }
    auto offset = static_cast<std::size_t>(number(input, "wire_offset"));
    if (offset > plaintext->size()) {
        throw std::runtime_error("fixture offset outside payload");
    }
    auto operation = setting(input, "wire_operation", "truncate");
    if (operation == "truncate") {
        plaintext->resize(offset);
    } else if (operation == "append") {
        plaintext->push_back(0);
    } else {
        unsigned width = operation == "byte" ? 1 : operation == "u32" ? 4 : 8;
        if (width > plaintext->size() - offset) {
            throw std::runtime_error("fixture patch outside payload");
        }
        auto value = static_cast<std::uint64_t>(number(input, "wire_value"));
        for (unsigned i = 0; i < width; ++i) {
            (*plaintext)[offset + i] = static_cast<std::uint8_t>(value >> ((width - i - 1) * 8));
        }
    }
    auto encrypted = aes256_gcm_encrypt(r, key, nonce, *plaintext, aad);
    wipe_bytes(*plaintext);
    wipe_bytes(key);
    wipe_bytes(master);
    CryptoBytes changed(nonce.begin(), nonce.end(), r);
    changed.insert(changed.end(), encrypted.begin(), encrypted.end());
    String out("tqd1.", r);
    out += id;
    out += '.';
    out += reference_base64(r, changed);
    return out;
}

static Vector<std::string_view> strings(Memory r, const Json& input) {
    Vector<std::string_view> out(r);
    for (const auto& value : std::get<Json::Array>(input.data)) {
        out.push_back(text(value));
    }
    return out;
}

static Vector<DocumentMutationKind> mutations(Memory r, const Json& input) {
    Vector<DocumentMutationKind> out(r);
    for (const auto& value : std::get<Json::Array>(input.data)) {
        auto name = text(value);
        out.push_back(name == "update"   ? DocumentMutationKind::Update
                      : name == "create" ? DocumentMutationKind::CreateChild
                                         : DocumentMutationKind::RemoveChild);
    }
    return out;
}

static DocumentSnapshot snapshot(Memory r, const Json& input) {
    const auto& raw = field(input, "snapshot");
    const auto& scope_data = field(raw, "scope");
    ReferenceDocumentScope scope(std::allocator_arg,
                                 Allocator(r),
                                 text(field(scope_data, "document")),
                                 text(field(scope_data, "purpose")),
                                 text(field(scope_data, "type")),
                                 number(scope_data, "id"),
                                 number(scope_data, "version"));
    const auto& root = field(raw, "aggregate");
    ReferenceIdentity aggregate(std::allocator_arg,
                                Allocator(r),
                                text(field(root, "type")),
                                number(root, "id"),
                                number(root, "version"));
    Vector<DocumentEntitySnapshot> entities(r);
    for (const auto& entity : std::get<Json::Array>(field(raw, "entities").data)) {
        ReferenceIdentity identity(std::allocator_arg,
                                   Allocator(r),
                                   text(field(entity, "type")),
                                   number(entity, "id"),
                                   number(entity, "version"));
        auto loaded = strings(r, field(entity, "loaded"));
        auto writable = strings(r, field(entity, "writable"));
        auto allowed = mutations(r, field(entity, "mutations"));
        auto& value =
            entities.emplace_back(text(field(entity, "key")), identity, loaded, writable, allowed);
        value.validate();
    }
    Vector<DocumentRelationSnapshot> relations(r);
    for (const auto& relation : std::get<Json::Array>(field(raw, "relations").data)) {
        auto rows = strings(r, field(relation, "rows"));
        auto allowed = mutations(r, field(relation, "mutations"));
        auto& value = relations.emplace_back(text(field(relation, "name")),
                                             rows,
                                             text(field(relation, "completeness")) == "complete"
                                                 ? DocumentRelationCompleteness::Complete
                                                 : DocumentRelationCompleteness::Partial,
                                             std::get<bool>(field(relation, "writable").data),
                                             allowed);
        value.validate();
    }
    DocumentSnapshot value(std::allocator_arg,
                           Allocator(r),
                           text(field(raw, "model")),
                           text(field(raw, "business")),
                           scope,
                           aggregate,
                           entities,
                           relations);
    value.validate();
    return value;
}

static TrustedReferencePrincipal principal(Memory r, std::string_view subject) {
    return TrustedReferencePrincipal(
        std::allocator_arg, Allocator(r), "oidc", subject, "Merchant", 7);
}

struct Policy final : ReferenceAuthorizationPolicy {
    using allocator_type = Allocator;
    bool deny;
    mutable std::size_t calls = 0;

    Policy(std::allocator_arg_t, Allocator, bool denied) : deny(denied) {}

    ReferenceAuthorizationResult authorize(Memory r,
                                           const TrustedReferencePrincipal&,
                                           const ReferenceDocumentScope&,
                                           const ReferenceIdentity&) const override {
        ++calls;
        return deny ? ReferenceAuthorizationResult::err(
                          r, RoundTripReferenceError::authorization_required())
                    : ReferenceAuthorizationResult::ok(r, {});
    }
};

template <class T>
struct ReadyDocument {
    using allocator_type = Allocator;
    DocumentResult<T> value;

    ReadyDocument(std::allocator_arg_t, Allocator a, DocumentResult<T> result)
        : value(std::allocator_arg, a, result) {}

    std::optional<DocumentResult<T>> poll(WakeSignal&) {
        return std::move(value);
    }

    void cancel_and_wait() noexcept {}

    void finish_and_wait() noexcept {}
};

static std::optional<DocumentRoundTripError> service_error(std::string_view code) {
    if (code == "revision") {
        return DocumentRoundTripError::revision_conflict();
    }
    if (code == "projection") {
        return DocumentRoundTripError::projection_violation();
    }
    if (code == "validation") {
        return DocumentRoundTripError::validation_failed();
    }
    if (code == "not_found") {
        return DocumentRoundTripError::not_found();
    }
    return std::nullopt;
}

struct DocumentServiceFixture final : ContextBoundDocumentService {
    using allocator_type = Allocator;
    String failure;
    mutable std::size_t calls = 0;

    DocumentServiceFixture(std::allocator_arg_t, Allocator a, std::string_view code)
        : failure(code, a.resource()) {}

    AsyncOperation<DocumentResult<ContextualDocument>>&
    open_document(const UserContext& user_context,
                  const TrustedReferencePrincipal& principal,
                  DocumentOpenRequest request) const override {
        ++calls;
        auto r = user_context.memory_resource();
        auto error = service_error(failure);
        if (error) {
            return make_operation<DocumentResult<ContextualDocument>,
                                  ReadyDocument<ContextualDocument>>(
                r, DocumentResult<ContextualDocument>::err(r, *error));
        }
        Json::Object body(r);
        body.emplace(String("subject", r), Json(r, principal.subject));
        body.emplace(String("type", r), Json(r, request.aggregate_type));
        body.emplace(String("purpose", r), Json(r, request.purpose));
        body.emplace(String("seconds", r), Json(r, request.lifetime.seconds));
        body.emplace(String("nanos", r), Json(r, request.lifetime.nanoseconds));
        auto value = DocumentResult<ContextualDocument>::ok(r,
                                                            ContextualDocument(std::allocator_arg,
                                                                               Allocator(r),
                                                                               request.business_id,
                                                                               "issued-token",
                                                                               Json(r, body)));
        return make_operation<DocumentResult<ContextualDocument>,
                              ReadyDocument<ContextualDocument>>(r, std::move(value));
    }

    AsyncOperation<DocumentResult<AcceptedContextualDocument>>&
    accept_document(const UserContext& user_context,
                    const TrustedReferencePrincipal& principal,
                    DocumentAcceptRequest request) const override {
        ++calls;
        auto r = user_context.memory_resource();
        auto error = service_error(failure);
        if (error) {
            return make_operation<DocumentResult<AcceptedContextualDocument>,
                                  ReadyDocument<AcceptedContextualDocument>>(
                r, DocumentResult<AcceptedContextualDocument>::err(r, *error));
        }
        Json::Object body(r);
        body.emplace(String("subject", r), Json(r, principal.subject));
        body.emplace(String("type", r), Json(r, request.expected_aggregate_type));
        body.emplace(String("purpose", r), Json(r, request.purpose));
        body.emplace(String("command", r), Json(r, request.submitted.command_id));
        body.emplace(String("submitted_token", r), Json(r, request.submitted.document_token));
        body.emplace(String("submitted_body", r), Json(r, request.submitted.body));
        ContextualDocument document(std::allocator_arg,
                                    Allocator(r),
                                    request.submitted.business_id,
                                    "accepted-token",
                                    Json(r, body));
        Vector<ReferenceIdentity> changed(r);
        changed.emplace_back("Order", 10, 5);
        changed.emplace_back("OrderItem", 20, 3);
        auto value = DocumentResult<AcceptedContextualDocument>::ok(
            r,
            AcceptedContextualDocument(std::allocator_arg, Allocator(r), document, 4, 5, changed));
        return make_operation<DocumentResult<AcceptedContextualDocument>,
                              ReadyDocument<AcceptedContextualDocument>>(r, std::move(value));
    }
};

static Json service_observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& service = user_context.make<DocumentServiceFixture>(text(field(input, "failure")));
    if (std::get<bool>(field(input, "service").data)) {
        user_context.set_context_bound_document_service(user_context, service);
    }
    if (std::get<bool>(field(input, "principal").data)) {
        user_context.set_trusted_reference_principal(principal(r, "alice"));
    }
    Json::Object out(r);
    auto render = [&](const ContextualDocument& document) {
        out.emplace(String("business", r), Json(r, document.business_id));
        out.emplace(String("token", r), Json(r, document.document_token));
        out.emplace(String("body", r), Json(r, document.body));
    };
    try {
        auto type = text(field(input, "type"));
        auto reason = text(field(input, "purpose"));
        auto id = text(field(input, "business"));
        bool validate = std::get<bool>(field(input, "validate").data);
        if (text(field(input, "method")) == "open") {
            DocumentOpenRequest request(
                std::allocator_arg,
                Allocator(r),
                type,
                id,
                reason,
                ReferenceLifetime(number(input, "lifetime"),
                                  static_cast<std::uint32_t>(number(input, "nanos"))));
            if (validate) {
                request.validate();
            }
            const auto& value = user_context.open_document(std::move(request)).wait();
            if (value) {
                render(value.value());
                out.emplace(String("error", r), Json(r));
            } else {
                out.emplace(String("error", r), Json(r, value.error().code()));
            }
        } else {
            SubmittedContextualDocument submitted(std::allocator_arg,
                                                  Allocator(r),
                                                  id,
                                                  text(field(input, "token")),
                                                  text(field(input, "command")),
                                                  field(input, "body"));
            DocumentAcceptRequest request(
                std::allocator_arg, Allocator(r), type, reason, submitted);
            if (validate) {
                request.validate();
            }
            const auto& value = user_context.accept_document(std::move(request)).wait();
            if (value) {
                render(value.value().document);
                out.emplace(String("error", r), Json(r));
                out.emplace(String("previous", r), Json(r, value.value().previous_revision));
                out.emplace(String("next", r), Json(r, value.value().new_revision));
                Json::Array identities(r);
                for (const auto& identity : value.value().changed_entities) {
                    Json::Array parts(r);
                    parts.emplace_back(identity.entity_type);
                    parts.emplace_back(identity.id);
                    parts.emplace_back(identity.version);
                    identities.emplace_back(parts);
                }
                out.emplace(String("changed", r), Json(r, identities));
            } else {
                out.emplace(String("error", r), Json(r, value.error().code()));
            }
        }
    } catch (const DocumentRoundTripError& error) {
        out.emplace(String("error", r), Json(r, error.code()));
    }
    out.emplace(String("calls", r), Json(r, service.calls));
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    const auto& object = std::get<Json::Object>(input.data);
    if (object.contains("kind") && text(field(input, "kind")) == "service") {
        return service_observe(user_context, input);
    }
    try {
        auto value = snapshot(r, input);
        auto issuer = principal(r, "alice");
        detail::CryptoKey bytes{};
        bytes.fill(0x42);
        Vector<ReferenceKey> keys(r);
        keys.emplace_back("k1", bytes);
        bytes.fill(0x24);
        keys.emplace_back("k2", bytes);
        auto& provider =
            user_context.make<StaticReferenceKeyProvider>(setting(input, "issue_key", "k1"), keys);
        auto& policy = user_context.make<Policy>(std::get<bool>(field(input, "deny_issue").data));
        ResourceBorrow<const ReferenceKeyProvider> keys_borrow(
            std::allocator_arg, Allocator(r), provider, detail::resource_arena(r));
        ResourceBorrow<const ReferenceAuthorizationPolicy> policy_borrow(
            std::allocator_arg, Allocator(r), policy, detail::resource_arena(r));
        auto& runtime = user_context.make<ContextBoundReferenceRuntime>(
            DeploymentProfile::Test,
            setting(input, "issue_service", "order-service"),
            setting(input, "issue_environment", "test"),
            keys_borrow,
            policy_borrow);
        runtime.with_clock(
            reference_fixture::Clock(std::allocator_arg,
                                     Allocator(r),
                                     *UtcDateTime::from_timestamp(number(input, "issued"), 0)));
        detail::CryptoNonce nonce{};
        nonce.fill(0x33);
        runtime.with_nonce_source(
            reference_fixture::Nonce(std::allocator_arg, Allocator(r), nonce));
        auto token = runtime.issue_document_snapshot(
            r,
            issuer,
            value,
            ReferenceLifetime(number(input, "lifetime"),
                              static_cast<std::uint32_t>(number(input, "nanos"))));
        Json::Object output(r);
        output.emplace(String("issue_calls", r), Json(r, policy.calls));
        if (!token) {
            output.emplace(String("error", r), Json(r, token.error().code()));
            return Json(r, output);
        }
        String submitted(token.value(), r);
        auto modification = text(field(input, "modification"));
        if (modification == "tag") {
            submitted.back() = submitted.back() == 'A' ? 'B' : 'A';
        } else if (modification == "prefix") {
            submitted[0] = 'x';
        } else if (modification == "extra") {
            submitted += ".extra";
        } else if (modification == "missing_key") {
            submitted.replace(5, 2, "k9");
        }
        if (std::get<Json::Object>(input.data).contains("wire_operation")) {
            submitted = authenticated_fixture(r, token.value(), input);
            output.emplace(String("authenticated_fixture", r), Json(r, submitted));
        }
        Vector<ReferenceKey> decode_keys(r);
        bool remove_old = setting(input, "old_key", "retain") == "remove";
        for (const auto& key : keys) {
            if (!remove_old || key.key_id != "k1") {
                decode_keys.emplace_back(key);
            }
        }
        auto& decode_provider = user_context.make<StaticReferenceKeyProvider>(
            setting(input, "decode_key", remove_old ? "k2" : "k1"), decode_keys);
        ResourceBorrow<const ReferenceKeyProvider> decode_borrow(
            std::allocator_arg, Allocator(r), decode_provider, detail::resource_arena(r));
        auto& decoder = user_context.make<ContextBoundReferenceRuntime>(
            DeploymentProfile::Test,
            setting(input, "decode_service", setting(input, "issue_service", "order-service")),
            setting(input, "decode_environment", setting(input, "issue_environment", "test")),
            decode_borrow,
            policy_borrow);
        decoder.with_clock(
            reference_fixture::Clock(std::allocator_arg,
                                     Allocator(r),
                                     *UtcDateTime::from_timestamp(number(input, "now"), 0)));
        policy.deny = std::get<bool>(field(input, "deny_consume").data);
        auto actor = principal(r, text(field(input, "subject")));
        auto verified = decoder.consume_document_snapshot(r,
                                                          actor,
                                                          submitted,
                                                          text(field(input, "expected_type")),
                                                          text(field(input, "expected_purpose")));
        output.emplace(String("token", r), Json(r, token.value()));
        output.emplace(String("calls", r), Json(r, policy.calls));
        output.emplace(String("error", r), verified ? Json(r) : Json(r, verified.error().code()));
        if (verified) {
            const auto& decoded = verified.value();
            output.emplace(String("equal", r), Json(r, decoded.snapshot == value));
            output.emplace(String("key", r), Json(r, decoded.key_id));
            output.emplace(String("issued", r), Json(r, decoded.issued_at.timestamp()));
            output.emplace(String("expires", r), Json(r, decoded.expires_at.timestamp()));
            output.emplace(String("entity_found", r),
                           Json(r, decoded.snapshot.entity("item:0") != nullptr));
            output.emplace(String("relation_found", r),
                           Json(r, decoded.snapshot.relation("items") != nullptr));
        }
        return Json(r, output);
    } catch (const DocumentRoundTripError& error) {
        Json::Object output(r);
        output.emplace(String("error", r), Json(r, error.code()));
        return Json(r, output);
    }
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid document fixture");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
