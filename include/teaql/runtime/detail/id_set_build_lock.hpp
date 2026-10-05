#pragma once
#include <teaql/async/operation.hpp>
#include <teaql/runtime/resources.hpp>
#include <limits>

namespace teaql::detail {
struct IdSetBuildAccess;
struct IdSetBuildWait;
class IdSetBuildRegistry;
class IdSetBuildGuard;

// Context borrow of an application-owned mutex. Logical leases select registry
// identity; they never own or extend a Context's memory.
class IdSetBuildLock final {
public:
    using allocator_type = Allocator;
    IdSetBuildLock(std::allocator_arg_t, Allocator, IdSetBuildRegistry&, void* entry);
    IdSetBuildLock(const IdSetBuildLock&) = delete;
    IdSetBuildLock& operator=(const IdSetBuildLock&) = delete;
    ~IdSetBuildLock();
    AsyncOperation<ResourceBorrow<IdSetBuildGuard>>& lock();
    void reset();

    bool active() const noexcept {
        return entry_ != nullptr;
    }

    Memory memory_resource() const noexcept {
        return registry_.memory_resource();
    }

private:
    friend class IdSetBuildRegistry;
    friend class IdSetBuildGuard;
    friend struct IdSetBuildWait;
    friend struct IdSetBuildAccess;
    void require_active() const;
    void release_lease() noexcept;
    ResourceBorrow<IdSetBuildRegistry> registry_;
    void* entry_;
    std::size_t children_ = 0;
    Arena::Quiescence quiescence_;
};

class IdSetBuildGuard final {
public:
    using allocator_type = Allocator;
    IdSetBuildGuard(std::allocator_arg_t, Allocator, IdSetBuildLock&);
    IdSetBuildGuard(const IdSetBuildGuard&) = delete;
    IdSetBuildGuard& operator=(const IdSetBuildGuard&) = delete;

    ~IdSetBuildGuard() {
        release();
    }

    void release() noexcept;

    bool owns_lock() const noexcept {
        return held_;
    }

    Memory memory_resource() const noexcept {
        return lock_.memory_resource();
    }

private:
    ResourceBorrow<IdSetBuildLock> lock_;
    bool held_ = false;
    Arena::Quiescence quiescence_;
};

class IdSetBuildRegistry final {
public:
    using allocator_type = Allocator;

    IdSetBuildRegistry(std::allocator_arg_t, Allocator a) : entries_(a.resource()) {}

    IdSetBuildRegistry(const IdSetBuildRegistry&) = delete;
    IdSetBuildRegistry& operator=(const IdSetBuildRegistry&) = delete;

    Memory memory_resource() const noexcept {
        return entries_.get_allocator().resource();
    }

    IdSetBuildLock& borrow(Memory, std::string_view query_key);

private:
    friend class IdSetBuildLock;
    friend class IdSetBuildGuard;
    friend struct IdSetBuildWait;
    friend struct IdSetBuildAccess;
    static constexpr std::size_t none = std::numeric_limits<std::size_t>::max();

    struct Slot {
        IdSetBuildWait* waiter = nullptr;
        std::size_t next_free = none;
    };

    struct Entry {
        using allocator_type = Allocator;

        Entry(std::allocator_arg_t, Allocator a) : slots(a.resource()) {}

        // Mirrors the source mutex's reusable waiter slots without promising
        // FIFO fairness. User nodes remain borrowed only until quiescence.
        Vector<Slot> slots;
        std::size_t free_slot = none, leases = 0;
        bool held = false;
        std::size_t insert(IdSetBuildWait&);
        void remove(std::size_t, bool wake_another) noexcept;
        void wake_first() noexcept;
    };

    std::pmr::map<String, Entry, std::less<>> entries_;
};
}
