#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

struct Registry final : EntityRegistry {
    using allocator_type = Allocator;
    Memory owner;

    Registry(std::allocator_arg_t, Allocator allocator) : owner(allocator.resource()) {}

    Memory memory_resource() const noexcept override {
        return owner;
    }

    bool contains(std::string_view) const override {
        return true;
    }
};

struct Provider final : runtime::SchemaProvider {
    using allocator_type = Allocator;
    Memory owner;

    Provider(std::allocator_arg_t, Allocator allocator) : owner(allocator.resource()) {}

    Memory memory_resource() const noexcept {
        return owner;
    }

    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext&,
                                                       const SchemaInvocation&) const override {
        throw std::runtime_error("Live provider was dispatched");
    }
};

int main(int argc, char** argv) {
    GlobalContext global;
    auto user = global.make_user_context();
    try {
        if (argc == 2 && std::string_view(argv[1]) == "registry") {
            auto reference = user->make_ref<Registry>();
            user->set_entity_registry(*user, user->get(reference));
            user->destroy(reference);
            (void)user->has_entity_data_service("Order");
        } else {
            auto reference = user->make_ref<Provider>();
            user->set_schema_provider(*user, user->get(reference));
            user->destroy(reference);
            (void)user->ensure_schema().poll();
        }
    } catch (const ReferenceError&) {
        std::cout << "Checked rejection before provider dispatch\n";
        return 0;
    }
    return 1;
}
