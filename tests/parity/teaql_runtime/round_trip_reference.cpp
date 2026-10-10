// Direct pinned context-bound reference source tests; no local codec algorithm.
#include "../../support/round_trip_reference_fixture.hpp"
#include <atomic>
#include <iostream>

using namespace teaql;
namespace inputs = teaql::round_trip_fixture;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class T>
static void error_is(const RoundTripResult<T>& result, std::string_view code) {
    check(!result && result.error().code() == code, "original exact coarse reference error");
}

struct SourcePolicy {
    std::atomic<std::uint8_t>* allowed;

    ReferenceAuthorizationResult operator()(Memory owner,
                                            const TrustedReferencePrincipal& principal,
                                            const ReferenceDocumentScope&,
                                            const ReferenceIdentity& identity) const {
        if (allowed->load(std::memory_order_seq_cst) == 1 && principal.subject == "alice" &&
            identity.entity_type == "OrderItem" && identity.id == 42) {
            return ReferenceAuthorizationResult::ok(owner, {});
        }
        return ReferenceAuthorizationResult::err(owner,
                                                 RoundTripReferenceError::authorization_required());
    }
};

struct Fixture {
    enum class Keys { Both, OldOnly, NewOnly };
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory owner = user->memory_resource();
    std::atomic<std::uint8_t>& allowed = user->make<std::atomic<std::uint8_t>>(1);
    FunctionReferenceAuthorizationPolicy<SourcePolicy>& policy =
        user->make<FunctionReferenceAuthorizationPolicy<SourcePolicy>>(SourcePolicy{&allowed});

    ContextBoundReferenceRuntime& runtime(DeploymentProfile profile = DeploymentProfile::Test,
                                          std::optional<std::string_view> ack = {},
                                          std::string_view service = "order-service",
                                          std::string_view environment = "test-a",
                                          Keys selection = Keys::Both,
                                          bool fixed = true) {
        Vector<ReferenceKey> keys(owner);
        detail::CryptoKey key;
        if (selection != Keys::NewOnly) {
            key.fill(0x11);
            keys.emplace_back("k1", key);
        }
        if (selection != Keys::OldOnly) {
            key.fill(0x22);
            keys.emplace_back("k2", key);
        }
        auto& provider = user->make<StaticReferenceKeyProvider>(
            selection == Keys::OldOnly ? "k1" : "k2", std::span<const ReferenceKey>(keys));
        ResourceBorrow<const ReferenceKeyProvider> kb(
            std::allocator_arg, Allocator(owner), provider, detail::resource_arena(owner));
        ResourceBorrow<const ReferenceAuthorizationPolicy> pb(
            std::allocator_arg, Allocator(owner), policy, detail::resource_arena(owner));
        auto& result =
            user->make<ContextBoundReferenceRuntime>(profile, service, environment, kb, pb, ack);
        if (fixed) {
            inputs::configure(result);
        }
        return result;
    }

    ExternalEntityReference issue(ContextBoundReferenceRuntime& runtime,
                                  ReferenceLifetime lifetime = {600, 0}) {
        auto result = runtime.issue(owner,
                                    inputs::principal(owner),
                                    inputs::scope(owner),
                                    inputs::identity(owner),
                                    lifetime);
        check(bool(result), "original reference issue succeeds");
        return std::move(result).value();
    }

    RoundTripResult<ResolvedEntityReference> consume(ContextBoundReferenceRuntime& runtime,
                                                     const ExternalEntityReference& reference) {
        return runtime.consume(
            owner, inputs::principal(owner), inputs::scope(owner), reference, "OrderItem");
    }
};

static void governed_reference_is_bound_opaque_rotatable_and_deterministic_under_vector_inputs() {
    Fixture f;
    auto& runtime = f.runtime();
    auto reference = f.issue(runtime);
    auto* token = std::get_if<String>(&reference.data);
    check(
        token &&
            *token ==
                "tqr1.k2.MzMzMzMzMzMzMzMzmhCET15vBdSc1BbFJcrvMMnRP1wAyjunShYlFPnDZL7Gk5V1LuptWyrbiQ8LQSDHFUmN3o6vb-MEgXZuh3N-YkeO55CYLeVgiyBBYslzuSu-fvWC2KMZcnZ5D8BRmWpGYA2Ok5sWe1Fg1aIyadTjVoKB-H2vPW_0okj-" &&
            token->starts_with("tqr1.k2.") && token->find("OrderItem") == String::npos &&
            token->find("alice") == String::npos,
        "all original exact governed golden/shape/opacity assertions");
    auto resolved = f.consume(runtime, reference);
    check(resolved && resolved.value().identity == inputs::identity(f.owner) &&
              resolved.value().key_id == "k2" && resolved.value().mode == ReferenceMode::Governed,
          "original complete resolved identity/key/mode");
    auto bob = inputs::principal(f.owner);
    bob.subject = "bob";
    error_is(runtime.consume(f.owner, bob, inputs::scope(f.owner), reference, "OrderItem"),
             "ROUND_TRIP_REFERENCE_SCOPE_MISMATCH");
    ReferenceDocumentScope other(
        std::allocator_arg, Allocator(f.owner), "doc-101", "edit-order", "Order", 100, 9);
    error_is(runtime.consume(f.owner, inputs::principal(f.owner), other, reference, "OrderItem"),
             "ROUND_TRIP_REFERENCE_SCOPE_MISMATCH");
    check(
        !runtime.consume(
            f.owner, inputs::principal(f.owner), inputs::scope(f.owner), reference, "InvoiceItem"),
        "original governed reference rejects substituted entity type");
}

