#include <teaql/runtime/data_service.hpp>
#include <cstdlib>
#include <exception>
using namespace teaql;

struct Executor final : QueryExecutor<unsigned>, MutationExecutor<unsigned> {
    DataServiceCapabilities capabilities() const override { return {}; }
    AsyncOperation<Result<QueryResult, unsigned>>& query(Memory, QueryRequest) override {
        throw std::logic_error("unused query");
    }
    AsyncOperation<Result<MutationResult, unsigned>>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unused mutation");
    }
};

[[noreturn]] static void close_and_report(const UserContext& context) {
    const_cast<UserContext&>(context).close();
    // Avoid any access or destructor after the failing control reclaims blocks.
    std::_Exit(1);
}

struct Behavior final : EntityDataServiceBehavior {
    bool select;
    explicit Behavior(bool select) : select(select) {}
    Vector<String> relation_loads(Memory, const UserContext& context) const override {
        close_and_report(context);
    }
    RuntimeHookResult before_select(Memory r, const UserContext& context, SelectQuery&) const override {
        if (select) { close_and_report(context); }
        return RuntimeHookResult::ok(r, {});
    }
};
struct Policy final : RequestPolicy {
    RuntimeHookResult enforce_select(Memory, const UserContext& context, SelectQuery&) const override {
        close_and_report(context);
    }
};
struct Registry final : EntityDataServiceBehaviorRegistry {
    Memory resource;
    const UserContext& context;
    Registry(Memory r, const UserContext& context) : resource(r), context(context) {}
    Memory memory_resource() const noexcept override { return resource; }
    std::optional<EntityBehaviorBorrow> behavior(Memory, std::string_view) const override {
        close_and_report(context);
    }
};

int main(int argc, char** argv) {
    if (argc != 2) { return 3; }
    std::set_terminate([] { std::_Exit(0); });
    try {
        GlobalContext global;
        auto user = global.make_user_context();
        auto mode = std::string_view(argv[1]);
        if (mode == "registry") {
            auto& registry = global.make_resource<Registry>(global.memory_resource(), *user);
            user->set_entity_data_service_behavior_registry(global, registry);
            (void)user->entity_data_service_behavior("Item");
        } else {
            auto& executor = global.make_resource<Executor>();
            auto& service = EntityDataService<Executor>::for_executor(*user, "Item", global, executor);
            if (mode == "select-policy") {
                auto& policy = global.make_resource<Policy>();
                user->set_request_policy(global, policy);
            } else {
                auto& behavior = global.make_resource<Behavior>(mode == "select-behavior");
                auto& registry = global.make<InMemoryEntityDataServiceBehaviorRegistry>();
                registry.register_behavior("Item", global, behavior);
                user->set_entity_data_service_behavior_registry(global, registry);
            }
            if (mode == "loads") { (void)service.relation_loads(); }
            else {
                SelectQuery query(std::allocator_arg, Allocator(user->memory_resource()), "Item");
                PurposedSelectQuery purposed(std::allocator_arg, Allocator(user->memory_resource()), query, "Review callback lifetime");
                (void)service.fetch_all(purposed).poll();
            }
        }
    } catch (...) { std::_Exit(2); }
    return 4;
}
