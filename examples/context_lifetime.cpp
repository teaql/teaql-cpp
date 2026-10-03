#include <teaql/runtime/context.hpp>
#include <iostream>
#include <string>
#include <vector>

int main() {
    teaql::GlobalContext global;
    auto& model_name = global.make<std::pmr::string>("EdgeDevice");
    auto user_context = global.make_user_context(1024);
    auto& rows = user_context->make<std::pmr::vector<std::pmr::string>>();
    rows.emplace_back("edge-device-001");
    // Explicit deep copy from context ownership into global ownership.
    auto& retained = global.make<std::pmr::vector<std::pmr::string>>(rows);
    std::cout << "Context blocks: " << user_context->allocation_stats().live_blocks << '\n';
    user_context.reset(); // Destroys rows first, then releases every context arena block.
    // rows is invalid from this point onward; retained belongs to global.
    std::cout << model_name << ": " << retained.front() << '\n';
}