static void old_decode_only_key_survives_rotation_and_unknown_key_fails_closed() {
    Fixture f;
    auto& old =
        f.runtime(DeploymentProfile::Test, {}, "order-service", "test-a", Fixture::Keys::OldOnly);
    detail::CryptoNonce nonce;
    nonce.fill(0x44);
    old.with_nonce_source(reference_fixture::Nonce(std::allocator_arg, Allocator(f.owner), nonce));
    auto reference = f.issue(old);
    check(bool(f.consume(f.runtime(), reference)),
          "original retained old decode key survives rotation");
    auto& current =
        f.runtime(DeploymentProfile::Test, {}, "order-service", "test-a", Fixture::Keys::NewOnly);
    error_is(f.consume(current, reference), "ROUND_TRIP_REFERENCE_INVALID");
}

static void tokens_are_environment_and_service_bound_and_tamper_evident() {
    Fixture f;
    auto& runtime = f.runtime();
    auto reference = f.issue(runtime);
    auto& environment = f.runtime(
        DeploymentProfile::Test, {}, "order-service", "test-b", Fixture::Keys::Both, false);
    auto& service = f.runtime(
        DeploymentProfile::Test, {}, "invoice-service", "test-a", Fixture::Keys::Both, false);
    check(!f.consume(environment, reference) && !f.consume(service, reference),
          "original environment/service domain separation");
    TrustedReferencePrincipal other(
        std::allocator_arg, Allocator(f.owner), "oidc", "alice", "Platform", 8);
    error_is(runtime.consume(f.owner, other, inputs::scope(f.owner), reference, "OrderItem"),
             "ROUND_TRIP_REFERENCE_SCOPE_MISMATCH");
    for (unsigned kind = 0; kind < 3; ++kind) {
        ReferenceDocumentScope substituted(std::allocator_arg,
                                           Allocator(f.owner),
                                           "doc-100",
                                           kind == 0 ? "view-order" : "edit-order",
                                           "Order",
                                           kind == 1 ? 101 : 100,
                                           kind == 2 ? 10 : 9);
        check(!runtime.consume(
                  f.owner, inputs::principal(f.owner), substituted, reference, "OrderItem"),
              "all three original purpose/aggregate/revision substitutions fail");
    }
    String token(std::get<String>(reference.data), f.owner);
    token.back() = token.back() == 'A' ? 'B' : 'A';
    ExternalEntityReference tampered(std::allocator_arg, Allocator(f.owner), token);
    check(!f.consume(runtime, tampered), "original final-character tampering is rejected");
}

static void fresh_nonces_make_equal_references_unlinkable() {
    Fixture f;
    auto& runtime = f.runtime();
    auto& next = f.user->make<std::atomic<std::uint8_t>>(1);
    runtime.with_nonce_source([&next] {
        detail::CryptoNonce result;
        result.fill(next.fetch_add(1, std::memory_order_seq_cst));
        return result;
    });
    auto first = f.issue(runtime);
    auto second = f.issue(runtime);
    check(first != second && f.consume(runtime, first) && f.consume(runtime, second),
          "original incrementing nonce bytes1/2 produce distinct valid references");
}

static void expired_reference_returns_coarse_expiry_code() {
    Fixture f;
    auto& issuer = f.runtime();
    detail::CryptoNonce nonce;
    nonce.fill(0x55);
    issuer.with_nonce_source(
        reference_fixture::Nonce(std::allocator_arg, Allocator(f.owner), nonce));
    auto reference = f.issue(issuer, {60, 0});
    auto& consumer = f.runtime();
    consumer.with_clock(
        reference_fixture::Clock(std::allocator_arg,
                                 Allocator(f.owner),
                                 *UtcDateTime::from_timestamp(inputs::now().timestamp() + 61, 0)));
    error_is(f.consume(consumer, reference), "ROUND_TRIP_REFERENCE_EXPIRED");
}

