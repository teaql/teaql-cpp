#include <teaql/memory/arena.hpp>
#include <algorithm>

namespace teaql {
Arena::TrackingResource::TrackingResource(std::pmr::memory_resource* upstream)
    : upstream_(upstream) {
    if (!upstream_) {
        throw std::invalid_argument("Arena requires an upstream resource");
    }
}

void* Arena::TrackingResource::do_allocate(std::size_t bytes, std::size_t alignment) {
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
             const Arena* parent)
    : upstream_(upstream), arena_(initial_block_size, &upstream_), parent_(parent) {}

Arena::~Arena() {
    close();
}

void Arena::Quiescence::attach(Arena& arena, void* object, void (*stop_and_wait)(void*) noexcept) {
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
    if (closed_) {
        throw std::logic_error("Allocation scope is closed");
    }
    return arena_.allocate(bytes, alignment);
}

void Arena::close() noexcept {
    if (closed_ || closing_) {
        return;
    }
    closing_ = true;
    // Diagnostics may allocate while quiescing. New asynchronous work may not
    // register. All ordinary objects stay alive until every callback returns.
    while (quiescence_head_) {
        auto* work = quiescence_head_;
        work->reset();
        work->stop_and_wait_(work->object_);
    }
    closed_ = true;
    while (cleanup_head_) {
        auto* cleanup = cleanup_head_;
        cleanup_head_ = cleanup->previous;
        cleanup->destroy(cleanup->object);
    }
    arena_.release();
    closing_ = false;
}
}
