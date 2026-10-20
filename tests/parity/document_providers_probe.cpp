#define main retained_document_provider_fixture_main
#include "document_round_trip_probe.cpp"
#undef main
#include <teaql/runtime/document_model_debug.hpp>

static void provider_put(Json::Object& object, std::string_view key, Json value) {
    object.emplace(String(key, object.get_allocator().resource()), std::move(value));
}

struct ProviderState {
    Memory resource;
    const Json& input;
    std::string_view stage = "startup";
    bool rotated = false;
    std::size_t authorizations = 0;
    std::uint8_t nonces = 0;
    Json::Array events;

    ProviderState(Memory r, const Json& input) : resource(r), input(input), events(r) {}

    void set_stage(std::string_view next) {
        stage = next;
        authorizations = 0;
    }

    bool flag(std::string_view key) const {
        return std::get<bool>(field(input, key).data);
    }

    std::string_view string(std::string_view key) const {
        return text(field(input, key));
    }

    void event(Json::Object value) {
        provider_put(value, "stage", Json(resource, stage));
        events.emplace_back(value);
    }

    std::string_view current_id() const {
        return rotated ? "k2" : "k1";
    }
};

static std::optional<RoundTripReferenceError> provider_error(std::string_view error) {
    if (error == "invalid") {
        return RoundTripReferenceError::invalid();
    }
    if (error == "expired") {
        return RoundTripReferenceError::expired();
    }
    if (error == "scope") {
        return RoundTripReferenceError::scope_mismatch();
    }
    if (error == "authorization") {
        return RoundTripReferenceError::authorization_required();
    }
    if (error == "configuration") {
        return RoundTripReferenceError::configuration();
    }
    return std::nullopt;
}

struct ProviderKeys final : ReferenceKeyProvider {
    using allocator_type = Allocator;
    ProviderState& state;

    ProviderKeys(std::allocator_arg_t, Allocator, ProviderState& state) : state(state) {}

    RoundTripResult<ReferenceKey> current_key(Memory r) const override {
        auto id = state.current_id();
        Json::Object event(state.resource);
        provider_put(event, "kind", Json(state.resource, "current"));
        provider_put(event, "key", Json(state.resource, id));
        state.event(std::move(event));
        auto key = state.stage == "startup" ? "startup_error" : "current_error";
        if (auto error = provider_error(state.string(key))) {
            return RoundTripResult<ReferenceKey>::err(r, *error);
        }
        detail::CryptoKey bytes;
        bytes.fill(id == "k1" ? 0x42 : 0x24);
        return ReferenceKey::create(r, id, bytes);
    }

    RoundTripResult<std::optional<ReferenceKey>> key_by_id(Memory r,
                                                           std::string_view id) const override {
        Json::Object event(state.resource);
        provider_put(event, "kind", Json(state.resource, "lookup"));
        provider_put(event, "key", Json(state.resource, id));
        state.event(std::move(event));
        using Output = RoundTripResult<std::optional<ReferenceKey>>;
        if (auto error = provider_error(state.string("lookup_error"))) {
            return Output::err(r, *error);
        }
        auto mode = state.string("lookup_mode");
        std::optional<ReferenceKey> value;
        if (mode != "missing") {
            auto returned = mode == "mismatch" ? std::string_view("k2") : id;
            detail::CryptoKey bytes;
            bytes.fill(mode == "wrong_master" ? 0x17 : returned == "k1" ? 0x42 : 0x24);
            value.emplace(std::allocator_arg, Allocator(r), returned, bytes);
        }
        return Output::ok(r, std::move(value));
    }
};

struct ProviderPolicy final : ReferenceAuthorizationPolicy {
    using allocator_type = Allocator;
    ProviderState& state;

    ProviderPolicy(std::allocator_arg_t, Allocator, ProviderState& state) : state(state) {}

