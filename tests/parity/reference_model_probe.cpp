#include <teaql/runtime/reference_model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "debug_modes.hpp"
#include <iostream>

static UtcDateTime time_value(const Json& input) {
    const auto& values = std::get<Json::Array>(input.data);
    return UtcDateTime::from_timestamp(
               number<std::int64_t>(values[0]),
               detail::model_integer<std::uint32_t>(values[1], "nanoseconds"))
        .value();
}

static ReferenceIdentity identity(Memory owner, const Json& input) {
    return ReferenceIdentity(std::allocator_arg,
                             Allocator(owner),
                             text(field(input, "entity")),
                             number<std::uint64_t>(field(input, "id")),
                             number<std::int64_t>(field(input, "version")));
}

static ReferenceMode mode(const Json& input) {
    return std::get<bool>(field(input, "raw").data) ? ReferenceMode::Raw : ReferenceMode::Governed;
}

template <class T>
static Json snapshot(Memory owner, const T& value) {
    auto copy = detail::clone_in(owner, value);
    Json::Object output(owner);
    output.emplace("debug", formatted_debug(owner, value));
    output.emplace("clone_debug", formatted_debug(owner, copy));
    if constexpr (requires { value == copy; }) {
        output.emplace("equal", Json(owner, value == copy));
    }
    return Json(owner, std::move(output));
}

template <class T>
static Json error_snapshot(Memory owner, const T& value) {
    auto output = snapshot(owner, value);
    auto& fields = std::get<Json::Object>(output.data);
    fields.emplace("display", Json(owner, value.code()));
    fields.emplace("source_none", Json(owner, value.source() == nullptr));
    return output;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto kind = text(field(input, "kind"));
    if (kind == "claims") {
        auto value = EntityReferenceClaims(
            std::allocator_arg,
            Allocator(owner),
            text(field(input, "entity")),
            number<std::uint64_t>(field(input, "id")),
            number<std::int64_t>(field(input, "version")),
            time_value(field(input, "issued")),
            time_value(field(input, "expires")),
            text(field(input, "purpose")),
            detail::model_integer<std::uint32_t>(field(input, "key_version"), "key_version"));
        return snapshot(owner, value);
    }
    if (kind == "principal") {
        auto value = TrustedReferencePrincipal(std::allocator_arg,
                                               Allocator(owner),
                                               text(field(input, "realm")),
                                               text(field(input, "subject")),
                                               text(field(input, "root")),
                                               number<std::uint64_t>(field(input, "id")));
        return snapshot(owner, value);
    }
    if (kind == "document") {
        auto value = ReferenceDocumentScope(std::allocator_arg,
                                            Allocator(owner),
                                            text(field(input, "document")),
                                            text(field(input, "purpose")),
                                            text(field(input, "aggregate")),
                                            number<std::uint64_t>(field(input, "id")),
                                            number<std::int64_t>(field(input, "version")));
        return snapshot(owner, value);
    }
    if (kind == "identity") {
        return snapshot(owner, identity(owner, input));
    }
    if (kind == "external") {
        if (mode(input) == ReferenceMode::Raw) {
            return snapshot(owner,
                            ExternalEntityReference(
                                std::allocator_arg,
                                Allocator(owner),
                                RawEntityReference{number<std::uint64_t>(field(input, "id")),
                                                   number<std::int64_t>(field(input, "version"))}));
        }
        return snapshot(owner,
                        ExternalEntityReference(
                            std::allocator_arg, Allocator(owner), text(field(input, "token"))));
    }
    if (kind == "resolved") {
        std::optional<std::string_view> key;
        std::optional<UtcDateTime> issued, expires;
        if (auto* value = std::get_if<String>(&field(input, "key").data)) {
            key = *value;
        }
        if (!std::holds_alternative<std::monostate>(field(input, "issued").data)) {
            issued = time_value(field(input, "issued"));
        }
        if (!std::holds_alternative<std::monostate>(field(input, "expires").data)) {
            expires = time_value(field(input, "expires"));
        }
        return snapshot(owner,
                        ResolvedEntityReference(std::allocator_arg,
                                                Allocator(owner),
                                                identity(owner, input),
                                                mode(input),
                                                key,
                                                issued,
                                                expires));
    }
    if (kind == "profile") {
        return snapshot(owner,
                        static_cast<DeploymentProfile>(
                            detail::model_integer<unsigned>(field(input, "variant"), "variant")));
    }
    if (kind == "mode") {
        return snapshot(owner, mode(input));
    }
    if (kind == "wire") {
        return snapshot(owner, ReferenceWireCodec{});
    }
    if (kind == "key") {
        detail::CryptoKey bytes{};
        bytes.fill(detail::model_integer<std::uint8_t>(field(input, "byte"), "byte"));
        return snapshot(
            owner,
            ReferenceKey(std::allocator_arg, Allocator(owner), text(field(input, "key")), bytes));
    }
    if (kind == "notice") {
        ReferenceStartupNotice value{
            text(field(input, "level")),
            text(field(input, "message")),
            text(field(input, "profile")),
            text(field(input, "telemetry_key")),
            text(field(input, "telemetry_value")),
            {{{"TeaQL-Reference-Mode", "raw-internal-id"}, {"Cache-Control", "no-store"}}}};
        return snapshot(owner, value);
    }
    if (kind == "token_error") {
        return error_snapshot(owner,
                              detail::model_integer<unsigned>(field(input, "variant"), "variant") ==
                                      0
                                  ? EntityReferenceTokenError::invalid()
                                  : EntityReferenceTokenError::codec_required());
    }
    const auto errors = std::array{RoundTripReferenceError::invalid(),
                                   RoundTripReferenceError::expired(),
                                   RoundTripReferenceError::scope_mismatch(),
                                   RoundTripReferenceError::authorization_required(),
                                   RoundTripReferenceError::configuration()};
    return error_snapshot(
        owner, errors.at(detail::model_integer<unsigned>(field(input, "variant"), "variant")));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid reference model fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
