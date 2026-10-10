#include <teaql/runtime/context.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <cstdlib>
#include <exception>
using namespace teaql;

[[noreturn]] static void close_and_report(const UserContext& context) {
    const_cast<UserContext&>(context).close();
    std::_Exit(1);
}

struct Metadata final : MetadataStore {
    Memory resource;
    const UserContext& context;

    Metadata(Memory r, const UserContext& context) : resource(r), context(context) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    const EntityDescriptor* entity(std::string_view) const override {
        close_and_report(context);
    }

    Vector<const EntityDescriptor*> all_entities(Memory) const override {
        close_and_report(context);
    }
};

struct Checkers final : CheckerRegistry {
    Memory resource;
    const UserContext& context;

    Checkers(Memory r, const UserContext& context) : resource(r), context(context) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    std::optional<CheckerBorrow> checker(Memory, std::string_view) const override {
        close_and_report(context);
    }
};

struct Telemetry final : RuntimeTelemetry {
    Memory resource;
    const UserContext& context;

    Telemetry(Memory r, const UserContext& context) : resource(r), context(context) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    bool is_noop() const override {
        close_and_report(context);
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory, const RuntimeOperation&) const override {
        close_and_report(context);
    }
};

static int reentrant(std::string_view mode) {
    std::set_terminate([] {
        std::_Exit(0);
    });
    try {
        GlobalContext global;
        auto user = global.make_user_context();
        auto r = global.memory_resource();
        if (mode == "entity" || mode == "all-entities") {
            auto& metadata = global.make_resource<Metadata>(r, *user);
            user->set_metadata(global, metadata);
            if (mode == "entity") {
                (void)user->entity("Item");
            } else {
                (void)user->all_entities();
            }
        } else if (mode == "checker") {
            auto& checkers = global.make_resource<Checkers>(r, *user);
            user->set_checker_registry(global, checkers);
            (void)user->has_checker("Item");
        } else {
            auto& telemetry = global.make_resource<Telemetry>(r, *user);
            user->set_runtime_telemetry(global, telemetry);
            if (mode == "telemetry-noop") {
                (void)user->runtime_telemetry_is_noop();
            } else {
                RuntimeOperation operation(
                    std::allocator_arg, Allocator(user->memory_resource()), "query", "Item");
                (void)user->start_runtime_operation(operation);
            }
        }
    } catch (...) {
        std::_Exit(2);
    }
    return 4;
}

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void dead(F&& call) {
    bool rejected = false;
    try {
        call();
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Dead;
    }
    check(rejected, "Destroyed telemetry rejects before returning or dispatching a borrow");
}

static int lifecycle() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    check(user->runtime_telemetry_is_noop(), "Default telemetry remains Noop");
    const auto* initial = &user->runtime_telemetry();
    check(initial == &user->runtime_telemetry(), "Default telemetry is reused in its User scope");
    auto shared = user->make_global<NoopRuntimeTelemetry>();
    user->set_runtime_telemetry(global, user->get(shared));
    sibling->set_runtime_telemetry(global, sibling->get(sibling->rebind_global(shared)));
    user->destroy(shared);
    dead([&] {
        (void)user->runtime_telemetry();
    });
    dead([&] {
        (void)sibling->runtime_telemetry_is_noop();
    });
    auto local = user->make_ref<NoopRuntimeTelemetry>();
    user->set_runtime_telemetry(*user, user->get(local));
    user->destroy(local);
    dead([&] {
        (void)user->runtime_telemetry();
    });
    auto replacement = user->make_ref<NoopRuntimeTelemetry>();
    user->set_runtime_telemetry(*user, user->get(replacement));
    check(user->runtime_telemetry_is_noop(),
          "Replacing expired telemetry restores ordinary access");
    bool refused = false;
    try {
        sibling->set_runtime_telemetry(*user, user->get(replacement));
    } catch (const std::logic_error&) {
        refused = true;
    }
    check(refused, "Sibling Context cannot retain a User telemetry provider");
    {
        NoopRuntimeTelemetry unmanaged(std::allocator_arg, Allocator(user->memory_resource()));
        refused = false;
        try {
            user->set_runtime_telemetry(*user, unmanaged);
        } catch (const std::invalid_argument&) {
            refused = true;
        }
        check(refused, "Telemetry allocation lifetime must be registered explicitly");
    }
    user.reset();
    sibling.reset();
    check(global.live_user_bytes() == 0, "Telemetry borrows do not extend User memory lifetime");
    return 0;
}

