#pragma once
#include <cstddef>
#include <cstdint>
#include <new>
#include <memory_resource>
#include <stdexcept>

namespace teaql {
inline constexpr std::uint32_t object_slot_limit = (1u << 20) - 1;
inline constexpr std::uint32_t context_identity_limit = (1u << 20) - 1;

struct StringPoolLimits {
    std::size_t max_entries = 4096;
    std::size_t max_bytes = 1024 * 1024;
};

struct MemoryLimits {
    std::uint32_t max_objects = 1'000'000;
    std::size_t max_bytes = 64 * 1024 * 1024;
    std::size_t shutdown_reserve_bytes = 4096;
    StringPoolLimits strings;
};

struct ContextLimits {
    MemoryLimits global;
    MemoryLimits user;
    std::uint32_t max_user_contexts = 1'000'000;
    std::size_t max_live_user_bytes = 64 * 1024 * 1024;
};

class MemoryLimitError final : public std::bad_alloc {
public:
    enum class Kind { Objects, Bytes, Contexts, RuntimeIdentities, StringPool };

    explicit MemoryLimitError(Kind kind) noexcept : kind_(kind) {}

    Kind kind() const noexcept {
        return kind_;
    }

    const char* what() const noexcept override {
        switch (kind_) {
        case Kind::Objects:
            return "Context object identity limit reached";
        case Kind::Bytes:
            return "Context byte budget exhausted";
        case Kind::Contexts:
            return "Application Context identity limit reached";
        case Kind::RuntimeIdentities:
            return "Process runtime identity space exhausted";
        case Kind::StringPool:
            return "Context string pool budget exhausted";
        }
        return "Context allocation limit reached";
    }

private:
    Kind kind_;
};

namespace detail {
// Accounting only; storage remains owned by each requesting User arena.
class ByteBudgetResource final : public std::pmr::memory_resource {
public:
    ByteBudgetResource(std::pmr::memory_resource* upstream, std::size_t limit)
        : upstream_(upstream), limit_(limit) {
        if (!upstream_) {
            throw std::invalid_argument("A byte budget requires an upstream resource");
        }
    }

    std::size_t used() const noexcept {
        return used_;
    }

    std::size_t peak() const noexcept {
        return peak_;
    }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (bytes > limit_ - used_) {
            throw MemoryLimitError(MemoryLimitError::Kind::Bytes);
        }
        auto* result = upstream_->allocate(bytes, alignment);
        used_ += bytes;
        if (used_ > peak_) {
            peak_ = used_;
        }
        return result;
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        upstream_->deallocate(pointer, bytes, alignment);
        used_ -= bytes;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::pmr::memory_resource* upstream_;
    std::size_t limit_;
    std::size_t used_ = 0;
    std::size_t peak_ = 0;
};
}
}
