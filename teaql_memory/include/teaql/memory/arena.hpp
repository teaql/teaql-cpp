#pragma once
#include <teaql/memory/reference.hpp>
#include <array>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <thread>
#include <utility>
#include <vector>

namespace teaql {
struct AllocationStats {
    std::size_t live_blocks = 0;
    std::size_t live_bytes = 0;
    std::size_t peak_bytes = 0;
    std::size_t total_blocks = 0;
    std::uint32_t issued_objects = 0;
    std::uint32_t live_objects = 0;
    std::size_t bitmap_bytes = 0;
};

// Owns the complete allocation region. Thread-confined, stable address, no shared ownership.
class Arena : public std::pmr::memory_resource {
public:
    // Intrusive registration: the record and callback target must outlive the
    // registration. Context-owned operations embed this record. No heap owner.
    class Quiescence {
    public:
        Quiescence() = default;

        ~Quiescence() {
            reset();
        }

        Quiescence(const Quiescence&) = delete;
        Quiescence& operator=(const Quiescence&) = delete;
        void attach(Arena&, void* object, void (*stop_and_wait)(void*) noexcept);
        void reset() noexcept;

    private:
        friend class Arena;
        Arena* arena_ = nullptr;
        Quiescence* previous_ = nullptr;
        Quiescence* next_ = nullptr;
        void* object_ = nullptr;
        void (*stop_and_wait_)(void*) noexcept = nullptr;
    };

    explicit Arena(std::size_t initial_block_size,
                   std::pmr::memory_resource* upstream,
                   const Arena* parent = nullptr,
                   MemoryLimits limits = {});
    ~Arena() override;
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    // Returns a borrow. Do not delete it, destroy it manually, or retain it after close().
    // Allocator-aware types (e.g. pmr::vector/string) receive this resource automatically.
    // Foreign RAII objects may own other allocations; their destructor must release them.
    template <class T, class... Args>
    T& make(Args&&... args) {
        check_thread();
        // Trusted kernel shutdown diagnostics are payloads, not new public identities.
        if (closing_) {
            return make_payload<T>(std::forward<Args>(args)...);
        }
        return make_identified<T>(std::forward<Args>(args)...).object;
    }

    template <class T>
    struct Identified {
        T& object;
        std::uint32_t slot;
    };

    template <class T, class... Args>
    Identified<T> make_identified(Args&&... args) {
        check_thread();
        if (closed_ || closing_) {
            throw std::logic_error("Cannot publish an object in a closing Context");
        }
        auto slot = reserve_slot();
        auto& record = object_record(slot);
        auto& object = construct<T>(&record, std::forward<Args>(args)...);
        auto& page = *pages_[(slot - 1) / slots_per_page];
        page.live |= std::uint64_t{1} << ((slot - 1) % slots_per_page);
        ++live_objects_;
        return {object, slot};
    }

    // Internal metadata and owned payloads have cleanup but no independently usable ID.
    template <class T, class... Args>
    T& make_payload(Args&&... args) {
        check_thread();
        if (closed_) {
            throw std::logic_error("Allocation scope is closed");
        }
        return construct<T>(nullptr, std::forward<Args>(args)...);
    }

    // Explicit resource entry for lower-level provider code that receives a
    // Context arena rather than the application-facing Context wrapper.
    template <class T, class... Args>
    T& make_resource(Args&&... args) {
        return make<T>(std::forward<Args>(args)...);
    }

    // Terminal and idempotent: quiesce work, run destructors, return every block.
    void close(std::source_location where = std::source_location::current()) noexcept;

    void* resolve(std::uint32_t slot,
                  const std::type_info& type,
                  std::uint64_t id,
                  std::source_location access = std::source_location::current()) const;
    void check_live(std::uint32_t slot,
                    std::uint64_t id,
                    std::source_location access = std::source_location::current(),
                    bool allow_closing = false) const;
    std::uint32_t slot_for_address(const void* object) const;

    // Logical allocation identity survives explicit payload copies without
    // retaining this arena. Neither arena namespaces nor object slots are reused.
    // This comparison token is not a checked Ref<T> and cannot resolve an object.
    std::uint64_t allocation_identity(std::uint32_t slot) const noexcept {
        return (std::uint64_t{identity_} << 32) | slot;
    }

    template <class T>
    struct WithIdentity {
        T& object;
        std::uint64_t identity;
    };

    // Kernel opaque borrows follow make(), including private shutdown diagnostics.
    // Shutdown tokens occupy a disjoint range and never become checked references.
    template <class T, class... Args>
    WithIdentity<T> make_with_identity(Args&&... args) {
        check_thread();
        if (closing_) {
            if (diagnostic_identities_ == UINT32_MAX - object_slot_limit) {
                throw std::overflow_error("Shutdown diagnostic identity namespace exhausted");
            }
            auto token = UINT32_MAX - diagnostic_identities_++;
            auto& object = make_payload<T>(std::forward<Args>(args)...);
            return {object, allocation_identity(token)};
        }
        auto value = make_identified<T>(std::forward<Args>(args)...);
        return {value.object, allocation_identity(value.slot)};
    }

