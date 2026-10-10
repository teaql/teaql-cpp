#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
#include <iostream>
#include <type_traits>
using namespace teaql;

static void check(bool b, const char* text) {
    if (!b) {
        throw std::runtime_error(text);
    }
}

template <class F>
static void rejected(F f, const char* text) {
    bool caught = false;
    try {
        f();
    } catch (const std::logic_error&) {
        caught = true;
    }
    check(caught, text);
}

static_assert(!std::is_default_constructible_v<SchemaInvocation>);
static_assert(!std::is_copy_constructible_v<SchemaInvocation>);
static_assert(!std::is_same_v<runtime::SchemaProvider, SchemaProvider>);

struct Control {
    using allocator_type = Allocator;
    Vector<String> calls;
    bool nested = false, ready = true, fail_provider = false, fail_business = false,
         fail_bootstrap = false, throw_bootstrap = false;
    UserContext* sibling = nullptr;
    const UserContext* isolated = nullptr;
    AsyncOperation<RuntimeSchemaResult>* child = nullptr;

    Control(std::allocator_arg_t, Allocator a) : calls(a.resource()) {}

    void note(std::string_view name, const UserContext& context) {
        String entry(name, calls.get_allocator().resource());
        entry += detail::is_generated_schema_bootstrap(context) ? ":bootstrap" : ":normal";
        calls.push_back(std::move(entry));
    }
};

struct Step {
    using allocator_type = Allocator;
    Memory r;
    const UserContext& context;
    Control& control;
    bool bootstrap;

    Step(std::allocator_arg_t,
         Allocator a,
         const UserContext& context,
         Control& control,
         bool bootstrap)
        : r(a.resource()), context(context), control(control), bootstrap(bootstrap) {}

    std::optional<RuntimeSchemaResult> poll(WakeSignal&) {
        control.note(bootstrap ? "callback.poll" : "provider.poll", context);
        if (!control.ready) {
            return {};
        }
        if (bootstrap && control.throw_bootstrap) {
            throw std::runtime_error("fixture panic");
        }
        if (bootstrap ? control.fail_bootstrap : control.fail_provider) {
            return RuntimeSchemaResult::err(
                r, RuntimeError::schema(r, bootstrap ? "bootstrap failed" : "provider failed"));
        }
        return RuntimeSchemaResult::ok(r, {});
    }

    void finish_and_wait() noexcept {
        control.note(bootstrap ? "callback.finish" : "provider.finish", context);
    }

    void cancel_and_wait() noexcept {
        control.note(bootstrap ? "callback.cancel" : "provider.cancel", context);
    }
};

struct Provider : runtime::SchemaProvider {
    Control& control;

    explicit Provider(Control& control) : control(control) {}

    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& context,
                                                       const SchemaInvocation&) const override {
        control.note("provider.create", context);
        auto r = context.memory_resource();
        auto& operation = make_operation<RuntimeSchemaResult, Step>(r, context, control, false);
        control.child = &operation;
        return operation;
    }
};

struct Business : BusinessIdAllocator, BusinessIdSchemaContributor {
    Control& control;

    explicit Business(Control& control) : control(control) {}

    Result<BusinessIdAllocation, BusinessIdError> allocate(Memory r,
                                                           const BusinessIdPlan&) const override {
        return Result<BusinessIdAllocation, BusinessIdError>::err(
            r,
            BusinessIdError(
                std::allocator_arg, Allocator(r), BusinessIdErrorCode::Allocation, "unused"));
    }

    Result<std::monostate, BusinessIdError>
    ensure_schema(Memory r, const UserContext& context) const override {
        control.note("business", context);
        if (control.fail_business) {
            return Result<std::monostate, BusinessIdError>::err(
                r,
                BusinessIdError(std::allocator_arg,
                                Allocator(r),
                                BusinessIdErrorCode::Allocation,
                                "business failed"));
        }
        return Result<std::monostate, BusinessIdError>::ok(r, {});
    }
};

static GeneratedSchemaBootstrapFuture& bootstrap(const UserContext& context) {
    auto& control =
        context.require_resource<Control>().value().get(); // tracked state is a test resource
    auto& mutable_control = const_cast<Control&>(control);
    mutable_control.note("callback.create", context);
    if (control.sibling) {
        check(detail::is_generated_schema_bootstrap(*control.sibling),
              "same Global context observes inherited callback scope");
    }
    if (control.isolated) {
        check(!detail::is_generated_schema_bootstrap(*control.isolated),
              "different Global does not inherit bootstrap mode");
    }
    auto r = context.memory_resource();
    auto& operation = make_operation<RuntimeSchemaResult, Step>(r, context, mutable_control, true);
    mutable_control.child = &operation;
    return operation;
}

