#include <teaql/runtime/context.hpp>
#include <teaql/runtime/sqlite.hpp>
#include <teaql/runtime/logging.hpp>
#include <teaql/core/detail/sha256.hpp>
#include <iostream>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

static SqlLogEntry sample(Memory r) {
    SqlLogEntry e(std::allocator_arg, Allocator(r), SqlLogOperation::Update);
    e.sql = "UPDATE item SET name=? WHERE id=?";
    e.params.emplace_back(Value(r, "private business value for allocation"));
    e.params.emplace_back(Value(r, std::uint64_t(1001)));
    e.log_context.database_kind.emplace("sqlite", r);
    e.log_context.generated_sql = true;
    e.log_context.parameter_policies = {SqlParameterLogPolicy::Masked,
                                        SqlParameterLogPolicy::Plain};
    e.log_context.execution_outcome = SqlExecutionOutcome::Success;
    e.comment.emplace("private business value for allocation", r);
    e.purpose.emplace("update private business value for allocation", r);
    e.trace_path.emplace_back(
        TraceKind::Entity, "Item", "private business value for allocation", 1001);
    e.debug_sql = "untrusted prior interpolation";
    e.pretty_sql = "untrusted pretty SQL";
    e.affected_rows = 1;
    e.result_summary = "1 row affected";
    e.elapsed = {1, 1001};
    return e;
}

static void lifetime() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource(), g = global.memory_resource();
    auto& source = user->make<SqlLogEntry>(sample(r));
    auto& sensitive = user->make<SqlLogEntry>(detail::sql_entry_projection(r, source, true));
    auto& expected = global.make<SqlLogEntry>(detail::sql_entry_projection(r, source, false));
    auto& retained = global.make<SqlLogEntry>(sensitive);
    auto& assigned = global.make<SqlLogEntry>();
    assigned = detail::clone_in(r, sensitive);
    auto& buffer = user->make<UnifiedLogBuffer>();
    LogPayload payload(
        std::in_place_type<SqlLogEntry>, std::allocator_arg, Allocator(r), sensitive);
    auto& unified = user->make<UnifiedLogEntry>(source.started_at, payload);
    unified.user_identifier.emplace("long user identifier belonging to UserContext", r);
    unified.trace_chain.emplace_back(
        TraceKind::Entity, "Item", "long trace comment belonging to UserContext");
    buffer.entries.emplace_back(unified);
    auto& copied_buffer = global.make<UnifiedLogBuffer>(buffer);
    auto& assigned_buffer = global.make<UnifiedLogBuffer>();
    assigned_buffer = detail::clone_in(r, buffer);
    // Destroy owning locals before their arena is returned, including payload.
    payload = InfoLogEntry(std::allocator_arg, Allocator(g), "released source payload");
    user.reset();
    for (const auto* e :
         {&retained, &assigned, &std::get<SqlLogEntry>(copied_buffer.entries[0].payload)}) {
        auto safe = detail::sql_entry_projection(g, *e, false);
        check(detail::debug_sql_log(g, safe) == detail::debug_sql_log(g, expected),
              "safe alternate survives explicit Global copy and source destruction");
        check(safe.memory_resource() == g && safe.params.get_allocator().resource() == g &&
                  safe.comment->get_allocator().resource() == g &&
                  safe.trace_path[0].comment.get_allocator().resource() == g,
              "all projected nested data belongs to Global");
        // A valid saved projection must not suppress intent; this detects broken
        // cross-domain fingerprints that would take the conservative fallback.
        check(safe.purpose != "[REDACTED]" && safe.result_summary == "1 row affected",
              "valid safe alternative preserves scrubbed intent and count facts");
    }
    check(assigned_buffer.entries[0].memory_resource() == g &&
              assigned_buffer.entries[0].user_identifier->get_allocator().resource() == g,
          "buffer cross-domain move assignment preserves destination arena");
    check(copied_buffer.entries[0].user_identifier->get_allocator().resource() == g &&
              copied_buffer.entries[0].trace_chain[0].comment.get_allocator().resource() == g,
          "unified buffer deep copy retains no User strings");
    retained.comment.emplace("tampered private comment", g);
    auto invalid = detail::sql_entry_projection(g, retained, false);
    check(invalid.comment == "[REDACTED]" && invalid.purpose == "[REDACTED]" &&
              invalid.trace_path[0].entity_type == "[REDACTED]",
          "fingerprint invalidation hides intent and trace");
    check(invalid.result_summary == "1 row affected",
          "recognized count summary survives fingerprint invalidation");
    auto again = detail::sql_entry_projection(g, expected, true);
    check(again.log_context.mode == "safe" && again.params == expected.params,
          "safe logs cannot be upgraded by a later plaintext request");
    check(retained.parameter_count() == 2 && invalid.parameter_count() == 2,
          "parameter slots remain stable through projection");
}

