#include <teaql/runtime/detail/id_set_build_lock.hpp>
#include <teaql/runtime/detail/context_pagination.hpp>

namespace teaql::detail {
struct IdSetBuildWait {
    using allocator_type = Allocator;
    ResourceBorrow<IdSetBuildLock> owner;
    std::size_t slot = IdSetBuildRegistry::none;
    WakeSignal* wake = nullptr;
    bool woken = false, finished = false;

    IdSetBuildWait(std::allocator_arg_t, Allocator a, IdSetBuildLock& lock)
        : owner(std::allocator_arg, a, lock, resource_arena(lock.memory_resource())) {
        lock.require_active();
        ++lock.children_;
    }

    ~IdSetBuildWait() {
        cancel_and_wait();
    }

    std::optional<ResourceBorrow<IdSetBuildGuard>> poll(WakeSignal& signal) {
        auto& lock = owner.get();
        lock.require_active();
        auto& entry = *static_cast<IdSetBuildRegistry::Entry*>(lock.entry_);
        if (!entry.held) {
            auto r = owner.memory_resource();
            auto& guard = resource_arena(r).make<IdSetBuildGuard>(lock);
            entry.remove(slot, false);
            slot = IdSetBuildRegistry::none;
            return ResourceBorrow<IdSetBuildGuard>(
                std::allocator_arg, Allocator(r), guard, resource_arena(r));
        }
        wake = &signal;
        woken = false;
        if (slot == IdSetBuildRegistry::none) {
            slot = entry.insert(*this);
        }
        return {};
    }

    void finish_and_wait() noexcept {
        stop(false);
    }

    void cancel_and_wait() noexcept {
        stop(true);
    }

private:
    void stop(bool another) noexcept {
        if (finished) {
            return;
        }
        auto& lock = owner.get();
        static_cast<IdSetBuildRegistry::Entry*>(lock.entry_)->remove(slot, another);
        slot = IdSetBuildRegistry::none;
        wake = nullptr;
        --lock.children_;
        finished = true;
    }
};

std::size_t IdSetBuildRegistry::Entry::insert(IdSetBuildWait& waiter) {
    if (free_slot == none) {
        slots.push_back(Slot{&waiter, none});
        return slots.size() - 1;
    }
    auto index = free_slot;
    free_slot = slots[index].next_free;
    slots[index] = {&waiter, none};
    return index;
}

void IdSetBuildRegistry::Entry::wake_first() noexcept {
    for (auto& slot : slots) {
        if (slot.waiter) {
            auto& waiter = *slot.waiter;
            if (!waiter.woken) {
                waiter.woken = true;
                waiter.wake->notify();
            }
            break;
        }
    }
}

void IdSetBuildRegistry::Entry::remove(std::size_t index, bool another) noexcept {
    if (index == none) {
        return;
    }
    bool was_woken = slots[index].waiter->woken;
    slots[index] = {nullptr, free_slot};
    free_slot = index;
    if (another && was_woken) {
        wake_first();
    }
}

IdSetBuildLock& IdSetBuildRegistry::borrow(Memory r, std::string_view key) {
    auto& owner = resource_arena(memory_resource());
    auto& result = async_arena(r);
    if (owner.closing() || !owner.can_lend_to(result)) {
        throw std::invalid_argument("ID set build registry belongs to an unrelated Context");
    }
    // As with the source Weak registry, prune only on the next lookup.
    std::erase_if(entries_, [](const auto& entry) {
        return entry.second.leases == 0;
    });
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        it = entries_.try_emplace(String(key, memory_resource())).first;
    }
    return result.make<IdSetBuildLock>(*this, &it->second);
}

IdSetBuildLock::IdSetBuildLock(std::allocator_arg_t,
                               Allocator a,
                               IdSetBuildRegistry& registry,
                               void* entry)
    : registry_(std::allocator_arg, a, registry, resource_arena(registry.memory_resource())),
      entry_(entry) {
    quiescence_.attach(async_arena(a.resource()), this, [](void* p) noexcept {
        static_cast<IdSetBuildLock*>(p)->release_lease();
    });
    ++static_cast<IdSetBuildRegistry::Entry*>(entry_)->leases;
}

IdSetBuildLock::~IdSetBuildLock() {
    release_lease();
}

void IdSetBuildLock::require_active() const {
    if (!entry_) {
        throw std::logic_error("ID set build lock lease was released");
    }
    (void)async_arena(memory_resource());
}

void IdSetBuildLock::release_lease() noexcept {
    if (!entry_) {
        return;
    }
    if (children_) {
        std::terminate();
    }
    --static_cast<IdSetBuildRegistry::Entry*>(entry_)->leases;
    entry_ = nullptr;
    quiescence_.reset();
}

void IdSetBuildLock::reset() {
    if (children_) {
        throw std::logic_error("ID set build lock still has pending work or a held guard");
    }
    release_lease();
}

AsyncOperation<ResourceBorrow<IdSetBuildGuard>>& IdSetBuildLock::lock() {
    require_active();
    return make_operation<ResourceBorrow<IdSetBuildGuard>, IdSetBuildWait>(memory_resource(),
                                                                           *this);
}

IdSetBuildGuard::IdSetBuildGuard(std::allocator_arg_t, Allocator a, IdSetBuildLock& lock)
    : lock_(std::allocator_arg, a, lock, resource_arena(lock.memory_resource())) {
    lock.require_active();
    auto& entry = *static_cast<IdSetBuildRegistry::Entry*>(lock.entry_);
    if (entry.held) {
        throw std::logic_error("ID set build mutex is already held");
    }
    quiescence_.attach(async_arena(a.resource()), this, [](void* p) noexcept {
        static_cast<IdSetBuildGuard*>(p)->release();
    });
    entry.held = true;
    ++lock.children_;
    held_ = true;
}

void IdSetBuildGuard::release() noexcept {
    if (!held_) {
        return;
    }
    auto& lock = lock_.get();
    auto& entry = *static_cast<IdSetBuildRegistry::Entry*>(lock.entry_);
    entry.held = false;
    held_ = false;
    --lock.children_;
    quiescence_.reset();
    entry.wake_first();
}

IdSetBuildLock& ContextPaginationAccess::build_lock(const UserContext& c, std::string_view key) {
    c.ensure_open();
    auto& global = c.global_;
    if (!global.id_set_build_registry_) {
        global.id_set_build_registry_ = &global.make<IdSetBuildRegistry>();
    }
    return global.id_set_build_registry_->borrow(c.memory_resource(), key);
}
}