static GeneratedSchemaBootstrapFuture& second(const UserContext& context) {
    auto& control = const_cast<Control&>(context.require_resource<Control>().value().get());
    control.note("second", context);
    return bootstrap(context);
}

struct Fixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Control& control = global.make<Control>();
    Provider& provider = global.make_resource<Provider>(control);
    Business& business = global.make_resource<Business>(control);
    RuntimeModule& module = global.make<RuntimeModule>();

    Fixture() {
        user->insert_resource(global, control);
        user->set_schema_provider(global, provider);
        user->set_business_id_infrastructure(global, business);
        module.generated_schema_bootstrap(bootstrap).generated_schema_bootstrap(second);
        module.apply_to(*user);
    }
};

static void pipeline() {
    Fixture f;
    auto& operation = f.user->ensure_schema();
    check(f.control.calls.empty(), "schema initialization is lazy");
    rejected(
        [&] {
            f.module.apply_to(*f.user);
        },
        "pending immutable schema borrow rejects module replacement");
    rejected(
        [&] {
            f.user->set_schema_provider(f.global, f.provider);
        },
        "pending borrow rejects provider replacement");
    check(bool(operation.wait()), "pipeline succeeds");
    Vector<String> expected(f.global.memory_resource());
    for (auto text : {"provider.create:normal",
                      "provider.poll:normal",
                      "provider.finish:normal",
                      "business:normal",
                      "callback.create:bootstrap",
                      "callback.poll:bootstrap",
                      "callback.finish:bootstrap",
                      "second:bootstrap",
                      "callback.create:bootstrap",
                      "callback.poll:bootstrap",
                      "callback.finish:bootstrap"}) {
        expected.emplace_back(text);
    }
    check(f.control.calls == expected,
          "provider then business contributor then sequential bootstraps");
    check(!detail::is_generated_schema_bootstrap(*f.user), "ready resets dynamic mode");
    f.module.apply_to(*f.user);
    for (int failure = 0; failure < 3; ++failure) {
        f.control.calls.clear();
        f.control.fail_provider = failure == 0;
        f.control.fail_business = failure == 1;
        f.control.fail_bootstrap = failure == 2;
        auto& result = f.user->ensure_schema().wait();
        check(!result, "errors short circuit");
        if (failure == 0) {
            check(f.control.calls.size() == 3, "provider failure skips contributor and callbacks");
        }
        if (failure == 1) {
            check(f.control.calls.size() == 4 &&
                      result.error().kind() == RuntimeError::Kind::Schema &&
                      result.error().message() == "Allocation: business failed",
                  "business display maps to Schema error before callbacks");
        }
        if (failure == 2) {
            check(f.control.calls.size() == 7, "first callback failure skips second");
        }
        check(!detail::is_generated_schema_bootstrap(*f.user), "failure resets mode");
    }
}

static GeneratedSchemaBootstrapFuture& root_bootstrap(const UserContext& context) {
    check(detail::is_generated_schema_bootstrap(context),
          "active root initialization runs inside the bootstrap scope");
    check(bool(context.set_generated_bootstrap_active_root("Tenant", 42)),
          "bootstrap assigns the active root while schema initialization borrows Context");
    check(bool(context.set_generated_bootstrap_active_root("Tenant", 42)),
          "same bootstrap root assignment is idempotent");
    auto conflict = context.set_generated_bootstrap_active_root("Tenant", 43);
    check(!conflict && conflict.error().kind() == RuntimeError::Kind::Schema &&
              conflict.error().message() == "active root already set to tenant:42",
          "different bootstrap root retains the canonical source conflict");
    auto invalid = context.set_generated_bootstrap_active_root("", 0);
    check(!invalid && invalid.error().kind() == RuntimeError::Kind::Schema &&
              invalid.error().message() == "invalid generated active root",
          "invalid bootstrap root retains source validation precedence");
    check(context.require_active_root("Tenant").value().get().id == 42,
          "bootstrap can read the installed typed root immediately");
    return bootstrap(context);
}