static void user_context_is_the_only_high_level_issue_and_consume_boundary() {
    Fixture f;
    auto& runtime = f.runtime();
    f.user->with_round_trip_reference_runtime(*f.user, runtime);
    error_is(f.user->reference_for(inputs::identity(f.owner), inputs::scope(f.owner), {600, 0}),
             "ROUND_TRIP_REFERENCE_AUTHORIZATION_REQUIRED");
    f.user->with_trusted_reference_principal(inputs::principal(f.owner));
    auto issued =
        f.user->reference_for(inputs::identity(f.owner), inputs::scope(f.owner), {600, 0});
    check(bool(issued), "original UserContext issues through installed runtime/principal");
    auto wire = f.user->serialize_reference(issued.value());
    check(bool(wire), "original Context wire serialization succeeds");
    auto decoded = f.user->deserialize_reference(wire.value());
    check(bool(decoded), "original Context wire deserialization succeeds");
    auto resolved = f.user->resolve_reference(decoded.value(), "OrderItem", inputs::scope(f.owner));
    check(resolved && resolved.value().identity == inputs::identity(f.owner),
          "original complete Context-resolved identity");
    auto missing = f.global.make_user_context();
    error_is(missing->reference_for(inputs::identity(missing->memory_resource()),
                                    inputs::scope(missing->memory_resource()),
                                    {600, 0}),
             "ROUND_TRIP_REFERENCE_CONFIGURATION_INVALID");
}

static void raw_01_unset_uses_governed_references() {
    Fixture f;
    check(f.runtime(DeploymentProfile::Development).mode() == ReferenceMode::Governed,
          "original absent raw acknowledgement stays governed");
}

static void raw_02_exact_acknowledgement_enables_development_raw_mode() {
    Fixture f;
    auto& runtime =
        f.runtime(DeploymentProfile::Development, UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT);
    check(runtime.mode() == ReferenceMode::Raw &&
              f.issue(runtime) == ExternalEntityReference(std::allocator_arg,
                                                          Allocator(f.owner),
                                                          RawEntityReference{42, 3}),
          "original exact development acknowledgement and raw42/version3 value");
}

static void raw_03_exact_acknowledgement_enables_test_raw_mode() {
    Fixture f;
    check(f.runtime(DeploymentProfile::Test, UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT).mode() ==
              ReferenceMode::Raw,
          "original exact test acknowledgement enables raw mode");
}

static void raw_04_near_match_does_not_enable_raw_mode() {
    Fixture f;
    for (auto acknowledgement :
         {"true",
          "1",
          "raw",
          " I_UNDERSTAND_THIS_EXPOSES_INTERNAL_ENTITY_IDS_FOR_LOCAL_DEBUGGING_ONLY",
          "I_UNDERSTAND_THIS_EXPOSES_INTERNAL_ENTITY_IDS_FOR_LOCAL_DEBUGGING_ONLY "}) {
        check(f.runtime(DeploymentProfile::Development, acknowledgement).mode() ==
                  ReferenceMode::Governed,
              "all five original near-match acknowledgements remain governed");
    }
}

static void raw_05_production_fails_fast_when_raw_mode_is_requested() {
    Fixture f;
    bool rejected = false;
    try {
        (void)f.runtime(DeploymentProfile::Production,
                        UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT,
                        "order-service",
                        "production");
    } catch (const RoundTripReferenceError& error) {
        rejected = error.code() == "ROUND_TRIP_REFERENCE_CONFIGURATION_INVALID";
    }
    check(rejected, "original production acknowledgement is an exact configuration failure");
}

static void raw_06_reference_from_another_actor_is_still_rejected_by_current_authorization() {
    Fixture f;
    auto& runtime =
        f.runtime(DeploymentProfile::Test, UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT);
    auto reference = f.issue(runtime);
    auto bob = inputs::principal(f.owner);
    bob.subject = "bob";
    error_is(runtime.consume(f.owner, bob, inputs::scope(f.owner), reference, "OrderItem"),
             "ROUND_TRIP_REFERENCE_AUTHORIZATION_REQUIRED");
}

static void raw_07_mode_change_rejects_old_wire_shape() {
    Fixture f;
    ExternalEntityReference raw(std::allocator_arg, Allocator(f.owner), RawEntityReference{42, 3});
    auto wire = ReferenceWireCodec::serialize(f.owner, raw);
    check(!ReferenceWireCodec::deserialize(f.owner, wire, ReferenceMode::Governed),
          "original raw wire rejected in governed mode");
    auto governed = f.issue(f.runtime());
    auto protected_wire = ReferenceWireCodec::serialize(f.owner, governed);
    check(!ReferenceWireCodec::deserialize(f.owner, protected_wire, ReferenceMode::Raw),
          "original governed wire rejected in raw mode");
}

