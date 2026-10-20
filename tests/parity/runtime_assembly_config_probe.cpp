#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <teaql/provider_sqlite/runtime_assembly.hpp>
#include <iostream>

using namespace teaql;

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        {
            auto resource = user_context->memory_resource();
            Json::Object output(resource);
            auto config =
                read_generated_runtime_config(resource, "CPP_DYNAMIC_JSON_CORE_DATABASE_URL");
            auto fail = [&](const GeneratedRuntimeConfigurationError& error) {
                output.emplace("ok", Json(resource, false));
                output.emplace("display", Json(resource, error.to_string(resource)));
                output.emplace("debug", Json(resource, debug_string(resource, error)));
                auto source = error.source(resource);
                output.emplace("source", source ? Json(resource, *source) : Json(resource));
            };
            if (!config) {
                fail(config.error());
            } else {
                auto connection = open_generated_sqlite_runtime_connection(
                    *user_context, config.value().database_url);
                if (!connection) {
                    fail(connection.error());
                } else {
                    auto& module = cpp_dynamic_json::moduleWithBehaviorsAndCheckers(*user_context);
                    auto executor =
                        assemble_generated_sqlite_runtime<cpp_dynamic_json::DataServiceExecutor>(
                            *user_context, module, connection.value());
                    (void)user_context->get(executor);
                    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
                    const auto& result =
                        Q.peopleMinimal()
                            .comment("Observe generated runtime assembly")
                            .purpose("Check explicit schema initialization boundary")
                            .executeForCount(*user_context)
                            .wait();
                    output.emplace("ok", Json(resource, true));
                    output.emplace("query_error",
                                   result ? Json(resource)
                                          : Json(resource, result.error().to_string(resource)));
                }
            }
            std::cout << Json(resource, std::move(output)).dump(resource) << '\n';
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Configured runtime assembly retained User storage");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