static void protection() {
    GlobalContext global;
    auto r = global.memory_resource();
    auto e = sample(r);
    auto before = detail::debug_sql_log(r, e);
    e.log_context.parameter_policies[0] = SqlParameterLogPolicy::Unknown;
    auto unknown = detail::sql_entry_projection(r, e, true);
    check(std::get<String>(unknown.params[0].data) == "[REDACTED]",
          "plaintext does not grant permission to unknown bindings");
    e.log_context.parameter_policies[0] = SqlParameterLogPolicy::Plain;
    Record nested(r);
    nested.emplace("authorization", Value(r, "Bearer long private credential"));
    e.params[0] = Value(r, nested);
    auto credential = detail::sql_entry_projection(r, e, true);
    check(std::get<String>(credential.params[0].data) == "[REDACTED]",
          "nested credentials override compiler Plain provenance");
    e = sample(r);
    e.log_context.parameter_policies = {SqlParameterLogPolicy::Plain};
    auto failed = detail::sql_entry_projection(r, e, false);
    check(failed.params.size() == 2 && failed.params[0].is_null() && failed.params[1].is_null() &&
              failed.sql == "[REDACTED SQL; NOT REPLAYABLE]",
          "failed rendering revokes all bindings");
    check(failed.log_context.omission_reason == "parameter policy count mismatch" &&
              failed.comment == "[REDACTED]",
          "render errors retain bounded reason and scrub intent");
    e = sample(r);
    e.log_context.intent_redactions.capture_target_id(e.params[1]);
    e.comment.emplace("target 1001", r);
    auto id = detail::sql_entry_projection(r, e, true);
    check(id.comment == "target [REDACTED]" && id.log_context.intent_redactions.is_empty(),
          "target ID provenance is consumed before delivery");
    e = sample(r);
    check(detail::debug_sql_log(r, e) == before,
          "fixture source was not changed through earlier projections");
    auto p = detail::sql_entry_projection(r, e, true);
    p.log_context.projection_state = SqlProjectionState(r);
    auto lost = detail::sql_entry_projection(r, p, false);
    check(lost.comment == "[REDACTED]", "lost private state cannot permit stale plaintext intent");
}

static void models() {
    static_assert(!std::same_as<SqlLogEntry, LegacySqlLogEntry>);
    for (auto op : {SqlLogOperation::Select,
                    SqlLogOperation::Insert,
                    SqlLogOperation::Update,
                    SqlLogOperation::Delete,
                    SqlLogOperation::Recover}) {
        check(SqlLogOptions::all().enabled_for(op) && !SqlLogOptions::disabled().enabled_for(op),
              "all and disabled options");
        check(SqlLogOptions::select_only().enabled_for(op) == is_select(op) &&
                  SqlLogOptions::mutation_only().enabled_for(op) == is_mutation(op),
              "select and mutation options remain distinct");
    }
    auto duration = LogDuration(1, 4294967295u);
    check(duration.seconds == 5 && duration.nanos == 294967295,
          "Duration normalizes nanosecond carry");
    bool overflow = false;
    try {
        LogDuration(std::numeric_limits<std::uint64_t>::max(), 1000000000);
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    check(overflow, "Duration rejects seconds overflow");
    // SHA is private machinery reused by V1 business IDs and safe-log integrity.
    detail::Sha256 sha;
    constexpr std::uint8_t abc[]{'a', 'b', 'c'};
    sha.update(abc);
    constexpr std::uint8_t digest[]{0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
                                    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
                                    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
                                    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
    auto actual = sha.finish();
    check(std::equal(actual.begin(), actual.end(), digest), "SHA256 abc known answer");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        lifetime();
        protection();
        models();
        std::pmr::set_default_resource(previous);
        std::cout << "Runtime log contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
