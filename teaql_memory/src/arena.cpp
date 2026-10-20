#include <teaql/memory/arena.hpp>
#include <algorithm>
#include <atomic>
#include <bit>
#include <limits>

namespace teaql {
namespace {
std::uint32_t issue_arena_identity() {
    // Scalar namespace issuance only; no retained Context data or ownership.
    // Use a 32-bit atomic so the edge target needs no 64-bit atomic library.
    static std::atomic<std::uint32_t> issued{0};
    auto current = issued.load(std::memory_order_relaxed);
    for (;;) {
        if (current == std::numeric_limits<std::uint32_t>::max()) {
            throw std::overflow_error("Arena identity namespace exhausted");
        }
        if (issued.compare_exchange_weak(current, current + 1, std::memory_order_relaxed)) {
            return current + 1;
        }
    }
}
}

Arena::TrackingResource::TrackingResource(std::pmr::memory_resource* upstream,
                                          std::size_t max_bytes)
    : upstream_(upstream), max_bytes_(max_bytes) {
    if (!upstream_) {
        throw std::invalid_argument("Arena requires an upstream resource");
    }
}

void* Arena::TrackingResource::do_allocate(std::size_t bytes, std::size_t alignment) {
    if (bytes > max_bytes_ - stats.live_bytes ||
        stats.total_blocks == std::numeric_limits<std::size_t>::max()) {
        throw MemoryLimitError(MemoryLimitError::Kind::Bytes);
    }
    void* ptr = upstream_->allocate(bytes, alignment);
    ++stats.live_blocks;
    ++stats.total_blocks;
    stats.live_bytes += bytes;
    stats.peak_bytes = std::max(stats.peak_bytes, stats.live_bytes);
    return ptr;
}

void Arena::TrackingResource::do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) {
    upstream_->deallocate(ptr, bytes, alignment);
    --stats.live_blocks;
    stats.live_bytes -= bytes;
}

Arena::Arena(std::size_t initial_block_size,
             std::pmr::memory_resource* upstream,
             const Arena* parent,
             MemoryLimits limits)
    : limits_(limits), identity_(issue_arena_identity()), owner_thread_(std::this_thread::get_id()),
      upstream_(upstream, limits.max_bytes), arena_(initial_block_size, &upstream_), pages_(this),
      parent_(parent) {
    if (limits.max_objects > object_slot_limit ||
        limits.shutdown_reserve_bytes > limits.max_bytes || initial_block_size == 0) {
        throw std::invalid_argument("Invalid Context memory limits");
    }
    if (limits.shutdown_reserve_bytes != 0) {
        shutdown_buffer_ =
            upstream_.allocate(limits.shutdown_reserve_bytes, alignof(std::max_align_t));
    }
}

Arena::~Arena() {
    close();
}

void Arena::Quiescence::attach(Arena& arena, void* object, void (*stop_and_wait)(void*) noexcept) {
    arena.check_thread();
    if (arena.closed_ || arena.closing_) {
        throw std::logic_error("Cannot start work in a closing Context");
    }
    if (!stop_and_wait) {
        throw std::invalid_argument("Quiescence callback is required");
    }
    reset();
    arena_ = &arena;
    object_ = object;
    stop_and_wait_ = stop_and_wait;
    next_ = arena.quiescence_head_;
    if (next_) {
        next_->previous_ = this;
    }
    arena.quiescence_head_ = this;
}

void Arena::Quiescence::reset() noexcept {
    if (!arena_) {
        return;
    }
    if (previous_) {
        previous_->next_ = next_;
    } else {
        arena_->quiescence_head_ = next_;
    }
    if (next_) {
        next_->previous_ = previous_;
    }
    arena_ = nullptr;
    previous_ = nullptr;
    next_ = nullptr;
}

void* Arena::do_allocate(std::size_t bytes, std::size_t alignment) {
    check_thread();
    if (closed_) {
        throw std::logic_error("Allocation scope is closed");
    }
    if (!std::has_single_bit(alignment)) {
        throw std::invalid_argument("Allocation alignment must be a power of two");
    }
    bytes = std::max<std::size_t>(bytes, 1);
    if (bytes > limits_.max_bytes || alignment > limits_.max_bytes ||
        bytes > std::numeric_limits<std::size_t>::max() - (alignment - 1)) {
        throw MemoryLimitError(MemoryLimitError::Kind::Bytes);
    }
    if (closing_) {
        if (shutdown_buffer_) {
            auto* position = static_cast<std::byte*>(shutdown_buffer_) + shutdown_used_;
            void* aligned = position;
            auto remaining = limits_.shutdown_reserve_bytes - shutdown_used_;
            if (std::align(alignment, bytes, aligned, remaining)) {
                shutdown_used_ = static_cast<std::byte*>(aligned) -
                                 static_cast<std::byte*>(shutdown_buffer_) + bytes;
                return aligned;
            }
        }
        // Larger final logs may use normal capacity, still under every byte cap.
        // Exhausted/failed upstream cannot prevent reserve-sized cleanup.
    }
    ++allocating_;
    try {
        auto* result = arena_.allocate(bytes, alignment);
        --allocating_;
        return result;
    } catch (...) {
        --allocating_;
        throw;
    }
}

void Arena::check_thread() const {
    if (owner_thread_ != std::this_thread::get_id()) {
        throw std::logic_error("Context accessed from a non-owner thread");
    }
}

