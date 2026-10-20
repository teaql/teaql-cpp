#include "../../examples/generated/include/cpp_examples.hpp"
#include <filesystem>
#include <iostream>

using namespace teaql;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "A temporary fixture directory is required\n";
        return 1;
    }
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        {
            auto user_context = global.make_user_context();
            bool rejected = false;
            try {
                using namespace std::string_view_literals;
                cpp_examples::configureRuntime(*user_context, "invalid\0filename"sv);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            if (!rejected || user_context->all_entities().size() != 3 ||
                user_context->get_resource<cpp_examples::DataServiceExecutor>()) {
                throw std::runtime_error(
                    "Filename failure changed module-before-open registration behavior");
            }
            user_context.reset();
        }
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Failed filename setup retained User storage");
        }
        auto directory = std::filesystem::path(argv[1]);
        auto filename = directory / "literal?mode=ro.db";
        {
            auto user_context = global.make_user_context();
            cpp_examples::configureRuntime(*user_context, filename.string());
            auto Q = user_context->queries<cpp_examples::query_factory>();
            const auto& before_schema = Q.productsMinimal()
                                            .comment("Inspect direct filename setup")
                                            .purpose("Preserve explicit schema initialization")
                                            .executeForCount(*user_context)
                                            .wait();
            if (before_schema) {
                throw std::runtime_error("Filename configuration implicitly created schema");
            }
            user_context.reset();
        }
        if (global.live_user_bytes() != 0 || !std::filesystem::exists(filename) ||
            std::filesystem::exists(directory / "literal")) {
            throw std::runtime_error("Direct filename input was implicitly parsed as a URL");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Direct filename, registration order and explicit schema contracts passed\n";
    return 0;
}
