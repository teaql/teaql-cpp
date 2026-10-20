#include <teaql/runtime/generated_runtime_config.hpp>
#include <cstdlib>

namespace teaql {
namespace {
// Return the length of a valid Unicode scalar at offset, or zero. Environment
// bytes are copied immediately; no process cache retains them.
std::size_t utf8_scalar_size(std::string_view text, std::size_t offset) noexcept {
    auto first = static_cast<unsigned char>(text[offset]);
    if (first < 0x80) {
        return 1;
    }
    std::size_t size = first >= 0xc2 && first <= 0xdf   ? 2
                       : first >= 0xe0 && first <= 0xef ? 3
                       : first >= 0xf0 && first <= 0xf4 ? 4
                                                        : 0;
    if (!size || size > text.size() - offset) {
        return 0;
    }
    for (std::size_t index = 1; index < size; ++index) {
        auto byte = static_cast<unsigned char>(text[offset + index]);
        if (byte < 0x80 || byte > 0xbf) {
            return 0;
        }
    }
    auto second = static_cast<unsigned char>(text[offset + 1]);
    if ((first == 0xe0 && second < 0xa0) || (first == 0xed && second >= 0xa0) ||
        (first == 0xf0 && second < 0x90) || (first == 0xf4 && second >= 0x90)) {
        return 0;
    }
    return size;
}

bool valid_environment_unicode(std::string_view text) noexcept {
    for (std::size_t offset = 0; offset < text.size();) {
        auto size = utf8_scalar_size(text, offset);
        if (!size) {
            return false;
        }
        offset += size;
    }
    return true;
}

String debug_environment_bytes(Memory resource, std::string_view bytes) {
    String output("\"", resource);
    constexpr char digits[] = "0123456789ABCDEF";
    for (std::size_t offset = 0; offset < bytes.size();) {
        auto size = utf8_scalar_size(bytes, offset);
        if (size) {
            auto begin = offset;
            do {
                offset += size;
                size = offset == bytes.size() ? 0 : utf8_scalar_size(bytes, offset);
            } while (size);
            auto quoted = detail::debug_text(resource, bytes.substr(begin, offset - begin));
            output.append(quoted, 1, quoted.size() - 2);
        } else {
            auto byte = static_cast<unsigned char>(bytes[offset++]);
            output += "\\x";
            output += digits[byte >> 4];
            output += digits[byte & 0x0f];
        }
    }
    output += '"';
    return output;
}
}

String GeneratedEnvironmentError::to_string(Memory resource) const {
    if (kind_ == Kind::NotPresent) {
        return String("environment variable not found", resource);
    }
    String message("environment variable was not valid unicode: ", resource);
    message += debug_environment_bytes(resource, bytes_);
    return message;
}

void write_debug(DebugWriter& writer, const GeneratedEnvironmentError& error) {
    if (error.kind_ == GeneratedEnvironmentError::Kind::NotPresent) {
        writer.raw("NotPresent");
    } else {
        writer.raw("NotUnicode(");
        writer.raw(debug_environment_bytes(writer.memory_resource(), error.bytes_));
        writer.raw(")");
    }
}

GeneratedRuntimeConfigurationError::GeneratedRuntimeConfigurationError(Memory resource,
                                                                       Kind kind,
                                                                       std::string_view message)
    : kind_(kind), name_(resource), message_(message, resource) {}

GeneratedRuntimeConfigurationError::GeneratedRuntimeConfigurationError(
    std::allocator_arg_t, Allocator allocator, const GeneratedRuntimeConfigurationError& other)
    : kind_(other.kind_), name_(other.name_, allocator.resource()),
      message_(other.message_, allocator.resource()) {
    if (other.environment_error_) {
        environment_error_.emplace(std::allocator_arg, allocator, *other.environment_error_);
    }
}

GeneratedRuntimeConfigurationError GeneratedRuntimeConfigurationError::missing_env(
    Memory resource, std::string_view name, const GeneratedEnvironmentError& source) {
    GeneratedRuntimeConfigurationError error(resource, Kind::MissingEnv, {});
    error.name_ = name;
    error.environment_error_.emplace(std::allocator_arg, Allocator(resource), source);
    return error;
}

GeneratedRuntimeConfigurationError
GeneratedRuntimeConfigurationError::connection_error(Memory resource, std::string_view message) {
    return GeneratedRuntimeConfigurationError(resource, Kind::ConnectionError, message);
}

GeneratedRuntimeConfigurationError
GeneratedRuntimeConfigurationError::configuration(Memory resource, std::string_view message) {
    return GeneratedRuntimeConfigurationError(resource, Kind::Configuration, message);
}

String GeneratedRuntimeConfigurationError::to_string(Memory resource) const {
    if (kind_ == Kind::MissingEnv) {
        String message("missing environment variable ", resource);
        message += name_;
        message += ": ";
        message += environment_error_->to_string(resource);
        return message;
    }
    String message(kind_ == Kind::ConnectionError ? "connection error: " : "configuration error: ",
                   resource);
    message += message_;
    return message;
}

std::optional<String> GeneratedRuntimeConfigurationError::source(Memory resource) const {
    if (environment_error_) {
        return environment_error_->to_string(resource);
    }
    return std::nullopt;
}

void write_debug(DebugWriter& writer, const GeneratedRuntimeConfigurationError& error) {
    if (error.kind_ == GeneratedRuntimeConfigurationError::Kind::MissingEnv) {
        writer.raw("MissingEnv { name: ");
        writer.text(error.name_);
        writer.raw(", source: ");
        write_debug(writer, *error.environment_error_);
        writer.raw(" }");
    } else {
        writer.raw(error.kind_ == GeneratedRuntimeConfigurationError::Kind::ConnectionError
                       ? "ConnectionError("
                       : "Configuration(");
        writer.text(error.message_);
        writer.raw(")");
    }
}

Result<String, GeneratedEnvironmentError>
read_generated_environment_value(Memory resource, std::string_view environment_name) {
    using Output = Result<String, GeneratedEnvironmentError>;
    String name(environment_name, resource);
    const char* value = name.find('\0') == name.npos ? std::getenv(name.c_str()) : nullptr;
    if (!value) {
        GeneratedEnvironmentError source(
            std::allocator_arg, Allocator(resource), GeneratedEnvironmentError::Kind::NotPresent);
        return Output::err(resource, std::move(source));
    }
    String bytes(value, resource);
    if (!valid_environment_unicode(bytes)) {
        GeneratedEnvironmentError source(std::allocator_arg,
                                         Allocator(resource),
                                         GeneratedEnvironmentError::Kind::NotUnicode,
                                         bytes);
        return Output::err(resource, std::move(source));
    }
    return Output::ok(resource, std::move(bytes));
}

Result<GeneratedServiceRuntimeConfig, GeneratedRuntimeConfigurationError>
read_generated_runtime_config(Memory resource, std::string_view environment_name) {
    using Output = Result<GeneratedServiceRuntimeConfig, GeneratedRuntimeConfigurationError>;
    auto value = read_generated_environment_value(resource, environment_name);
    if (!value) {
        return Output::err(resource,
                           GeneratedRuntimeConfigurationError::missing_env(
                               resource, environment_name, value.error()));
    }
    return Output::ok(
        resource,
        GeneratedServiceRuntimeConfig(std::allocator_arg, Allocator(resource), value.value()));
}

void write_debug(DebugWriter& writer, const GeneratedServiceRuntimeConfig& config) {
    writer.raw("ServiceRuntimeConfig { database_url: ");
    writer.text(config.database_url);
    writer.raw(" }");
}
}
