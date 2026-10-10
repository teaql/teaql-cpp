// Original formatter/telemetry contracts using controlled failure adapters.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_formatter.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void bounded_diagnostic_log_marks_partial_sql_non_executable(Memory owner) {
    check(detail::truncate_sensitive_sql_log(owner, "SELECT 1", 100) == "SELECT 1",
          "original short diagnostic SQL is unchanged");
    auto truncated = detail::truncate_sensitive_sql_log(owner, "SELECT '🔐private-value'", 11);
    check(truncated.find("TRUNCATED; NOT EXECUTABLE") != String::npos &&
              truncated.find("private-value") == String::npos,
          "original bounded diagnostic SQL is visibly non-executable and excludes its canary");
    check(truncated == "SELECT '\n[TRUNCATED; NOT EXECUTABLE: diagnostic SQL exceeded 11 bytes]",
          "a limit inside the four-byte lock scalar retreats to the original UTF-8 boundary");
}

static String ordinary_audit_formatters_disclose_field_names_but_not_values(Memory owner) {
    Record values(owner);
    values.emplace(String("id", owner), Value(owner, std::uint64_t(7)));
    values.emplace(String("name", owner), Value(owner, "private-school-name"));
    auto event = RawAuditEvent::created(owner, "School", values);
    HumanReaderFormatter human;
    DebugReaderFormatter debug;
    String retained(owner);
    for (const LogFormatter* formatter :
         {static_cast<const LogFormatter*>(&human), static_cast<const LogFormatter*>(&debug)}) {
        auto safe = formatter->format_audit_log(owner, event);
        check(safe.find("School") != String::npos && safe.find("name") != String::npos &&
                  safe.find("private-school-name") == String::npos,
              "all original ordinary audit disclosure assertions for both formatters");
        auto sensitive = formatter->format_sensitive_audit_log(owner, event);
        check(sensitive.find("private-school-name") != String::npos,
              "original explicit sensitive formatter preserves its payload contract");
        retained = std::move(safe);
    }
    return retained;
}

static void bounded_sensitive_audit_log_does_not_split_utf8(Memory owner) {
    auto truncated = detail::truncate_sensitive_audit_log(owner, "audit 🔐private-value", 10);
    check(truncated.find("TRUNCATED") != String::npos &&
              truncated.find("private-value") == String::npos,
          "original bounded audit rendering excludes the canary");
    check(truncated == "audit 🔐\n[TRUNCATED: sensitive audit event exceeded 10 bytes]",
          "original audit rendering retains the complete four-byte scalar at the exact limit");
    check(detail::truncate_sensitive_audit_log(owner, "audit 🔐private-value", 9) ==
              "audit \n[TRUNCATED: sensitive audit event exceeded 9 bytes]",
          "a limit within the scalar omits every byte of that scalar");
}

static void raw_audit_output_requires_full_with_payload_level(Memory owner) {
    LogConfig config(std::allocator_arg, Allocator(owner));
    config.sql_level = LogLevel::Silent;
    config.tool_level = LogLevel::Silent;
    config.audit_level = LogLevel::Summary;
    check(!config.should_log_sensitive_audit("School"),
          "original Summary excludes raw audit payload");
    config.audit_level = LogLevel::Full;
    check(!config.should_log_sensitive_audit("School"), "original Full excludes raw audit payload");
    config.audit_level = LogLevel::FullWithPayload;
    check(config.should_log_sensitive_audit("School"),
          "original FullWithPayload permits raw audit payload");
}

static void classifies_native_error_types_without_messages() {
    check(runtime_error_category("DatabaseTimeoutError") == "timeout",
          "original native timeout category");
    check(runtime_error_category("PermissionError") == "authorization",
          "original native authorization category");
    check(runtime_error_category("UnknownTeaQLError") == "internal",
          "original unknown native category");
}

struct Counts {
    unsigned starts = 0, activations = 0, polls = 0, destroys = 0;
};

class BrokenTelemetry final : public RuntimeTelemetry {
public:
    using allocator_type = Allocator;
    Memory owner;
    Counts& counts;

    BrokenTelemetry(std::allocator_arg_t, Allocator a, Counts& counts)
        : owner(a.resource()), counts(counts) {}

