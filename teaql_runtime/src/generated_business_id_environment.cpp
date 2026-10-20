#include <teaql/runtime/generated_business_id_environment.hpp>
#include <teaql/runtime/context.hpp>
#include <charconv>
#include <stdexcept>

namespace teaql {
namespace {
constexpr std::string_view key_environment = "TEAQL_BUSINESS_ID_KEY_V1_HEX";
constexpr std::string_view version_environment = "TEAQL_BUSINESS_ID_KEY_VERSION";

int hex_digit(char value) {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}
}

Result<BusinessIdEncodingKey, GeneratedRuntimeConfigurationError>
read_generated_business_id_key(Memory resource) {
    using Output = Result<BusinessIdEncodingKey, GeneratedRuntimeConfigurationError>;
    auto encoded = read_generated_environment_value(resource, key_environment);
    if (!encoded) {
        return Output::err(resource,
                           GeneratedRuntimeConfigurationError::missing_env(
                               resource, key_environment, encoded.error()));
    }
    if (encoded.value().size() != 64) {
        return Output::err(
            resource,
            GeneratedRuntimeConfigurationError::configuration(
                resource,
                "TEAQL_BUSINESS_ID_KEY_V1_HEX must contain exactly 64 hexadecimal characters"));
    }
    std::array<std::uint8_t, 32> bytes{};
    for (std::size_t index = 0; index != bytes.size(); ++index) {
        auto end = (index + 1) * 2;
        if (end < encoded.value().size() &&
            (static_cast<unsigned char>(encoded.value()[end]) & 0xc0) == 0x80) {
            throw std::out_of_range("Business-ID key slice is not on a UTF-8 character boundary");
        }
        char first = encoded.value()[index * 2];
        int high = first == '+' ? 0 : hex_digit(first);
        int low = hex_digit(encoded.value()[index * 2 + 1]);
        if (high < 0 || low < 0) {
            return Output::err(
                resource,
                GeneratedRuntimeConfigurationError::configuration(
                    resource,
                    "TEAQL_BUSINESS_ID_KEY_V1_HEX must contain only hexadecimal characters"));
        }
        bytes[index] = static_cast<std::uint8_t>((high << 4) | low);
    }
    auto encoded_version = read_generated_environment_value(resource, version_environment);
    std::string_view version_text =
        encoded_version ? std::string_view(encoded_version.value()) : "1";
    if (version_text.starts_with('+')) {
        version_text.remove_prefix(1);
    }
    std::uint32_t version = 0;
    auto parsed =
        std::from_chars(version_text.data(), version_text.data() + version_text.size(), version);
    if (parsed.ec != std::errc{} || parsed.ptr != version_text.data() + version_text.size()) {
        return Output::err(resource,
                           GeneratedRuntimeConfigurationError::configuration(
                               resource, "TEAQL_BUSINESS_ID_KEY_VERSION must be a positive u32"));
    }
    auto key = BusinessIdEncodingKey::create(resource, version, bytes);
    if (!key) {
        return Output::err(resource,
                           GeneratedRuntimeConfigurationError::configuration(
                               resource, key.error().to_string(resource)));
    }
    return Output::ok(resource, key.value());
}

Result<std::monostate, GeneratedRuntimeConfigurationError>
install_generated_business_id_environment_key(UserContext& user_context) {
    using Output = Result<std::monostate, GeneratedRuntimeConfigurationError>;
    auto resource = user_context.memory_resource();
    auto key = read_generated_business_id_key(resource);
    if (!key) {
        return Output::err(resource, std::move(key).error());
    }
    auto& provider = user_context.make<StaticBusinessIdKeyProvider>(key.value());
    auto installed = user_context.set_business_id_key_provider(user_context, provider);
    if (!installed) {
        return Output::err(resource,
                           GeneratedRuntimeConfigurationError::configuration(
                               resource, installed.error().to_string(resource)));
    }
    return Output::ok(resource, {});
}
}
