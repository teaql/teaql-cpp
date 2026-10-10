#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
#include <iostream>
using namespace teaql;

struct Ready {
    using allocator_type = Allocator;
    Memory memory;

    Ready(std::allocator_arg_t, Allocator allocator) : memory(allocator.resource()) {}

    std::optional<RuntimeSchemaResult> poll(WakeSignal&) {
        return RuntimeSchemaResult::ok(memory, {});
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Provider : runtime::SchemaProvider {
    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& context,
                                                       const SchemaInvocation&) const override {
        return make_operation<RuntimeSchemaResult, Ready>(context.memory_resource());
    }
};

GeneratedSchemaBootstrapFuture& bootstrap(const UserContext& context) {
    auto result = context.set_generated_bootstrap_active_root("Tenant", 42);
    if (!result) {
        throw std::runtime_error("unexpected root rejection");
    }
    return make_operation<RuntimeSchemaResult, Ready>(context.memory_resource());
}

int main() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& provider = global.make_resource<Provider>();
    auto& module = global.make<RuntimeModule>();
    module.generated_schema_bootstrap(bootstrap);
    module.apply_to(*user);
    user->set_schema_provider(global, provider);
    try {
        auto& result = user->ensure_schema().wait();
        if (!result || user->require_active_root("Tenant").value().get().id != 42) {
            return 3;
        }
        std::cout << "bootstrap root accepted\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