    void set_close_guard(void* target, void (*guard)(void*) noexcept) noexcept {
        close_guard_target_ = target;
        close_guard_ = guard;
    }

    void destroy(std::uint32_t slot,
                 const std::type_info& type,
                 std::uint64_t id,
                 std::source_location access = std::source_location::current());

    void check_thread() const;

    const MemoryLimits& limits() const noexcept {
        return limits_;
    }

    bool closed() const noexcept {
        return closed_;
    }

    // Runtime Contexts supply application-owned diagnostics; generic arenas
    // retain their ordinary owner-lifetime contract without a runtime dependency.
    virtual const detail::ContextState* context_state() const noexcept {
        return nullptr;
    }

    bool closing() const noexcept {
        return closing_;
    }

    // The explicit child is destroyed before its parent; no shared ownership.
    bool can_lend_to(const Arena& destination) const noexcept {
        return this == &destination || destination.parent_ == this;
    }

    AllocationStats stats() const noexcept {
        auto result = upstream_.stats;
        result.issued_objects = issued_objects_;
        result.live_objects = live_objects_;
        result.bitmap_bytes = pages_.size() * sizeof(std::uint64_t);
        return result;
    }

protected:
    virtual void on_closing(std::source_location) noexcept {}

    virtual void on_closed() noexcept {}

private:
    struct Cleanup {
        Cleanup* previous;
        void* object;
        void (*destroy)(void*) noexcept;
    };

    struct ObjectRecord {
        void* object = nullptr;
        const std::type_info* type = nullptr;
        Cleanup* cleanup = nullptr;
        std::size_t bytes = 0;
    };

    static constexpr std::uint32_t slots_per_page = 64;

    struct ObjectPage {
        std::array<ObjectRecord, slots_per_page> objects{};
        std::uint64_t live = 0;
    };

    template <class T, class... Args>
    T& construct(ObjectRecord* record, Args&&... args) {
        static_assert(std::is_nothrow_destructible_v<T>,
                      "Context cleanup requires noexcept destruction");
        Cleanup* cleanup = nullptr;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            cleanup = static_cast<Cleanup*>(allocate(sizeof(Cleanup), alignof(Cleanup)));
        }
        auto* object = static_cast<T*>(allocate(sizeof(T), alignof(T)));
        ++constructing_;
        try {
            std::uninitialized_construct_using_allocator(
                object, std::pmr::polymorphic_allocator<T>(this), std::forward<Args>(args)...);
        } catch (...) {
            --constructing_;
            throw;
        }
        --constructing_;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::construct_at(cleanup, Cleanup{cleanup_head_, object, [](void* ptr) noexcept {
                                                   std::destroy_at(static_cast<T*>(ptr));
                                               }});
            cleanup_head_ = cleanup;
        }
        if (record) {
            *record = {object, &typeid(T), cleanup, sizeof(T)};
        }
        return *object;
    }

    std::uint32_t reserve_slot();
    ObjectRecord& object_record(std::uint32_t slot) const;

    class TrackingResource final : public std::pmr::memory_resource {
    public:
        TrackingResource(std::pmr::memory_resource* upstream, std::size_t max_bytes);
        AllocationStats stats;

    private:
        std::pmr::memory_resource* upstream_;
        std::size_t max_bytes_;
        void* do_allocate(std::size_t bytes, std::size_t alignment) override;
        void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override;

        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
            return this == &other;
        }
    };

    MemoryLimits limits_;
    const std::uint32_t identity_;
    std::uint32_t diagnostic_identities_ = 0;
    std::thread::id owner_thread_;
    TrackingResource upstream_;
    std::pmr::monotonic_buffer_resource arena_;
    std::pmr::vector<ObjectPage*> pages_;
    void* shutdown_buffer_ = nullptr;
    std::size_t shutdown_used_ = 0;
    std::uint32_t issued_objects_ = 0;
    std::uint32_t live_objects_ = 0;
    std::size_t constructing_ = 0;
    std::size_t allocating_ = 0;
    std::size_t destroying_ = 0;
    std::source_location closed_at_;
    void* close_guard_target_ = nullptr;
    void (*close_guard_)(void*) noexcept = nullptr;
    Cleanup* cleanup_head_ = nullptr;
    Quiescence* quiescence_head_ = nullptr;
    bool closing_ = false;
    bool closed_ = false;
    const Arena* parent_;
    void* do_allocate(std::size_t bytes, std::size_t alignment) override;

    void do_deallocate(void*, std::size_t, std::size_t) override {} // Region reclamation only.

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};
}
