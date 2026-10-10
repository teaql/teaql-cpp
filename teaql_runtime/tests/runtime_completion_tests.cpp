#include "../../tests/support/runtime_completion_fixture.hpp"
#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include <iostream>

using namespace completion_fixture;
using Access = detail::EntityDataServiceAccess;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void run(Controls controls,
                F&& call,
                unsigned expected_errors = 1,
                std::string_view expected_label = {}) {
    Counts counts;
    GlobalContext global;
    auto user = global.make_user_context();
    {
        metadata(*user);
        auto& executor = user->make<Executor>(counts, controls);
        auto& work = call(*user, executor);
        if (controls.has_pending()) {
            check(!work.poll() && counts.wake, "Nested native work remains Pending");
            auto ticket = work.wake_signal().ticket();
            counts.wake->notify();
            check(work.wake_signal().ticket() != ticket, "Wrapper forwards the native wake");
        } else {
            const auto& result = work.wait();
            check(!result, "Native failure is propagated by the real entry");
            auto text = result.error().to_string(user->memory_resource());
            check(
                text.find("native failure") != String::npos && text.size() >= 4096,
                "Full native payload survives concrete or source-defined RuntimeError conversion");
            check(expected_label.empty() || text.find(expected_label) != String::npos,
                  "Terminal failure precedence retains the expected native error");
            check(&work.wait() == &result && counts.produced == expected_errors,
                  "Completed outer result is stable and does not re-execute native work");
        }
    }
    const auto finished = counts.finished;
    user.reset();
    check(global.live_user_bytes() == 0 && !counts.wake,
          "Actual User deletion reclaims blocks after detaching native work");
    check(counts.produced == counts.destroyed && counts.finished == finished &&
              counts.cancelled == (controls.has_pending() ? 1u : 0u),
          "Native error destruction and terminal hooks run exactly once");
}

enum class Scope { Base, Context, Entity };

template <Scope S, unsigned Kind>
static void query_entry(bool pending) {
    run(Controls{.fail_query = 1, .pending = pending},
        [](UserContext& user, Executor& executor) -> auto& {
            auto r = user.memory_resource();
            auto& query = user.make<SelectQuery>("Parent");
            auto& context = user.make<detail::ContextDataService<Executor>>(
                user,
                ResourceBorrow<Executor>(
                    std::allocator_arg, Allocator(r), executor, detail::resource_arena(r)));
            auto dispatch = [&](auto& service) -> auto& {
                if constexpr (Kind == 0) {
                    return service.fetch_all(query);
                } else if constexpr (Kind == 1) {
                    return service.fetch_smart_list(query);
                } else {
                    return service.template fetch_entities<Model>(query);
                }
            };
            if constexpr (S == Scope::Base) {
                auto base = context.data_service();
                return dispatch(base);
            } else if constexpr (S == Scope::Context) {
                return dispatch(context);
            } else {
                auto& service =
                    EntityDataService<Executor>::for_executor(user, "Parent", user, executor);
                PurposedSelectQuery purpose(
                    std::allocator_arg, Allocator(r), query, "Inspect native error");
                if constexpr (Kind == 0) {
                    return service.fetch_all(purpose);
                } else if constexpr (Kind == 1) {
                    return service.fetch_smart_list(purpose);
                } else {
                    return service.template fetch_entities<Model>(purpose);
                }
            }
        });
}

