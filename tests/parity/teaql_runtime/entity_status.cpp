// Original entity-status contracts and complete public transition diagnostics.
#include <teaql/runtime/entity_status.hpp>
#include <unordered_set>
#include <type_traits>
#include <array>
#include <iostream>
#include <optional>
#include <utility>

using namespace teaql;
using S = EntityStatus;
using A = EntityAction;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void default_is_new() {
    check(S{} == S::New, "original default is New");
}

static void copy_equality_and_hash_capabilities() {
    std::unordered_set<S> statuses;
    std::unordered_set<A> actions;
    for (unsigned index = 0; index < 7; ++index) {
        auto value = S(index);
        auto copy = value;
        check(copy == value, "source Copy/Clone retain status identity");
        statuses.insert(copy);
        statuses.insert(value);
    }
    for (unsigned index = 0; index < 4; ++index) {
        auto value = A(index);
        auto copy = value;
        check(copy == value, "source Copy/Clone retain action identity");
        actions.insert(copy);
        actions.insert(value);
    }
    check(statuses.size() == 7 && actions.size() == 4,
          "both enum hash/equality contracts distinguish every source variant");
}

static void original_valid_transitions() {
    struct Transition {
        S before;
        A action;
        S after;
    };

    constexpr std::array original{
        Transition{S::New, A::Update, S::New},
        Transition{S::New, A::Persist, S::Persisted},
        Transition{S::Persisted, A::Update, S::Updated},
        Transition{S::Persisted, A::Delete, S::UpdatedDeleted},
        Transition{S::PersistedDeleted, A::Recover, S::UpdatedRecover},
        Transition{S::Updated, A::Update, S::Updated},
        Transition{S::Updated, A::Persist, S::Persisted},
        Transition{S::UpdatedDeleted, A::Persist, S::PersistedDeleted},
        Transition{S::UpdatedDeleted, A::Delete, S::UpdatedDeleted},
        Transition{S::UpdatedRecover, A::Persist, S::Persisted},
        Transition{S::UpdatedRecover, A::Recover, S::UpdatedRecover},
    };
    for (const auto& transition : original) {
        check(next_status(transition.before, transition.action) == transition.after,
              "original valid transition has its exact next state");
    }
}

static void invalid_transition_returns_err() {
    constexpr std::array original{
        std::pair{S::New, A::Delete},
        std::pair{S::New, A::Recover},
        std::pair{S::Persisted, A::Persist},
        std::pair{S::Persisted, A::Recover},
        std::pair{S::PersistedDeleted, A::Update},
        std::pair{S::PersistedDeleted, A::Delete},
        std::pair{S::PersistedDeleted, A::Persist},
        std::pair{S::Updated, A::Delete},
        std::pair{S::Updated, A::Recover},
        std::pair{S::UpdatedDeleted, A::Update},
        std::pair{S::UpdatedDeleted, A::Recover},
        std::pair{S::UpdatedRecover, A::Update},
        std::pair{S::UpdatedRecover, A::Delete},
        std::pair{S::Refer, A::Update},
        std::pair{S::Refer, A::Delete},
        std::pair{S::Refer, A::Persist},
        std::pair{S::Refer, A::Recover},
    };
    // Independent names preserve the Rust error payload, not only rejection.
    constexpr std::array<std::string_view, 7> states{"New",
                                                     "Persisted",
                                                     "PersistedDeleted",
                                                     "Updated",
                                                     "UpdatedDeleted",
                                                     "UpdatedRecover",
                                                     "Refer"};
    constexpr std::array<std::string_view, 4> actions{"Update", "Delete", "Persist", "Recover"};
    for (const auto& [status, action] : original) {
        bool rejected = false;
        try {
            (void)next_status(status, action);
        } catch (const std::invalid_argument& error) {
            const auto expected = "invalid entity status transition: " +
                                  std::string(states.at(static_cast<std::size_t>(status))) + " + " +
                                  std::string(actions.at(static_cast<std::size_t>(action)));
            check(error.what() == expected, "original complete invalid-transition reason");
            rejected = true;
        }
        check(rejected, "original invalid transition rejects");
    }
    for (std::size_t index = 0; index < states.size(); ++index) {
        check(to_string(static_cast<S>(index)) == states[index], "original status Display");
    }
    for (std::size_t index = 0; index < actions.size(); ++index) {
        check(to_string(static_cast<A>(index)) == actions[index], "original action Display");
    }
}

static void need_persist_flags() {
    constexpr std::array original{
        std::pair{S::New, true},
        std::pair{S::Persisted, false},
        std::pair{S::PersistedDeleted, false},
        std::pair{S::Updated, true},
        std::pair{S::UpdatedDeleted, true},
        std::pair{S::UpdatedRecover, true},
        std::pair{S::Refer, false},
    };
    for (const auto& [status, expected] : original) {
        check(need_persist(status) == expected, "original seven persistence flags");
    }
}

static void helper_predicates() {
    check(is_new(S::New) && !is_new(S::Persisted), "original is_new assertions");
    check(is_updated(S::Updated) && !is_updated(S::New), "original is_updated assertions");
    check(is_deleted(S::UpdatedDeleted) && is_deleted(S::PersistedDeleted) &&
              !is_deleted(S::Updated),
          "original is_deleted assertions");
    check(is_recover(S::UpdatedRecover) && !is_recover(S::Updated),
          "original is_recover assertions");
    for (S state : {S::New,
                    S::Persisted,
                    S::PersistedDeleted,
                    S::Updated,
                    S::UpdatedDeleted,
                    S::UpdatedRecover,
                    S::Refer}) {
        check(is_new(state) == (state == S::New) && is_updated(state) == (state == S::Updated) &&
                  is_deleted(state) ==
                      (state == S::UpdatedDeleted || state == S::PersistedDeleted) &&
                  is_recover(state) == (state == S::UpdatedRecover),
              "complete public predicate domain");
    }
}

static void full_lifecycle_create_update_delete() {
    auto state = S{};
    check(state == S::New, "original lifecycle initial state");
    constexpr std::array original{
        std::pair{A::Persist, S::Persisted},
        std::pair{A::Update, S::Updated},
        std::pair{A::Persist, S::Persisted},
        std::pair{A::Delete, S::UpdatedDeleted},
        std::pair{A::Persist, S::PersistedDeleted},
        std::pair{A::Recover, S::UpdatedRecover},
        std::pair{A::Persist, S::Persisted},
    };
    for (const auto& [action, expected] : original) {
        state = next_status(state, action);
        check(state == expected, "every original lifecycle intermediate state");
    }
}

static_assert(next_status(S::New, A::Persist) == S::Persisted);
static_assert(std::is_trivially_copyable_v<S> && std::is_trivially_copyable_v<A>);
static_assert(to_string(A::Recover) == "Recover");
static_assert(need_persist(S::UpdatedRecover));

int main() {
    try {
        default_is_new();
        copy_equality_and_hash_capabilities();
        original_valid_transitions();
        invalid_transition_returns_err();
        need_persist_flags();
        helper_predicates();
        full_lifecycle_create_update_delete();
        std::cout << "Original sixteen entity-status contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
