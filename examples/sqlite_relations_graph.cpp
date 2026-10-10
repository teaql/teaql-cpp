// Semantic port of the locked Rust sqlite_relations_graph example.
#include "scenarios.hpp"

int main() {
    try {
        teaql::GlobalContext global;
        auto user = global.make_user_context();
        const auto& orders = example::relations_graph(*user);
        std::cout
            << "relation+graph example rows: "
            << example::observations(user->memory_resource(), orders).dump(user->memory_resource())
            << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
