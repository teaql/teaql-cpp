#include <teaql/runtime/context.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <cstdlib>
#include <exception>
using namespace teaql;
[[noreturn]] static void close_and_report(const UserContext& context) {
    const_cast<UserContext&>(context).close();
    std::_Exit(1);
}
struct Metadata final : MetadataStore {
    Memory resource;
    const UserContext& context;
    Metadata(Memory r, const UserContext& context) : resource(r), context(context) {}
    Memory memory_resource() const noexcept override { return resource; }
    const EntityDescriptor* entity(std::string_view) const override { close_and_report(context); }
    Vector<const EntityDescriptor*> all_entities(Memory) const override { close_and_report(context); }
};
struct Checkers final : CheckerRegistry {
    Memory resource;
    const UserContext& context;
    Checkers(Memory r, const UserContext& context) : resource(r), context(context) {}
    Memory memory_resource() const noexcept override { return resource; }
    std::optional<CheckerBorrow> checker(Memory, std::string_view) const override {
        close_and_report(context);
    }
};
struct Telemetry final : RuntimeTelemetry {
    Memory resource;
    const UserContext& context;
    Telemetry(Memory r, const UserContext& context) : resource(r), context(context) {}
    Memory memory_resource() const noexcept override { return resource; }
    bool is_noop() const override { close_and_report(context); }
    ContextBox<RuntimeTelemetryScope>& start(Memory, const RuntimeOperation&) const override {
        close_and_report(context);
    }
};
int main(int argc, char** argv) {
    if (argc != 2) { return 3; }
    std::set_terminate([] { std::_Exit(0); });
    try {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = global.memory_resource();
        auto mode = std::string_view(argv[1]);
        if (mode == "entity" || mode == "all-entities") {
            auto& metadata = global.make_resource<Metadata>(r, *user);
            user->set_metadata(global, metadata);
            if (mode == "entity") { (void)user->entity("Item"); }
            else { (void)user->all_entities(); }
        } else if (mode == "checker") {
            auto& checkers = global.make_resource<Checkers>(r, *user);
            user->set_checker_registry(global, checkers);
            (void)user->has_checker("Item");
        } else {
            auto& telemetry = global.make_resource<Telemetry>(r, *user);
            user->set_runtime_telemetry(global, telemetry);
            if (mode == "telemetry-noop") { (void)user->runtime_telemetry_is_noop(); }
            else {
                RuntimeOperation operation(std::allocator_arg, Allocator(user->memory_resource()), "query", "Item");
                (void)user->start_runtime_operation(operation);
            }
        }
    } catch (...) { std::_Exit(2); }
    return 4;
}
