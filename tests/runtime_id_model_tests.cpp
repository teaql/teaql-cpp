#include <teaql/runtime.hpp>
#include <teaql/runtime/runtime_id_debug.hpp>
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
    auto owner = user->memory_resource();
    auto counter = user->make_ref<AtomicCounterIdGenerator>(41);
    auto& generator = user->get(counter);
    auto& allocator = user->make<InMemoryBusinessIdAllocator>();
    auto& snowflake = user->make<SnowflakeIdGenerator>(31, 17);
    auto& clock = user->make<FixedBusinessClock>(*UtcDateTime::from_timestamp(-1, 123000000));
    {
        auto before = debug_string(owner, generator);
        check(before == "AtomicCounterIdGenerator { counter: 41 }",
              "Debug reads the actual counter");
        check(generator.generate_id(owner, "Item").value() == 42, "Debug never advances IDs");
        auto state = debug_string(owner, snowflake);
        check(state.find("worker_id: 31") != state.npos && state.find("sequence: 0") != state.npos,
              "Debug reads worker, datacenter and private state without generation");
        check(debug_string(owner, clock) ==
                  "FixedBusinessClock { value: 1969-12-31T23:59:59.123Z }",
              "Fixed clock diagnostics retain fractional UTC time");
        BusinessIdScope scope(std::allocator_arg, Allocator(owner), "Root", "Item", "sequence", "");
        BusinessIdPlan plan(std::allocator_arg, Allocator(owner), scope, "ORD", {}, 6, "-", 0, 2);
        check(allocator.allocate(owner, plan).value().sequence == 0,
              "Allocate stores initial scope level");
        auto levels = debug_string(owner, allocator);
        check(levels.find("poisoned: false") != levels.npos && levels.find(": 1") != levels.npos,
              "Diagnostics expose the stored next level and poison flag");
        check(allocator.allocate(owner, plan).value().sequence == 1,
              "Diagnostics never allocate an ID");
    }
    // Retain only diagnostic text through the explicit User-to-Global proxy.
    auto retained = [&] {
        auto diagnostic = debug_string(owner, generator);
        return user->make_global<String>(diagnostic);
    }();
    user.reset();
    check(global.live_user_bytes() == 0, "Closing User returns every arena block");
    auto next = global.make_user_context();
    check(next->get(next->rebind_global(retained)) == "AtomicCounterIdGenerator { counter: 42 }",
          "Explicit retained diagnostics survive by destination-domain copy");
    bool rejected = false;
    try {
        (void)next->get(counter);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Expired counter reference rejects before atomic access");
    std::cout << "Runtime ID diagnostic lifetime contracts pass\n";
}
