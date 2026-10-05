#include <teaql/runtime/detail/governance_scope.hpp>
#include <teaql/runtime/event.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <chrono>
#include <iostream>
#include <fstream>
#include <unistd.h>
using namespace teaql;
using Outcome = Result<std::monostate, RuntimeError>;

static void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

template <class F>
void rejected(F&& f, const char* message) {
    bool caught = false;
    try {
        f();
    } catch (const std::logic_error&) {
        caught = true;
    }
    check(caught, message);
}

static MutationGovernanceSnapshot snapshot(Memory r, std::string_view id) {
    MutationGovernanceSnapshot s(std::allocator_arg,
                                 Allocator(r),
                                 id,
                                 "request",
                                 MutationPolicySource::Customer,
                                 MutationPolicyApprovalStatus::Approved);
    s.warning_codes.emplace_back("long warning stored in the original arena");
    return s;
}

static std::string_view active(const UserContext& c) {
    auto s = detail::current_mutation_governance(c);
    if (!s)
        return "none"; // return a Context-owned clone, not this temporary's string
    return c.make<String>(s->execution_id);
}

static std::string file_text(const char* path) {
    std::ifstream in(path);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

struct State {
    using allocator_type = Allocator;
    Vector<String> calls;
    Vector<RawAuditEvent> raw;
    Vector<SafeAuditEvent> safe;
    bool raw_error = false, safe_error = false, raw_throw = false;
    unsigned drops = 0;
    const char* path = nullptr;
    std::size_t previous_size = 0;

    State(std::allocator_arg_t, Allocator a)
        : calls(a.resource()), raw(a.resource()), safe(a.resource()) {}

    void note(std::string_view s) {
        calls.emplace_back(s);
    }
};

struct Raw final : RawAuditEventSink {
    State& state;

    explicit Raw(State& state) : state(state) {}

    Outcome on_event(Memory r, const UserContext&, const RawAuditEvent& e) const override {
        if (state.path)
            check(file_text(state.path).size() == state.previous_size,
                  "raw sink precedes log output");
        state.note("raw");
        state.raw.emplace_back(e);
        if (state.raw_throw)
            throw std::runtime_error("sink panic");
        if (state.raw_error)
            return Outcome::err(r, RuntimeError::event(r, "raw failed"));
        return Outcome::ok(r, {});
    }
};

struct Safe final : SafeAuditEventSink {
    State& state;

    explicit Safe(State& state) : state(state) {}

    Outcome on_safe_event(Memory r, const UserContext&, const SafeAuditEvent& e) const override {
        if (state.path)
            check(file_text(state.path).size() == state.previous_size,
                  "safe sink precedes log output");
        state.note("safe");
        state.safe.emplace_back(e);
        if (state.safe_error)
            return Outcome::err(r, RuntimeError::event(r, "safe failed"));
        return Outcome::ok(r, {});
    }
};

struct Scope final : RuntimeTelemetryScope {
    State& state;

    explicit Scope(State& state) : state(state) {}

    ~Scope() {
        ++state.drops;
        state.note("drop");
    }

    void success(const RuntimeAttributes& attrs) override {
        check(attrs.empty(), "audit success attributes empty");
        if (state.path)
            check(file_text(state.path).size() > state.previous_size,
                  "log output precedes telemetry completion");
        state.note("success");
    }

    void failure(std::string_view type) override {
        check(type == "audit_error", "fixed audit failure category");
        state.note("failure");
    }
};

struct Telemetry final : RuntimeTelemetry {
    using allocator_type = Allocator;
    Memory r;
    State& state;

    Telemetry(std::allocator_arg_t, Allocator a, State& s) : r(a.resource()), state(s) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory target,
                                             const RuntimeOperation& op) const override {
        check(op.family == "audit" && op.name == "Order.event", "audit operation labels");
        check(std::get<String>(
                  op.attributes.find(std::string_view("teaql.entity.type"))->second.data) ==
                  "Order",
              "entity attribute");
        state.note("start");
        return detail::resource_arena(target).make_resource<ContextBox<RuntimeTelemetryScope>>(
            std::in_place_type<Scope>, state);
    }
};

static RawAuditEvent event(Memory r) {
    Record values(r);
    values.emplace(String("name", r), Value(r, "private name"));
    values.emplace(String("version", r), Value(r, 7));
    return RawAuditEvent::created(r, "Order", values);
}

