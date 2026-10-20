#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <iostream>

using namespace teaql;

static void error_output(Json::Object& output,
                         Memory resource,
                         const cpp_dynamic_json::ServiceRuntimeError& error) {
    output.emplace("ok", Json(resource, false));
    output.emplace("display", Json(resource, error.to_string(resource)));
    output.emplace("debug", Json(resource, debug_string(resource, error)));
    auto source = error.source(resource);
    output.emplace("source", source ? Json(resource, *source) : Json(resource));
}

static Json observe(UserContext& user_context, std::string_view mode) {
    auto resource = user_context.memory_resource();
    Json::Object output(resource);
    if (mode == "config") {
        auto result = cpp_dynamic_json::ServiceRuntimeConfig::fromEnv(user_context);
        if (result) {
            auto copied = detail::clone_in(resource, result.value());
            output.emplace("ok", Json(resource, true));
            output.emplace("database_url", Json(resource, result.value().databaseUrl()));
            output.emplace("debug", Json(resource, debug_string(resource, result.value())));
            output.emplace("clone_equal", Json(resource, copied == result.value()));
        } else {
            error_output(output, resource, result.error());
        }
    } else if (mode == "nul_parent" || mode == "nul_file") {
        using namespace std::string_view_literals;
        auto input = mode == "nul_parent" ? "folder\0name/data.db"sv : "folder/data\0name.db"sv;
        cpp_dynamic_json::ServiceRuntimeConfig config(
            std::allocator_arg, Allocator(resource), input);
        auto result = cpp_dynamic_json::serviceRuntime(user_context, config);
        if (result) {
            output.emplace("ok", Json(resource, true));
        } else {
            error_output(output, resource, result.error());
        }
    } else if (mode == "connect") {
        auto result = cpp_dynamic_json::serviceRuntimeFromEnv(user_context);
        if (!result) {
            error_output(output, resource, result.error());
        } else {
            (void)user_context.get(result.value());
            auto Q = user_context.queries<cpp_dynamic_json::query_factory>();
            const auto& count = Q.peopleMinimal()
                                    .comment("Observe generated runtime assembly")
                                    .purpose("Check explicit schema initialization boundary")
                                    .executeForCount(user_context)
                                    .wait();
            output.emplace("ok", Json(resource, true));
            output.emplace("query_error",
                           count ? Json(resource)
                                 : Json(resource, count.error().to_string(resource)));
        }
    } else {
        throw std::invalid_argument("Unknown generated configuration probe mode");
    }
    return Json(resource, std::move(output));
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        {
            auto output = observe(*user_context, argc == 2 ? argv[1] : "connect");
            std::cout << output.dump(user_context->memory_resource()) << '\n';
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Generated configuration retained User bytes");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