    ReferenceAuthorizationResult authorize(Memory r,
                                           const TrustedReferencePrincipal& principal,
                                           const ReferenceDocumentScope& scope,
                                           const ReferenceIdentity& identity) const override {
        ++state.authorizations;
        auto g = state.resource;
        Json::Object event(g), actor(g), document(g), entity(g);
        provider_put(event, "kind", Json(g, "authorize"));
        provider_put(actor, "realm", Json(g, principal.authentication_realm));
        provider_put(actor, "subject", Json(g, principal.subject));
        provider_put(actor, "root_type", Json(g, principal.domain_root_type));
        provider_put(actor, "root_id", Json(g, principal.domain_root_id));
        provider_put(document, "document", Json(g, scope.document_id));
        provider_put(document, "purpose", Json(g, scope.purpose));
        provider_put(document, "type", Json(g, scope.aggregate_type));
        provider_put(document, "id", Json(g, scope.aggregate_id));
        provider_put(document, "revision", Json(g, scope.aggregate_revision));
        provider_put(entity, "type", Json(g, identity.entity_type));
        provider_put(entity, "id", Json(g, identity.id));
        provider_put(entity, "version", Json(g, identity.version));
        provider_put(event, "principal", Json(g, actor));
        provider_put(event, "scope", Json(g, document));
        provider_put(event, "identity", Json(g, entity));
        state.event(std::move(event));
        auto prefix = state.stage == "consume" ? "consume" : "issue";
        String at("deny_", g);
        at += prefix;
        String error_key(at, g);
        error_key += "_error";
        at += "_at";
        if (static_cast<std::size_t>(number(state.input, at)) == state.authorizations) {
            if (auto error = provider_error(state.string(error_key))) {
                return ReferenceAuthorizationResult::err(r, *error);
            }
        }
        return ReferenceAuthorizationResult::ok(r, {});
    }
};

static ContextBoundReferenceRuntime& provider_runtime(UserContext& user_context,
                                                      ProviderState& state,
                                                      ProviderKeys& keys,
                                                      ProviderPolicy& policy,
                                                      std::string_view service) {
    auto r = user_context.memory_resource();
    ResourceBorrow<const ReferenceKeyProvider> key_borrow(
        std::allocator_arg, Allocator(r), keys, detail::resource_arena(r));
    ResourceBorrow<const ReferenceAuthorizationPolicy> policy_borrow(
        std::allocator_arg, Allocator(r), policy, detail::resource_arena(r));
    auto& runtime = user_context.make<ContextBoundReferenceRuntime>(
        DeploymentProfile::Test, service, "test", key_borrow, policy_borrow);
    runtime.with_clock([state = &state] {
        auto now =
            std::int64_t(1800000000) + (state->stage == "consume"   ? number(state->input, "delta")
                                        : state->stage == "reissue" ? 1
                                                                    : 0);
        auto nanos = static_cast<std::uint32_t>(number(state->input, "nanos"));
        Json::Object event(state->resource);
        provider_put(event, "kind", Json(state->resource, "clock"));
        provider_put(event, "time", Json(state->resource, now));
        provider_put(event, "nanos", Json(state->resource, nanos));
        state->event(std::move(event));
        return *UtcDateTime::from_timestamp(now, nanos);
    });
    runtime.with_nonce_source([state = &state] {
        auto seed = static_cast<std::uint8_t>(number(state->input, "seed") + state->nonces++);
        Json::Object event(state->resource);
        provider_put(event, "kind", Json(state->resource, "nonce"));
        provider_put(event, "seed", Json(state->resource, seed));
        state->event(std::move(event));
        detail::CryptoNonce bytes;
        bytes.fill(seed);
        return bytes;
    });
    return runtime;
}

static Json provider_issued(Memory r, const DocumentResult<String>& value) {
    Json::Object result(r);
    provider_put(result,
                 value ? "token" : "error",
                 Json(r, value ? std::string_view(value.value()) : value.error().code()));
    return Json(r, result);
}

