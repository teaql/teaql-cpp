#include "fixture_value.hpp"
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("Invalid bootstrap classifier fixture");
            }
            auto kind = text(field(*input, "kind"));
            auto message = text(field(*input, "message"));
            auto error = kind == "graph"    ? RuntimeError::graph(r, message)
                         : kind == "schema" ? RuntimeError::schema(r, message)
                         : kind == "policy" ? RuntimeError::policy(r, message)
                         : kind == "id"     ? RuntimeError::id_generation(r, message)
                                        : RuntimeError::optimistic_lock_conflict(r, "Status", "11");
            std::cout << (generated_bootstrap_retryable_conflict(error) ? "true" : "false") << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