struct ThrowingMetadata final : MetadataStore {
    Memory resource;

    explicit ThrowingMetadata(Memory r) : resource(r) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    const EntityDescriptor* entity(std::string_view) const override {
        throw std::runtime_error("controlled metadata failure");
    }

    Vector<const EntityDescriptor*> all_entities(Memory) const override {
        throw std::runtime_error("controlled metadata failure");
    }
};

struct ThrowingCheckers final : CheckerRegistry {
    Memory resource;

    explicit ThrowingCheckers(Memory r) : resource(r) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    std::optional<CheckerBorrow> checker(Memory, std::string_view) const override {
        throw std::runtime_error("controlled checker failure");
    }
};

struct ThrowingTelemetry final : RuntimeTelemetry {
    Memory resource;

    explicit ThrowingTelemetry(Memory r) : resource(r) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    bool is_noop() const override {
        throw std::runtime_error("controlled telemetry failure");
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory, const RuntimeOperation&) const override {
        throw std::runtime_error("controlled telemetry failure");
    }
};

template <class F>
static void failed(F&& call, std::string_view message) {
    bool rejected = false;
    try {
        call();
    } catch (const std::runtime_error& error) {
        rejected = error.what() == message;
    }
    check(rejected, "Provider exceptions preserve their exact error after guard cleanup");
}

static int exception_cleanup() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = global.memory_resource();
    auto& metadata = global.make_resource<ThrowingMetadata>(r);
    auto& checkers = global.make_resource<ThrowingCheckers>(r);
    auto& telemetry = global.make_resource<ThrowingTelemetry>(r);
    user->set_metadata(global, metadata);
    user->set_checker_registry(global, checkers);
    user->set_runtime_telemetry(global, telemetry);
    failed(
        [&] {
            (void)user->entity("Item");
        },
        "controlled metadata failure");
    failed(
        [&] {
            (void)user->all_entities();
        },
        "controlled metadata failure");
    failed(
        [&] {
            (void)user->has_checker("Item");
        },
        "controlled checker failure");
    failed(
        [&] {
            (void)user->runtime_telemetry_is_noop();
        },
        "controlled telemetry failure");
    {
        RuntimeOperation operation(
            std::allocator_arg, Allocator(user->memory_resource()), "query", "Item");
        auto& scope = user->start_runtime_operation(operation);
        RuntimeAttributes attributes(user->memory_resource());
        scope.success(attributes);
    }
    user.reset();
    check(global.live_user_bytes() == 0, "Ordinary exceptions detach every synchronous guard");
    return 0;
}

static int shutdown_reads() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& metadata = global.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(global.memory_resource()), "Item");
    metadata.register_entity(std::move(descriptor));
    auto& telemetry = global.make<NoopRuntimeTelemetry>();
    user->set_metadata(global, metadata);
    user->set_runtime_telemetry(global, telemetry);

    struct Observation {
        const UserContext& context;
        bool read = false;
    } observation{*user};

    Arena::Quiescence stop;
    stop.attach(
        *dynamic_cast<Arena*>(user->memory_resource()), &observation, [](void* raw) noexcept {
            auto& state = *static_cast<Observation*>(raw);
            auto rows = state.context.all_entities();
            state.read = state.context.entity("Item") && rows.size() == 1 &&
                         state.context.runtime_telemetry_is_noop();
        });
    user->close();
    check(observation.read && global.live_user_bytes() == 0,
          "Trusted shutdown reads remain valid before blocks return");
    return 0;
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc != 2) {
        return 3;
    }
    auto mode = std::string_view(argv[1]);
    if (mode == "lifecycle") {
        return lifecycle();
    }
    if (mode == "exceptions") {
        return exception_cleanup();
    }
    if (mode == "shutdown") {
        return shutdown_reads();
    }
    return reentrant(mode);
}