template <Scope S, unsigned Kind>
static void mutation_entry(bool pending) {
    run(Controls{.fail_mutation = 1, .pending = pending},
        [](UserContext& user, Executor& executor) -> auto& {
            auto r = user.memory_resource();
            auto command = [&] {
                if constexpr (Kind == 0) {
                    InsertCommand value(std::allocator_arg, Allocator(r), "Parent");
                    value.value("id", std::uint64_t{7}).value("name", "input");
                    return value;
                } else if constexpr (Kind == 1) {
                    UpdateCommand value(
                        std::allocator_arg, Allocator(r), "Parent", std::uint64_t{7});
                    value.value("name", "input");
                    return value;
                } else if constexpr (Kind == 2) {
                    return DeleteCommand(
                        std::allocator_arg, Allocator(r), "Parent", std::uint64_t{7});
                } else if constexpr (Kind == 3) {
                    return RecoverCommand(
                        std::allocator_arg, Allocator(r), "Parent", std::uint64_t{7}, 3);
                } else if constexpr (Kind == 4) {
                    BatchInsertCommand value(std::allocator_arg, Allocator(r), "Parent");
                    auto& fields = value.batch_values.emplace_back();
                    fields.fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
                    return value;
                } else {
                    BatchUpdateCommand value(std::allocator_arg, Allocator(r), "Parent");
                    value.update_fields.emplace_back("name");
                    value.batch_ids.emplace_back(std::uint64_t{7});
                    auto& fields = value.batch_values.emplace_back();
                    fields.fields.emplace(String("name", r), Value(r, "input"));
                    return value;
                }
            }();
            auto dispatch = [&](auto& service) -> auto& {
                if constexpr (Kind == 0) {
                    return service.insert(command);
                } else if constexpr (Kind == 1) {
                    return service.update(command);
                } else if constexpr (Kind == 2) {
                    return service.delete_command(command);
                } else if constexpr (Kind == 3) {
                    return service.recover(command);
                } else if constexpr (Kind == 4) {
                    return service.batch_insert(command);
                } else {
                    return service.batch_update(command);
                }
            };
            auto& context = user.make<detail::ContextDataService<Executor>>(
                user,
                ResourceBorrow<Executor>(
                    std::allocator_arg, Allocator(r), executor, detail::resource_arena(r)));
            if constexpr (S == Scope::Base) {
                auto base = context.data_service();
                return dispatch(base);
            } else if constexpr (S == Scope::Context) {
                return dispatch(context);
            } else {
                auto& service =
                    EntityDataService<Executor>::for_executor(user, "Parent", user, executor);
                if constexpr (Kind == 0) {
                    return Access::insert_internal(service, command);
                } else if constexpr (Kind == 1) {
                    return Access::update_internal(service, command);
                } else if constexpr (Kind == 2) {
                    return Access::delete_internal(service, command);
                } else if constexpr (Kind == 3) {
                    return Access::recover_internal(service, command);
                } else if constexpr (Kind == 4) {
                    return Access::execute_prepared_batch_insert(service, command);
                } else {
                    return Access::execute_prepared_batch_update(service, command);
                }
            }
        });
}

template <Scope S>
static void entries() {
    for (bool pending : {false, true}) {
        query_entry<S, 0>(pending);
        query_entry<S, 1>(pending);
        query_entry<S, 2>(pending);
        mutation_entry<S, 0>(pending);
        mutation_entry<S, 1>(pending);
        mutation_entry<S, 2>(pending);
        mutation_entry<S, 3>(pending);
        mutation_entry<S, 4>(pending);
        mutation_entry<S, 5>(pending);
    }
}

struct ScopedChild {
    using allocator_type = Allocator;
    const UserContext& user;
    Counts& counts;
    Controls& controls;

    ScopedChild(std::allocator_arg_t,
                Allocator,
                const UserContext& user,
                Counts& counts,
                Controls& controls)
        : user(user), counts(counts), controls(controls) {}

    bool scopes() const {
        auto governance = detail::current_mutation_governance(user);
        return governance && governance->execution_id == "native execution" &&
               user.fix_time() == Timestamp(1234);
    }

    std::optional<Result<std::monostate, MoveOnlyError>> poll(WakeSignal& wake) {
        counts.scope_seen = scopes();
        if (controls.pending) {
            counts.wake = &wake;
            return {};
        }
        return failure<std::monostate>(user.memory_resource(), counts);
    }

    void finish_and_wait() noexcept {
        ++counts.finished;
        counts.wake = nullptr;
    }

    void cancel_and_wait() noexcept {
        try {
            counts.cancel_scope_seen = scopes();
        } catch (...) {
            counts.cancel_scope_seen = false;
        }
        ++counts.cancelled;
        counts.wake = nullptr;
    }
};

static void scoped_completions() {
    for (bool pending : {false, true}) {
        Counts counts;
        Controls controls{.pending = pending};
        GlobalContext global;
        auto user = global.make_user_context();
        {
            auto r = user->memory_resource();
            auto& child = make_operation<Result<std::monostate, MoveOnlyError>, ScopedChild>(
                r, *user, counts, controls);
            MutationGovernanceSnapshot snapshot(std::allocator_arg,
                                                Allocator(r),
                                                "native execution",
                                                "native request",
                                                MutationPolicySource::GeneratedDefault,
                                                MutationPolicyApprovalStatus::NotApplicable);
            auto& governed = detail::with_mutation_governance(*user, snapshot, child);
            auto& work = detail::with_graph_fix_scope(*user, governed, Timestamp(1234));
            if (pending) {
                check(!work.poll(), "Scoped native work stays Pending");
            } else {
                const auto& result = work.wait();
                check(
                    !result && result.error().message.size() == 4096 && counts.destroyed == 0,
                    "Nested scopes transfer the concrete error without Clone or early destruction");
            }
            check(counts.scope_seen && !detail::current_mutation_governance(*user),
                  "Both snapshots are active inside native polling and detach between polls");
        }
        user.reset();
        check(counts.produced == counts.destroyed && global.live_user_bytes() == 0 &&
                  counts.cancelled == (pending ? 1u : 0u) &&
                  counts.finished == (pending ? 0u : 1u) && (!pending || counts.cancel_scope_seen),
              "Actual User deletion cancels native work under both scopes and reclaims it once");
    }
}

