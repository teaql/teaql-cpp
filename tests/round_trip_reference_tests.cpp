#include "round_trip_reference_fixture.hpp"
#include <teaql/core/detail/hmac_sha256.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>
#include <charconv>
#include <iostream>

using namespace teaql;
using namespace teaql::round_trip_fixture;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static constexpr std::string_view golden =
    "tqr1.k2.MzMzMzMzMzMzMzMzmhCET15vBdSc1BbFJcrvMMnRP1wAyjunShYlFPnDZL7Gk5V1LuptWyrbiQ8LQSDHFUmN3o6vb-MEgXZuh3N-YkeO55CYLeVgiyBBYslzuSu-fvWC2KMZcnZ5D8BRmWpGYA2Ok5sWe1Fg1aIyadTjVoKB-H2vPW_0okj-";

static detail::CryptoBytes hex(Memory r, std::string_view text) {
    detail::CryptoBytes out(r);
    for (std::size_t i = 0; i < text.size(); i += 2) {
        unsigned value{};
        auto result = std::from_chars(text.data() + i, text.data() + i + 2, value, 16);
        check(result.ec == std::errc{}, "RFC vector hex");
        out.push_back(value);
    }
    return out;
}

static void primitive_vectors(Memory r) {
    auto key = hex(r, "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
    auto expected = hex(r, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    auto digest = detail::hmac_sha256(key, detail::reference_bytes("Hi There"));
    check(std::ranges::equal(digest, expected), "RFC 4231 HMAC-SHA256 vector");
    key = hex(r, "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
    auto salt = hex(r, "000102030405060708090a0b0c");
    auto info = hex(r, "f0f1f2f3f4f5f6f7f8f9");
    auto derived = detail::hkdf_sha256_32(salt, key, info);
    expected = hex(r, "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf");
    check(std::ranges::equal(derived, expected), "RFC 5869 A.1 first HKDF output block");
}

static void ownership_and_policy() {
    GlobalContext global;
    primitive_vectors(global.memory_resource());
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto kp =
        user->make_global<StaticReferenceKeyProvider>("k2", std::span<const ReferenceKey>(keys(r)));
    auto policy = user->make_global<Policy>();
    Ref<ContextBoundReferenceRuntime> retained;
    Ref<ResolvedEntityReference> kept;
    StringRef token;
    {
        ResourceBorrow<const ReferenceKeyProvider> key_borrow(
            std::allocator_arg,
            Allocator(r),
            user->get(kp),
            detail::resource_arena(global.memory_resource()));
        ResourceBorrow<const ReferenceAuthorizationPolicy> policy_borrow(
            std::allocator_arg,
            Allocator(r),
            user->get(policy),
            detail::resource_arena(global.memory_resource()));
        auto runtime = user->make_ref<ContextBoundReferenceRuntime>(
            DeploymentProfile::Test, "order-service", "test-a", key_borrow, policy_borrow);
        configure(user->get(runtime));
        auto p = principal(r), other = principal(r);
        auto s = scope(r);
        auto id = identity(r);
        auto missing = user->reference_for(id, s, {600, 0});
        check(!missing && missing.error() == RoundTripReferenceError::configuration(),
              "Missing runtime precedes missing principal");
        user->with_round_trip_reference_runtime(runtime);
        missing = user->reference_for(id, s, {600, 0});
        check(!missing && missing.error() == RoundTripReferenceError::authorization_required(),
              "Missing trusted principal fails closed");
        user->with_trusted_reference_principal(p);
        auto issued = user->reference_for(id, s, {600, 0});
        check(issued && std::get<String>(issued.value().data) == golden,
              "Complete original HKDF/HMAC/AES-GCM golden token");
        auto decoded = user->resolve_reference(issued.value(), "OrderItem", s);
        check(decoded && decoded.value().identity == id && decoded.value().key_id == "k2",
              "Scoped identity and key survive return authorization");
        kept = user->make_global<ResolvedEntityReference>(decoded.value());
        retained = user->make_global<ContextBoundReferenceRuntime>(user->get(runtime));
        token = user->save_global_string(std::get<String>(issued.value().data));
        other.subject = "bob";
        auto rejected = user->get(runtime).consume(r, other, s, issued.value(), "OrderItem");
        check(!rejected && rejected.error() == RoundTripReferenceError::scope_mismatch(),
              "Actor binding precedes current authorization");
        user->get(policy).allowed = false;
        rejected = user->resolve_reference(issued.value(), "OrderItem", s);
        check(!rejected && rejected.error() == RoundTripReferenceError::authorization_required(),
              "Current authorization is rechecked on return");
        user->get(policy).allowed = true;
        auto wire = user->serialize_reference(issued.value());
        check(wire && user->deserialize_reference(wire.value()),
              "UserContext wire path retains installed mode");
        auto raw =
            ExternalEntityReference(std::allocator_arg, Allocator(r), RawEntityReference{42, 3});
        check(!user->serialize_reference(raw), "Governed Context refuses raw serialization");
    }
    user.reset();
    auto next = global.make_user_context();
    auto runtime = next->rebind_global(retained);
    next->set_round_trip_reference_runtime(runtime);
    {
        auto nr = next->memory_resource();
        next->set_trusted_reference_principal(principal(nr));
        auto external = ExternalEntityReference(
            std::allocator_arg, Allocator(nr), next->get(next->rebind_global(token)));
        auto decoded = next->resolve_reference(external, "OrderItem", scope(nr));
        const auto& persisted = next->get(next->rebind_global(kept));
        check(
            decoded && persisted.identity.entity_type == "OrderItem" && persisted.key_id == "k2" &&
                persisted.identity.memory_resource() == global.memory_resource(),
            "Global runtime, providers, callbacks and nested claims survive actual User destruction");
        next->destroy(next->rebind_global(kp));
        bool dead = false;
        try {
            (void)next->resolve_reference(external, "OrderItem", scope(nr));
        } catch (const ReferenceError& error) {
            dead = error.kind() == ReferenceError::Kind::Dead;
        }
        check(dead, "Destroyed key provider is rejected before virtual lookup");
    }
}

static void shorter_provider_cannot_be_retained() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& key_provider =
        user->make<StaticReferenceKeyProvider>("k2", std::span<const ReferenceKey>(keys(r)));
    auto& policy = user->make<Policy>();
    ResourceBorrow<const ReferenceKeyProvider> kb(
        std::allocator_arg, Allocator(r), key_provider, detail::resource_arena(r));
    ResourceBorrow<const ReferenceAuthorizationPolicy> pb(
        std::allocator_arg, Allocator(r), policy, detail::resource_arena(r));
    auto runtime = user->make_ref<ContextBoundReferenceRuntime>(
        DeploymentProfile::Test, "service", "test", kb, pb);
    bool rejected = false;
    try {
        (void)user->make_global<ContextBoundReferenceRuntime>(user->get(runtime));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "Global runtime copy cannot retain shorter-lived providers");
    check(user->get(runtime).mode() == ReferenceMode::Governed,
          "Rejected retention preserves original runtime");
}

static void reentrant_policy_close(bool owner) {
    std::set_terminate([] {
        std::_Exit(0);
    });
    GlobalContext global;
    auto user = global.make_user_context();
    auto output = global.make_user_context();
    auto r = user->memory_resource();
    auto& key_provider =
        user->make<StaticReferenceKeyProvider>("k2", std::span<const ReferenceKey>(keys(r)));
    auto function = [target = owner ? user.get() : output.get()](Memory r,
                                                                 const TrustedReferencePrincipal&,
                                                                 const ReferenceDocumentScope&,
                                                                 const ReferenceIdentity&) {
        target->close();
        return ReferenceAuthorizationResult::ok(r, {});
    };
    auto& policy = user->make<FunctionReferenceAuthorizationPolicy<decltype(function)>>(function);
    ResourceBorrow<const ReferenceKeyProvider> kb(
        std::allocator_arg, Allocator(r), key_provider, detail::resource_arena(r));
    ResourceBorrow<const ReferenceAuthorizationPolicy> pb(
        std::allocator_arg, Allocator(r), policy, detail::resource_arena(r));
    auto& runtime = user->make<ContextBoundReferenceRuntime>(
        DeploymentProfile::Test, "service", "test", kb, pb);
    (void)runtime.issue(output->memory_resource(), principal(r), scope(r), identity(r), {60, 0});
    throw std::runtime_error("Reentrant policy reclamation was accepted");
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2) {
            reentrant_policy_close(std::string_view(argv[1]) == "reentrant-owner");
        }
        ownership_and_policy();
        shorter_provider_cannot_be_retained();
        std::cout << "Round-trip reference ownership and policy contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
