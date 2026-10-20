#pragma once
#include <teaql/runtime/generated_runtime_config.hpp>
#include <teaql/runtime/error_debug.hpp>
#include <variant>

namespace teaql {
// Preserve the generated service error's complete source alternatives without
// making the runtime depend on a concrete provider. Every payload has one scope.
template <class ProviderError>
    requires detail::domain_copyable<ProviderError>
class GeneratedServiceRuntimeError {
    static_assert(!std::same_as<ProviderError, RuntimeError> &&
                  !std::same_as<ProviderError, GeneratedRuntimeConfigurationError>);

public:
    using allocator_type = Allocator;
    enum class Kind { MissingEnv, ConnectionError, Configuration, Rusqlite, Runtime };

    static GeneratedServiceRuntimeError
    from_preparation(Memory resource, const GeneratedRuntimeConfigurationError& error) {
        return GeneratedServiceRuntimeError(resource, detail::clone_in(resource, error));
    }

    static GeneratedServiceRuntimeError from_provider(Memory resource, const ProviderError& error) {
        return GeneratedServiceRuntimeError(resource, detail::clone_in(resource, error));
    }

    static GeneratedServiceRuntimeError from_runtime(Memory resource, const RuntimeError& error) {
        return GeneratedServiceRuntimeError(resource, detail::clone_in(resource, error));
    }

    GeneratedServiceRuntimeError(std::allocator_arg_t,
                                 Allocator allocator,
                                 const GeneratedServiceRuntimeError& other)
        : resource_(allocator.resource()), data_(std::visit(
                                               [&](const auto& value) -> Storage {
                                                   return detail::clone_in(resource_, value);
                                               },
                                               other.data_)) {}

    GeneratedServiceRuntimeError(std::allocator_arg_t,
                                 Allocator allocator,
                                 GeneratedServiceRuntimeError&& other)
        : GeneratedServiceRuntimeError(detail::move_to(allocator.resource(), std::move(other))) {}

    GeneratedServiceRuntimeError(const GeneratedServiceRuntimeError&) = delete;
    GeneratedServiceRuntimeError(GeneratedServiceRuntimeError&&) noexcept = default;

    GeneratedServiceRuntimeError& operator=(GeneratedServiceRuntimeError&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const noexcept {
        return resource_;
    }

    Kind kind() const noexcept {
        if (auto* error = preparation_error()) {
            switch (error->kind()) {
            case GeneratedRuntimeConfigurationError::Kind::MissingEnv:
                return Kind::MissingEnv;
            case GeneratedRuntimeConfigurationError::Kind::ConnectionError:
                return Kind::ConnectionError;
            case GeneratedRuntimeConfigurationError::Kind::Configuration:
                return Kind::Configuration;
            }
        }
        return provider_error() ? Kind::Rusqlite : Kind::Runtime;
    }

    const GeneratedRuntimeConfigurationError* preparation_error() const noexcept {
        return std::get_if<GeneratedRuntimeConfigurationError>(&data_);
    }

    const ProviderError* provider_error() const noexcept {
        return std::get_if<ProviderError>(&data_);
    }

    const RuntimeError* runtime_error() const noexcept {
        return std::get_if<RuntimeError>(&data_);
    }

    String to_string(Memory destination) const {
        if (auto* error = preparation_error()) {
            return error->to_string(destination);
        }
        String output(provider_error() ? "rusqlite error: " : "runtime error: ", destination);
        output += std::visit(
            [&](const auto& error) {
                return error.to_string(destination);
            },
            data_);
        return output;
    }

    std::optional<String> source(Memory destination) const {
        if (auto* error = preparation_error()) {
            return error->source(destination);
        }
        return std::visit(
            [&](const auto& error) {
                return error.to_string(destination);
            },
            data_);
    }

    std::optional<String> source_debug(Memory destination) const {
        if (auto* error = preparation_error()) {
            if (error->environment_error()) {
                return debug_string(destination, *error->environment_error());
            }
            return std::nullopt;
        }
        return std::visit(
            [&](const auto& error) {
                return debug_string(destination, error);
            },
            data_);
    }

    friend void write_debug(DebugWriter& writer, const GeneratedServiceRuntimeError& error) {
        if (auto* preparation = error.preparation_error()) {
            write_debug(writer, *preparation);
        } else if (auto* provider = error.provider_error()) {
            writer.tuple("Rusqlite", [&](auto& output) {
                write_debug(output, *provider);
            });
        } else {
            writer.tuple("Runtime", [&](auto& output) {
                write_debug(output, *error.runtime_error());
            });
        }
    }

private:
    using Storage = std::variant<GeneratedRuntimeConfigurationError, ProviderError, RuntimeError>;

    template <class Error>
    GeneratedServiceRuntimeError(Memory resource, Error&& error)
        : resource_(resource), data_(std::forward<Error>(error)) {}

    Memory resource_;
    Storage data_;
};
}
