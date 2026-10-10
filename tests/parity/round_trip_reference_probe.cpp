#include "fixture_value.hpp"
#include <teaql/runtime/round_trip_reference.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/detail/reference_crypto.hpp>
#include <iostream>
#include <cstdlib>

static String decimal(Memory r, auto number) {
    std::array<char, 32> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number);
    return String(buffer.data(), result.ptr, r);
}

static detail::CryptoBytes hex(Memory r, std::string_view value) {
    detail::CryptoBytes out(r);
    for (std::size_t i = 0; i < value.size(); i += 2) {
        unsigned byte{};
        auto result = std::from_chars(value.data() + i, value.data() + i + 2, byte, 16);
        if (result.ec != std::errc{}) {
            throw std::runtime_error("hex fixture");
        }
        out.push_back(byte);
    }
    return out;
}

static TrustedReferencePrincipal principal(Memory r, const Json& j) {
    auto& a = std::get<Json::Array>(j.data);
    return TrustedReferencePrincipal(std::allocator_arg,
                                     Allocator(r),
                                     text(a[0]),
                                     text(a[1]),
                                     text(a[2]),
                                     number<std::uint64_t>(a[3]));
}

static ReferenceDocumentScope scope(Memory r, const Json& j) {
    auto& a = std::get<Json::Array>(j.data);
    return ReferenceDocumentScope(std::allocator_arg,
                                  Allocator(r),
                                  text(a[0]),
                                  text(a[1]),
                                  text(a[2]),
                                  number<std::uint64_t>(a[3]),
                                  number<std::int64_t>(a[4]));
}

static ReferenceIdentity identity(Memory r, const Json& j) {
    auto& a = std::get<Json::Array>(j.data);
    return ReferenceIdentity(std::allocator_arg,
                             Allocator(r),
                             text(a[0]),
                             number<std::uint64_t>(a[1]),
                             number<std::int64_t>(a[2]));
}

static ExternalEntityReference reference(Memory r, const Json& j) {
    if (auto* token = optional(j, "token")) {
        return ExternalEntityReference(std::allocator_arg, Allocator(r), text(*token));
    }
    return ExternalEntityReference(std::allocator_arg,
                                   Allocator(r),
                                   RawEntityReference{number<std::uint64_t>(field(j, "id")),
                                                      number<std::int64_t>(field(j, "version"))});
}

static Json describe(Memory r, const ExternalEntityReference& ref) {
    auto out = detail::model_json(r);
    if (auto* token = std::get_if<String>(&ref.data)) {
        detail::model_put(out, "token", *token);
    } else {
        auto raw = std::get<RawEntityReference>(ref.data);
        detail::model_put(out, "id", decimal(r, raw.id));
        detail::model_put(out, "version", decimal(r, raw.version));
    }
    return out;
}

static Json describe_identity(Memory r, const ReferenceIdentity& id) {
    Json::Array out(r);
    out.emplace_back(id.entity_type);
    out.emplace_back(decimal(r, id.id));
    out.emplace_back(decimal(r, id.version));
    return Json(r, out);
}

static Json describe_principal(Memory r, const TrustedReferencePrincipal& p) {
    Json::Array out(r);
    out.emplace_back(p.authentication_realm);
    out.emplace_back(p.subject);
    out.emplace_back(p.domain_root_type);
    out.emplace_back(decimal(r, p.domain_root_id));
    return Json(r, out);
}

static Json describe_scope(Memory r, const ReferenceDocumentScope& s) {
    Json::Array out(r);
    out.emplace_back(s.document_id);
    out.emplace_back(s.purpose);
    out.emplace_back(s.aggregate_type);
    out.emplace_back(decimal(r, s.aggregate_id));
    out.emplace_back(decimal(r, s.aggregate_revision));
    return Json(r, out);
}

static Json resolved(Memory r, const ResolvedEntityReference& ref) {
    auto out = detail::model_json(r);
    detail::model_put(out, "identity", describe_identity(r, ref.identity));
    detail::model_put(out, "mode", ref.mode == ReferenceMode::Raw ? "Raw" : "Governed");
    detail::model_put(out, "key", ref.key_id ? Json(r, *ref.key_id) : Json(r));
    detail::model_put(
        out, "issued", ref.issued_at ? Json(r, decimal(r, ref.issued_at->timestamp())) : Json(r));
    detail::model_put(out,
                      "expires",
                      ref.expires_at ? Json(r, decimal(r, ref.expires_at->timestamp())) : Json(r));
    return out;
}

