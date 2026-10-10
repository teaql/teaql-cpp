#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include "checker_fixture.hpp"
#include <iostream>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class E, class F>
static void rejected(F&& call, const char* message) {
    bool caught = false;
    try {
        call();
    } catch (const E&) {
        caught = true;
    }
    check(caught, message);
}

static FixEvidence evidence(Memory r, std::string_view label = "framework.clock") {
    return FixEvidence(std::allocator_arg,
                       Allocator(r),
                       "Long entity type to force owned allocation",
                       "long.modelPath",
                       FixEvidenceSource::Clock,
                       label);
}

struct Notes {
    using allocator_type = Allocator;
    Vector<Timestamp> times;
    unsigned cancels = 0, finishes = 0;

    Notes(std::allocator_arg_t, Allocator a) : times(a.resource()) {}
};

struct Child {
    using allocator_type = Allocator;
    const UserContext& user;
    Notes& notes;
    bool pending = true, fail = false;
    AsyncOperation<int>* descendant = nullptr;

    Child(std::allocator_arg_t,
          Allocator,
          const UserContext& user,
          Notes& notes,
          bool pending,
          bool fail)
        : user(user), notes(notes), pending(pending), fail(fail) {}

    std::optional<int> poll(WakeSignal& wake) {
        notes.times.push_back(user.fix_time());
        user.record_fix_evidence(evidence(user.memory_resource()));
        if (fail) {
            throw std::runtime_error("child failure");
        }
        if (pending) {
            wake.notify();
            return {};
        }
        return 7;
    }

    void finish_and_wait() noexcept {
        ++notes.finishes;
        notes.times.push_back(user.fix_time());
    }

    void cancel_and_wait() noexcept {
        ++notes.cancels;
        notes.times.push_back(user.fix_time());
        if (descendant) {
            descendant->cancel();
            descendant = nullptr;
        }
    }
};

struct Parent {
    using allocator_type = Allocator;
    const UserContext& user;
    Notes& notes;
    AsyncOperation<int>* descendant = nullptr;

    Parent(std::allocator_arg_t, Allocator, const UserContext& user, Notes& notes)
        : user(user), notes(notes) {}

    std::optional<int> poll(WakeSignal&) {
        if (!descendant) {
            descendant =
                &make_operation<int, Child>(user.memory_resource(), user, notes, true, false);
        }
        (void)descendant->poll();
        return {};
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        if (descendant) {
            descendant->cancel();
            descendant = nullptr;
        }
    }
};

static void scopes_and_copies() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    GlobalContext foreign;
    auto unrelated = foreign.make_user_context();
    auto before = Timestamp::now();
    auto now = user->fix_time();
    auto after = Timestamp::now();
    check(now.as_millis() >= before.as_millis() && now.as_millis() <= after.as_millis(),
          "standalone Fix time comes from the clock");
    auto r = user->memory_resource();
    {
        auto value = evidence(r);
        user->record_fix_evidence(value);
    }
    check(user->last_fix_evidence().empty(), "unscoped evidence is ignored");
    auto& outer = user->make<detail::GraphFixState>(Timestamp(-777));
    auto& inner = user->make<detail::GraphFixState>(Timestamp(99));
    {
        detail::GraphFixActivation active(*user, outer);
        check(sibling->fix_time() == Timestamp(-777),
              "same-application synchronous calls inherit the Fix scope");
        unrelated->record_fix_evidence(evidence(unrelated->memory_resource()));
        check(outer.evidence.empty(), "another application does not inherit borrowed Fix state");
        sibling->record_fix_evidence(evidence(sibling->memory_resource()));
        check(outer.evidence.size() == 1 && outer.evidence[0].memory_resource() == r,
              "cross-User evidence is copied into the operation owner");
        try {
            detail::GraphFixActivation nested(*user, inner);
            check(user->fix_time() == Timestamp(99), "nested time activates");
            throw std::runtime_error("unwind");
        } catch (const std::runtime_error&) {
        }
        check(user->fix_time() == Timestamp(-777), "nested exception restores the parent scope");
    }
    rejected<std::invalid_argument>(
        [&] {
            detail::GraphFixActivation wrong(*sibling, outer);
        },
        "activation cannot retain another User's state");
    detail::ContextFixAccess::replace_last(*user, outer.evidence);
    auto& kept = global.make<Vector<FixEvidence>>(user->last_fix_evidence());
    {
        auto snapshot = user->last_fix_evidence();
        snapshot.front().source_label = "independent snapshot";
        check(user->last_fix_evidence().front().source_label == "framework.clock",
              "last evidence getters return independent copies");
    }
    user.reset();
    check(kept.size() == 1 && kept.front().entity_type.starts_with("Long entity"),
          "explicit Global evidence copies survive actual User destruction");
}