    Memory memory_resource() const noexcept override {
        return owner;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory, const RuntimeOperation&) const override {
        ++counts.starts;
        throw std::runtime_error("adapter failed");
    }
};

class BrokenContextScope final : public RuntimeTelemetryScope {
public:
    using allocator_type = Allocator;
    Counts& counts;

    BrokenContextScope(std::allocator_arg_t, Allocator, Counts& counts) : counts(counts) {}

    ~BrokenContextScope() noexcept override {
        ++counts.destroys;
    }

    void with_context(FunctionRef<void()>) const override {
        ++counts.activations;
        throw std::runtime_error("context adapter failed");
    }

    void success(const RuntimeAttributes&) override {}

    void failure(std::string_view) override {}
};

class BrokenContextTelemetry final : public RuntimeTelemetry {
public:
    using allocator_type = Allocator;
    Memory owner;
    Counts& counts;

    BrokenContextTelemetry(std::allocator_arg_t, Allocator a, Counts& counts)
        : owner(a.resource()), counts(counts) {}

    Memory memory_resource() const noexcept override {
        return owner;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory destination,
                                             const RuntimeOperation&) const override {
        ++counts.starts;
        return detail::resource_arena(destination)
            .make<ContextBox<RuntimeTelemetryScope>>(std::in_place_type<BrokenContextScope>,
                                                     counts);
    }
};

struct Ready42 {
    Counts& counts;

    std::optional<int> poll(WakeSignal&) {
        ++counts.polls;
        return 42;
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

static void strips_forbidden_attributes_and_is_fail_open(UserContext& user, Counts& counts) {
    auto owner = user.memory_resource();
    RuntimeOperation operation(std::allocator_arg, Allocator(owner), "query", "School.list");
    operation.attribute("teaql.entity.type", "School")
        .attribute("teaql.entity.id", std::int64_t(42));
    auto found = operation.attributes.find("teaql.entity.type");
    check(found != operation.attributes.end() && std::get<String>(found->second.data) == "School" &&
              !operation.attributes.contains("teaql.entity.id"),
          "original safe String attribute remains and forbidden ID is removed");
    auto& telemetry = user.make<BrokenTelemetry>(counts);
    auto& scope = start_runtime_operation(owner, telemetry, operation);
    RuntimeAttributes attributes(owner);
    scope.success(attributes);
    scope.failure("late");
    check(counts.starts == 1,
          "original throwing telemetry start is attempted once without escaping");
}

static void context_activation_is_fail_open(UserContext& user, Counts& counts) {
    auto owner = user.memory_resource();
    auto& telemetry = user.make<BrokenContextTelemetry>(counts);
    RuntimeOperation operation(std::allocator_arg, Allocator(owner), "query", "School.list");
    auto& scope = start_runtime_operation(owner, telemetry, operation);
    auto& future = make_operation<int, Ready42>(owner, counts);
    auto& wrapped = scope.run(owner, future);
    check(wrapped.wait() == 42,
          "original async business result survives throwing context activation");
    check(counts.starts == 1 && counts.activations == 1 && counts.polls == 1,
          "context failure neither suppresses nor duplicates the actual future poll");
}

static void original_contracts() {
    Counts broken, context;
    GlobalContext global;
    Ref<String> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        bounded_diagnostic_log_marks_partial_sql_non_executable(owner);
        auto safe = ordinary_audit_formatters_disclose_field_names_but_not_values(owner);
        retained = user->make_global<String>(safe);
        bounded_sensitive_audit_log_does_not_split_utf8(owner);
        raw_audit_output_requires_full_with_payload_level(owner);
        classifies_native_error_types_without_messages();
        strips_forbidden_attributes_and_is_fail_open(*user, broken);
        context_activation_is_fail_open(*user, context);
    }
    check(global.live_user_bytes() == 0 && context.destroys == 1,
          "actual User deletion releases every block and destroys the delegate exactly once");
    auto next = global.make_user_context();
    const auto& safe = next->get(next->rebind_global(retained));
    check(safe.find("School") != String::npos && safe.find("name") != String::npos &&
              safe.find("private-school-name") == String::npos,
          "explicit Global ordinary audit output remains safe after source User deletion");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        original_contracts();
        std::pmr::set_default_resource(previous);
        std::cout << "Original seven log formatter/telemetry contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