static void bootstrap_root_installation() {
    Fixture f;
    auto& module = f.global.make<RuntimeModule>();
    module.generated_schema_bootstrap(root_bootstrap);
    module.apply_to(*f.user);
    check(!f.user->require_active_root("Tenant"), "root is absent before the first schema poll");
    auto& operation = f.user->ensure_schema();
    rejected(
        [&] {
            f.user->with_active_root("Tenant", 42);
        },
        "ordinary root assignment cannot mutate a pending schema Context");
    check(bool(operation.wait()), "schema callback installs the generated active root");
    check(f.user->require_active_root("Tenant").value().get().id == 42,
          "installed root survives schema completion");
    check(bool(f.user->ensure_schema().wait()), "repeated bootstrap retains the same root");
    check(!detail::is_generated_schema_bootstrap(*f.user), "root installation restores scope");
    f.user->close();
    rejected(
        [&] {
            f.user->set_generated_bootstrap_active_root("Tenant", 42);
        },
        "generated root initialization still rejects a closed Context");
}

static void cancellation_and_interleaving() {
    Fixture f;
    auto sibling = f.global.make_user_context();
    GlobalContext isolated;
    auto other = isolated.make_user_context();
    f.control.sibling = sibling.get();
    f.control.isolated = other.get();
    f.control.ready = false;
    auto& first = f.user->ensure_schema();
    check(!first.poll(), "provider can suspend");
    auto ticket = first.wake_signal().ticket();
    f.control.child->wake_signal().notify();
    check(first.wake_signal().ticket() != ticket, "provider wake forwards to outer operation");
    first.cancel();
    check(f.control.calls.back() == "provider.cancel:normal",
          "provider cancellation is not bootstrap");
    f.control.calls.clear();
    f.control.ready = true;
    auto& second_op =
        f.user->ensure_schema(); // provider will be ready; suspend via an alternate callback
    f.control.ready = false;
    check(!second_op.poll(), "second provider suspends");
    f.control.ready = true;
    check(second_op.poll() != nullptr, "resume finishes all callbacks");
    f.control.calls.clear();
    f.control.throw_bootstrap = true;
    auto& panicking = f.user->ensure_schema();
    bool caught = false;
    try {
        panicking.poll();
    } catch (const std::runtime_error&) {
        caught = true;
    }
    check(caught && !detail::is_generated_schema_bootstrap(*f.user),
          "callback exception restores scope");
    check(f.control.calls.back() == "callback.cancel:bootstrap",
          "panicking callback cancellation retains mode");
    f.control.throw_bootstrap = false;
    f.module.apply_to(*f.user);
}

static GeneratedSchemaBootstrapFuture& pending_bootstrap(const UserContext& context) {
    auto& control = const_cast<Control&>(context.require_resource<Control>().value().get());
    control.ready = false;
    return bootstrap(context);
}

static void closing() {
    Fixture f;
    auto& module = f.global.make<RuntimeModule>();
    module.generated_schema_bootstrap(pending_bootstrap);
    module.apply_to(*f.user);
    auto& operation = f.user->ensure_schema();
    check(!operation.poll(), "bootstrap suspends");
    check(!detail::is_generated_schema_bootstrap(*f.user),
          "pending bootstrap does not elevate ordinary operations");
    auto ticket = operation.wake_signal().ticket();
    f.control.child->wake_signal().notify();
    check(operation.wake_signal().ticket() != ticket, "bootstrap wake forwards");
    auto sibling = f.global.make_user_context();
    check(!detail::is_generated_schema_bootstrap(*sibling), "interleaved sibling stays normal");
    f.user->close();
    check(f.control.calls.back() == "callback.cancel:bootstrap",
          "Context close cancels descendant in bootstrap scope before arena destruction");
    check(!detail::is_generated_schema_bootstrap(*sibling), "close resets Global mode");
    rejected(
        [&] {
            f.user->ensure_schema();
        },
        "closed Context rejects initialization");
}

static GeneratedSchemaBootstrapFuture& nested_bootstrap(const UserContext& context) {
    auto& control = const_cast<Control&>(context.require_resource<Control>().value().get());
    if (!control.nested) {
        control.nested = true;
        return context.ensure_schema();
    }
    control.ready = false;
    return bootstrap(context);
}

static void nested_close() {
    Fixture f;
    auto& module = f.global.make<RuntimeModule>();
    module.generated_schema_bootstrap(nested_bootstrap);
    module.apply_to(*f.user);
    check(!f.user->ensure_schema().poll(), "nested schema can suspend");
    check(f.control.calls.at(4) == "provider.create:bootstrap",
          "nested provider inherits outer bootstrap scope");
    auto sibling = f.global.make_user_context();
    check(!detail::is_generated_schema_bootstrap(*sibling), "nested pending restores outer scope");
    f.user->close();
    check(f.control.calls.back() == "callback.cancel:bootstrap" &&
              !detail::is_generated_schema_bootstrap(*sibling),
          "nested cancellation restores dynamic scope in stack order");
}