static void pipeline() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& state = global.make<State>();
    auto& raw = global.make_resource<Raw>(state);
    auto& safe = global.make_resource<Safe>(state);
    auto& telemetry = global.make<Telemetry>(state);
    auto& module = global.make<RuntimeModule>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(global.memory_resource()), "Order");
    descriptor.audit_mask_fields.emplace_back("name");
    module.descriptor(std::move(descriptor)).event_sink(global, raw);
    module.apply_to(*user);
    user->with_custom_event_sink(global, safe).with_runtime_telemetry(global, telemetry);
    auto& input = user->make<RawAuditEvent>(event(r));
    input.mutation_governance = snapshot(r, "caller forged");
    check(bool(user->send_event(input)), "event sent");
    check(state.calls.size() == 5 && state.calls[0] == "start" && state.calls[1] == "raw" &&
              state.calls[2] == "safe" && state.calls[3] == "success" && state.calls[4] == "drop",
          "sink and telemetry order");
    check(!state.raw.back().mutation_governance && !state.safe.back().mutation_governance,
          "caller governance cleared outside scope");
    check(input.mutation_governance->execution_id == "caller forged", "caller input unchanged");
    check(state.safe.back().fields[0].masked, "metadata masks reach safe sink");
    check(state.drops == 1, "scope released on return");
    {
        auto s = snapshot(r, "accepted");
        detail::GovernanceActivation scope(*user, s);
        check(bool(user->send_event(input)), "scoped send");
        check(state.raw.back().mutation_governance->execution_id == "accepted" &&
                  state.safe.back().mutation_governance->execution_id == "accepted",
              "snapshot attached before both sinks");
    }
    for (bool first : {true, false}) {
        state.calls.clear();
        state.raw_error = first;
        state.safe_error = !first;
        auto result = user->send_event(input);
        check(!result && result.error().message() == (first ? "raw failed" : "safe failed"),
              "original error returned");
        check(state.calls.size() == (first ? 4 : 5) &&
                  state.calls[state.calls.size() - 2] == "failure",
              "first error stops downstream and completes telemetry");
    }
    state.raw_error = state.safe_error = false;
    state.raw_throw = true;
    state.calls.clear();
    bool caught = false;
    try {
        user->send_event(input);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    check(caught && state.calls.size() == 3 && state.calls.back() == "drop",
          "exception abandons telemetry without false failure/success");
    state.raw_throw = false;
    auto sibling = global.make_user_context();
    auto& local = user->make_resource<Raw>(state);
    rejected(
        [&] {
            sibling->set_event_sink(*user, local);
        },
        "sibling sink borrow rejected");
    auto& local_module = user->make<RuntimeModule>();
    local_module.event_sink(*user, local);
    rejected(
        [&] {
            local_module.apply_to(*sibling);
        },
        "module cannot extend User sink lifetime");
    user->close();
    check(state.raw.back().values.find(std::string_view("name"))->second.try_text() ==
              "private name",
          "Global sink retained explicit deep copy");
    check(state.raw[1].mutation_governance->warning_codes.front().get_allocator().resource() ==
              global.memory_resource(),
          "nested snapshot cloned to Global");
    check(user->allocation_stats().live_blocks == 0, "all event blocks released");
}

struct Pending {
    using allocator_type = Allocator;
    const UserContext& context;
    State& state;
    bool ready = false, panic = false;

    Pending(std::allocator_arg_t, Allocator, const UserContext& c, State& s)
        : context(c), state(s) {}

    std::optional<int> poll(WakeSignal&) {
        state.note(active(context));
        if (panic)
            throw std::runtime_error("poll panic");
        if (ready)
            return 7;
        return {};
    }

    void finish_and_wait() noexcept {
        state.note(active(context));
    }

    void cancel_and_wait() noexcept {
        state.note(active(context));
    }
};

struct Nested {
    using allocator_type = Allocator;
    const UserContext& context;
    State& state;
    AsyncOperation<int>* child = nullptr;

    Nested(std::allocator_arg_t, Allocator, const UserContext& c, State& s)
        : context(c), state(s) {}

    std::optional<int> poll(WakeSignal&) {
        if (!child) {
            auto s = snapshot(context.memory_resource(), "inner");
            auto& work = make_operation<int, Pending>(context.memory_resource(), context, state);
            child = &detail::with_mutation_governance(context, s, work);
        }
        child->poll();
        state.note(active(context));
        return {};
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        child->cancel();
        state.note(active(context));
    }
};

