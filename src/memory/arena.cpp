#include <teaql/memory/arena.hpp>
#include <algorithm>

namespace teaql {
Arena::TrackingResource::TrackingResource(std::pmr::memory_resource* upstream)
    : upstream_(upstream) {
    if (!upstream_) throw std::invalid_argument("Arena requires an upstream resource");
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
Arena::Arena(std::size_t initial_block_size, std::pmr::memory_resource* upstream)
    : upstream_(upstream), arena_(initial_block_size, &upstream_) {}
Arena::~Arena() { close(); }
void* Arena::do_allocate(std::size_t bytes, std::size_t alignment) {
    if (closed_) throw std::logic_error("Allocation scope is closed");
    return arena_.allocate(bytes, alignment);
}
void Arena::close() noexcept {
    if (closed_) return;
    closed_ = true;
    while (cleanup_head_) {
        auto* cleanup = cleanup_head_;
        cleanup_head_ = cleanup->previous;
        cleanup->destroy(cleanup->object);
    }
    arena_.release();
}
}
