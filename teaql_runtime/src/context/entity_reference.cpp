#include <teaql/runtime/context.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>

namespace teaql {
EntityReferenceEncodeResult UserContext::encode_entity_reference(std::string_view entity,
                                                                 std::uint64_t id,
                                                                 std::int64_t version,
                                                                 std::string_view purpose,
                                                                 ReferenceLifetime lifetime) const {
    check_public();
    detail::ReferenceCallGuard guard(memory_resource(), memory_resource());
    if (auto* codec = get_resource<EntityReferenceCodec>()) {
        return codec->encode(memory_resource(), entity, id, version, purpose, lifetime);
    }
    return EntityReferenceEncodeResult::err(memory_resource(),
                                            EntityReferenceTokenError::codec_required());
}

EntityReferenceDecodeResult UserContext::decode_entity_reference(std::string_view token,
                                                                 std::string_view entity,
                                                                 std::string_view purpose) const {
    check_public();
    detail::ReferenceCallGuard guard(memory_resource(), memory_resource());
    if (auto* codec = get_resource<EntityReferenceCodec>()) {
        return codec->decode(memory_resource(), token, entity, purpose);
    }
    return EntityReferenceDecodeResult::err(memory_resource(),
                                            EntityReferenceTokenError::codec_required());
}
}
