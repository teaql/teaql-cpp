#pragma once
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace teaql {
struct AllocationStats {
    std::size_t live_blocks = 0;
    std::size_t live_bytes = 0;
    std::size_t peak_bytes = 0;
    std::size_t total_blocks = 0;
};

// Owns the complete allocation region. Thread-confined, stable address, no shared ownership.
class Arena final : public std::pmr::memory_resource {
public:
    explicit Arena(std::size_t initial_block_size,
                   std::pmr::memory_resource* upstream);
    ~Arena() override;
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    // Returns a borrow. Do not delete it, destroy it manually, or retain it after close().
    // Allocator-aware types (e.g. pmr::vector/string) receive this resource automatically.
    // Foreign RAII objects may own other allocations; their destructor must release them.
    template<class T, class... Args> T& make(Args&&... args) {
        static_assert(std::is_nothrow_destructible_v<T>, "Context cleanup requires noexcept destruction");
        if (closed_) throw std::logic_error("Allocation scope is closed");
        Cleanup* cleanup = nullptr;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            cleanup = static_cast<Cleanup*>(allocate(sizeof(Cleanup), alignof(Cleanup)));
        }
        T* object = static_cast<T*>(allocate(sizeof(T), alignof(T)));
        // Failure leaves unused arena space but never registers an unconstructed object.
        std::uninitialized_construct_using_allocator(object,
            std::pmr::polymorphic_allocator<T>(this), std::forward<Args>(args)...);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::construct_at(cleanup, Cleanup{cleanup_head_, object, [](void* ptr) noexcept {
                std::destroy_at(static_cast<T*>(ptr));
            }});
            cleanup_head_ = cleanup;
        }
        return *object;
    }

    // Terminal and idempotent: run all destructors, then return every arena block upstream.
    void close() noexcept;
    bool closed() const noexcept { return closed_; }
    AllocationStats stats() const noexcept { return upstream_.stats; }

private:
    struct Cleanup { Cleanup* previous; void* object; void (*destroy)(void*) noexcept; };
    class TrackingResource final : public std::pmr::memory_resource {
    public:
        explicit TrackingResource(std::pmr::memory_resource* upstream);
        AllocationStats stats;
    private:
        std::pmr::memory_resource* upstream_;
        void* do_allocate(std::size_t bytes, std::size_t alignment) override;
        void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override;
        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
    };
    TrackingResource upstream_;
    std::pmr::monotonic_buffer_resource arena_;
    Cleanup* cleanup_head_ = nullptr;
    bool closed_ = false;
    void* do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void*, std::size_t, std::size_t) override {} // Region reclamation only.
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};
}
