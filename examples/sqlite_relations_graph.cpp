// Semantic port of the locked Rust sqlite_relations_graph example.
#include "scenarios.hpp"

int main() {
    try {
        teaql::GlobalContext global;
        auto user_context = global.make_user_context();
        const auto& orders = example::relations_graph(*user_context);
        std::cout << "relation+graph example rows: "
                  << example::observations(user_context->memory_resource(), orders)
                         .dump(user_context->memory_resource())
                  << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
