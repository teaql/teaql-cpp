#include <teaql/runtime/context.hpp>
#include <teaql/runtime/generated_service_runtime_error.hpp>
#include <teaql/provider_sqlite/driver_error.hpp>
#include <array>
#include <iostream>

using namespace teaql;
using ServiceError = GeneratedServiceRuntimeError<SqliteDriverError>;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

static Json observe(Memory resource, std::string_view name, const ServiceError& error) {
    Json::Object output(resource);
    output.emplace("name", Json(resource, name));
    output.emplace("display", Json(resource, error.to_string(resource)));
    output.emplace("debug", Json(resource, debug_string(resource, error)));
    auto source = error.source(resource);
    auto source_debug = error.source_debug(resource);
    output.emplace("source", source ? Json(resource, *source) : Json(resource));
    output.emplace("source_debug", source_debug ? Json(resource, *source_debug) : Json(resource));
    return Json(resource, std::move(output));
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto resource = user_context->memory_resource();
        std::array<Ref<ServiceError>, 5> retained, expired;
        constexpr std::array<std::string_view, 5> names{
            "missing", "connection", "configuration", "sqlite", "runtime"};
        {
            GeneratedEnvironmentError environment(std::allocator_arg,
                                                  Allocator(resource),
                                                  GeneratedEnvironmentError::Kind::NotPresent);
            auto missing = GeneratedRuntimeConfigurationError::missing_env(
                resource, "CPP_DYNAMIC_JSON_CORE_DATABASE_URL", environment);
            auto connection = GeneratedRuntimeConfigurationError::connection_error(
                resource, "a retained connection diagnostic");
            auto configuration = GeneratedRuntimeConfigurationError::configuration(
                resource, "an invalid configured profile");
            auto provider = SqliteDriverError::invalid_query(resource);
            auto runtime = RuntimeError::graph(resource, "a retained runtime diagnostic");
            std::array errors{ServiceError::from_preparation(resource, missing),
                              ServiceError::from_preparation(resource, connection),
                              ServiceError::from_preparation(resource, configuration),
                              ServiceError::from_provider(resource, provider),
                              ServiceError::from_runtime(resource, runtime)};
            Json::Array observations(resource);
            for (std::size_t index = 0; index < errors.size(); ++index) {
                check(errors[index].kind() == static_cast<ServiceError::Kind>(index),
                      "Five source service error branches remain distinct");
                expired[index] = user_context->make_ref<ServiceError>(errors[index]);
                retained[index] = user_context->make_global<ServiceError>(errors[index]);
                observations.emplace_back(observe(resource, names[index], errors[index]));
            }
            std::cout << Json(resource, std::move(observations)).dump(resource) << '\n';
        }
        user_context.reset();
        check(global.live_user_bytes() == 0,
              "Actual User deletion releases all service-error storage");
        auto next_user_context = global.make_user_context();
        auto destination = next_user_context->memory_resource();
        for (std::size_t index = 0; index < retained.size(); ++index) {
            bool closed = false;
            try {
                (void)next_user_context->get(expired[index]);
            } catch (const ReferenceError&) {
                closed = true;
            }
            check(closed, "Every source service-error reference expires");
            auto& error = next_user_context->get(next_user_context->rebind_global(retained[index]));
            check(error.memory_resource() == global.memory_resource(),
                  "The retained error carrier belongs to Global");
            if (auto* provider = error.provider_error()) {
                check(provider->memory_resource() == global.memory_resource(),
                      "Typed provider error is copied into Global");
            }
            if (auto* runtime = error.runtime_error()) {
                check(runtime->memory_resource() == global.memory_resource(),
                      "Typed RuntimeError is copied into Global");
            }
            check(!error.to_string(destination).empty(),
                  "Retained source diagnostics remain readable");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
