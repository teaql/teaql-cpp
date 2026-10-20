#include <teaql/runtime/generated_sample_data.hpp>
#include <iostream>
#include <cstdlib>
#include <exception>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

class Sink final : public SampleDataLogSink {
public:
    using allocator_type = Allocator;

    Sink(std::allocator_arg_t,
         Allocator a,
         bool active = true,
         bool fail = false,
         bool close = false)
        : messages(a.resource()), active_(active), fail_(fail), close_(close) {}

    Memory memory_resource() const noexcept override {
        return messages.get_allocator().resource();
    }

    bool enabled(const UserContext&) const override {
        return active_;
    }

    void info(const UserContext& user_context, std::string_view message) const override {
        if (close_) {
            const_cast<UserContext&>(user_context).close();
        }
        if (fail_) {
            throw std::runtime_error("Controlled sample log failure");
        }
        messages.emplace_back(message);
    }

    mutable Vector<String> messages;
    bool active_, fail_, close_;
};

struct Gate {
    bool ready = false;
    unsigned started = 0, cancelled = 0;
};

struct GateResource {
    using allocator_type = Allocator;
    Gate& gate;

    GateResource(std::allocator_arg_t, Allocator, Gate& value) : gate(value) {}
};

struct Work {
    using allocator_type = Allocator;
    Memory r;
    Gate& gate;

    Work(std::allocator_arg_t, Allocator a, Gate& gate) : r(a.resource()), gate(gate) {
        ++gate.started;
    }

    std::optional<SampleDataStepResult> poll(WakeSignal&) {
        if (!gate.ready) {
            return {};
        }
        return SampleDataStepResult::ok(r, {});
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        ++gate.cancelled;
    }
};

static SampleDataStepFuture& step(UserContext& user_context, SampleDataState&) {
    auto& gate = user_context.get_resource<GateResource>()->gate;
    return make_operation<SampleDataStepResult, Work>(user_context.memory_resource(), gate);
}

static void register_sink(UserContext& user_context, Sink& sink) {
    user_context.insert_resource(user_context, static_cast<SampleDataLogSink&>(sink));
}

static SampleDataFuture& empty_sample(UserContext& user_context) {
    const std::array<GeneratedSampleStep, 0> steps{};
    return run_generated_sample_data(user_context, {}, steps);
}

static void lazy_and_replacement() {
    Gate gate;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto& resource = user_context->make<GateResource>(gate);
    user_context->insert_resource(*user_context, resource);
    auto& first = user_context->make<Sink>();
    register_sink(*user_context, first);
    const std::array<GeneratedSampleStep, 1> steps{step};
    auto& operation = run_generated_sample_data(*user_context, {}, steps);
    check(first.messages.empty(), "Unpolled samples emit no progress");
    check(!operation.poll() && !operation.poll() && first.messages.size() == 1,
          "Pending re-polls do not duplicate start");
    auto& second = user_context->make<Sink>();
    register_sink(*user_context, second);
    gate.ready = true;
    check(operation.poll() && second.messages.size() == 1 && first.messages.size() == 1,
          "Resource replacement receives only later completion");
    check(
        second.messages[0] ==
            "Sample data generation completed successfully. Generated: 0 tables, Skipped: 0 tables.",
        "Completion retains source report counts");
}

static void cancellation_and_deletion() {
    for (bool explicit_cancel : {false, true}) {
        Gate gate;
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto& resource = user_context->make<GateResource>(gate);
        user_context->insert_resource(*user_context, resource);
        auto& sink = user_context->make<Sink>();
        register_sink(*user_context, sink);
        const std::array<GeneratedSampleStep, 1> steps{step};
        auto& operation = run_generated_sample_data(*user_context, {}, steps);
        check(!operation.poll() && sink.messages.size() == 1,
              "Pending operation records only start");
        if (explicit_cancel) {
            operation.cancel();
            check(sink.messages.size() == 1, "Cancellation emits no successful completion");
        }
        user_context.reset();
        check(gate.cancelled == 1 && global.live_user_bytes() == 0,
              "Deleting Context cancels child before reclamation");
    }
}

static void missing_disabled_and_exceptions() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    check(empty_sample(*user_context).poll(), "Missing sink permits sample execution");
    auto& disabled = user_context->make<Sink>(false);
    register_sink(*user_context, disabled);
    check(empty_sample(*user_context).poll() && disabled.messages.empty(),
          "Disabled sink permits execution without delivery");
    auto& throwing = user_context->make<Sink>(true, true);
    register_sink(*user_context, throwing);
    const std::array<GeneratedSampleStep, 0> steps{};
    auto& operation = run_generated_sample_data(*user_context, {}, steps);
    bool rejected = false;
    try {
        (void)operation.poll();
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected && operation.status() == AsyncStatus::Failed,
          "Sink exceptions propagate without fake completion");
    user_context.reset();
    check(global.live_user_bytes() == 0, "Throwing sink leaves callback registrations detachable");
}

static void owner_and_expired_resource() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto& shared = global.make<Sink>();
    user_context->insert_resource(global, static_cast<SampleDataLogSink&>(shared));
    check(empty_sample(*user_context).poll(), "Global-owned sink permits execution");
    user_context.reset();
    check(shared.messages.size() == 2 && shared.messages[0].starts_with("Starting sample data") &&
              global.live_user_bytes() == 0,
          "Global sink retains an explicit owner-domain copy after User deletion");
    auto next_user_context = global.make_user_context();
    auto reference = next_user_context->make_ref<Sink>();
    register_sink(*next_user_context, next_user_context->get(reference));
    next_user_context->destroy(reference);
    bool rejected = false;
    try {
        (void)empty_sample(*next_user_context).poll();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, "Expired resource rejects before callback dereference");
    GlobalContext foreign_global;
    auto& foreign = foreign_global.make<Sink>();
    rejected = false;
    try {
        next_user_context->insert_resource(foreign_global,
                                           static_cast<SampleDataLogSink&>(foreign));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "Unrelated Global resource cannot be registered");
}

int main(int argc, char**) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    if (argc > 1) {
        std::set_terminate([] {
            std::_Exit(86);
        });
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto& sink = user_context->make<Sink>(true, false, true);
        register_sink(*user_context, sink);
        (void)empty_sample(*user_context).poll();
        return 4;
    }
    try {
        lazy_and_replacement();
        cancellation_and_deletion();
        missing_disabled_and_exceptions();
        owner_and_expired_resource();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