static UtcDateTime time(const Json& j) {
    auto& a = std::get<Json::Array>(j.data);
    return *UtcDateTime::from_timestamp(number<std::int64_t>(a[0]),
                                        detail::model_integer<std::uint32_t>(a[1], "nanos"));
}

static Json notice(Memory r, std::optional<ReferenceStartupNotice> n) {
    if (!n) {
        return Json(r);
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "level", n->level);
    detail::model_put(out, "message", n->message);
    detail::model_put(out, "profile", n->deployment_profile);
    Json::Array telemetry(r);
    telemetry.emplace_back(n->telemetry_key);
    telemetry.emplace_back(n->telemetry_value);
    detail::model_put(out, "telemetry", telemetry);
    Json::Array headers(r);
    for (auto [name, value] : n->response_headers) {
        Json::Array pair(r);
        pair.emplace_back(name);
        pair.emplace_back(value);
        headers.emplace_back(pair);
    }
    detail::model_put(out, "headers", headers);
    return out;
}

static RoundTripReferenceError error(std::string_view code) {
    if (code == "expired") {
        return RoundTripReferenceError::expired();
    }
    if (code == "scope") {
        return RoundTripReferenceError::scope_mismatch();
    }
    if (code == "auth") {
        return RoundTripReferenceError::authorization_required();
    }
    if (code == "invalid") {
        return RoundTripReferenceError::invalid();
    }
    return RoundTripReferenceError::configuration();
}

struct Keys final : ReferenceKeyProvider {
    using allocator_type = Allocator;
    StaticReferenceKeyProvider values;
    Json::Array& calls;
    String config_error, issue_error, lookup_error;
    mutable unsigned current_count = 0;

    Keys(std::allocator_arg_t,
         Allocator a,
         const StaticReferenceKeyProvider& values,
         Json::Array& calls,
         const Json& j)
        : values(std::allocator_arg, a, values), calls(calls),
          config_error(text(field(j, "config_error")), a.resource()),
          issue_error(text(field(j, "issue_error")), a.resource()),
          lookup_error(text(field(j, "lookup_error")), a.resource()) {}

    RoundTripResult<ReferenceKey> current_key(Memory r) const override {
        calls.emplace_back("current");
        auto& failure = current_count++ ? issue_error : config_error;
        if (failure != "none") {
            return RoundTripResult<ReferenceKey>::err(r, error(failure));
        }
        return values.current_key(r);
    }

    RoundTripResult<std::optional<ReferenceKey>> key_by_id(Memory r,
                                                           std::string_view id) const override {
        String label("lookup:", calls.get_allocator().resource());
        label += id;
        calls.emplace_back(label);
        if (lookup_error != "none") {
            return RoundTripResult<std::optional<ReferenceKey>>::err(r, error(lookup_error));
        }
        return values.key_by_id(r, id);
    }
};

struct Policy final : ReferenceAuthorizationPolicy {
    using allocator_type = Allocator;
    Json::Array& calls;
    bool allowed;

    Policy(std::allocator_arg_t, Allocator, Json::Array& calls, bool allowed)
        : calls(calls), allowed(allowed) {}

    ReferenceAuthorizationResult authorize(Memory r,
                                           const TrustedReferencePrincipal& p,
                                           const ReferenceDocumentScope& s,
                                           const ReferenceIdentity& id) const override {
        auto call = detail::model_json(r), args = detail::model_json(r);
        detail::model_put(args, "principal", describe_principal(r, p));
        detail::model_put(args, "scope", describe_scope(r, s));
        detail::model_put(args, "identity", describe_identity(r, id));
        detail::model_put(call, "authorize", args);
        calls.emplace_back(std::move(call));
        return allowed && p.subject == "alice" && id.entity_type == "OrderItem" && id.id == 42
                   ? ReferenceAuthorizationResult::ok(r, {})
                   : ReferenceAuthorizationResult::err(
                         r, RoundTripReferenceError::authorization_required());
    }
};

