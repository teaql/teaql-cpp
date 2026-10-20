#include "../generated_checkers/include/cpp_checker_fixture.hpp"
#include "../support/generated_schema_only_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/event_debug.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <iostream>
#include <sqlite3.h>

using namespace cpp_checker_fixture;

struct State {
    using allocator_type = Allocator;
    mutable Json::Array calls, events;
    std::int64_t version;
    bool sink_fail;
    bool ready = false;

    State(std::allocator_arg_t, Allocator a, std::int64_t value, bool fail)
        : calls(a.resource()), events(a.resource()), version(value), sink_fail(fail) {}

    void record(Memory r,
                std::string_view stage,
                std::string_view kind,
                std::optional<std::int64_t> version) const {
        Json::Object value(r);
        value.emplace(String("stage", r), Json(r, stage));
        value.emplace(String("kind", r), Json(r, kind));
        if (stage != "event") {
            value.emplace(String("version", r), version ? Json(r, *version) : Json(r));
        }
        calls.emplace_back(value);
    }
};

class Behavior final : public NoteBehavior {
public:
    using allocator_type = Allocator;
    State& state;
    String mode;

    Behavior(std::allocator_arg_t, Allocator a, State& value, std::string_view kind)
        : NoteBehavior(std::allocator_arg, a), state(value), mode(kind, a.resource()) {}

    RuntimeHookResult
    call(Memory r, std::string_view kind, std::optional<std::int64_t> version) const {
        state.record(r, "behavior", kind, version);
        if (mode == "deny") {
            return RuntimeHookResult::err(r, RuntimeError::behavior(r, "business behavior denied"));
        }
        return RuntimeHookResult::ok(r, {});
    }

    RuntimeHookResult before_delete(Memory r,
                                    const UserContext& user_context,
                                    DeleteCommand& command) const override {
        auto result = NoteBehavior::before_delete(r, user_context, command);
        if (!result) {
            return result;
        }
        result = call(r, "delete", command.expected_version_value);
        if (result && mode == "rewrite") {
            command.expected_version_value = state.version;
        }
        return result;
    }

    RuntimeHookResult before_recover(Memory r,
                                     const UserContext& user_context,
                                     RecoverCommand& command) const override {
        auto result = NoteBehavior::before_recover(r, user_context, command);
        if (!result) {
            return result;
        }
        result = call(r, "recover", command.expected_version);
        if (result && mode == "rewrite") {
            command.expected_version = state.version;
        }
        return result;
    }
};

class Policy final : public RequestPolicy {
public:
    using allocator_type = Allocator;
    State& state;
    String mode;

    Policy(std::allocator_arg_t, Allocator a, State& value, std::string_view kind)
        : state(value), mode(kind, a.resource()) {}

    RuntimeHookResult
    call(Memory r, std::string_view kind, std::optional<std::int64_t> version) const {
        state.record(r, "policy", kind, version);
        if (mode == "deny") {
            return RuntimeHookResult::err(r, RuntimeError::policy(r, "business policy denied"));
        }
        return RuntimeHookResult::ok(r, {});
    }

    RuntimeHookResult
    enforce_delete(Memory r, const UserContext&, DeleteCommand& command) const override {
        auto result = call(r, "delete", command.expected_version_value);
        if (result && mode == "rewrite") {
            command.expected_version_value = state.version;
        }
        return result;
    }

    RuntimeHookResult
    enforce_recover(Memory r, const UserContext&, RecoverCommand& command) const override {
        auto result = call(r, "recover", command.expected_version);
        if (result && mode == "rewrite") {
            command.expected_version = state.version;
        }
        return result;
    }
};

class Sink final : public RawAuditEventSink {
public:
    using allocator_type = Allocator;
    State& state;

    Sink(std::allocator_arg_t, Allocator, State& value) : state(value) {}

    RuntimeHookResult
    on_event(Memory r, const UserContext&, const RawAuditEvent& event) const override {
        if (!state.ready) {
            return RuntimeHookResult::ok(r, {});
        }
        auto kind = debug_string(r, event.kind);
        state.record(r, "event", kind, {});
        Json::Object out(r), values(r);
        for (const auto& [name, value] : event.values) {
            values.emplace(String(name, r), value.to_json_value(r));
        }
        Json::Array trace(r);
        for (const auto& node : event.trace_chain) {
            trace.emplace_back(node.comment);
        }
        out.emplace(String("kind", r), Json(r, kind));
        out.emplace(String("entity", r), Json(r, event.entity));
        out.emplace(String("values", r), Json(r, values));
        out.emplace(String("old", r), Json(r, bool(event.old_values)));
        out.emplace(String("new", r), Json(r, bool(event.new_values)));
        out.emplace(String("trace", r), Json(r, trace));
        state.events.emplace_back(out);
        if (state.sink_fail) {
            return RuntimeHookResult::err(r,
                                          RuntimeError::event(r, "business event delivery failed"));
        }
        return RuntimeHookResult::ok(r, {});
    }
};

static void execute(sqlite3* database, const std::string& sql) {
    char* message = nullptr;
    if (sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &message) != SQLITE_OK) {
        std::string error = message ? message : sqlite3_errmsg(database);
        sqlite3_free(message);
        throw std::runtime_error(error);
    }
}

