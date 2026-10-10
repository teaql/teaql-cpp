#include <teaql/runtime.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void retained_models_and_redaction() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    const auto issued = UtcDateTime::from_timestamp(59, 1'000'000'001).value();
    const auto expires = UtcDateTime::from_timestamp(3600, 123'000'000).value();
    auto& long_text = user->make<String>(1024, 'r');
    auto principal =
        user->make_ref<TrustedReferencePrincipal>(long_text, long_text, long_text, UINT64_MAX);
    // Public-field mutation and Clone do not rerun constructor validation.
    user->get(principal).subject.clear();
    auto retained_principal = user->make_global<TrustedReferencePrincipal>(user->get(principal));
    auto& claims = user->make<EntityReferenceClaims>(
        long_text, UINT64_MAX, INT64_MIN, issued, expires, long_text, UINT32_MAX);
    auto retained_claims = user->make_global<EntityReferenceClaims>(claims);
    auto& identity = user->make<ReferenceIdentity>(long_text, UINT64_MAX, INT64_MAX);
    auto& resolved = user->make<ResolvedEntityReference>(
        identity, ReferenceMode::Governed, std::string_view(long_text), issued, expires);
    auto retained_resolved = user->make_global<ResolvedEntityReference>(resolved);
    detail::CryptoKey secret{};
    secret.fill(0x7b);
    auto& key = user->make<ReferenceKey>("visible-key", secret);
    auto retained_key = user->make_global<ReferenceKey>(key);
    {
        const auto output = debug_string(owner, key, DebugOptions{.pretty = true});
        check(output.find("[REDACTED]") != output.npos && output.find("123") == output.npos,
              "reference key diagnostic exposes its ID and redacts all master material");
        check(key.debug(owner) == debug_string(owner, key),
              "legacy key diagnostic delegates to structural Debug");
        check(debug_string(owner, issued) == "1970-01-01T00:00:60.000000001Z",
              "UTC diagnostics preserve leap-second nanoseconds");
    }
    user.reset();
    check(global.live_user_bytes() == 0, "all original reference model User blocks are reclaimed");
    auto next = global.make_user_context();
    bool stale = false;
    try {
        (void)next->get(principal);
    } catch (const ReferenceError&) {
        stale = true;
    }
    check(stale, "expired principal access rejects before its storage is read");
    const auto& p = next->get(next->rebind_global(retained_principal));
    const auto& c = next->get(next->rebind_global(retained_claims));
    const auto& r = next->get(next->rebind_global(retained_resolved));
    const auto& k = next->get(next->rebind_global(retained_key));
    const auto destination = global.memory_resource();
    check(p.subject.empty() && p.authentication_realm.size() == 1024 &&
              p.authentication_realm.get_allocator().resource() == destination &&
              p.domain_root_type.get_allocator().resource() == destination &&
              c.entity_type.size() == 1024 && c.purpose.get_allocator().resource() == destination &&
              r.identity.memory_resource() == destination &&
              r.key_id->get_allocator().resource() == destination &&
              k.key_id.get_allocator().resource() == destination,
          "all reference model strings and nested identities retain explicit Global allocation");
    const auto output = debug_string(next->memory_resource(), r);
    check(output.get_allocator().resource() == next->memory_resource() &&
              output.find("1970-01-01T00:00:60.000000001Z") != output.npos,
          "retained complete diagnostics allocate output in the invoking User");
    check(EntityReferenceTokenError::invalid().source() == nullptr &&
              RoundTripReferenceError::configuration().source() == nullptr,
          "both source Error types have no nested cause");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        retained_models_and_redaction();
        std::cout << "Reference model contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