static void scopes() {
    GlobalContext global, isolated;
    auto user = global.make_user_context();
    auto sibling = global.make_user_context();
    auto other = isolated.make_user_context();
    auto& state = global.make<State>();
    auto r = user->memory_resource();
    auto& s = user->make<MutationGovernanceSnapshot>(snapshot(r, "outer"));
    {
        detail::GovernanceActivation a(*user, s);
        check(active(*sibling) == "outer" && active(*other) == "none",
              "explicit application activation isolation");
        auto inner = snapshot(r, "inner");
        {
            detail::GovernanceActivation b(*user, inner);
            check(active(*user) == "inner", "nested replaces snapshot");
        }
        check(active(*user) == "outer", "nested restores snapshot");
    }
    auto& pending = user->make<Pending>(*user, state);
    auto& child = user->make<AsyncOperation<int>>(pending);
    auto& wrapped = detail::with_mutation_governance(*user, s, child);
    s.execution_id = "modified after cloning";
    check(!wrapped.poll() && active(*user) == "none" && state.calls.back() == "outer",
          "snapshot cloned and deactivated on suspension");
    auto ticket = wrapped.wake_signal().ticket();
    child.wake_signal().notify();
    check(ticket != wrapped.wake_signal().ticket(), "governance forwards child wake");
    pending.ready = true;
    check(wrapped.wait() == 7 && active(*user) == "none", "ready restores scope");
    auto& bad = user->make<Pending>(*user, state);
    bad.panic = true;
    auto& bad_child = user->make<AsyncOperation<int>>(bad);
    auto& bad_outer = detail::with_mutation_governance(*user, s, bad_child);
    bool caught = false;
    try {
        bad_outer.poll();
    } catch (const std::runtime_error&) {
        caught = true;
    }
    check(caught && active(*user) == "none" && state.calls.back() == s.execution_id,
          "throw cancellation sees scope and unwinds");
    auto& wrong = make_operation<int, Pending>(*sibling, *sibling, state);
    rejected(
        [&] {
            detail::with_mutation_governance(*user, s, wrong);
        },
        "foreign operation rejected without cancellation");
    check(wrong.status() == AsyncStatus::Pending, "foreign operation remains owned by sibling");
    wrong.cancel();
    state.calls.clear();
    auto& nested = make_operation<int, Nested>(*user, *user, state);
    auto& outer = user->make<MutationGovernanceSnapshot>(snapshot(r, "outer"));
    auto& operation = detail::with_mutation_governance(*user, outer, nested);
    check(!operation.poll(), "nested lazy child pending");
    check(state.calls.size() == 2 && state.calls[0] == "inner" && state.calls[1] == "outer",
          "nested poll scopes");
    user->close();
    check(state.calls.size() == 4 && state.calls[2] == "inner" && state.calls[3] == "outer",
          "Context close restores each nested cancellation scope");
    check(active(*sibling) == "none" && user->allocation_stats().live_blocks == 0,
          "close restores application slot and frees blocks");
}

struct Ready {
    std::optional<Outcome> poll(WakeSignal&) {
        return Outcome::ok(r, {});
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}

    Memory r;
};

struct Provider final : runtime::SchemaProvider {
    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& c,
                                                       const SchemaInvocation&) const override {
        return make_operation<Outcome, Ready>(c.memory_resource(), Ready{c.memory_resource()});
    }
};

static GeneratedSchemaBootstrapFuture& bootstrap(const UserContext& c) {
    auto r = c.memory_resource();
    for (int kind = 0; kind < 8; ++kind)
        for (int variant = 0; variant < 4; ++variant) {
            auto e = event(r);
            e.kind = static_cast<RawAuditEventKind>(kind);
            e.bootstrap_audit.emplace(
                std::allocator_arg, Allocator(r), "caller", "caller", "caller", 99, 1);
            if (variant == 1)
                e.new_values->insert_or_assign(String("version", r), Value(r));
            if (variant == 2)
                e.new_values->clear();
            if (variant == 3)
                e.trace_chain.emplace_back(TraceKind::Comment, "Order", "");
            check(bool(c.send_event(e)), "bootstrap audit send");
        }
    return make_operation<Outcome, Ready>(r, Ready{r});
}

