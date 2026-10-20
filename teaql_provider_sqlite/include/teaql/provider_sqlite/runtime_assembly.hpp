#pragma once
#include <teaql/provider_sqlite/runtime_connection.hpp>
#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/provider_sqlite/id.hpp>
#include <teaql/provider_sqlite/business_id.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/generated_runtime_config.hpp>
#include <teaql/sql/executor.hpp>

namespace teaql {
// Install provider infrastructure and the distinct application executor after
// the caller installs its module. Filename and URL entry points share this step
// while preserving their respective connection/registration ordering.
template <class Executor, bool HasBusinessIds = false, class Provider, class Initialize>
    requires std::derived_from<Provider, SchemaProvider> &&
             std::same_as<std::invoke_result_t<Initialize, UserContext&>,
                          Result<std::monostate, GeneratedRuntimeConfigurationError>>
Result<Ref<Executor>, GeneratedRuntimeConfigurationError>
bind_generated_sqlite_runtime(UserContext& user_context,
                              const Provider& metadata,
                              Ref<SqliteConnection> connection_reference,
                              Initialize&& before_executor) {
    using Output = Result<Ref<Executor>, GeneratedRuntimeConfigurationError>;
    auto resource = user_context.memory_resource();
    if (metadata.memory_resource() != resource) {
        throw std::invalid_argument(
            "Generated runtime metadata requires an explicit destination copy");
    }
    auto& connection = user_context.get(connection_reference);
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
    user_context.set_internal_id_generator(user_context, ids);
    if constexpr (HasBusinessIds) {
        auto& business_ids = user_context.make<SqliteBusinessIdAllocator>(transport);
        user_context.set_business_id_infrastructure(user_context, business_ids);
    }
    // Initialization is synchronous and borrowed for this call. Failure leaves
    // provider/ID infrastructure owned by the Context, without a new executor.
    auto initialized = std::forward<Initialize>(before_executor)(user_context);
    if (!initialized) {
        return Output::err(resource, std::move(initialized).error());
    }
    auto dialect = user_context.require_resource<SqliteDialect>();
    if (!dialect) {
        throw std::runtime_error(std::string(dialect.error().to_string(resource)));
    }
    auto& state = user_context.make<SqlExecutorState>();
    auto executor =
        user_context.make_ref<Executor>(dialect.value().get(), transport, metadata, state);
    user_context.register_executor(executor);
    return Output::ok(resource, executor);
}

template <class Executor, bool HasBusinessIds = false, class Provider>
    requires std::derived_from<Provider, SchemaProvider>
Ref<Executor> bind_generated_sqlite_runtime(UserContext& user_context,
                                            const Provider& metadata,
                                            Ref<SqliteConnection> connection_reference) {
    auto initialized = bind_generated_sqlite_runtime<Executor, HasBusinessIds>(
        user_context, metadata, connection_reference, [](UserContext& context) {
            return Result<std::monostate, GeneratedRuntimeConfigurationError>::ok(
                context.memory_resource(), {});
        });
    return initialized.value();
}

template <class Executor, bool HasBusinessIds = false>
Ref<Executor> assemble_generated_sqlite_runtime(UserContext& user_context,
                                                RuntimeModule& module,
                                                Ref<SqliteConnection> connection_reference) {
    if (module.memory_resource() != user_context.memory_resource()) {
        throw std::invalid_argument(
            "Generated runtime module requires an explicit destination copy");
    }
    (void)user_context.get(connection_reference);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    return bind_generated_sqlite_runtime<Executor, HasBusinessIds>(
        user_context, module.metadata, connection_reference);
}

// A generated static schema provider is independent of runtime module metadata.
// Both are owned by the assembling UserContext; the SQL executor borrows the
// former while Context/Checker/Behavior dispatch retains the latter.
template <class Executor, bool HasBusinessIds = false, class Provider>
    requires std::derived_from<Provider, SchemaProvider>
Ref<Executor> assemble_generated_sqlite_runtime(UserContext& user_context,
                                                RuntimeModule& module,
                                                Ref<SqliteConnection> connection_reference,
                                                const Provider& schema_provider) {
    if (module.memory_resource() != user_context.memory_resource()) {
        throw std::invalid_argument(
            "Generated runtime module requires an explicit destination copy");
    }
    (void)user_context.get(connection_reference);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    return bind_generated_sqlite_runtime<Executor, HasBusinessIds>(
        user_context, schema_provider, connection_reference);
}

// Source-shaped startup can fail after infrastructure installation and before
// executor registration. Model adapters supply only their required initializer;
// provider setup, registration and checked connection ownership stay shared.
template <class Executor, bool HasBusinessIds = false, class Provider, class Initialize>
    requires std::derived_from<Provider, SchemaProvider> &&
             std::same_as<std::invoke_result_t<Initialize, UserContext&>,
                          Result<std::monostate, GeneratedRuntimeConfigurationError>>
Result<Ref<Executor>, GeneratedRuntimeConfigurationError>
assemble_generated_sqlite_runtime(UserContext& user_context,
                                  RuntimeModule& module,
                                  Ref<SqliteConnection> connection_reference,
                                  const Provider& schema_provider,
                                  Initialize&& before_executor) {
    if (module.memory_resource() != user_context.memory_resource()) {
        throw std::invalid_argument(
            "Generated runtime module requires an explicit destination copy");
    }
    (void)user_context.get(connection_reference);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    return bind_generated_sqlite_runtime<Executor, HasBusinessIds>(
        user_context,
        schema_provider,
        connection_reference,
        std::forward<Initialize>(before_executor));
}
}