static void operations_and_close() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& notes = user->make<Notes>();
    notes.times.reserve(16);
    auto& state = user->make<Child>(*user, notes, true, false);
    auto& child = user->make_resource<AsyncOperation<int>>(state);
    auto& operation = detail::with_graph_fix_scope(*user, child, Timestamp(12));
    auto ticket = operation.wake_signal().ticket();
    check(!operation.poll(), "pending child remains pending");
    check(operation.wake_signal().ticket() != ticket, "child wakes are forwarded");
    check(user->last_fix_evidence().empty(), "pending scope does not publish evidence");
    state.pending = false;
    check(*operation.poll() == 7, "ready child produces result");
    check(notes.times.size() == 3 && std::all_of(notes.times.begin(),
                                                 notes.times.end(),
                                                 [](Timestamp time) {
                                                     return time == Timestamp(12);
                                                 }),
          "all polls and finish run under one fixed time");
    check(user->last_fix_evidence().size() == 2,
          "normal completion publishes all collected evidence");
    auto& failures = user->make<Notes>();
    failures.times.reserve(4);
    auto& failing = make_operation<int, Child>(r, *user, failures, false, true);
    auto& wrapped = detail::with_graph_fix_scope(*user, failing, Timestamp(24));
    rejected<std::runtime_error>(
        [&] {
            (void)wrapped.poll();
        },
        "exception propagates from inner child");
    check(failures.cancels == 1 && failures.times.back() == Timestamp(24) &&
              user->last_fix_evidence().size() == 2,
          "failed child cleans up in scope without replacing prior evidence");
    auto& closing = global.make<Notes>();
    closing.times.reserve(4);
    auto& parent = make_operation<int, Parent>(r, *user, closing);
    auto& pending = detail::with_graph_fix_scope(*user, parent, Timestamp(36));
    check(!pending.poll(), "lazy descendant work starts under the outer scope");
    user->close();
    check(
        closing.cancels == 1 && closing.times.size() == 2 && closing.times[0] == Timestamp(36) &&
            closing.times[1] == Timestamp(36),
        "close cancels lazy descendants under their original Fix time before reclaiming host memory");
    check(user->allocation_stats().live_bytes == 0,
          "close reclaims all operation and evidence blocks");
    rejected<std::logic_error>(
        [&] {
            (void)user->fix_time();
        },
        "closed Context rejects Fix access");
}

static void payload_does_not_repeat_checker() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto callback = user->make_global<checker_fixture::Plain>("fill");
    auto& registry = user->make<InMemoryCheckerRegistry>();
    registry.register_checker(*user, callback);
    user->set_checker_registry(*user, registry);
    auto& metadata = user->make<InMemoryMetadataStore>();
    metadata.register_entity(checker_fixture::Item::entity_descriptor(r));
    user->set_metadata(*user, metadata);
    auto& values = user->make<EntityValues>();
    values.fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
    mark_entity_status(values, CheckObjectStatus::Update);
    auto result = detail::ContextFixAccess::validate_required_create_payload(
        *user, "Item", values, ObjectLocation::root(r));
    check(!result && result.error().check_results().size() == 2 && user->get(callback).calls == 0,
          "CREATE payload ignores update marker and checks metadata without rerunning custom Fix");
    check(!values.fields.contains("name"), "validation leaves sparse mutation intent unchanged");
}

static void failed_handoff() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    auto& notes = global.make<Notes>();
    notes.times.reserve(4);
    auto& child =
        make_operation<int, Child>(sibling->memory_resource(), *sibling, notes, true, false);
    rejected<std::invalid_argument>(
        [&] {
            (void)detail::with_graph_fix_scope(*user, child);
        },
        "foreign child operation is rejected");
    check(child.status() == AsyncStatus::Cancelled && notes.cancels == 1,
          "rejected child is cancelled exactly once");
}

int main() {
    auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scopes_and_copies();
        operations_and_close();
        payload_does_not_repeat_checker();
        failed_handoff();
        std::pmr::set_default_resource(old);
        std::cout << "Graph Fix scope/evidence/CREATE contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(old);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
