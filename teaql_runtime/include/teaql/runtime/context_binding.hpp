#pragma once
#include <teaql/runtime/context.hpp>

namespace teaql {
// A non-owning value. Check application-owned metadata before touching the User
// object or arena, so the value remains safely destructible after User deletion.
// The application GlobalContext must still exist, as with all Context diagnostics.
class ContextBinding {
public:
    explicit ContextBinding(UserContext& user_context)
        : user_context_(&user_context), arena_(&user_context.memory_), state_(user_context.state_),
          identity_(user_context.scope_id(false)) {
        user_context.check_public();
    }

    UserContext& get(std::source_location access = std::source_location::current()) const {
        if (state_->phase != detail::ContextState::Phase::Open) {
            throw ReferenceError(
                ReferenceError::Kind::Closed, identity_, access, state_->closed_at);
        }
        arena_->check_thread();
        return *user_context_;
    }

    Memory memory_resource() const {
        return get().memory_resource();
    }

    std::uint64_t identity() const noexcept {
        return identity_;
    }

    void require_same(const ContextBinding& other) const {
        (void)get();
        (void)other.get();
        if (identity_ != other.identity_) {
            throw ReferenceError(ReferenceError::Kind::ForeignContext,
                                 other.identity_,
                                 std::source_location::current());
        }
    }

private:
    UserContext* user_context_;
    Arena* arena_;
    const detail::ContextState* state_;
    std::uint64_t identity_;
};

static_assert(std::is_trivially_destructible_v<ContextBinding>);
}
