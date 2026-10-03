#pragma once
#include <teaql/memory/arena.hpp>
#include <exception>

namespace teaql {
class UserContext;

// Reject common accidental off-arena containers (std::string/vector).
// Foreign RAII wrappers use the explicitly named make_resource() entry point.
template<class T>
concept ArenaObject = std::uses_allocator_v<T, std::pmr::polymorphic_allocator<T>>
                   || std::is_trivially_destructible_v<T>;

// Application-owned global lifetime; never a hidden singleton. Outlives every UserContext.
class GlobalContext final {
public:
    explicit GlobalContext(std::pmr::memory_resource* upstream = std::pmr::new_delete_resource(),
                           std::size_t initial_block_size = 4096)
        : upstream_(upstream), memory_(initial_block_size, upstream) {}
    GlobalContext(const GlobalContext&) = delete;
    GlobalContext& operator=(const GlobalContext&) = delete;
    GlobalContext(GlobalContext&&) = delete;
    GlobalContext& operator=(GlobalContext&&) = delete;
    ~GlobalContext() {
        // Detect lifetime inversion before freeing global metadata still borrowed by a context.
        if (live_contexts_ != 0) std::terminate();
    }

    template<ArenaObject T, class... Args> T& make(Args&&... args) {
        return memory_.make<T>(std::forward<Args>(args)...);
    }
    // Lifecycle-owned external resources; internal allocations remain the wrapper's responsibility.
    template<class T, class... Args> T& make_resource(Args&&... args) {
        return memory_.make<T>(std::forward<Args>(args)...);
    }
    std::unique_ptr<UserContext> make_user_context(std::size_t initial_block_size = 4096);
    std::pmr::memory_resource* memory_resource() noexcept { return &memory_; }
    AllocationStats allocation_stats() const noexcept { return memory_.stats(); }
private:
    friend class UserContext;
    std::pmr::memory_resource* upstream_;
    Arena memory_;
    std::size_t live_contexts_ = 0;
};

// Thread-confined, synchronous lifecycle foundation. No implicit global allocation domain.
class UserContext final {
public:
    explicit UserContext(GlobalContext& global, std::size_t initial_block_size = 4096)
        : global_(global), memory_(initial_block_size, global.upstream_) { ++global_.live_contexts_; }
    UserContext(const UserContext&) = delete;
    UserContext& operator=(const UserContext&) = delete;
    UserContext(UserContext&&) = delete;
    UserContext& operator=(UserContext&&) = delete;
    ~UserContext() { close(); --global_.live_contexts_; }

    template<ArenaObject T, class... Args> T& make(Args&&... args) {
        return memory_.make<T>(std::forward<Args>(args)...);
    }
    template<class T, class... Args> T& make_resource(Args&&... args) {
        return memory_.make<T>(std::forward<Args>(args)...);
    }
    GlobalContext& global() noexcept { return global_; }
    std::pmr::memory_resource* memory_resource() noexcept { return &memory_; }
    AllocationStats allocation_stats() const noexcept { return memory_.stats(); }
    bool closed() const noexcept { return memory_.closed(); }
    void close() noexcept { memory_.close(); }
private:
    GlobalContext& global_;
    Arena memory_;
};

inline std::unique_ptr<UserContext> GlobalContext::make_user_context(std::size_t initial_block_size) {
    return std::make_unique<UserContext>(*this, initial_block_size);
}
}
