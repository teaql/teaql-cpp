// Semantic port of the locked Rust sqlite_schema_crud example.
#include "scenarios.hpp"

int main() {
    try {
        teaql::GlobalContext global;
        auto user_context = global.make_user_context();
        const auto& orders = example::schema_crud(*user_context);
        std::cout << "schema+crud example rows: "
                  << example::observations(user_context->memory_resource(), orders)
                         .dump(user_context->memory_resource())
                  << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
