#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    auto reference = user->make_ref<UnifiedLogBuffer>();
    auto& source = user->get(reference);
    auto& copied = user->make<UnifiedLogBuffer>(source);
    check(&source.entries == &copied.entries, "Same-domain Clone shares actual entry storage");
    {
        InfoLogEntry info(std::allocator_arg, Allocator(user->memory_resource()), "shared🧊");
        LogPayload payload(detail::clone_in(user->memory_resource(), info));
        copied.entries.emplace_back(SystemTime{}, payload);
    }
    check(source.entries.size() == 1, "Clone writes are visible through the original buffer");
    auto retained = user->make_global<UnifiedLogBuffer>(source);
    auto& retained_buffer = user->get(retained);
    check(&retained_buffer.entries != &source.entries &&
              retained_buffer.entries.get_allocator().resource() == global.memory_resource(),
          "Explicit cross-domain copy owns independent Global entry storage");
    source.entries.clear();
    check(copied.entries.empty() && retained_buffer.entries.size() == 1,
          "Clearing shared storage preserves the explicit independent retention copy");
    {
        InfoLogEntry info(std::allocator_arg, Allocator(user->memory_resource()), "after clear");
        LogPayload payload(detail::clone_in(user->memory_resource(), info));
        copied.entries.emplace_back(SystemTime{}, payload);
    }
    user->destroy(reference);
    check(copied.entries.size() == 1,
          "Destroying one handle does not destroy another live same-Context alias");
    user.reset();
    check(global.live_user_bytes() == 0, "All User aliases and their shared vector are reclaimed");
    auto next = global.make_user_context();
    const auto& kept = next->get(next->rebind_global(retained));
    check(std::get<InfoLogEntry>(kept.entries[0].payload).message == "shared🧊" &&
              std::get<InfoLogEntry>(kept.entries[0].payload).memory_resource() ==
                  global.memory_resource(),
          "Nested retained payload belongs to Global after actual source User destruction");
    bool rejected = false;
    try {
        (void)next->get(reference);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Expired source reference rejects before shared storage access");
    std::cout << "Unified buffer Clone and explicit retention contracts pass\n";
}
