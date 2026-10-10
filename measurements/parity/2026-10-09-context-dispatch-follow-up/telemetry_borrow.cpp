#include <teaql/runtime/context.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <iostream>
using namespace teaql;
int main() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto telemetry = user->make_global<NoopRuntimeTelemetry>();
    user->set_runtime_telemetry(global, user->get(telemetry));
    user->destroy(telemetry);
    bool rejected = false;
    try {
        // Observe only the getter; never dispatch a virtual call on a dead object.
        const auto& borrow = user->runtime_telemetry();
        (void)&borrow;
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Dead;
    }
    std::cout << "dead telemetry rejected: " << rejected << std::endl;
    return rejected ? 0 : 1;
}
