#pragma once
#include <teaql/runtime/resources.hpp>
#include <limits>

namespace teaql {
// Context metadata is checked before the arena pointer. Global expressions obey
// the application-owned Global lifetime; no diagnostics are promised past it.
class ExpressionScope {
public:
    explicit ExpressionScope(Memory resource) : arena_(&detail::resource_arena(resource)) {
        auto* context = dynamic_cast<detail::ContextArena*>(arena_);
        // Generic Arena use remains valid for standalone runtime-model fixtures.
        // Context-backed domains additionally provide deletion-safe metadata.
        state_ = context ? context->state() : nullptr;
        identity_ = arena_->allocation_identity(0);
    }

    Arena& get(std::source_location access = std::source_location::current()) const {
        if (state_ && state_->phase != detail::ContextState::Phase::Open) {
            throw ReferenceError(
                ReferenceError::Kind::Closed, identity_, access, state_->closed_at);
        }
        arena_->check_thread();
        if (arena_->closed() || arena_->closing()) {
            throw ReferenceError(ReferenceError::Kind::Closed, identity_, access);
        }
        return *arena_;
    }

    bool is_open() const noexcept {
        if (state_) {
            return state_->phase == detail::ContextState::Phase::Open;
        }
        return !arena_->closed() && !arena_->closing();
    }

    Memory memory_resource() const {
        return &get();
    }

    void require_same(const ExpressionScope& other) const {
        (void)get();
        (void)other.get();
        if (identity_ != other.identity_) {
            throw ReferenceError(ReferenceError::Kind::ForeignContext,
                                 other.identity_,
                                 std::source_location::current());
        }
    }

private:
    Arena* arena_;
    const detail::ContextState* state_ = nullptr;
    std::uint64_t identity_;
};

namespace detail {
struct EntityLifetimeStamp {
    std::uint64_t generation = 1;
    bool alive = true;
};

// This anchor describes one C++ object lifetime, independently of the shared
// mutation ledger/graph. Moves invalidate earlier borrows without allocating.
class EntityLifetimeAnchor {
public:
    explicit EntityLifetimeAnchor(Memory resource) : scope_(resource) {}

    EntityLifetimeAnchor(const EntityLifetimeAnchor&) = delete;

    EntityLifetimeAnchor(EntityLifetimeAnchor&& other) noexcept
        : scope_(other.scope_), stamp_(std::exchange(other.stamp_, nullptr)),
          slot_(std::exchange(other.slot_, 0)) {
        if (stamp_ && scope_.is_open()) {
            if (stamp_->generation == std::numeric_limits<std::uint64_t>::max()) {
                stamp_->alive = false;
            } else {
                ++stamp_->generation;
            }
        }
    }

    ~EntityLifetimeAnchor() {
        // During Context close stamps may already have been destroyed. Metadata
        // permits this decision without reading a reclaimed User arena.
        if (stamp_ && scope_.is_open()) {
            stamp_->alive = false;
        }
    }

    struct Borrow {
        ExpressionScope scope;
        std::uint32_t slot;
        std::uint64_t generation;

        void check(std::source_location access = std::source_location::current()) const {
            auto& arena = scope.get(access);
            auto identity = arena.allocation_identity(slot);
            auto& stamp = *static_cast<EntityLifetimeStamp*>(
                arena.resolve(slot, typeid(EntityLifetimeStamp), identity, access));
            if (!stamp.alive || stamp.generation != generation) {
                throw ReferenceError(ReferenceError::Kind::Dead, identity, access);
            }
        }
    };

    Borrow borrow() const {
        auto& arena = scope_.get();
        if (!stamp_ || !stamp_->alive) {
            auto value = arena.make_identified<EntityLifetimeStamp>();
            stamp_ = &value.object;
            slot_ = value.slot;
        }
        return {scope_, slot_, stamp_->generation};
    }

private:
    ExpressionScope scope_;
    mutable EntityLifetimeStamp* stamp_ = nullptr;
    mutable std::uint32_t slot_ = 0;
};
}
}
