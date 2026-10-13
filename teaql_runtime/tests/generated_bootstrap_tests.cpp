#include <array>
#include <algorithm>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <optional>
#include <stdexcept>
#ifdef TEAQL_USE_MODULES
import teaql_runtime;
#else
#include <teaql/runtime.hpp>
#endif

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Control {
    using allocator_type = Allocator;
    Memory resource;
    std::array<unsigned, 3> created{};
    unsigned failures = 0, finished = 0, cancelled = 0;
    bool pending = false, throw_now = false, scope_ok = true;
    RuntimeError::Kind error_kind = RuntimeError::Kind::OptimisticLockConflict;
    String message;
    WakeSignal* child_wake = nullptr;
    UserContext* foreign = nullptr;

    Control(std::allocator_arg_t, Allocator a)
        : resource(a.resource()), message("UNIQUE constraint failed: status_data.id", resource) {}

    Memory memory_resource() const {
        return resource;
    }
};

struct ReadySchema {
    using allocator_type = Allocator;
    Memory resource;

    ReadySchema(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    std::optional<RuntimeSchemaResult> poll(WakeSignal&) {
        return RuntimeSchemaResult::ok(resource, {});
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Provider : runtime::SchemaProvider {
    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& user_context,
                                                       const SchemaInvocation&) const override {
        return make_operation<RuntimeSchemaResult, ReadySchema>(user_context.memory_resource());
    }
};

struct Recipe {
    using allocator_type = Allocator;
    Memory resource;
    const UserContext& user_context;
    Control& control;
    unsigned index;

    Recipe(std::allocator_arg_t,
           Allocator a,
           const UserContext& user_context,
           Control& control,
           unsigned index)
        : resource(a.resource()), user_context(user_context), control(control), index(index) {}

    std::optional<RuntimeSchemaResult> poll(WakeSignal& wake) {
        control.scope_ok &= detail::is_generated_schema_bootstrap(user_context);
        if (index == 1) {
            if (control.throw_now) {
                throw std::runtime_error("recipe threw");
            }
            if (control.pending) {
                control.child_wake = &wake;
                return {};
            }
            if (control.failures) {
                --control.failures;
                auto error = control.error_kind == RuntimeError::Kind::OptimisticLockConflict
                                 ? RuntimeError::optimistic_lock_conflict(resource, "Status", "11")
                             : control.error_kind == RuntimeError::Kind::Graph
                                 ? RuntimeError::graph(resource, control.message)
                                 : RuntimeError::schema(resource, control.message);
                return RuntimeSchemaResult::err(resource, std::move(error));
            }
        }
        return RuntimeSchemaResult::ok(resource, {});
    }

    void finish_and_wait() noexcept {
        control.scope_ok &= detail::is_generated_schema_bootstrap(user_context);
        ++control.finished;
        control.child_wake = nullptr;
    }

    void cancel_and_wait() noexcept {
        control.scope_ok &= detail::is_generated_schema_bootstrap(user_context);
        ++control.cancelled;
        control.child_wake = nullptr;
    }
};

template <unsigned Index>
static GeneratedSchemaBootstrapFuture& recipe(const UserContext& user_context) {
    auto& control = const_cast<Control&>(user_context.require_resource<Control>().value().get());
    ++control.created[Index];
    auto resource = Index == 1 && control.foreign ? control.foreign->memory_resource()
                                                  : user_context.memory_resource();
    return make_operation<RuntimeSchemaResult, Recipe>(resource, user_context, control, Index);
}

static GeneratedSchemaBootstrapFuture& sequence(const UserContext& user_context) {
    // The runner must copy this stack-owned list before the callback returns.
    const std::array<GeneratedSchemaBootstrap, 3> recipes{recipe<0>, recipe<1>, recipe<2>};
    return run_generated_schema_bootstrap(user_context, recipes);
}

struct Fixture {
    GlobalContext global;
    Control& control = global.make<Control>();
    std::unique_ptr<UserContext> user_context = global.make_user_context();

    Fixture() {
        auto& provider = user_context->make_resource<Provider>();
        user_context->set_schema_provider(*user_context, provider);
        user_context->insert_resource(global, control);
        auto& module = user_context->make<RuntimeModule>();
        module.generated_schema_bootstrap(sequence);
        module.apply_to(*user_context);
    }
};

static void classifier_contract() {
    GlobalContext global;
    auto r = global.memory_resource();
    check(
        generated_bootstrap_retryable_conflict(RuntimeError::optimistic_lock_conflict(r, "A", "1")),
        "optimistic conflicts are retryable");
    for (auto message : {"UNIQUE constraint failed: status_data.id",
                         "prefix UNIQUE constraint failed: status_data.id suffix",
                         "duplicate key value violates unique constraint status_data_pkey",
                         "Duplicate entry 1 for PRIMARY"}) {
        check(generated_bootstrap_retryable_conflict(RuntimeError::graph(r, message)),
              "source primary-key conflict strings are recognized");
        check(!generated_bootstrap_retryable_conflict(RuntimeError::schema(r, message)),
              "matching text in another error kind does not authorize retry");
    }
    for (auto message : {"UNIQUE constraint failed: status_data.code",
                         "UNIQUE constraint failed: status.id",
                         "unique constraint failed: status_data.id",
                         "constraint failed",
                         "duplicate key value violates unique constraint other_key",
                         "Duplicate entry 1 for secondary"}) {
        check(!generated_bootstrap_retryable_conflict(RuntimeError::graph(r, message)),
              "source classifier does not retry secondary keys or similar text");
    }
}

static void retries_and_precedence() {
    for (auto kind : {RuntimeError::Kind::OptimisticLockConflict, RuntimeError::Kind::Graph}) {
        for (unsigned failures : {0U, 1U, 4U, 5U, 100U}) {
            Fixture fixture;
            fixture.control.failures = failures;
            fixture.control.error_kind = kind;
            auto& operation = fixture.user_context->ensure_schema();
            unsigned pending = 0;
            const RuntimeSchemaResult* result = nullptr;
            while (!(result = operation.poll())) {
                ++pending;
                check(pending <= 4 && !detail::is_generated_schema_bootstrap(*fixture.user_context),
                      "each retry yields without leaving bootstrap privileges active");
            }
            auto attempts = std::min(failures + 1, 5U);
            check(bool(*result) == (failures < 5) && pending == attempts - 1 &&
                      fixture.control.created[0] == attempts &&
                      fixture.control.created[1] == attempts &&
                      fixture.control.created[2] == (failures < 5 ? 1U : 0U) &&
                      fixture.control.finished == attempts * 2 + (failures < 5 ? 1U : 0U) &&
                      fixture.control.cancelled == 0 && fixture.control.scope_ok,
                  "retry restarts at the root and stops after the fifth attempt");
            if (!*result) {
                check(result->error().kind() == kind,
                      "the final failure preserves its original kind");
            }
        }
    }
    Fixture fixture;
    fixture.control.failures = 10;
    fixture.control.error_kind = RuntimeError::Kind::Schema;
    const auto& failed = fixture.user_context->ensure_schema().wait();
    check(!failed && fixture.control.created == std::array<unsigned, 3>{1, 1, 0} &&
              failed.error().message() == fixture.control.message,
          "nonretryable failure stops later recipes and preserves its message");
}

static void pending_wake_and_close() {
    Fixture fixture;
    fixture.control.pending = true;
    auto& operation = fixture.user_context->ensure_schema();
    check(!operation.poll() && fixture.control.child_wake &&
              !detail::is_generated_schema_bootstrap(*fixture.user_context),
          "recipe Pending leaves no ambient bootstrap privilege");
    auto ticket = operation.wake_signal().ticket();
    fixture.control.child_wake->notify();
    check(operation.wake_signal().ticket() != ticket, "recipe wakes forward through both parents");
    fixture.user_context.reset();
    check(fixture.control.cancelled == 1 && fixture.control.scope_ok && !fixture.control.child_wake,
          "Context destruction cancels its active recipe in scope before reclaiming memory");
}

static void exceptions_and_foreign_operations() {
    for (bool foreign : {false, true}) {
        Fixture fixture;
        auto next_user_context = fixture.global.make_user_context();
        fixture.control.foreign = foreign ? next_user_context.get() : nullptr;
        fixture.control.throw_now = !foreign;
        bool rejected = false;
        try {
            fixture.user_context->ensure_schema().wait();
        } catch (const std::exception&) {
            rejected = true;
        }
        check(rejected && fixture.control.cancelled == 1 && fixture.control.scope_ok &&
                  !detail::is_generated_schema_bootstrap(*fixture.user_context),
              "throwing or foreign recipes quiesce exactly once and restore bootstrap scope");
    }
    Fixture fixture;
    const std::array<GeneratedSchemaBootstrap, 1> recipes{recipe<0>};
    const auto& outside = run_generated_schema_bootstrap(*fixture.user_context, recipes).wait();
    check(!outside && outside.error().kind() == RuntimeError::Kind::Schema &&
              fixture.control.created[0] == 0,
          "calling shared generated support directly cannot grant bootstrap privileges");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        classifier_contract();
        retries_and_precedence();
        pending_wake_and_close();
        exceptions_and_foreign_operations();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated bootstrap sequence contracts passed\n";
}