static Json database_view(Memory r, sqlite3* database) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(
            database, "SELECT id,name,version FROM note_data ORDER BY id", -1, &raw, nullptr) !=
        SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array rows(r);
    int status;
    while ((status = sqlite3_step(raw)) == SQLITE_ROW) {
        Json::Array row(r);
        row.emplace_back(std::int64_t(sqlite3_column_int64(raw, 0)));
        row.emplace_back(
            std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(raw, 1)),
                             sqlite3_column_bytes(raw, 1)));
        row.emplace_back(std::int64_t(sqlite3_column_int64(raw, 2)));
        rows.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, rows);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto operation = text(field(input, "operation"));
    bool recover = operation == "recover";
    std::int64_t version = recover ? -3 : 3;
    auto& state = user_context.make<State>(version, std::get<bool>(field(input, "sink_fail").data));
    auto& behavior = user_context.make<Behavior>(state, text(field(input, "behavior")));
    auto& sink = user_context.make<Sink>(state);
    auto& module = user_context.make<RuntimeModule>();
    module.entity<Tenant>()
        .entity_with_behavior<Note>(user_context, behavior)
        .event_sink(user_context, sink);
    user_context.disable_sql_log();
    auto executor = teaql::test::configure_schema_only<cpp_checker_fixture::DataServiceExecutor>(
        user_context, module);
    auto transport = user_context.require_resource<SqliteMutationExecutor>();
    auto* database = transport.value().get().connection().native_handle();
    execute(database, "INSERT INTO tenant_data(id,name,version) VALUES(42,'trusted',3);");
    auto fixture_state = text(field(input, "state"));
    if (fixture_state != "missing") {
        execute(
            database,
            "INSERT INTO note_data(id,name,tenant,created_at,updated_at,version) VALUES(7,'stored',42,111,222," +
                std::to_string(version) + ")");
    }
    if (fixture_state == "trigger") {
        execute(database,
                "CREATE TRIGGER fixture_failure BEFORE " +
                    std::string(operation == "hard_delete" ? "DELETE" : "UPDATE") +
                    " ON note_data BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;");
    }
    auto& policy = user_context.make<Policy>(state, text(field(input, "policy")));
    user_context.set_request_policy(user_context, policy);
    state.ready = true;
    user_context.enable_mutation_sql_log();
    auto& service = EntityDataService<cpp_checker_fixture::DataServiceExecutor>::for_executor(
        user_context, "Note", user_context, user_context.get(executor));
    bool traced = std::get<bool>(field(input, "trace").data);
    TraceChain lineage(r);
    if (traced) {
        lineage.emplace_back("Note", "scope trace", std::uint64_t(7));
    }
    service.with_trace_context(lineage);
    auto expected = fixture_state == "stale" ? version + (recover ? -1 : 1) : version;
    auto intent = text(field(input, "intent"));
    RuntimeHookResult bound = RuntimeHookResult::ok(r, {});
    if (intent != "missing") {
        bound = detail::EntityDataServiceAccess::bind_mutation_intent(
            service, intent == "blank" ? "  " : "Review protected business deletion/recovery");
    }
    Json::Object outcome(r);
    auto inspect = [&](const auto& result) {
        if (result) {
            outcome.emplace(String("rows", r), Json(r, result.value()));
        } else {
            outcome.emplace(String("error", r), Json(r, result.error().to_string(r)));
        }
    };
    if (!bound) {
        using R = Result<std::uint64_t,
                         DataServiceError<cpp_checker_fixture::DataServiceExecutor::Error>>;
        inspect(R::err(r,
                       DataServiceError<cpp_checker_fixture::DataServiceExecutor::Error>::runtime(
                           r, std::move(bound).error())));
    } else if (recover) {
        auto command = service.recover_command(std::uint64_t(7), expected);
        if (traced) {
            command.trace_chain.emplace_back("Note", "command trace", std::uint64_t(7));
        }
        inspect(detail::EntityDataServiceAccess::recover_internal(service, command).wait());
    } else {
        auto command = service.delete_command(std::uint64_t(7));
        command.expected_version(expected);
        if (operation == "hard_delete") {
            command.soft_delete = false;
        }
        if (traced) {
            command.trace_chain.emplace_back("Note", "command trace", std::uint64_t(7));
        }
        inspect(detail::EntityDataServiceAccess::delete_internal(service, command).wait());
    }
    Json::Array logs(r);
    for (const auto& log : user_context.sql_logs()) {
        Json::Object value(r);
        value.emplace(String("sql", r), Json(r, log.sql));
        value.emplace(String("comment", r), log.comment ? Json(r, *log.comment) : Json(r));
        value.emplace(String("purpose", r), log.purpose ? Json(r, *log.purpose) : Json(r));
        value.emplace(String("audit", r), log.audit_reason ? Json(r, *log.audit_reason) : Json(r));
        value.emplace(String("operation", r), Json(r, debug_string(r, log.operation)));
        value.emplace(String("summary", r), Json(r, log.result_summary));
        value.emplace(String("rows", r), log.affected_rows ? Json(r, *log.affected_rows) : Json(r));
        logs.emplace_back(value);
    }
    Json::Object result(r);
    result.emplace(String("outcome", r), Json(r, outcome));
    result.emplace(String("calls", r), Json(r, state.calls));
    result.emplace(String("events", r), Json(r, state.events));
    result.emplace(String("database", r), database_view(r, database));
    result.emplace(String("logs", r), Json(r, logs));
    return Json(r, result);
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            Json retained(global.memory_resource());
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("invalid protected-behavior fixture");
                }
                auto result = observe(*user_context, *input);
                retained = detail::clone_in(global.memory_resource(), result);
            }
            user_context.reset();
            std::cout << retained.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