static void provider_register(UserContext& user_context,
                              ContextBoundReferenceRuntime& runtime,
                              const TrustedReferencePrincipal& actor,
                              bool builders,
                              bool has_runtime,
                              bool has_principal) {
    if (has_runtime) {
        if (builders) {
            user_context.with_round_trip_reference_runtime(user_context, runtime);
        } else {
            user_context.set_round_trip_reference_runtime(user_context, runtime);
        }
    }
    if (has_principal) {
        if (builders) {
            user_context.with_trusted_reference_principal(actor);
        } else {
            user_context.set_trusted_reference_principal(actor);
        }
    }
}

static Json provider_observe(GlobalContext& global, const Json& input) {
    auto g = global.memory_resource();
    ProviderState state(g, input);
    auto user_context_owner = global.make_user_context();
    auto output = [&] {
        auto& user_context = *user_context_owner;
        auto r = user_context.memory_resource();
        auto& keys = user_context.make<ProviderKeys>(state);
        auto& policy = user_context.make<ProviderPolicy>(state);
        ContextBoundReferenceRuntime* runtime = nullptr;
        Json::Object result(g);
        try {
            runtime = &provider_runtime(user_context, state, keys, policy, "order-service");
        } catch (const RoundTripReferenceError& error) {
            provider_put(result, "startup_error", Json(g, error.code()));
            return Json(g, result);
        }
        provider_put(result, "startup_error", Json(g));
        auto actor = principal(r, "alice");
        if (state.string("invalid") == "principal") {
            actor.subject.clear();
        }
        provider_register(user_context,
                          *runtime,
                          actor,
                          state.flag("builders"),
                          state.flag("has_runtime"),
                          state.flag("has_principal"));
        auto value = snapshot(r, input);
        if (state.string("invalid") == "snapshot") {
            value.model_fingerprint.clear();
        }
        ReferenceLifetime lifetime(state.string("invalid") == "lifetime" ? 0 : 60);
        auto context = state.string("entry") == "context";
        state.set_stage("issue");
        auto issued = context ? user_context.issue_document_snapshot(value, lifetime)
                              : runtime->issue_document_snapshot(r, actor, value, lifetime);
        provider_put(result, "issued", provider_issued(g, issued));
        if (!issued) {
            return Json(g, result);
        }
        const auto& token = issued.value();
        state.rotated = state.flag("rotation");
        if (state.flag("reissue")) {
            state.set_stage("reissue");
            auto reissued = context ? user_context.issue_document_snapshot(value, lifetime)
                                    : runtime->issue_document_snapshot(r, actor, value, lifetime);
            provider_put(result, "reissued", provider_issued(g, reissued));
        }
        if (state.flag("replace")) {
            state.set_stage("replace");
            runtime = &provider_runtime(user_context,
                                        state,
                                        keys,
                                        policy,
                                        state.string("namespace") == "different" ? "other-service"
                                                                                 : "order-service");
            auto replacement_actor =
                principal(r, state.string("actor") == "different" ? "bob" : "alice");
            actor = std::move(replacement_actor);
            provider_register(user_context, *runtime, actor, state.flag("builders"), true, true);
        }
        state.set_stage("consume");
        auto consumed =
            context ? user_context.consume_document_snapshot(token, "Order", "edit-order")
                    : runtime->consume_document_snapshot(r, actor, token, "Order", "edit-order");
        Json::Object decoded(g);
        if (!consumed) {
            provider_put(decoded, "error", Json(g, consumed.error().code()));
        } else {
            const auto& verified = consumed.value();
            provider_put(decoded, "snapshot", Json(g, debug_string(g, verified.snapshot)));
            provider_put(decoded, "equal", Json(g, verified.snapshot == value));
            provider_put(decoded, "key", Json(g, verified.key_id));
            provider_put(decoded, "issued", Json(g, verified.issued_at.timestamp()));
            provider_put(decoded, "expires", Json(g, verified.expires_at.timestamp()));
        }
        provider_put(result, "consumed", Json(g, decoded));
        return Json(g, result);
    }();
    user_context_owner.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Custom document provider retained UserContext storage");
    }
    provider_put(std::get<Json::Object>(output.data), "events", Json(g, state.events));
    return output;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid custom document provider input");
            }
            auto output = provider_observe(global, *input);
            std::cout << output.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
