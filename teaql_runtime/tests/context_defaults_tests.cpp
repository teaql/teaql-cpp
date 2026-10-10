#include <teaql/runtime.hpp>
#include <charconv>
#include <chrono>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void clock_and_timezone_lifetime() {
    GlobalContext global;
    const auto before = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    auto user = global.make_user_context();
    const auto after = std::chrono::duration_cast<std::chrono::microseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
    const auto trace = user->trace_id();
    const auto suffix = trace.substr(trace.rfind('-') + 1);
    std::uint64_t value{};
    const auto parsed = std::from_chars(suffix.data(), suffix.data() + suffix.size(), value, 16);
    check(parsed.ec == std::errc{} && parsed.ptr == suffix.data() + suffix.size() &&
              value >= static_cast<std::uint64_t>(before) &&
              value <= static_cast<std::uint64_t>(after),
          "the default hexadecimal trace counter uses original epoch microseconds");
    check(user->timezone() == "UTC", "the original timezone defaults to Some UTC");
    auto& source = user->make<String>(2048, 'z');
    user->set_timezone(source);
    source.assign(2048, 'x');
    check(user->timezone()->size() == 2048 && user->timezone()->front() == 'z',
          "timezone installation owns a User copy of caller text");
    auto retained = user->make_global<String>(user->timezone().value());
    {
        auto assembly = global.make_user_context();
        auto& module = assembly->make<RuntimeModule>();
        auto& entity = assembly->make<EntityDescriptor>("ContextAssembly");
        entity.property("id", DataType::U64).id();
        module.descriptor(detail::clone_in(assembly->memory_resource(), entity));
        module.language(Language::Japanese);
        check(&user->with_module(module) == user.get(), "module builder returns the invoking User");
        assembly.reset();
    }
    const auto* installed = user->entity("ContextAssembly");
    check(
        installed && installed->memory_resource() == user->memory_resource() &&
            user->has_entity_data_service("ContextAssembly") &&
            user->language() == Language::Japanese,
        "module installation snapshots its independent registries after the source User is deleted");

    check(&user->with_timezone("") == user.get() && user->timezone() && user->timezone()->empty(),
          "the fluent timezone builder retains present empty text without extra validation");
    user->with_timezone("Asia/Shanghai");
    check(user->timezone() == "Asia/Shanghai",
          "the fluent timezone builder preserves unmodified text");
    user->close();
    bool rejected = false;
    try {
        (void)user->timezone();
    } catch (const std::logic_error&) {
        rejected = true;
    }
    check(rejected, "timezone borrowing rejects a closed User");
    user.reset();
    check(global.live_user_bytes() == 0, "all timezone and trace User blocks are reclaimed");
    auto next = global.make_user_context();
    const auto& kept = next->get(next->rebind_global(retained));
    check(kept.size() == 2048 && kept.front() == 'z' &&
              kept.get_allocator().resource() == global.memory_resource(),
          "explicit Global retention survives actual User deletion");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        clock_and_timezone_lifetime();
        std::cout << "Context defaults contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
