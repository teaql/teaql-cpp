#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
int main() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& source = user->make<UnifiedLogBuffer>();
    auto& copied = user->make<UnifiedLogBuffer>(source);
    auto owner = user->memory_resource();
    InfoLogEntry info(std::allocator_arg, Allocator(owner), "shared");
    LogPayload payload(detail::clone_in(owner, info));
    copied.entries.emplace_back(SystemTime{}, payload);
    auto source_count = source.entries.size();
    source.entries.clear();
    std::cout << "{\"shares_entries\":" << (&source.entries == &copied.entries ? "true" : "false")
              << ",\"source_after_clone_push\":" << source_count
              << ",\"clone_after_source_clear\":" << copied.entries.size() << "}\n";
}