static GraphNode graph_input(Memory r, bool create) {
    GraphNode node(std::allocator_arg, Allocator(r), "Parent");
    node.value("id", std::uint64_t{7}).value("name", "input");
    node.with_operation(create ? GraphOperation::Create : GraphOperation::Upsert);
    return node;
}

static void graph_and_ledger() {
    for (bool pending : {false, true}) {
        run(Controls{.fail_query = 1, .pending = pending},
            [](UserContext& user, Executor& executor) -> auto& {
                auto& service =
                    EntityDataService<Executor>::for_executor(user, "Parent", user, executor);
                return service.plan_graph(graph_input(user.memory_resource(), false));
            });
        run(Controls{.fail_mutation = 1, .pending = pending},
            [](UserContext& user, Executor& executor) -> auto& {
                auto& service =
                    EntityDataService<Executor>::for_executor(user, "Parent", user, executor);
                return Access::save_graph_internal(service,
                                                   graph_input(user.memory_resource(), true));
            });
        run(Controls{.fail_query = 1, .pending_query = pending ? 1u : 0u},
            [](UserContext& user, Executor& executor) -> auto& {
                auto& service =
                    EntityDataService<Executor>::for_executor(user, "Parent", user, executor);
                return Access::save_graph_internal(service,
                                                   graph_input(user.memory_resource(), true));
            });
        for (bool read : {false, true}) {
            Controls controls{
                .fail_query = read ? 1u : 0u, .fail_mutation = read ? 0u : 1u, .pending = pending};
            run(controls, [read](UserContext& user, Executor& executor) -> auto& {
                auto r = user.memory_resource();
                auto& service =
                    EntityDataService<Executor>::for_executor(user, "Parent", user, executor);
                auto& root = user.make<EntityRuntimeState>();
                EntityKey key(
                    std::allocator_arg, Allocator(r), "Parent", Value(r, std::uint64_t{7}));
                root.set(key, "name", Value(r, "input"));
                if (!read) {
                    root.mark_as_new(key);
                }
                auto& locations = user.make<detail::LedgerObjectLocations>();
                return Access::execute_ledger_plan_internal(service, root, locations);
            });
        }
    }
}

static void transactional_saves() {
    for (unsigned stage = 0; stage < 4; ++stage) {
        Controls controls;
        controls.fail_begin = stage == 0;
        controls.fail_commit = stage == 1;
        controls.fail_mutation = stage >= 2 ? 1 : 0;
        controls.fail_rollback = stage == 3;
        run(
            controls,
            [](UserContext& user, Executor& executor) -> auto& {
                user.register_executor(user, executor);
                auto saver = user.require_resource<detail::DynGraphSaver>();
                check(bool(saver),
                      "Registered native executor exposes the actual transactional saver");
                return saver.value().get().save_graph_dyn(
                    user, graph_input(user.memory_resource(), true));
            },
            stage == 3 ? 2 : 1,
            stage == 0   ? "begin"
            : stage == 1 ? "commit"
            : stage == 2 ? "mutation"
                         : "rollback");
    }
    for (unsigned stage = 0; stage < 5; ++stage) {
        Controls controls;
        controls.pending_begin = stage == 0;
        controls.pending_mutation = stage == 1 ? 1 : 0;
        controls.pending_query = stage == 2 ? 1 : 0;
        controls.pending_commit = stage == 3;
        controls.pending_rollback = stage == 4;
        controls.fail_mutation = stage == 4 ? 1 : 0;
        run(controls, [](UserContext& user, Executor& executor) -> auto& {
            user.register_executor(user, executor);
            auto saver = user.require_resource<detail::DynGraphSaver>();
            return saver.value().get().save_graph_dyn(user,
                                                      graph_input(user.memory_resource(), true));
        });
    }
    for (unsigned stage = 0; stage < 3; ++stage) {
        Controls controls;
        controls.fail_mutation = stage == 0 ? 1 : 0;
        controls.fail_query = stage == 1 ? 1 : stage == 2 ? 2 : 0;
        run(controls, [stage](UserContext& user, Executor& executor) -> auto& {
            auto r = user.memory_resource();
            user.register_executor(user, executor);
            auto saver = user.require_resource<detail::DynGraphSaver>();
            auto& root = user.make<EntityRuntimeState>();
            EntityKey key(std::allocator_arg, Allocator(r), "Parent", Value(r, std::uint64_t{7}));
            root.set(key, "name", Value(r, "input"));
            if (stage == 2) {
                root.set_original_version(key, 3);
            } else {
                root.mark_as_new(key);
            }
            return saver.value().get().save_ledger_dyn(user, graph_input(r, true), root);
        });
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        entries<Scope::Base>();
        entries<Scope::Context>();
        entries<Scope::Entity>();
        scoped_completions();
        graph_and_ledger();
        transactional_saves();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime generic completion contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
