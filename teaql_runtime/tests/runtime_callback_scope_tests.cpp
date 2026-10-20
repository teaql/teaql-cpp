#include <teaql/runtime/data_service.hpp>
#include <cstdlib>
#include <exception>
using namespace teaql;

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    DataServiceCapabilities capabilities() const override {
        return {};
    }

    AsyncOperation<Result<QueryResult, String>>& query(Memory, QueryRequest) override {
        throw std::logic_error("unused query");
    }

    AsyncOperation<Result<MutationResult, String>>& mutate(Memory, MutationRequest) override {
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

    RuntimeHookResult
    before_select(Memory r, const UserContext& context, SelectQuery&) const override {
        if (select) {
            close_and_report(context);
        }
        return RuntimeHookResult::ok(r, {});
    }
};

struct Policy final : RequestPolicy {
    RuntimeHookResult
    enforce_select(Memory, const UserContext& context, SelectQuery&) const override {
        close_and_report(context);
    }
};

struct Registry final : EntityDataServiceBehaviorRegistry {
    Memory resource;
    const UserContext& context;

    Registry(Memory r, const UserContext& context) : resource(r), context(context) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    std::optional<EntityBehaviorBorrow> behavior(Memory, std::string_view) const override {
        close_and_report(context);
    }
};

struct Slot {
    const UserContext& context;
    bool close_current;

    std::optional<std::string_view> current_business_id() const {
        if (close_current) {
            close_and_report(context);
        }
        return {};
    }

    bool is_new_aggregate() const {
        return true;
    }

    void assign_business_id(BusinessIdValue) {}
};

struct Key final : BusinessIdKeyProvider {
    Result<BusinessIdEncodingKey, BusinessIdError>
    current_key(Memory, const UserContext& context, const BusinessIdDefinition&) const override {
        close_and_report(context);
    }
};

struct Allocation final : BusinessIdAllocator {
    const UserContext& context;

    explicit Allocation(const UserContext& context) : context(context) {}

    Result<BusinessIdAllocation, BusinessIdError> allocate(Memory,
                                                           const BusinessIdPlan&) const override {
        close_and_report(context);
    }
};

static int relation_callback(std::string_view mode) {
    GlobalContext global;
    auto user = global.make_user_context();
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
        if (mode == "loads") {
            (void)service.relation_loads();
        } else {
            SelectQuery query(std::allocator_arg, Allocator(user->memory_resource()), "Item");
            query.comment("Inspect callback close protection");
            PurposedSelectQuery purposed(std::allocator_arg,
                                         Allocator(user->memory_resource()),
                                         query,
                                         "Review callback lifetime");
            (void)service.fetch_all(purposed).poll();
        }
    }
    return 4;
}

static int business_callback(std::string_view mode) {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& allocator = global.make<InMemoryBusinessIdAllocator>();
    auto& closing = global.make_resource<Allocation>(*user);
    auto& service = mode == "allocator" ? global.make<BusinessIdService>(global, closing)
                                        : global.make<BusinessIdService>(global, allocator);
    auto definition = BusinessIdDefinition::legacy_daily_sequence(r, "number", "ORD", "order");
    if (mode == "key") {
        definition.profile = DEFAULT_BUSINESS_ID_PROFILE;
        auto& key = global.make_resource<Key>();
        service.with_key_provider(global, key);
    }
    Slot slot{*user, mode == "slot"};
    (void)service.ensure(r, *user, definition, "Root", "Order", slot);
    return 4;
}

struct ThrowingBehavior final : EntityDataServiceBehavior {
    Vector<String> relation_loads(Memory, const UserContext&) const override {
        throw std::runtime_error("controlled relation callback failure");
    }
};

struct ThrowingSlot {
    std::optional<std::string_view> current_business_id() const {
        throw std::runtime_error("controlled slot callback failure");
    }

    bool is_new_aggregate() const {
        return true;
    }

    void assign_business_id(BusinessIdValue) {}
};

static int exception_cleanup() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& executor = global.make_resource<Executor>();
    auto& behavior = global.make_resource<ThrowingBehavior>();
    auto& registry = global.make<InMemoryEntityDataServiceBehaviorRegistry>();
    registry.register_behavior("Item", global, behavior);
    user->set_entity_data_service_behavior_registry(global, registry);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Item", global, executor);
    bool relation_failed = false;
    try {
        (void)service.relation_loads();
    } catch (const std::runtime_error&) {
        relation_failed = true;
    }
    auto& allocator = global.make<InMemoryBusinessIdAllocator>();
    auto& ids = global.make<BusinessIdService>(global, allocator);
    bool slot_failed = false;
    {
        auto definition = BusinessIdDefinition::legacy_daily_sequence(
            user->memory_resource(), "number", "ORD", "order");
        ThrowingSlot slot;
        try {
            (void)ids.ensure(user->memory_resource(), *user, definition, "Root", "Order", slot);
        } catch (const std::runtime_error&) {
            slot_failed = true;
        }
    }
    user->close();
    return relation_failed && slot_failed && global.live_user_bytes() == 0 ? 0 : 1;
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc == 1) {
        return exception_cleanup();
    }
    if (argc != 2) {
        return 3;
    }
    std::set_terminate([] {
        std::_Exit(0);
    });
    try {
        auto mode = std::string_view(argv[1]);
        if (mode.starts_with("business-")) {
            return business_callback(mode.substr(9));
        }
        return relation_callback(mode);
    } catch (...) {
        std::_Exit(2);
    }
}
