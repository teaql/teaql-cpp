#include <teaql/runtime/round_trip_reference.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>
#include <teaql/runtime/detail/context/identity.hpp>
#include <teaql/core/detail/text.hpp>
#include <bit>
#include <algorithm>

namespace teaql {
namespace {
using namespace detail;

void require_text(std::string_view value) {
    auto trimmed = value;
    trim(trimmed);
    if (trimmed.empty() || value.size() > 16384 || !identity_utf8(value)) {
        throw DocumentRoundTripError::invalid();
    }
}

void integer(CryptoBytes& out, std::uint64_t value, unsigned width = 8) {
    for (unsigned i = width; i > 0; --i) {
        out.push_back(static_cast<std::uint8_t>(value >> ((i - 1) * 8)));
    }
}

void count(CryptoBytes& out, std::size_t value) {
    if (value > UINT32_MAX) {
        throw DocumentRoundTripError::invalid();
    }
    integer(out, value, 4);
}

void text(CryptoBytes& out, std::string_view value) {
    require_text(value);
    count(out, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

void identity(CryptoBytes& out, const ReferenceIdentity& value) {
    text(out, value.entity_type);
    integer(out, value.id);
    integer(out, std::bit_cast<std::uint64_t>(value.version));
}

void mutations(CryptoBytes& out, const DocumentMutationKinds& values) {
    count(out, values.size());
    for (auto value : values) {
        switch (value) {
        case DocumentMutationKind::Update:
            out.push_back(1);
            break;
        case DocumentMutationKind::CreateChild:
            out.push_back(2);
            break;
        case DocumentMutationKind::RemoveChild:
            out.push_back(3);
            break;
        default:
            throw DocumentRoundTripError::invalid();
        }
    }
}

void fields(CryptoBytes& out, const DocumentFieldNames& values) {
    count(out, values.size());
    for (const auto& value : values) {
        text(out, value);
    }
}

CryptoBytes aad(Memory r,
                std::string_view key,
                std::string_view service,
                std::string_view environment,
                std::string_view purpose) {
    CryptoBytes out(r);
    for (auto value : {std::string_view("tqd1"), key, service, environment, purpose}) {
        text(out, value);
    }
    return out;
}

CryptoBytes encode(Memory r,
                   const DocumentSnapshot& value,
                   CryptoView actor,
                   std::int64_t issued,
                   std::int64_t expires) {
    CryptoBytes out(r);
    out.push_back(1);
    out.insert(out.end(), actor.begin(), actor.end());
    integer(out, std::bit_cast<std::uint64_t>(issued));
    integer(out, std::bit_cast<std::uint64_t>(expires));
    text(out, value.model_fingerprint);
    text(out, value.business_id);
    text(out, value.scope.document_id);
    text(out, value.scope.purpose);
    text(out, value.scope.aggregate_type);
    integer(out, value.scope.aggregate_id);
    integer(out, std::bit_cast<std::uint64_t>(value.scope.aggregate_revision));
    identity(out, value.aggregate);
    count(out, value.entities.size());
    for (const auto& entity : value.entities) {
        text(out, entity.row_key);
        identity(out, entity.identity);
        fields(out, entity.loaded_fields);
        fields(out, entity.writable_fields);
        mutations(out, entity.allowed_mutations);
    }
    count(out, value.relations.size());
    for (const auto& relation : value.relations) {
        text(out, relation.relation_name);
        out.push_back(relation.completeness == DocumentRelationCompleteness::Complete ? 1 : 2);
        out.push_back(relation.writable ? 1 : 0);
        mutations(out, relation.allowed_mutations);
        count(out, relation.row_keys.size());
        for (const auto& key : relation.row_keys) {
            text(out, key);
        }
    }
    return out;
}

class Reader {
public:
    Reader(Memory r, CryptoView bytes) : r_(r), bytes_(bytes) {}

    CryptoView exact(std::size_t size) {
        if (size > bytes_.size() - offset_) {
            throw DocumentRoundTripError::invalid();
        }
        auto value = bytes_.subspan(offset_, size);
        offset_ += size;
        return value;
    }

    std::uint64_t integer(unsigned width = 8) {
        std::uint64_t value = 0;
        for (auto byte : exact(width)) {
            value = (value << 8) | byte;
        }
        return value;
    }

    std::int64_t signed_integer() {
        return std::bit_cast<std::int64_t>(integer());
    }

    std::size_t count(std::size_t maximum) {
        auto value = integer(4);
        if (value > maximum) {
            throw DocumentRoundTripError::invalid();
        }
        return static_cast<std::size_t>(value);
    }

    String text() {
        auto bytes = exact(count(16384));
        std::string_view value(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        require_text(value);
        return String(value, r_);
    }

    ReferenceIdentity identity() {
        auto type = text();
        auto id = integer();
        auto version = signed_integer();
        ReferenceIdentity value(std::allocator_arg, Allocator(r_), type, id, version);
        value.validate();
        return value;
    }

    DocumentFieldNames fields() {
        DocumentFieldNames out(r_);
        auto size = count(1024);
        for (std::size_t i = 0; i < size; ++i) {
            if (!out.insert(text()).second) {
                throw DocumentRoundTripError::invalid();
            }
        }
        return out;
    }

    DocumentMutationKinds mutations() {
        DocumentMutationKinds out(r_);
        auto size = count(3);
        for (std::size_t i = 0; i < size; ++i) {
            auto wire = integer(1);
            if (wire < 1 || wire > 3 ||
                !out.insert(static_cast<DocumentMutationKind>(wire - 1)).second) {
                throw DocumentRoundTripError::invalid();
            }
        }
        return out;
    }

    bool finished() const {
        return offset_ == bytes_.size();
    }

private:
    Memory r_;
    CryptoView bytes_;
    std::size_t offset_ = 0;
};

struct Wipe {
    CryptoBytes& value;

    ~Wipe() {
        wipe_bytes(value);
    }
};

struct Decoded {
    DocumentSnapshot snapshot;
    CryptoKey actor;
    std::int64_t issued, expires;
};

Decoded decode(Memory r, CryptoView bytes) {
    Reader reader(r, bytes);
    if (reader.integer(1) != 1) {
        throw DocumentRoundTripError::invalid();
    }
    CryptoKey actor{};
    auto fingerprint = reader.exact(32);
    std::copy(fingerprint.begin(), fingerprint.end(), actor.begin());
    auto issued = reader.signed_integer();
    auto expires = reader.signed_integer();
    auto model = reader.text();
    auto business = reader.text();
    auto document = reader.text();
    auto purpose = reader.text();
    auto type = reader.text();
    auto id = reader.integer();
    auto revision = reader.signed_integer();
    ReferenceDocumentScope scope(
        std::allocator_arg, Allocator(r), document, purpose, type, id, revision);
    scope.validate();
    auto aggregate = reader.identity();
    Vector<DocumentEntitySnapshot> entities(r);
    auto entity_count = reader.count(10000);
    for (std::size_t i = 0; i < entity_count; ++i) {
        auto key = reader.text();
        auto identity = reader.identity();
        auto loaded = reader.fields();
        auto writable = reader.fields();
        auto mutations = reader.mutations();
        auto& entity = entities.emplace_back(key,
                                             identity,
                                             std::span<const std::string_view>{},
                                             std::span<const std::string_view>{},
                                             std::span<const DocumentMutationKind>{});
        entity.loaded_fields = std::move(loaded);
        entity.writable_fields = std::move(writable);
        entity.allowed_mutations = std::move(mutations);
        entity.validate();
    }
    Vector<DocumentRelationSnapshot> relations(r);
    auto relation_count = reader.count(1024);
    for (std::size_t i = 0; i < relation_count; ++i) {
        auto name = reader.text();
        auto completeness = reader.integer(1);
        auto writable = reader.integer(1);
        if (completeness < 1 || completeness > 2 || writable > 1) {
            throw DocumentRoundTripError::invalid();
        }
        auto mutations = reader.mutations();
        auto row_count = reader.count(10000);
        auto& relation =
            relations.emplace_back(name,
                                   std::span<const std::string_view>{},
                                   completeness == 1 ? DocumentRelationCompleteness::Complete
                                                     : DocumentRelationCompleteness::Partial,
                                   writable != 0,
                                   std::span<const DocumentMutationKind>{});
        relation.allowed_mutations = std::move(mutations);
        for (std::size_t j = 0; j < row_count; ++j) {
            relation.row_keys.push_back(reader.text());
        }
        relation.validate();
    }
    if (!reader.finished()) {
        throw DocumentRoundTripError::invalid();
    }
    DocumentSnapshot snapshot(
        std::allocator_arg, Allocator(r), model, business, scope, aggregate, entities, relations);
    snapshot.validate();
    return {std::move(snapshot), actor, issued, expires};
}
}

DocumentResult<String>
ContextBoundReferenceRuntime::issue_document_snapshot(Memory r,
                                                      const TrustedReferencePrincipal& principal,
                                                      const DocumentSnapshot& snapshot,
                                                      ReferenceLifetime lifetime) const {
    detail::ReferenceCallGuard guard(memory_resource(), r);
    try {
        principal.validate();
        snapshot.validate();
        if (!lifetime.seconds && !lifetime.nanoseconds) {
            throw DocumentRoundTripError::invalid();
        }
        auto authorize = [&](const ReferenceIdentity& identity) {
            auto value = authorization_.get().authorize(r, principal, snapshot.scope, identity);
            if (!value) {
                throw value.error();
            }
        };
        authorize(snapshot.aggregate);
        for (const auto& entity : snapshot.entities) {
            authorize(entity.identity);
        }
        auto selected = keys_.get().current_key(r);
        if (!selected) {
            throw selected.error();
        }
        const auto& key = selected.value();
        auto now = clock_ ? (*clock_)() : UtcDateTime::now();
        UtcDateTime expires = [&] {
            try {
                return detail::reference_add(now, lifetime);
            } catch (const std::out_of_range&) {
                throw DocumentRoundTripError::invalid();
            }
        }();
        auto actor = document_actor_fingerprint(r, key, principal);
        auto plaintext = encode(r, snapshot, actor, now.timestamp(), expires.timestamp());
        detail::wipe_bytes(actor);
        Wipe wipe{plaintext};
        auto encryption = document_derived_key(r, key);
        auto bound = aad(r, key.key_id, service_, environment_, snapshot.scope.purpose);
        auto nonce = nonce_ ? (*nonce_)() : detail::reference_nonce();
        auto encrypted = detail::aes256_gcm_encrypt(r, encryption.bytes, nonce, plaintext, bound);
        detail::CryptoBytes envelope(nonce.begin(), nonce.end(), r);
        envelope.insert(envelope.end(), encrypted.begin(), encrypted.end());
        String token("tqd1.", r);
        token += key.key_id;
        token += '.';
        token += detail::reference_base64(r, envelope);
        return DocumentResult<String>::ok(r, std::move(token));
    } catch (const DocumentRoundTripError& error) {
        return DocumentResult<String>::err(r, error);
    } catch (const RoundTripReferenceError& error) {
        return DocumentResult<String>::err(r, DocumentRoundTripError(error));
    }
}

DocumentResult<VerifiedDocumentSnapshot>
ContextBoundReferenceRuntime::consume_document_snapshot(Memory r,
                                                        const TrustedReferencePrincipal& principal,
                                                        std::string_view token,
                                                        std::string_view expected_type,
                                                        std::string_view expected_purpose) const {
    detail::ReferenceCallGuard guard(memory_resource(), r);
    try {
        principal.validate();
        require_text(expected_type);
        require_text(expected_purpose);
        auto separator = token.find('.', 5);
        if (token.size() > 256000 || !token.starts_with("tqd1.") ||
            separator == std::string_view::npos || separator == 5 ||
            separator + 1 == token.size() ||
            token.find('.', separator + 1) != std::string_view::npos) {
            throw DocumentRoundTripError::invalid();
        }
        auto selected = keys_.get().key_by_id(r, token.substr(5, separator - 5));
        if (!selected) {
            throw selected.error();
        }
        if (!selected.value()) {
            throw DocumentRoundTripError::invalid();
        }
        const auto& key = *selected.value();
        auto envelope = detail::reference_unbase64(r, token.substr(separator + 1));
        if (!envelope || envelope->size() < 28) {
            throw DocumentRoundTripError::invalid();
        }
        auto encryption = document_derived_key(r, key);
        auto bound = aad(r, key.key_id, service_, environment_, expected_purpose);
        detail::CryptoNonce nonce{};
        std::copy_n(envelope->begin(), nonce.size(), nonce.begin());
        auto plaintext = detail::aes256_gcm_decrypt(
            r, encryption.bytes, nonce, detail::CryptoView(*envelope).subspan(12), bound);
        if (!plaintext) {
            throw DocumentRoundTripError::invalid();
        }
        Wipe wipe{*plaintext};
        auto decoded = decode(r, *plaintext);
        auto now = clock_ ? (*clock_)() : UtcDateTime::now();
        if (decoded.expires <= now.timestamp()) {
            throw DocumentRoundTripError::expired();
        }
        if (decoded.issued > now.timestamp() + 60) {
            throw DocumentRoundTripError::invalid();
        }
        auto actor = document_actor_fingerprint(r, key, principal);
        std::uint8_t difference = 0;
        for (std::size_t i = 0; i < actor.size(); ++i) {
            difference |= actor[i] ^ decoded.actor[i];
        }
        detail::wipe_bytes(actor);
        detail::wipe_bytes(decoded.actor);
        if (difference || decoded.snapshot.scope.purpose != expected_purpose ||
            decoded.snapshot.scope.aggregate_type != expected_type) {
            throw DocumentRoundTripError::scope_mismatch();
        }
        decoded.snapshot.validate();
        auto authorize = [&](const ReferenceIdentity& identity) {
            auto value =
                authorization_.get().authorize(r, principal, decoded.snapshot.scope, identity);
            if (!value) {
                throw value.error();
            }
        };
        authorize(decoded.snapshot.aggregate);
        for (const auto& entity : decoded.snapshot.entities) {
            authorize(entity.identity);
        }
        auto issued = UtcDateTime::from_timestamp(decoded.issued, 0);
        auto expires = UtcDateTime::from_timestamp(decoded.expires, 0);
        if (!issued || !expires) {
            throw DocumentRoundTripError::invalid();
        }
        return DocumentResult<VerifiedDocumentSnapshot>::ok(
            r,
            VerifiedDocumentSnapshot(
                std::allocator_arg, Allocator(r), decoded.snapshot, key.key_id, *issued, *expires));
    } catch (const DocumentRoundTripError& error) {
        return DocumentResult<VerifiedDocumentSnapshot>::err(r, error);
    } catch (const RoundTripReferenceError& error) {
        return DocumentResult<VerifiedDocumentSnapshot>::err(r, DocumentRoundTripError(error));
    }
}
}
