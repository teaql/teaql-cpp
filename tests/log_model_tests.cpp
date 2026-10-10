#include <teaql/runtime.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto source = user->make_ref<SqlLogEntry>(SqlLogOperation::Update);
    auto& sql = user->get(source);
    sql.sql.assign(512, 's');
    sql.debug_sql = "raw diagnostic";
    sql.pretty_sql = "pretty diagnostic";
    sql.params.emplace_back(String(512, 'p', owner));
    sql.log_context.database_kind.emplace("sqlite", owner);
    sql.log_context.parameter_policies.push_back(SqlParameterLogPolicy::Masked);
    sql.log_context.masked_parameters.push_back(true);
    sql.log_context.intent_redactions.capture_target_id(Value(owner, "target-id"));
    sql.trace_path.emplace_back(TraceKind::Sql, "sqlite", "trace🧊");
    sql.elapsed = LogDuration(UINT64_MAX, 999999999);
    sql.started_at = SystemTime::from_unix_seconds(-1, 999999999);
    sql.ended_at = SystemTime::from_unix_seconds(253402300800, 123456789);
    sql.result_summary = "summary";
    auto& buffer = user->make<UnifiedLogBuffer>();
    {
        LogPayload payload(detail::clone_in(owner, sql));
        auto& entry = buffer.entries.emplace_back(sql.started_at, payload);
        entry.user_identifier.emplace("user🧊", owner);
        entry.trace_chain = detail::clone_in(owner, sql.trace_path);
        InfoLogEntry info(std::allocator_arg, Allocator(owner), "info message");
        LogPayload info_payload(detail::clone_in(owner, info));
        buffer.entries.emplace_back(sql.ended_at, info_payload);
    }
    auto kept = user->make_global<UnifiedLogBuffer>(buffer);
    auto retained = kept;
    {
        auto text = debug_string(owner, buffer.entries[0]);
        check(
            text.find("target-id") == text.npos && text.find("raw diagnostic") != text.npos,
            "Derived raw model Debug preserves fields while provenance redactions keep their source count-only shape");
    }
    user.reset();
    check(global.live_user_bytes() == 0, "Source log User is fully reclaimed");
    auto next = global.make_user_context();
    const auto& copied = next->get(next->rebind_global(retained));
    check(copied.entries.size() == 2 &&
              std::get<SqlLogEntry>(copied.entries[0].payload).params[0].memory_resource() ==
                  global.memory_resource(),
          "Unified buffer copies both variants and all nested SQL payloads into Global");
    {
        auto diagnostic =
            debug_string(next->memory_resource(), copied.entries[0], DebugOptions{.pretty = true});
        check(diagnostic.get_allocator().resource() == next->memory_resource() &&
                  diagnostic.find("summary") != diagnostic.npos,
              "Retained log diagnostics use the caller's current User");
    }
    bool rejected = false;
    try {
        (void)next->get(source);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Expired SQL log reference rejects before diagnostics or parameter access");
    std::cout << "Runtime log model ownership contracts pass\n";
}
