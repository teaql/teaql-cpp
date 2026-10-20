#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/core/result.hpp>
#include <optional>

namespace teaql {
// Configuration values have the explicit allocation domain of their owner.
struct GeneratedServiceRuntimeConfig {
    using allocator_type = Allocator;
    String database_url;

    GeneratedServiceRuntimeConfig(std::allocator_arg_t, Allocator a, std::string_view url)
        : database_url(url, a.resource()) {}

    GeneratedServiceRuntimeConfig(std::allocator_arg_t,
                                  Allocator a,
                                  const GeneratedServiceRuntimeConfig& other)
        : GeneratedServiceRuntimeConfig(std::allocator_arg, a, other.database_url) {}

    GeneratedServiceRuntimeConfig(std::allocator_arg_t,
                                  Allocator a,
                                  GeneratedServiceRuntimeConfig&& other)
        : GeneratedServiceRuntimeConfig(detail::move_to(a.resource(), std::move(other))) {}

    GeneratedServiceRuntimeConfig(const GeneratedServiceRuntimeConfig&) = delete;
    GeneratedServiceRuntimeConfig(GeneratedServiceRuntimeConfig&&) noexcept = default;

    GeneratedServiceRuntimeConfig& operator=(GeneratedServiceRuntimeConfig&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return database_url.get_allocator().resource();
    }

    bool operator==(const GeneratedServiceRuntimeConfig& other) const {
        return database_url == other.database_url;
    }
};

// This is the source VarError responsibility, separate from connection failure.
class GeneratedEnvironmentError {
public:
    using allocator_type = Allocator;
    enum class Kind { NotPresent, NotUnicode };

    GeneratedEnvironmentError(std::allocator_arg_t,
                              Allocator a,
                              Kind kind,
                              std::string_view bytes = {})
        : kind_(kind), bytes_(bytes, a.resource()) {}

    GeneratedEnvironmentError(std::allocator_arg_t,
                              Allocator a,
                              const GeneratedEnvironmentError& other)
        : GeneratedEnvironmentError(std::allocator_arg, a, other.kind_, other.bytes_) {}

    GeneratedEnvironmentError(std::allocator_arg_t, Allocator a, GeneratedEnvironmentError&& other)
        : GeneratedEnvironmentError(detail::move_to(a.resource(), std::move(other))) {}

    GeneratedEnvironmentError(const GeneratedEnvironmentError&) = delete;
    GeneratedEnvironmentError(GeneratedEnvironmentError&&) noexcept = default;

    GeneratedEnvironmentError& operator=(GeneratedEnvironmentError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return bytes_.get_allocator().resource();
    }

    Kind kind() const noexcept {
        return kind_;
    }

    std::string_view bytes() const noexcept {
        return bytes_;
    }

    String to_string(Memory) const;
    friend void write_debug(DebugWriter&, const GeneratedEnvironmentError&);

private:
    Kind kind_;
    String bytes_;
};

// Configuration/connection preparation precedes provider execution. Its three
// source branches stay distinct from the provider and RuntimeError carriers.
class GeneratedRuntimeConfigurationError {
public:
    using allocator_type = Allocator;
    enum class Kind { MissingEnv, ConnectionError, Configuration };

    static GeneratedRuntimeConfigurationError
    missing_env(Memory, std::string_view name, const GeneratedEnvironmentError&);
    static GeneratedRuntimeConfigurationError connection_error(Memory, std::string_view);
    static GeneratedRuntimeConfigurationError configuration(Memory, std::string_view);

    GeneratedRuntimeConfigurationError(std::allocator_arg_t,
                                       Allocator,
                                       const GeneratedRuntimeConfigurationError&);

    GeneratedRuntimeConfigurationError(std::allocator_arg_t,
                                       Allocator a,
                                       GeneratedRuntimeConfigurationError&& other)
        : GeneratedRuntimeConfigurationError(detail::move_to(a.resource(), std::move(other))) {}

    GeneratedRuntimeConfigurationError(const GeneratedRuntimeConfigurationError&) = delete;
    GeneratedRuntimeConfigurationError(GeneratedRuntimeConfigurationError&&) noexcept = default;

    GeneratedRuntimeConfigurationError& operator=(GeneratedRuntimeConfigurationError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return message_.get_allocator().resource();
    }

    Kind kind() const noexcept {
        return kind_;
    }

    std::string_view environment_name() const noexcept {
        return name_;
    }

    const std::optional<GeneratedEnvironmentError>& environment_error() const noexcept {
        return environment_error_;
    }

    String to_string(Memory) const;
    std::optional<String> source(Memory) const;
    friend void write_debug(DebugWriter&, const GeneratedRuntimeConfigurationError&);

private:
    GeneratedRuntimeConfigurationError(Memory, Kind, std::string_view);
    Kind kind_;
    String name_, message_;
    std::optional<GeneratedEnvironmentError> environment_error_;
};

Result<GeneratedServiceRuntimeConfig, GeneratedRuntimeConfigurationError>
read_generated_runtime_config(Memory, std::string_view environment_name);

Result<String, GeneratedEnvironmentError>
read_generated_environment_value(Memory, std::string_view environment_name);

void write_debug(DebugWriter&, const GeneratedServiceRuntimeConfig&);
}
