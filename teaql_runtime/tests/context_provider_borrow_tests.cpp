#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void dead(F&& call) {
    bool rejected = false;
    try {
        call();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Dead;
    }
    check(rejected, "Destroyed provider rejects before dispatch");
}

struct Registry final : EntityRegistry {
    using allocator_type = Allocator;
    Memory owner;
    const UserContext* close_owner = nullptr;

    Registry(std::allocator_arg_t, Allocator allocator) : owner(allocator.resource()) {}

    Memory memory_resource() const noexcept override {
        return owner;
    }

    bool contains(std::string_view name) const override {
        if (close_owner) {
            const_cast<UserContext*>(close_owner)->close();
        }
        return name == "Order";
    }
};

struct Ready {
    using allocator_type = Allocator;
    Memory owner;

    Ready(std::allocator_arg_t, Allocator allocator) : owner(allocator.resource()) {}

    std::optional<RuntimeSchemaResult> poll(WakeSignal&) {
        return RuntimeSchemaResult::ok(owner, {});
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Provider final : runtime::SchemaProvider {
    using allocator_type = Allocator;
    Memory owner;
    mutable unsigned calls = 0;
    bool close_owner = false;

    Provider(std::allocator_arg_t, Allocator allocator) : owner(allocator.resource()) {}

    Memory memory_resource() const noexcept {
        return owner;
    }

    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& user,
                                                       const SchemaInvocation&) const override {
        ++calls;
        if (close_owner) {
            const_cast<UserContext&>(user).close();
        }
        return make_operation<RuntimeSchemaResult, Ready>(user.memory_resource());
    }
};

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    if (argc == 2) {
        std::set_terminate([] {
            std::_Exit(0);
        });
        try {
            if (std::string_view(argv[1]) == "reentrant-registry") {
                auto& registry = user->make<Registry>();
                registry.close_owner = user.get();
                user->set_entity_registry(*user, registry);
                (void)user->has_entity_data_service("Order");
            } else {
                auto& provider = user->make<Provider>();
                provider.close_owner = true;
                user->set_schema_provider(*user, provider);
                (void)user->ensure_schema().poll();
            }
        } catch (...) {
            std::_Exit(2);
        }
        return 1;
    }
    auto registry = user->make_ref<Registry>();
    user->set_entity_registry(*user, user->get(registry));
    check(user->has_entity_data_service("Order") && !user->has_entity_data_service("Absent"),
          "Live registry preserves presence and metadata fallback");
    user->destroy(registry);
    dead([&] {
        (void)user->has_entity_data_service("Order");
    });

    auto shared_registry = user->make_global<Registry>();
    auto next = global.make_user_context();
    user->set_entity_registry(global, user->get(shared_registry));
    next->set_entity_registry(global, next->get(next->rebind_global(shared_registry)));
    user->destroy(shared_registry);
    dead([&] {
        (void)user->has_entity_data_service("Order");
    });
    dead([&] {
        (void)next->has_entity_data_service("Order");
    });

    auto provider = user->make_ref<Provider>();
    user->set_schema_provider(*user, user->get(provider));
    auto& live = user->ensure_schema();
    check(bool(live.wait()) && user->get(provider).calls == 1,
          "Live schema provider is lazy and called once");
    auto& pending = user->ensure_schema();
    user->destroy(provider);
    dead([&] {
        (void)pending.poll();
    });
    pending.cancel();

    auto shared_provider = user->make_global<Provider>();
    next->set_schema_provider(global, next->get(next->rebind_global(shared_provider)));
    auto& shared_work = next->ensure_schema();
    user->destroy(shared_provider);
    dead([&] {
        (void)shared_work.poll();
    });
    shared_work.cancel();
    user.reset();
    next.reset();
    check(global.live_user_bytes() == 0, "Provider borrows release all User arena blocks");
    std::cout << "Checked Context provider lifecycle contracts pass\n";
}
