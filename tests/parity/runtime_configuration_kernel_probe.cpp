#include <teaql/runtime/context.hpp>
#include <teaql/runtime/generated_runtime_config.hpp>
#include <teaql/provider_sqlite/runtime_config.hpp>
#include <iostream>

using namespace teaql;

static void error_output(Json::Object& output,
                         Memory resource,
                         const GeneratedRuntimeConfigurationError& error) {
    output.emplace("ok", Json(resource, false));
    output.emplace("display", Json(resource, error.to_string(resource)));
    output.emplace("debug", Json(resource, debug_string(resource, error)));
    auto source = error.source(resource);
    output.emplace("source", source ? Json(resource, *source) : Json(resource));
}

static Json observe(Memory resource, std::string_view mode) {
    Json::Object output(resource);
    if (mode == "config") {
        auto result = read_generated_runtime_config(resource, "CPP_DYNAMIC_JSON_CORE_DATABASE_URL");
        if (result) {
            auto copied = detail::clone_in(resource, result.value());
            output.emplace("ok", Json(resource, true));
            output.emplace("database_url", Json(resource, result.value().database_url));
            output.emplace("debug", Json(resource, debug_string(resource, result.value())));
            output.emplace("clone_equal", Json(resource, copied == result.value()));
        } else {
            error_output(output, resource, result.error());
        }
    } else if (mode == "nul_parent" || mode == "nul_file") {
        using namespace std::string_view_literals;
        auto input = mode == "nul_parent" ? "folder\0name/data.db"sv : "folder/data\0name.db"sv;
        auto result = prepare_generated_sqlite_runtime_path(resource, input);
        if (result) {
            output.emplace("ok", Json(resource, true));
        } else {
            error_output(output, resource, result.error());
        }
    } else {
        throw std::invalid_argument("Unknown configuration kernel probe mode");
    }
    return Json(resource, std::move(output));
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "A configuration kernel probe mode is required\n";
        return 1;
    }
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        {
            auto output = observe(user_context->memory_resource(), argv[1]);
            std::cout << output.dump(user_context->memory_resource()) << '\n';
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Configuration probe retained User bytes");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
