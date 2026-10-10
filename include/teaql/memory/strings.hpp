#pragma once
#include <teaql/memory/arena.hpp>
#include <cstring>
#include <limits>
#include <string_view>
#include <unordered_set>

namespace teaql {
struct StringStorageStats {
    std::size_t saved_bytes = 0;
    std::size_t interned_entries = 0;
    std::size_t pool_bytes = 0;
    std::size_t hits = 0;
    std::size_t fallbacks = 0;
};

// LLVM-style stable saving and explicit interning. Both die with their owner.
class StringStorage final {
public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

    StringStorage(std::allocator_arg_t, allocator_type allocator, StringPoolLimits limits)
        : owner_(allocator.resource()), limits_(limits), pool_(owner_, limits.max_bytes),
          index_(&pool_) {}

    StringStorage(const StringStorage&) = delete;
    StringStorage& operator=(const StringStorage&) = delete;

    std::string_view save(std::string_view value) {
        return copy(value, owner_);
    }

    std::string_view intern(std::string_view value) {
        auto found = index_.find(value);
        if (found != index_.end()) {
            increment(stats_.hits);
            return *found;
        }
        if (index_.size() == limits_.max_entries) {
            increment(stats_.fallbacks);
            return save(value);
        }
        std::string_view saved;
        try {
            saved = copy(value, &pool_);
        } catch (const MemoryLimitError& error) {
            if (error.kind() != MemoryLimitError::Kind::StringPool) {
                throw;
            }
            increment(stats_.fallbacks);
            return save(value);
        }
        try {
            index_.insert(saved);
        } catch (const MemoryLimitError& error) {
            if (error.kind() != MemoryLimitError::Kind::StringPool) {
                throw;
            }
            // The saved characters remain valid; no second copy or eviction.
            increment(stats_.fallbacks);
        }
        return saved;
    }

    StringStorageStats stats() const noexcept {
        auto result = stats_;
        result.interned_entries = index_.size();
        result.pool_bytes = pool_.used();
        return result;
    }

private:
    static void increment(std::size_t& counter) noexcept {
        if (counter != std::numeric_limits<std::size_t>::max()) {
            ++counter;
        }
    }

    class PoolResource final : public std::pmr::memory_resource {
    public:
        PoolResource(std::pmr::memory_resource* owner, std::size_t limit)
            : owner_(owner), limit_(limit) {}

        std::size_t used() const noexcept {
            return used_;
        }

    private:
        void* do_allocate(std::size_t bytes, std::size_t alignment) override {
            if (bytes > limit_ - used_) {
                throw MemoryLimitError(MemoryLimitError::Kind::StringPool);
            }
            auto* result = owner_->allocate(bytes, alignment);
            used_ += bytes;
            return result;
        }

        // Monotonic storage remains charged until the complete owner closes.
        void do_deallocate(void*, std::size_t, std::size_t) override {}

        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
            return this == &other;
        }

        std::pmr::memory_resource* owner_;
        std::size_t limit_;
        std::size_t used_ = 0;
    };

    std::string_view copy(std::string_view value, std::pmr::memory_resource* target) {
        if (value.size() == std::numeric_limits<std::size_t>::max() ||
            value.size() + 1 > std::numeric_limits<std::size_t>::max() - stats_.saved_bytes) {
            throw MemoryLimitError(MemoryLimitError::Kind::Bytes);
        }
        auto* data = static_cast<char*>(target->allocate(value.size() + 1, alignof(char)));
        if (!value.empty()) {
            std::memcpy(data, value.data(), value.size());
        }
        data[value.size()] = '\0';
        stats_.saved_bytes += value.size() + 1;
        return {data, value.size()};
    }

    std::pmr::memory_resource* owner_;
    StringPoolLimits limits_;
    PoolResource pool_;
    std::pmr::unordered_set<std::string_view> index_;
    StringStorageStats stats_;
};
}
