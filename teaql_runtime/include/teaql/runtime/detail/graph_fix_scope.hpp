#pragma once
#include <teaql/runtime/context.hpp>

namespace teaql::detail {
struct GraphFixState {
    using allocator_type = Allocator;
    Timestamp time;
    Vector<FixEvidence> evidence;

    GraphFixState(std::allocator_arg_t, Allocator a, Timestamp time)
        : time(time), evidence(a.resource()) {}

    GraphFixState(const GraphFixState&) = delete;

    Memory memory_resource() const {
        return evidence.get_allocator().resource();
    }
};

// Borrowed application activation; each operation owns its state in User memory.
class GraphFixActivation {
public:
    GraphFixActivation(const UserContext& context, GraphFixState& state)
        : global_(context.global_), previous_(global_.active_graph_fix_) {
        context.memory_.check_thread();
        context.ensure_open();
        if (state.memory_resource() != context.memory_resource()) {
            throw std::invalid_argument("Graph Fix state must belong to its operation UserContext");
        }
        global_.active_graph_fix_ = &state;
    }

    ~GraphFixActivation() {
        global_.active_graph_fix_ = previous_;
    }

    GraphFixActivation(const GraphFixActivation&) = delete;
    GraphFixActivation& operator=(const GraphFixActivation&) = delete;

    static GraphFixState* current(const UserContext& context) {
        context.memory_.check_thread();
        context.ensure_open();
        return context.global_.active_graph_fix_;
    }

private:
    GlobalContext& global_;
    GraphFixState* previous_;
};

struct ContextFixAccess {
    static void replace_last(const UserContext& context, std::span<const FixEvidence> evidence) {
        context.replace_last_fix_evidence(evidence);
    }

    static RuntimeHookResult validate_required_create_payload(const UserContext& context,
                                                              std::string_view entity,
                                                              const EntityValues& values,
                                                              const ObjectLocation& location) {
        return context.validate_required_create_payload(entity, values, location);
    }
};

template <class T>
class GraphFixedOperation {
public:
    using allocator_type = Allocator;

    GraphFixedOperation(std::allocator_arg_t,
                        Allocator a,
                        const UserContext& context,
                        AsyncOperation<T>& child,
                        std::optional<Timestamp> fixed_time)
        : context_(context), child_(&child), fixed_time_(fixed_time), resource_(a.resource()) {
        if (resource_ != context.memory_resource() || child.memory_resource() != resource_) {
            throw std::invalid_argument("Graph Fix operation must belong to its UserContext");
        }
    }

    std::optional<T> poll(WakeSignal& wake) {
        if (!state_) {
            // Source async save samples time when first polled, not at construction.
            auto* active = GraphFixActivation::current(context_);
            if (!fixed_time_ && active && active->memory_resource() == resource_) {
                // Nested adapters belong to the same save operation. Preserve
                // its clock and evidence instead of replacing them with an empty scope.
                state_ = active;
            } else {
                state_ =
                    &context_.make<GraphFixState>(fixed_time_ ? *fixed_time_ : Timestamp::now());
            }
        }
        const T* result;
        {
            GraphFixActivation active(context_, *state_);
            forwarding_.attach(child_->wake_signal(), wake);
            result = child_->poll();
        }
        if (result) {
            auto owned = copy_or_take_completed(*child_, *result);
            ContextFixAccess::replace_last(context_, state_->evidence);
            return owned;
        }
        // Descendant work created by a poll must be cancelled under this scope
        // before its independently registered stop callbacks are visited.
        closing_.attach(resource_arena(resource_), this, [](void* self) noexcept {
            static_cast<GraphFixedOperation*>(self)->cancel_and_wait();
        });
        return {};
    }

    void finish_and_wait() noexcept {
        closing_.reset();
        forwarding_.reset();
        child_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        closing_.reset();
        forwarding_.reset();
        if (child_) {
            if (state_) {
                GraphFixActivation active(context_, *state_);
                child_->cancel();
            } else {
                child_->cancel();
            }
            child_ = nullptr;
        }
        // Cancellation and exceptions do not reach source last-evidence publication.
    }

private:
    const UserContext& context_;
    AsyncOperation<T>* child_;
    std::optional<Timestamp> fixed_time_;
    Memory resource_;
    GraphFixState* state_ = nullptr;
    WakeSignal::Forward forwarding_;
    Arena::Quiescence closing_;
};

// Internal scope primitive for the audited save entry. It does not itself save.
template <class T>
AsyncOperation<T>& with_graph_fix_scope(const UserContext& context,
                                        AsyncOperation<T>& child,
                                        std::optional<Timestamp> time = {}) {
    try {
        return make_operation<T, GraphFixedOperation<T>>(
            context.memory_resource(), context, child, time);
    } catch (...) {
        child.cancel();
        throw;
    }
}
}