static void bootstrap_identity() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& state = global.make<State>();
    auto& sink = global.make_resource<Raw>(state);
    auto& provider = global.make_resource<Provider>();
    auto& module = global.make<RuntimeModule>();
    module.event_sink(global, sink).generated_schema_bootstrap(bootstrap);
    module.apply_to(*user);
    user->set_schema_provider(global, provider);
    auto before = std::chrono::system_clock::now();
    check(bool(user->ensure_schema().wait()), "schema callback audit succeeds");
    auto after = std::chrono::system_clock::now();
    for (std::size_t i = 0; i < state.raw.size(); ++i) {
        auto& b = *state.raw[i].bootstrap_audit;
        if (i / 4 < 2) {
            check(b.actor == "teaql-generated-bootstrap" && b.category == "runtime-bootstrap",
                  "bootstrap identity replaced only for create/update");
            check(b.reason == (i % 4 == 3 ? "" : "generated runtime bootstrap"),
                  "empty last comment preserved");
            check(b.resulting_version ==
                      (i % 4 == 1 ? std::nullopt : std::optional<std::int64_t>(7)),
                  "present null prevents old version fallback");
            check(b.occurred_at_millis >= std::chrono::duration_cast<std::chrono::milliseconds>(
                                              before.time_since_epoch())
                                              .count() &&
                      b.occurred_at_millis <= std::chrono::duration_cast<std::chrono::milliseconds>(
                                                  after.time_since_epoch())
                                                  .count(),
                  "bootstrap timestamp taken during send");
        } else
            check(b.actor == "caller" && b.occurred_at_millis == 1,
                  "non-create/update identity retained");
    }
}

static void log_order() {
    char path[] = "/tmp/teaql-event-log-XXXXXX";
    int fd = mkstemp(path);
    check(fd >= 0, "temporary event file");
    close(fd);

    struct Remove {
        const char* path;

        ~Remove() {
            std::remove(path);
        }
    } remove{path};

    setenv("TEAQL_LOG_ENDPOINT", path, 1);
    setenv("TEAQL_LOG_FORMAT", "debug", 1);
    unsetenv("TEAQL_AUDIT_LOG");
    unsetenv("TEAQL_AUDIT_LOG_ENTITIES");
    {
        GlobalContext global;
        auto user = global.make_user_context();
        auto& state = global.make<State>();
        state.path = path;
        auto& raw = global.make_resource<Raw>(state);
        auto& safe = global.make_resource<Safe>(state);
        auto& telemetry = global.make<Telemetry>(state);
        user->with_event_sink(global, raw)
            .with_custom_event_sink(global, safe)
            .with_runtime_telemetry(global, telemetry);
        auto e = event(user->memory_resource());
        check(bool(user->send_event(e)), "event reaches file");
        auto output = file_text(path);
        check(output.find("[AUDIT_LOG]") != output.npos &&
                  output.find("private name") == output.npos,
              "ordinary event log projects private values");
        state.previous_size = output.size();
        state.safe_error = true;
        check(!user->send_event(e) && file_text(path).size() == state.previous_size,
              "safe sink error prevents log output");
        state.raw_error = true;
        check(!user->send_event(e) && file_text(path).size() == state.previous_size,
              "raw sink error prevents safe sink and log");
    }
    setenv("TEAQL_LOG_ENDPOINT", "stdout", 1);
    {
        GlobalContext global;
        auto user = global.make_user_context();
        auto& state = global.make<State>();
        auto& raw = global.make_resource<Raw>(state);
        auto& safe = global.make_resource<Safe>(state);
        auto& telemetry = global.make<Telemetry>(state);
        user->with_event_sink(global, raw)
            .with_custom_event_sink(global, safe)
            .with_runtime_telemetry(global, telemetry);
        auto e = event(user->memory_resource());
        int saved = dup(fileno(stdout));
        auto* full = std::fopen("/dev/full", "wb");
        check(saved >= 0 && full, "stdout failure setup");
        std::fflush(stdout);
        dup2(fileno(full), fileno(stdout));
        bool failed = false;
        try {
            user->send_event(e);
        } catch (const std::runtime_error&) {
            failed = true;
        }
        std::fflush(stdout);
        dup2(saved, fileno(stdout));
        close(saved);
        std::fclose(full);
        std::clearerr(stdout);
        check(failed && state.calls.size() == 4 && state.calls[1] == "raw" &&
                  state.calls[2] == "safe" && state.calls[3] == "drop",
              "log exception occurs after sinks and abandons telemetry without a false result");
    }
    setenv("TEAQL_LOG_ENDPOINT", "off", 1);
}

int main() {
    try {
        setenv("TEAQL_LOG_ENDPOINT", "off", 1);
        unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
        auto* old = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        pipeline();
        scopes();
        bootstrap_identity();
        log_order();
        std::pmr::set_default_resource(old);
        std::cout << "Context event and governance scope contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
