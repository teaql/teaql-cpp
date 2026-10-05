#pragma once
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/resources.hpp>
#include <teaql/core/query.hpp>
#include <teaql/core/mutation.hpp>

namespace teaql {
class UserContext;
using RuntimeHookResult = Result<std::monostate, RuntimeError>;

// The source defaults are intentionally permissive. These are independent SPI
// contracts; the entity runtime controls when each hook and policy is invoked.
class RequestPolicy {
public:
    virtual ~RequestPolicy() = default;

    virtual RuntimeHookResult enforce_select(Memory r, const UserContext&, SelectQuery&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual RuntimeHookResult enforce_insert(Memory r, const UserContext&, InsertCommand&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual RuntimeHookResult enforce_update(Memory r, const UserContext&, UpdateCommand&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual RuntimeHookResult enforce_delete(Memory r, const UserContext&, DeleteCommand&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual RuntimeHookResult enforce_recover(Memory r, const UserContext&, RecoverCommand&) const {
        return RuntimeHookResult::ok(r, {});
    }
};

class EntityDataServiceBehavior {
public:
    virtual ~EntityDataServiceBehavior() = default;

    virtual RuntimeHookResult before_select(Memory r, const UserContext&, SelectQuery&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual RuntimeHookResult before_insert(Memory r, const UserContext&, InsertCommand&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual RuntimeHookResult before_update(Memory r, const UserContext&, UpdateCommand&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual RuntimeHookResult before_delete(Memory r, const UserContext&, DeleteCommand&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual RuntimeHookResult before_recover(Memory r, const UserContext&, RecoverCommand&) const {
        return RuntimeHookResult::ok(r, {});
    }

    virtual Vector<String> relation_loads(Memory r, const UserContext&) const {
        return Vector<String>(r);
    }
};

using EntityBehaviorBorrow = ResourceBorrow<const EntityDataServiceBehavior>;

class EntityDataServiceBehaviorRegistry {
public:
    virtual ~EntityDataServiceBehaviorRegistry() = default;
    virtual Memory memory_resource() const noexcept = 0;
    virtual std::optional<EntityBehaviorBorrow> behavior(Memory, std::string_view entity) const = 0;
};

class InMemoryEntityDataServiceBehaviorRegistry final : public EntityDataServiceBehaviorRegistry {
public:
    using allocator_type = Allocator;

    InMemoryEntityDataServiceBehaviorRegistry(std::allocator_arg_t, Allocator a)
        : behaviors_(a.resource()) {}

    InMemoryEntityDataServiceBehaviorRegistry(
        std::allocator_arg_t, Allocator a, const InMemoryEntityDataServiceBehaviorRegistry& other)
        : behaviors_(other.behaviors_, a.resource()) {}

    InMemoryEntityDataServiceBehaviorRegistry(const InMemoryEntityDataServiceBehaviorRegistry&) =
        delete;

    Memory memory_resource() const noexcept override {
        return behaviors_.get_allocator().resource();
    }

    template <class Owner, class T>
        requires std::derived_from<T, EntityDataServiceBehavior>
    void register_behavior(std::string_view entity, Owner& owner, T& behavior) {
        auto r = memory_resource();
        auto entry = detail::ContextResources::entry(
            detail::resource_arena(r), owner.memory_resource(), behavior);
        EntityBehaviorBorrow value(std::allocator_arg, Allocator(r), behavior, *entry.owner);
        auto found = behaviors_.find(entity);
        if (found == behaviors_.end())
            behaviors_.emplace(String(entity, r), std::move(value));
        else
            detail::assign_scoped(found->second, std::move(value));
    }

    template <class Owner, class T>
        requires std::derived_from<T, EntityDataServiceBehavior>
    InMemoryEntityDataServiceBehaviorRegistry&
    with_behavior(std::string_view entity, Owner& owner, T& behavior) & {
        register_behavior(entity, owner, behavior);
        return *this;
    }

    std::optional<EntityBehaviorBorrow> behavior(Memory r, std::string_view entity) const override {
        auto found = behaviors_.find(entity);
        if (found == behaviors_.end())
            return {};
        return detail::clone_in(r, found->second);
    }

private:
    std::pmr::map<String, EntityBehaviorBorrow, std::less<>> behaviors_;
};
}