static Json evaluate(UserContext& user, const Json& j, Json::Array& calls) {
    auto r = user.memory_resource();
    auto op = text(field(j, "op"));
    auto out = detail::model_json(r);
    if (op == "model") {
        auto model = text(field(j, "model"));
        try {
            if (model == "principal") {
                principal(r, field(j, "principal")).validate();
            } else if (model == "scope") {
                scope(r, field(j, "scope")).validate();
            } else if (model == "identity") {
                identity(r, field(j, "identity")).validate();
            } else {
                auto created = ReferenceKey::create(
                    r, text(field(j, "current")), hex(r, text(field(j, "key"))));
                if (!created) {
                    throw created.error();
                }
                detail::model_put(out, "debug", created.value().debug(r));
                return out;
            }
            detail::model_put(out, "ok", true);
        } catch (const RoundTripReferenceError& e) {
            detail::model_put(out, "error", e.code());
        }
        return out;
    }
    if (op == "wire") {
        auto decoded = ReferenceWireCodec::deserialize(
            r,
            field(j, "wire"),
            std::get<bool>(field(j, "raw").data) ? ReferenceMode::Raw : ReferenceMode::Governed);
        if (decoded) {
            detail::model_put(out, "reference", describe(r, decoded.value()));
            detail::model_put(out, "wire", ReferenceWireCodec::serialize(r, decoded.value()));
        } else {
            detail::model_put(out, "error", decoded.error().code());
        }
        return out;
    }
    Vector<ReferenceKey> material(r);
    for (auto& key : std::get<Json::Array>(field(j, "keys").data)) {
        auto& k = std::get<Json::Array>(key.data);
        auto made = ReferenceKey::create(r, text(k[0]), hex(r, text(k[1])));
        if (!made) {
            detail::model_put(out, "error", made.error().code());
            return out;
        }
        material.emplace_back(std::move(made).value());
    }
    auto made = StaticReferenceKeyProvider::create(r, text(field(j, "current")), material);
    if (!made) {
        detail::model_put(out, "error", made.error().code());
        return out;
    }
    if (op == "provider") {
        detail::model_put(out, "current", made.value().current_key(r).value().debug(r));
        auto lookup = made.value().key_by_id(r, text(field(j, "lookup")));
        detail::model_put(
            out, "lookup", lookup.value() ? Json(r, lookup.value()->debug(r)) : Json(r));
        return out;
    }
    auto& keys = user.make<Keys>(made.value(), calls, j);
    auto& policy = user.make<Policy>(calls, std::get<bool>(field(j, "issue_allowed").data));
    ResourceBorrow<const ReferenceKeyProvider> key_borrow(
        std::allocator_arg, Allocator(r), keys, detail::resource_arena(r));
    ResourceBorrow<const ReferenceAuthorizationPolicy> policy_borrow(
        std::allocator_arg, Allocator(r), policy, detail::resource_arena(r));
    auto profile =
        DeploymentProfile(detail::model_integer<unsigned>(field(j, "profile"), "profile"));
    const auto& ack = field(j, "ack");
    if (std::holds_alternative<std::monostate>(ack.data)) {
        ::unsetenv(UNSAFE_EXPOSE_RAW_ENTITY_IDS_ENVIRONMENT.data());
    } else {
        ::setenv(
            UNSAFE_EXPOSE_RAW_ENTITY_IDS_ENVIRONMENT.data(), std::get<String>(ack.data).c_str(), 1);
    }
    auto configured =
        std::get<bool>(field(j, "governed").data)
            ? ContextBoundReferenceRuntime::governed(r,
                                                     profile,
                                                     text(field(j, "service")),
                                                     text(field(j, "environment")),
                                                     key_borrow,
                                                     policy_borrow)
            : ContextBoundReferenceRuntime::from_process_environment(r,
                                                                     profile,
                                                                     text(field(j, "service")),
                                                                     text(field(j, "environment")),
                                                                     key_borrow,
                                                                     policy_borrow);
    if (!configured) {
        detail::model_put(out, "error", configured.error().code());
        return out;
    }
    auto& runtime = user.make<ContextBoundReferenceRuntime>(std::move(configured).value());
    auto& clock = user.make<UtcDateTime>(time(field(j, "time")));
    runtime.with_clock([clock = &clock, calls = &calls] {
        calls->emplace_back("clock");
        return *clock;
    });
    auto nonce_bytes = hex(r, text(field(j, "nonce")));
    detail::CryptoNonce nonce{};
    std::copy(nonce_bytes.begin(), nonce_bytes.end(), nonce.begin());
    runtime.with_nonce_source([nonce, calls = &calls] {
        calls->emplace_back("nonce");
        return nonce;
    });
    bool context = std::get<bool>(field(j, "context").data),
         installed = std::get<bool>(field(j, "installed").data),
         trusted = std::get<bool>(field(j, "principal_installed").data);
    if (installed) {
        user.set_round_trip_reference_runtime(user, runtime);
    }
    if (trusted) {
        user.set_trusted_reference_principal(principal(r, field(j, "principal")));
    }
    detail::model_put(out, "mode", runtime.mode() == ReferenceMode::Raw ? "Raw" : "Governed");
    constexpr std::array names{"Development", "Test", "Production"};
    detail::model_put(out, "profile", names[std::size_t(profile)]);
    detail::model_put(out, "notice", notice(r, runtime.startup_notice()));
    if (op == "config") {
        return out;
    }
    if (op == "mode") {
        auto m = user.reference_mode();
        if (m) {
            detail::model_put(out, "result", m.value() == ReferenceMode::Raw ? "Raw" : "Governed");
        } else {
            detail::model_put(out, "error", m.error().code());
        }
        return out;
    }
    if (op == "notice") {
        auto n = user.reference_startup_notice();
        if (n) {
            detail::model_put(out, "result", notice(r, n.value()));
        } else {
            detail::model_put(out, "error", n.error().code());
        }
        return out;
    }
    if (op == "serialize") {
        auto ref = reference(r, field(j, "reference"));
        auto result = user.serialize_reference(ref);
        if (result) {
            detail::model_put(out, "wire", result.value());
        } else {
            detail::model_put(out, "error", result.error().code());
        }
        return out;
    }
    if (op == "deserialize") {
        auto result = user.deserialize_reference(field(j, "wire"));
        if (result) {
            detail::model_put(out, "reference", describe(r, result.value()));
        } else {
            detail::model_put(out, "error", result.error().code());
        }
        return out;
    }
    auto issued = [&] {
        if (op == "consume") {
            return RoundTripResult<ExternalEntityReference>::ok(
                r, reference(r, field(j, "reference")));
        }
        auto& d = std::get<Json::Array>(field(j, "lifetime").data);
        ReferenceLifetime duration(number<std::uint64_t>(d[0]),
                                   detail::model_integer<std::uint32_t>(d[1], "duration nanos"));
        auto id = identity(r, field(j, "identity"));
        auto sc = scope(r, field(j, "scope"));
        auto p = principal(r, field(j, "principal"));
        auto issued =
            context ? user.reference_for(id, sc, duration) : runtime.issue(r, p, sc, id, duration);
        return issued;
    }();
    if (!issued) {
        detail::model_put(out, "error", issued.error().code());
        return out;
    }
    auto ref = std::move(issued).value();
    detail::model_put(out, "reference", describe(r, ref));
    if (op == "issue") {
        return out;
    }
    policy.allowed = std::get<bool>(field(j, "consume_allowed").data);
    clock = time(field(j, "consume_time"));
    auto p = principal(r, field(j, "consume_principal"));
    auto sc = scope(r, field(j, "consume_scope"));
    if (context && trusted) {
        user.set_trusted_reference_principal(p);
    }
    auto decoded = context ? user.resolve_reference(ref, text(field(j, "expected_entity")), sc)
                           : runtime.consume(r, p, sc, ref, text(field(j, "expected_entity")));
    if (decoded) {
        detail::model_put(out, "resolved", resolved(r, decoded.value()));
    } else {
        detail::model_put(out, "consume_error", decoded.error().code());
    }
    return out;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            return 2;
        }
        auto& calls = user->make<Json::Array>();
        Json result(r);
        try {
            result = evaluate(*user, *input, calls);
        } catch (const RoundTripReferenceError& e) {
            result = detail::model_json(r);
            detail::model_put(result, "error", e.code());
        } catch (const std::out_of_range&) {
            result = detail::model_json(r);
            detail::model_put(result, "panic", true);
        }
        detail::model_put(result, "calls", calls);
        std::cout << result.dump(r) << '\n';
    }
}