std::uint32_t Arena::reserve_slot() {
    if (issued_objects_ == limits_.max_objects) {
        throw MemoryLimitError(MemoryLimitError::Kind::Objects);
    }
    auto slot = issued_objects_ + 1;
    if ((slot - 1) / slots_per_page == pages_.size()) {
        auto& page = make_payload<ObjectPage>();
        pages_.push_back(&page);
    }
    // Failed construction consumes a reservation but never publishes a live bit.
    issued_objects_ = slot;
    return slot;
}

Arena::ObjectRecord& Arena::object_record(std::uint32_t slot) const {
    return pages_[(slot - 1) / slots_per_page]->objects[(slot - 1) % slots_per_page];
}

void Arena::check_live(std::uint32_t slot,
                       std::uint64_t id,
                       std::source_location access,
                       bool allow_closing) const {
    check_thread();
    if (closed_ || (closing_ && !allow_closing)) {
        throw ReferenceError(ReferenceError::Kind::Closed, id, access, closed_at_);
    }
    if (slot == 0 || slot > issued_objects_) {
        throw ReferenceError(ReferenceError::Kind::Invalid, id, access);
    }
    auto& page = *pages_[(slot - 1) / slots_per_page];
    if (!(page.live & (std::uint64_t{1} << ((slot - 1) % slots_per_page)))) {
        throw ReferenceError(ReferenceError::Kind::Dead, id, access);
    }
}

std::uint32_t Arena::slot_for_address(const void* object) const {
    check_thread();
    if (closed_) {
        throw std::logic_error("Allocation scope is closed");
    }
    auto address = reinterpret_cast<std::uintptr_t>(object);
    // Borrowing a freshly registered result must not scan all earlier objects.
    // Older objects keep the existing lookup and identical dead-storage checks.
    if (issued_objects_ != 0) {
        auto& latest = object_record(issued_objects_);
        auto start = reinterpret_cast<std::uintptr_t>(latest.object);
        if (latest.object && address >= start && address - start < latest.bytes) {
            check_live(issued_objects_, 0, std::source_location::current(), true);
            return issued_objects_;
        }
    }
    for (std::uint32_t slot = 1; slot <= issued_objects_; ++slot) {
        auto& record = object_record(slot);
        auto start = reinterpret_cast<std::uintptr_t>(record.object);
        if (record.object && address >= start && address - start < record.bytes) {
            // Dead managed storage cannot become an unchecked external borrow.
            check_live(slot, 0, std::source_location::current(), true);
            return slot;
        }
    }
    return 0;
}

void* Arena::resolve(std::uint32_t slot,
                     const std::type_info& type,
                     std::uint64_t id,
                     std::source_location access) const {
    check_live(slot, id, access);
    auto& record = object_record(slot);
    if (*record.type != type) {
        throw ReferenceError(ReferenceError::Kind::Type, id, access);
    }
    return record.object;
}

void Arena::destroy(std::uint32_t slot,
                    const std::type_info& type,
                    std::uint64_t id,
                    std::source_location access) {
    (void)resolve(slot, type, id, access);
    auto& record = object_record(slot);
    pages_[(slot - 1) / slots_per_page]->live &=
        ~(std::uint64_t{1} << ((slot - 1) % slots_per_page));
    --live_objects_;
    if (record.cleanup) {
        auto destroy = record.cleanup->destroy;
        record.cleanup->destroy = nullptr;
        ++destroying_;
        destroy(record.object);
        --destroying_;
    }
    // Keep the extent as a tombstone until arena reclamation. Monotonic storage
    // and object identities are never reused within this owner.
}

void Arena::close(std::source_location where) noexcept {
    if (owner_thread_ != std::this_thread::get_id() || constructing_ || allocating_ ||
        destroying_) {
        std::terminate();
    }
    if (closed_ || closing_) {
        return;
    }
    if (close_guard_) {
        close_guard_(close_guard_target_);
    }
    closing_ = true;
    closed_at_ = where;
    on_closing(where);
    // Diagnostics may allocate while quiescing. New asynchronous work may not
    // register. All ordinary objects stay alive until every callback returns.
    while (quiescence_head_) {
        auto* work = quiescence_head_;
        work->reset();
        work->stop_and_wait_(work->object_);
    }
    closed_ = true;
    for (auto* page : pages_) {
        page->live = 0;
    }
    live_objects_ = 0;
    while (cleanup_head_) {
        auto* cleanup = cleanup_head_;
        cleanup_head_ = cleanup->previous;
        if (cleanup->destroy) {
            cleanup->destroy(cleanup->object);
        }
    }
    {
        std::pmr::vector<ObjectPage*> empty(this);
        pages_.swap(empty);
    }
    arena_.release();
    if (shutdown_buffer_) {
        upstream_.deallocate(
            shutdown_buffer_, limits_.shutdown_reserve_bytes, alignof(std::max_align_t));
        shutdown_buffer_ = nullptr;
    }
    closing_ = false;
    on_closed();
}

std::uint32_t detail::issue_runtime_identity() {
    // Namespace issuer only: no Context pointers, storage, caches or ownership.
    static std::atomic<std::uint32_t> issued{0};
    auto current = issued.load(std::memory_order_relaxed);
    for (;;) {
        if (current == runtime_identity_limit) {
            throw MemoryLimitError(MemoryLimitError::Kind::RuntimeIdentities);
        }
        if (issued.compare_exchange_weak(current, current + 1, std::memory_order_relaxed)) {
            return current + 1;
        }
    }
}
}
