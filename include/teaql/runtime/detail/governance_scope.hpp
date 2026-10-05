#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/mutation_governance.hpp>

namespace teaql::detail {
// Internal task-local equivalent. The application owns the activation slot;
// each future owns its snapshot. No snapshot is active between polls.
class GovernanceActivation final {
public:
    GovernanceActivation(const UserContext& context, const MutationGovernanceSnapshot& snapshot)
        : global_(context.global_), previous_(global_.active_mutation_governance_) {
        context.ensure_open();
        global_.active_mutation_governance_ = &snapshot;
    }

    ~GovernanceActivation() {
        global_.active_mutation_governance_ = previous_;
    }

    GovernanceActivation(const GovernanceActivation&) = delete;
    GovernanceActivation& operator=(const GovernanceActivation&) = delete;

    static std::optional<MutationGovernanceSnapshot> current(const UserContext& context) {
        context.ensure_open();
        auto* value = context.global_.active_mutation_governance_;
        if (value) {
            return clone_in(context.memory_resource(), *value);
        }
        return {};
    }

private:
    GlobalContext& global_;
    const MutationGovernanceSnapshot* previous_;
};

inline std::optional<MutationGovernanceSnapshot>
current_mutation_governance(const UserContext& context) {
    return GovernanceActivation::current(context);
}

template <class T>
class GovernedOperation {
public:
    using allocator_type = Allocator;

    GovernedOperation(std::allocator_arg_t,
                      Allocator a,
                      const UserContext& context,
                      const MutationGovernanceSnapshot& snapshot,
                      AsyncOperation<T>& child)
        : context_(context), snapshot_(std::allocator_arg, a, snapshot), child_(&child) {
        if (a.resource() != context.memory_resource() || child.memory_resource() != a.resource()) {
            throw std::invalid_argument("Governed operation must belong to its UserContext");
        }
    }

    std::optional<T> poll(WakeSignal& wake) {
        GovernanceActivation active(context_, snapshot_);
        forwarding_.attach(child_->wake_signal(), wake);
        if (auto* value = child_->poll()) {
            return clone_in(context_.memory_resource(), *value);
        }
        // Lazily created descendants must be cancelled under this snapshot
        // before the arena walks their independently registered stop records.
        closing_.attach(resource_arena(context_.memory_resource()), this, [](void* p) noexcept {
            static_cast<GovernedOperation*>(p)->cancel_and_wait();
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
            GovernanceActivation active(context_, snapshot_);
            child_->cancel();
            child_ = nullptr;
        }
    }

private:
    const UserContext& context_;
    MutationGovernanceSnapshot snapshot_;
    AsyncOperation<T>* child_;
    WakeSignal::Forward forwarding_;
    Arena::Quiescence closing_;
};

template <class T>
AsyncOperation<T>& with_mutation_governance(const UserContext& context,
                                            const MutationGovernanceSnapshot& snapshot,
                                            AsyncOperation<T>& child) {
    return make_operation<T, GovernedOperation<T>>(
        context.memory_resource(), context, snapshot, child);
}
}