static void module_and_lifetimes() {
    GlobalContext global;
    auto source = global.make_user_context();
    auto& module = source->make<RuntimeModule>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(source->memory_resource()), "Order");
    descriptor.property("name", DataType::Text);
    module.descriptor(std::move(descriptor));
    auto target = module.into_context(global);
    source.reset();
    check(target->entity("Order") && target->has_entity_data_service("Order"),
          "module materializes independent Context metadata and registry");
    auto& result = target->ensure_schema().wait();
    check(!result && result.error().message() == "missing schema provider",
          "missing provider returns exact lazy Schema error");
    auto& empty = global.make<RuntimeModule>();
    empty.apply_to(*target);
    check(!target->entity("Order") && !target->has_entity_data_service("Order"),
          "applying module replaces registration");
    auto sibling = global.make_user_context();
    auto& state = target->make<Control>();
    auto& provider = target->make_resource<Provider>(state);
    rejected(
        [&] {
            sibling->set_schema_provider(*target, provider);
        },
        "cannot borrow sibling-owned provider");
}

struct ForeignSchemaProvider final : runtime::SchemaProvider {
    GlobalContext& owner;
    bool foreign_provider = true;
    mutable unsigned cancelled = 0;
    mutable bool saw_live_context = false, bootstrap_cancel = false;
    mutable AsyncOperation<RuntimeSchemaResult>* child = nullptr;

    explicit ForeignSchemaProvider(GlobalContext& owner) : owner(owner) {}

    struct Work {
        const ForeignSchemaProvider& provider;
        const UserContext& context;

        std::optional<RuntimeSchemaResult> poll(WakeSignal&) {
            return {};
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            ++provider.cancelled;
            provider.saw_live_context = context.trace_id() == "live-schema-context";
            provider.bootstrap_cancel = detail::is_generated_schema_bootstrap(context);
        }
    };

    AsyncOperation<RuntimeSchemaResult>& foreign_work(const UserContext& context) const {
        child = &make_operation<RuntimeSchemaResult, Work>(owner, Work{*this, context});
        return *child;
    }

    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& context,
                                                       const SchemaInvocation&) const override {
        if (foreign_provider) {
            return foreign_work(context);
        }

        struct Ready {
            Memory owner;

            std::optional<RuntimeSchemaResult> poll(WakeSignal&) {
                return RuntimeSchemaResult::ok(owner, {});
            }

            void finish_and_wait() noexcept {}

            void cancel_and_wait() noexcept {}
        };

        return make_operation<RuntimeSchemaResult, Ready>(context.memory_resource(),
                                                          Ready{context.memory_resource()});
    }
};

static GeneratedSchemaBootstrapFuture& foreign_schema_bootstrap(const UserContext& context) {
    return context.require_resource<ForeignSchemaProvider>().value().get().foreign_work(context);
}

static void foreign_schema_children_are_quiesced() {
    for (bool bootstrap : {false, true}) {
        GlobalContext global;
        auto& provider = global.make_resource<ForeignSchemaProvider>(global);
        provider.foreign_provider = !bootstrap;
        auto user = global.make_user_context();
        user->set_trace_id("live-schema-context");
        user->set_schema_provider(global, provider);
        if (bootstrap) {
            user->insert_resource(global, provider);
            auto& module = global.make<RuntimeModule>();
            module.generated_schema_bootstrap(foreign_schema_bootstrap);
            module.apply_to(*user);
        }
        bool failed = false;
        try {
            (void)user->ensure_schema().poll();
        } catch (const std::invalid_argument&) {
            failed = true;
        }
        const bool quiesced = provider.cancelled == 1 && provider.saw_live_context &&
                              provider.bootstrap_cancel == bootstrap &&
                              provider.child->status() == AsyncStatus::Cancelled;
        if (!quiesced) {
            provider.child->cancel();
        }
        check(failed && quiesced,
              "foreign schema child stops before rejection with the correct bootstrap scope");
        check(!detail::is_generated_schema_bootstrap(*user),
              "foreign schema rejection restores activation");
        auto& replacement = global.make_resource<Provider>(global.make<Control>());
        user->set_schema_provider(global, replacement);
        user.reset();
        check(provider.cancelled == 1 && global.live_user_bytes() == 0,
              "foreign schema rejection releases Context borrow without deferred child access");
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        pipeline();
        bootstrap_root_installation();
        cancellation_and_interleaving();
        closing();
        nested_close();
        module_and_lifetimes();
        foreign_schema_children_are_quiesced();
        std::pmr::set_default_resource(previous);
        std::cout << "Schema runtime contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
