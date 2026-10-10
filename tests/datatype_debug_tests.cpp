#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void enum_capabilities() {
    GlobalContext global;
    String* retained = nullptr;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto before = user->allocation_stats().issued_objects;
        check(core::debug_string(owner, DataType::LargeText) == "LargeText",
              "standalone enum diagnostics are available through the core facade");
        DebugOptions options;
        options.pretty = true;
        options.width = 40;
        options.precision = 1;
        options.sign_plus = true;
        check(debug_string(owner, DataType::Timestamp, options) == "Timestamp",
              "unit variants preserve original Debug behavior independently of numeric flags");
        check(user->allocation_stats().issued_objects == before,
              "formatting does not allocate a managed object identity");
        retained =
            &global.make<String>(core::debug_string(global.memory_resource(), DataType::Timestamp));
        bool invalid = false;
        try {
            (void)debug_string(owner, static_cast<DataType>(-1));
        } catch (const std::invalid_argument&) {
            invalid = true;
        }
        check(invalid, "invalid native enum values reject before indexing diagnostic names");
    }
    check(*retained == "Timestamp" &&
              retained->get_allocator().resource() == global.memory_resource(),
          "explicit destination output survives actual User destruction");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        enum_capabilities();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