static void raw_08_retains_version_type_and_authorization_guards() {
    Fixture f;
    auto& runtime =
        f.runtime(DeploymentProfile::Test, UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT);
    ExternalEntityReference raw(std::allocator_arg, Allocator(f.owner), RawEntityReference{42, 3});
    auto resolved = f.consume(runtime, raw);
    check(resolved && resolved.value().identity.version == 3 &&
              resolved.value().identity.entity_type == "OrderItem",
          "original raw resolved version/type");
    check(!runtime.consume(
              f.owner, inputs::principal(f.owner), inputs::scope(f.owner), raw, "InvoiceItem"),
          "original raw mode still authenticates expected entity type");
}

static void raw_09_exposes_safe_downgrade_metadata_without_secret_material() {
    Fixture f;
    auto notice = f.runtime(DeploymentProfile::Test, UNSAFE_EXPOSE_RAW_ENTITY_IDS_ACKNOWLEDGEMENT)
                      .startup_notice();
    const std::array<std::pair<std::string_view, std::string_view>, 2> headers{
        {{"TeaQL-Reference-Mode", "raw-internal-id"}, {"Cache-Control", "no-store"}}};
    check(notice && notice->level == "ERROR" && notice->deployment_profile == "test" &&
              notice->telemetry_key == "teaql.reference.mode" && notice->telemetry_value == "raw" &&
              notice->response_headers == headers &&
              notice->message.find("k2") == std::string_view::npos &&
              notice->message.find("22") == std::string_view::npos,
          "all original downgrade notice/header/secret-omission assertions");
}

static void current_authorization_is_rechecked_when_a_reference_returns() {
    Fixture f;
    // The original uses default OS clock/nonce for this case, not vector inputs.
    auto& runtime = f.runtime(
        DeploymentProfile::Test, {}, "order-service", "test-a", Fixture::Keys::Both, false);
    auto reference = f.issue(runtime);
    f.allowed.store(0, std::memory_order_seq_cst);
    error_is(f.consume(runtime, reference), "ROUND_TRIP_REFERENCE_AUTHORIZATION_REQUIRED");
}

static void wire_codec_rejects_extra_raw_fields_and_invalid_numbers() {
    Fixture f;
    for (auto input : {R"({"id":42,"version":3,"type":"OrderItem"})",
                       R"({"id":0,"version":3})",
                       R"({"id":42,"version":-1})"}) {
        auto wire = Json::parse(f.owner, input);
        check(wire && !ReferenceWireCodec::deserialize(f.owner, *wire, ReferenceMode::Raw),
              "all three original invalid raw wire shapes/numbers");
    }
    auto& runtime = f.runtime();
    check(!runtime.issue(f.owner,
                         inputs::principal(f.owner),
                         inputs::scope(f.owner),
                         inputs::identity(f.owner),
                         {0, 999000000}),
          "original999ms lifetime is rejected");
    String token("tqr1.k2.", f.owner);
    token.append(24001, 'A');
    ExternalEntityReference oversized(std::allocator_arg, Allocator(f.owner), token);
    check(!f.consume(runtime, oversized), "original24001-byte token body is rejected");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        governed_reference_is_bound_opaque_rotatable_and_deterministic_under_vector_inputs();
        old_decode_only_key_survives_rotation_and_unknown_key_fails_closed();
        tokens_are_environment_and_service_bound_and_tamper_evident();
        fresh_nonces_make_equal_references_unlinkable();
        expired_reference_returns_coarse_expiry_code();
        user_context_is_the_only_high_level_issue_and_consume_boundary();
        raw_01_unset_uses_governed_references();
        raw_02_exact_acknowledgement_enables_development_raw_mode();
        raw_03_exact_acknowledgement_enables_test_raw_mode();
        raw_04_near_match_does_not_enable_raw_mode();
        raw_05_production_fails_fast_when_raw_mode_is_requested();
        raw_06_reference_from_another_actor_is_still_rejected_by_current_authorization();
        raw_07_mode_change_rejects_old_wire_shape();
        raw_08_retains_version_type_and_authorization_guards();
        raw_09_exposes_safe_downgrade_metadata_without_secret_material();
        current_authorization_is_rechecked_when_a_reference_returns();
        wire_codec_rejects_extra_raw_fields_and_invalid_numbers();
        std::cout << "Original seventeen round-trip reference contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
