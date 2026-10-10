#pragma once
#include <teaql/runtime/context.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace teaql::detail {
// Separate seconds/nanoseconds preserve u64 millisecond leases without narrowing
// them to steady_clock's signed nanosecond duration. No wall-clock deadlines.
struct LockInstant {
    std::int64_t seconds;
    std::uint32_t nanos;
    static LockInstant now();
    LockInstant add_millis(std::uint64_t) const;
    std::chrono::nanoseconds wait_from(LockInstant now) const;
    auto operator<=>(const LockInstant&) const = default;
};

class LocalLocks final {
public:
    using allocator_type = Allocator;

    LocalLocks(std::allocator_arg_t, Allocator a) : resource_(a.resource()), entries_(resource_) {}

    LocalLocks(const LocalLocks&) = delete;
    LocalLocks& operator=(const LocalLocks&) = delete;
    bool
    acquire(std::string_view key, std::uint64_t owner, std::uint64_t timeout, std::uint64_t expiry);
    void release(std::string_view key, std::uint64_t owner);

    Memory memory_resource() const {
        return resource_;
    }

private:
    struct Entry {
        std::uint64_t owner;
        std::optional<LockInstant> expiry;
    };

    Memory resource_;
    std::pmr::map<String, Entry, std::less<>> entries_;
    std::mutex mutex_;
    std::condition_variable changed_;
};

struct ContextLockAccess {
    static LocalLocks& local(const UserContext&);
    static std::uint64_t owner(const UserContext&);
    static const String& remote_owner(const UserContext&);
    static std::optional<ResourceBorrow<const RemoteLockProvider>> remote(const UserContext&);
};
}
