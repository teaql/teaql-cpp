#include "../../tests/support/entity_reference_fixture.hpp"
#include <iostream>

using namespace teaql;
using namespace teaql::reference_fixture;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static constexpr std::string_view golden =
    "tqr1.AAAAAjMzMzMzMzMzMzMzM3bKiZgRSQQhfIj2cBXRDZIloUGHWLBp8QrXL_aejwIXPFtvV_E71O7wbOXy3cvYo_SwxvuS-89x572T9CO_pDAY4tbjWCNv";

static void portable_reference_original_contract() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto material = keys(owner);
    auto codec =
        user->make_ref<AeadEntityReferenceCodec>(2, std::span<const EntityReferenceKey>(material));
    configure(user->get(codec), *UtcDateTime::from_ymd_hms_nano(2026, 9, 26, 12, 0, 0, 0));
    auto token = user->get(codec).encode(owner, "OrderItem", 42, 7, "edit-order", {3600, 0});
    check(token && token.value() == golden && token.value().find("OrderItem") == String::npos,
          "Original direct codec golden token is exact and opaque");
    auto claims = user->get(codec).decode(owner, token.value(), "OrderItem", "edit-order");
    check(claims && claims.value().id == 42 && claims.value().version == 7 &&
              claims.value().key_version == 2,
          "Original direct decode retains the complete identity/version/key tuple");
    check(!user->get(codec).decode(owner, token.value(), "InvoiceItem", "edit-order") &&
              !user->get(codec).decode(owner, token.value(), "OrderItem", "other-purpose"),
          "Original direct codec rejects both type and purpose mismatches");
    String tampered(token.value(), owner);
    tampered.back() = 'A';
    check(!user->get(codec).decode(owner, tampered, "OrderItem", "edit-order"),
          "Original final-character token tampering fails authentication");
}

static void native_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto now = *UtcDateTime::from_ymd_hms_nano(2026, 9, 26, 12, 0, 0, 0);
    auto missing = user->encode_entity_reference("Order", 1, 1, "edit", {60, 0});
    check(!missing && missing.error().code() == "ENTITY_REFERENCE_CODEC_REQUIRED",
          "Missing codec fails closed");
    Ref<AeadEntityReferenceCodec> retained;
    Ref<EntityReferenceClaims> claims;
    StringRef token;
    {
        auto material = keys(r);
        auto codec = user->make_ref<AeadEntityReferenceCodec>(
            2, std::span<const EntityReferenceKey>(material));
        configure(user->get(codec), now);
        user->with_entity_reference_codec(codec);
        auto encoded = user->encode_entity_reference("OrderItem", 42, 7, "edit-order", {3600, 0});
        check(encoded && encoded.value() == golden &&
                  encoded.value().get_allocator().resource() == r,
              "Original golden token matches exactly and belongs to User");
        auto decoded = user->decode_entity_reference(encoded.value(), "OrderItem", "edit-order");
        check(decoded && decoded.value().id == 42 && decoded.value().version == 7 &&
                  decoded.value().key_version == 2,
              "Opaque token resolves original identity and active key version");
        claims = user->make_global<EntityReferenceClaims>(decoded.value());
        token = user->save_global_string(encoded.value());
        retained = user->make_global<AeadEntityReferenceCodec>(user->get(codec));
        check(!user->decode_entity_reference(encoded.value(), "InvoiceItem", "edit-order") &&
                  !user->decode_entity_reference(encoded.value(), "OrderItem", "other-purpose"),
              "Entity type and purpose are authenticated boundaries");
        user->destroy(codec);
        bool rejected = false;
        try {
            (void)user->decode_entity_reference(encoded.value(), "OrderItem", "edit-order");
        } catch (const ReferenceError& error) {
            rejected = error.kind() == ReferenceError::Kind::Dead;
        }
        check(rejected, "Resource lookup rejects early-destroyed codec before virtual use");
    }
    user.reset();
    auto next = global.make_user_context();
    auto rebound = next->rebind_global(retained);
    next->set_entity_reference_codec(rebound);
    const auto& kept = next->get(next->rebind_global(claims));
    check(kept.entity_type == "OrderItem" && kept.purpose == "edit-order" &&
              kept.memory_resource() == global.memory_resource(),
          "Explicit Global claims and nested strings survive actual User destruction");
    auto decoded = next->decode_entity_reference(
        next->get(next->rebind_global(token)), "OrderItem", "edit-order");
    check(
        decoded && decoded.value().memory_resource() == next->memory_resource(),
        "Deep-copied Global codec callbacks and keys survive User without extending its ownership");
    auto later = *UtcDateTime::from_timestamp(now.timestamp() + 3600, 0);
    configure(next->get(rebound), later);
    check(!next->decode_entity_reference(golden, "OrderItem", "edit-order"),
          "Expiry is inclusive at the expiration instant");
    {
        auto raw = next->make_ref<AeadEntityReferenceCodec>(
            2, std::span<const EntityReferenceKey>(keys(next->memory_resource())));
        auto token1 = next->get(raw).encode(next->memory_resource(), "Item", 1, -1, "", {60, 0});
        auto token2 = next->get(raw).encode(next->memory_resource(), "Item", 1, -1, "", {60, 0});
        check(token1 && token2 && token1.value() != token2.value(),
              "Default OS nonce source produces separate opaque tokens");
    }
    decoded = EntityReferenceDecodeResult::err(next->memory_resource(),
                                               EntityReferenceTokenError::invalid());
    next->close();
    bool closed = false;
    try {
        (void)next->decode_entity_reference(golden, "OrderItem", "edit-order");
    } catch (const ReferenceError& error) {
        closed = error.kind() == ReferenceError::Kind::Closed;
    }
    check(closed, "Closed Context rejects reference decoding before key access");
}

static void reentrant_close(bool owner) {
    std::set_terminate([] {
        std::_Exit(0);
    });
    GlobalContext global;
    auto first = global.make_user_context();
    auto output = global.make_user_context();
    auto material = keys(first->memory_resource());
    auto codec =
        first->make_ref<AeadEntityReferenceCodec>(2, std::span<const EntityReferenceKey>(material));
    first->get(codec).with_clock([target = owner ? first.get() : output.get()] {
        target->close();
        return UtcDateTime::now();
    });
    (void)first->get(codec).encode(output->memory_resource(), "Item", 1, 1, "edit", {60, 0});
    throw std::runtime_error("Reentrant close was accepted");
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2) {
            reentrant_close(std::string_view(argv[1]) == "reentrant-owner");
        } else {
            portable_reference_original_contract();
            native_contracts();
            std::cout << "Entity reference ownership contracts passed\n";
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
