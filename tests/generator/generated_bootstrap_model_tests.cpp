#include "../generated_candidates/include/cpp_candidates.hpp"
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct QueryControl {
    using allocator_type = Allocator;
    Memory resource;
    bool ready = false, scope_ok = true;
    unsigned cancelled = 0;
    WakeSignal* wake = nullptr;
    UserContext* foreign = nullptr;

    QueryControl(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    Memory memory_resource() const {
        return resource;
    }
};

using Model = cpp_candidates::Platform;
using DeferredResult = Result<SmartList<Model>, RuntimeError>;

struct DeferredQuery {
    using allocator_type = Allocator;
    Memory resource;
    const UserContext& user_context;
    QueryControl& control;

    DeferredQuery(std::allocator_arg_t,
                  Allocator a,
                  const UserContext& user_context,
                  QueryControl& control)
        : resource(a.resource()), user_context(user_context), control(control) {}

    std::optional<DeferredResult> poll(WakeSignal& wake) {
        control.scope_ok &= detail::is_generated_schema_bootstrap(user_context);
        if (!control.ready) {
            control.wake = &wake;
            return {};
        }
        SmartList<Model> rows(std::allocator_arg, Allocator(resource));
        rows.data.emplace_back();
        rows.data.back().updateId(1).updateName("existing");
        return DeferredResult::ok(resource, std::move(rows));
    }

    void finish_and_wait() noexcept {
        control.wake = nullptr;
    }

    void cancel_and_wait() noexcept {
        ++control.cancelled;
        control.scope_ok &= detail::is_generated_schema_bootstrap(user_context);
        control.wake = nullptr;
    }
};

struct DeferredRecipe {
    using Model = cpp_candidates::Platform;
    static constexpr bool is_root = true;
    static constexpr std::uint64_t fixed_id = 1;
    static constexpr std::string_view create_reason = "Create test root";
    static constexpr std::string_view reconcile_reason = "Reconcile test root";

    static void set_id(Model& entity) {
        entity.updateId(1);
    }

    static bool configure(const UserContext&, Model& entity, bool) {
        entity.updateName("new");
        return true;
    }

    static auto& query(const UserContext& user_context) {
        auto& control =
            const_cast<QueryControl&>(user_context.require_resource<QueryControl>().value().get());
        auto r =
            control.foreign ? control.foreign->memory_resource() : user_context.memory_resource();
        return make_operation<DeferredResult, DeferredQuery>(r, user_context, control);
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

static GeneratedSchemaBootstrapFuture& bootstrap(const UserContext& user_context) {
    const std::array<GeneratedSchemaBootstrap, 1> recipes{
        run_generated_bootstrap_model<DeferredRecipe>};
    return run_generated_schema_bootstrap(user_context, recipes);
}

static void lifecycle(bool close, bool foreign) {
    GlobalContext global;
    auto& control = global.make<QueryControl>();
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    auto& provider = user_context->make_resource<Provider>();
    user_context->set_schema_provider(*user_context, provider);
    user_context->insert_resource(global, control);
    auto& module = user_context->make<RuntimeModule>();
    module.entity<Model>().generated_schema_bootstrap(bootstrap);
    module.apply_to(*user_context);
    control.foreign = foreign ? next_user_context.get() : nullptr;
    auto& operation = user_context->ensure_schema();
    if (foreign) {
        bool rejected = false;
        try {
            operation.wait();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected && control.cancelled == 1 && control.scope_ok,
              "foreign typed query cancels once inside schema scope before rejection");
        return;
    }
    check(!operation.poll() && control.wake &&
              !detail::is_generated_schema_bootstrap(*user_context),
          "typed query Pending does not retain bootstrap privilege");
    auto ticket = operation.wake_signal().ticket();
    control.wake->notify();
    check(operation.wake_signal().ticket() != ticket,
          "typed query wakes traverse model, sequence and schema parents");
    if (close) {
        user_context.reset();
        check(control.cancelled == 1 && control.scope_ok && !control.wake,
              "actual Context deletion joins pending typed query before arena reclamation");
    } else {
        control.ready = true;
        check(bool(operation.wait()) && control.cancelled == 0 && control.scope_ok &&
                  user_context->require_active_root("Platform").value().get().id == 1,
              "existing root installs without requiring a save or an ID generator");
    }
}

static void generated_assignment_scope() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto& status = user_context->make<cpp_candidates::OrderStatus>();
    bool rejected = false;
    try {
        cpp_candidates::generated_detail::OrderStatusBootstrapRecipe::set_platform_id(
            *user_context, status, 1);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    check(rejected && status.platformId() == 0 && !status.changedPlatformId(),
          "generated recipe assignment does not expose a public Context-root setter bypass");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        lifecycle(false, false);
        lifecycle(true, false);
        lifecycle(false, true);
        generated_assignment_scope();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Typed bootstrap model lifecycle contracts passed\n";
}
