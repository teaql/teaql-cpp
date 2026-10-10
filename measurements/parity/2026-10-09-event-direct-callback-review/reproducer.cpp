#include <teaql/runtime/context.hpp>
#include <teaql/runtime/event.hpp>
#include <cstdlib>
#include <iostream>
using namespace teaql;

struct Closing final : RawAuditEventSink {
    Result<std::monostate, RuntimeError>
    on_event(Memory, const UserContext& context, const RawAuditEvent&) const override {
        const_cast<UserContext&>(context).close();
        throw std::runtime_error("direct fan-out callback returned after Context close");
    }
};

int main() {
    std::set_terminate([] {
        std::_Exit(0);
    });
    GlobalContext global;
    auto user = global.make_user_context();
    auto& callback = global.make_resource<Closing>();
    auto& fanout = global.make<InMemoryRawAuditEventSink>();
    fanout.register_sink(global, callback);
    auto& values = global.make<Record>();
    auto& event = global.make<RawAuditEvent>(
        RawAuditEvent::created(global.memory_resource(), "Item", values));
    try {
        (void)fanout.on_event(global.memory_resource(), *user, event);
    } catch (const std::runtime_error& error) {
        std::cout << error.what() << '\n';
        return 2;
    }
    return 1;
}
