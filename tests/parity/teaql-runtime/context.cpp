// Original Context root/log/ledger tests and the shared-graph factory contract.
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/log_formatter.hpp>
#include <array>
#include <cstdlib>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void active_root_is_typed_and_fails_closed() {
    GlobalContext global;
    auto user = global.make_user_context();
    user->with_active_root("Tenant", 42);
    auto root = user->require_active_root("Tenant");
    check(root && root.value().get().id == 42, "original typed Tenant root ID");
    auto mismatch = user->require_active_root("Organization");
    check(!mismatch, "original wrong root type fails closed");
    auto empty = global.make_user_context();
    auto missing = empty->require_active_root("Tenant");
    check(!missing, "original absent root fails closed");
    check(mismatch.error().to_string(user->memory_resource()) ==
                  "active root type is tenant, expected organization" &&
              missing.error().to_string(empty->memory_resource()) ==
                  "active root tenant is missing from UserContext",
          "complete public root errors retain actual and expected types");
}

static void diagnostic_sql_log_is_enabled_by_default_with_independent_switches(UserContext& user) {
    check(user.sql_log_options() == SqlLogOptions::all() && user.sql_logs().empty(),
          "original default options/history");
    user.disable_select_sql_log();
    check(user.sql_log_options() == SqlLogOptions::mutation_only(),
          "original independent select disable");
    user.enable_select_sql_log();
    user.disable_mutation_sql_log();
    check(user.sql_log_options() == SqlLogOptions::select_only(),
          "original independent mutation disable");
    user.disable_sql_log();
    check(user.sql_log_options() == SqlLogOptions::disabled(), "original all-log disable");
}

static ExecutionMetadata
metadata(Memory owner, DataServiceOperation operation, std::string_view sql) {
    ExecutionMetadata result(std::allocator_arg, Allocator(owner), operation);
    result.backend = "sqlite";
    result.started_at = std::chrono::system_clock::now();
    result.ended_at = result.started_at;
    result.parameterized_query.emplace(sql, owner);
    return result;
}

static void disabled_sql_log_rejects_executor_metadata_before_recording(UserContext& user) {
    user.disable_sql_log();
    auto owner = user.memory_resource();
    auto input =
        metadata(owner, DataServiceOperation::Query, "SELECT id FROM sample WHERE id = $1");
    input.backend = "sql";
    input.result_count = 1;
    input.comment.emplace("disabled log test", owner);
    input.params.emplace_back(std::int64_t(1));
    input.debug_query.emplace("SELECT id FROM sample WHERE id = 1", owner);
    user.record_metadata_log(input);
    check(user.sql_logs().empty(), "original disabled logging rejects complete executor metadata");
}

static SqlLogEntry
default_sql_telemetry_retains_expanded_masked_sql_not_raw_values(UserContext& user) {
    auto owner = user.memory_resource();
    auto& buffer = user.make<UnifiedLogBuffer>();
    user.insert_resource(user, buffer);
    auto input =
        metadata(owner, DataServiceOperation::Query, "SELECT id FROM account WHERE secret = ?");
    input.result_count = 1;
    input.comment.emplace("what: load an account", owner);
    input.params.emplace_back("private-value-0123");
    input.debug_query.emplace("SELECT id FROM account WHERE secret = 'private-value-0123'", owner);
    user.record_metadata_log(input);
    auto entries = user.sql_logs();
    check(entries.size() == 1, "original safe SQL entry exists");
    auto& entry = entries[0];
    check(entry.sql == "SELECT id FROM account WHERE secret = ?" && entry.result_count == 1 &&
              entry.parameter_count() == 1 && entry.params.size() == 1 &&
              entry.params[0] == Value(owner, "[REDACTED]") &&
              entry.debug_sql.find("secret = '[REDACTED]' /* masked */") != String::npos &&
              entry.pretty_sql == entry.debug_sql,
          "all original safe SQL/parameter/expanded-form assertions");
    check(buffer.entries.size() == 1 &&
              std::holds_alternative<SqlLogEntry>(buffer.entries[0].payload) &&
              std::get<SqlLogEntry>(buffer.entries[0].payload) == entry,
          "original unified buffer retains the same complete projected SQL entry");
    HumanReaderFormatter formatter;
    auto output = formatter.format_sql_log(owner, entry.trace_path, entry);
    check(output.find("private-value-0123") == String::npos &&
              output.find("Debug SQL:") == String::npos &&
              output.find("secret = ?") == String::npos &&
              output.find("/* masked */") != String::npos &&
              output.find("NOT REPLAYABLE") != String::npos,
          "all original ordinary formatter assertions");
    return std::move(entry);
}

static void trace_kinds(const SqlLogEntry& entry, std::span<const TraceKind> expected) {
    check(entry.trace_path.size() == expected.size(), "complete original trace-path length");
    for (std::size_t index = 0; index < expected.size(); ++index) {
        check(entry.trace_path[index].kind == expected[index],
              "every original trace kind in order");
    }
}

static void
metadata_log_retains_structured_intent_sql_forms_and_multilevel_trace(UserContext& user) {
    auto owner = user.memory_resource();
    auto input =
        metadata(owner, DataServiceOperation::Query, "SELECT name FROM school_data WHERE id = ?");
    input.result_count = 2;
    input.params.emplace_back(std::int64_t(7));
    input.debug_query.emplace("SELECT name FROM school_data WHERE id = 7", owner);
    input.trace_chain.emplace_back(TraceKind::Comment, "School", "what: load school graph");
    input.trace_chain.emplace_back(TraceKind::Purpose, "School", "why: render school details");
    input.trace_chain.emplace_back(TraceKind::Relation, "platform", "School.platform");
    input.trace_chain.emplace_back(TraceKind::Relation, "organization", "Platform.organization");
    input.trace_chain.emplace_back(TraceKind::Relation, "region", "Organization.region");
    user.record_metadata_log(input);
    auto entries = user.sql_logs();
    check(entries.size() == 1, "original traced query entry exists");
    const auto& query = entries[0];
    check(query.comment == "what: load school graph" &&
              query.purpose == "why: render school details" && !query.audit_reason,
          "original structured query intent");
    constexpr std::array query_kinds{TraceKind::Operation,
                                     TraceKind::Request,
                                     TraceKind::Relation,
                                     TraceKind::Relation,
                                     TraceKind::Relation,
                                     TraceKind::Provider,
                                     TraceKind::Sql};
    trace_kinds(query, query_kinds);
    check(query.trace_path[0].entity_type == "School" &&
              query.trace_path[5].entity_type == "sqlite" &&
              query.trace_path[6].entity_type == "select" &&
              query.sql == "SELECT name FROM school_data WHERE id = ?" &&
              query.parameter_count() == 1 && query.params.size() == 1 &&
              query.params[0] == Value(owner, "[REDACTED]") &&
              query.debug_sql.find("id = '[REDACTED]' /* masked */") != String::npos &&
              query.result_count == 2,
          "all original traced query identity/binding/form/count assertions");
    auto mutation_input = metadata(
        owner, DataServiceOperation::Update, "UPDATE school_data SET name = ? WHERE id = ?");
    mutation_input.started_at = input.started_at;
    mutation_input.ended_at = input.ended_at;
    mutation_input.affected_rows = 1;
    mutation_input.params.emplace_back("Academy");
    mutation_input.params.emplace_back(std::int64_t(7));
    mutation_input.debug_query.emplace("UPDATE school_data SET name = 'Academy' WHERE id = 7",
                                       owner);
    mutation_input.trace_chain.emplace_back(
        TraceKind::AuditReason, "School", "correct school name", 7);
    user.record_metadata_log(mutation_input);
    entries = user.sql_logs();
    check(entries.size() == 2, "original subsequent mutation appends its own log");
    const auto& mutation = entries[1];
    check(mutation.parameter_count() == 2 && mutation.params.size() == 2 &&
              mutation.params[0] == Value(owner, "[REDACTED]") &&
              mutation.params[1] == Value(owner, "[REDACTED]") && !mutation.comment &&
              !mutation.purpose && mutation.audit_reason == "correct school name",
          "original mutation binding and intent assertions");
    constexpr std::array mutation_kinds{
        TraceKind::Operation, TraceKind::Entity, TraceKind::Provider, TraceKind::Sql};
    trace_kinds(mutation, mutation_kinds);
    check(mutation.affected_rows == 1 &&
              mutation.debug_sql.find(
                  "name = '[REDACTED]' /* masked */ WHERE id = '[REDACTED]' /* masked */") !=
                  String::npos &&
              mutation.debug_sql.find("Academy") == String::npos,
          "original mutation affected count and private expanded SQL");
}

static EntityRuntimeState
reused_user_context_returns_independent_mutation_ledgers(UserContext& user) {
    auto owner = user.memory_resource();
    auto first = user.entity_runtime_state();
    EntityKey key(std::allocator_arg, Allocator(owner), "School", Value(owner, std::uint64_t(1)));
    first.set(key, "name", Value(owner, "First"));
    auto second = user.entity_runtime_state();
    check(first.changed_field_names(key).size() == 1 && second.changed_field_names(key).empty(),
          "original repeated Context factory call returns independent mutation ledgers");
    // The source factory shares only the immutable graph anchor, not the first ledger.
    EntityGraphBuilder graph(std::allocator_arg, Allocator(owner));
    graph.install(1, String("immutable School graph payload", owner));
    check(first.freeze_graph(std::move(graph)).is_ok(), "first state can publish its shared graph");
    auto third = user.entity_runtime_state();
    auto* shared = third.resolve_entity<String>(1);
    check(shared && *shared == "immutable School graph payload" &&
              second.resolve_entity<String>(1) == shared && third.changed_field_names(key).empty(),
          "all Context operation states share the graph while retaining fresh ledgers");
    return first;
}

static void factory_exhaustion_preserves_existing_objects() {
    ContextLimits limits;
    limits.user.max_objects = 3;
    GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
    auto user = global.make_user_context();
    auto existing = user->make_ref<int>(7);
    bool exhausted = false;
    try {
        (void)user->entity_runtime_state();
    } catch (const MemoryLimitError& error) {
        exhausted = error.kind() == MemoryLimitError::Kind::Objects;
    }
    check(exhausted && user->get(existing) == 7,
          "factory exhaustion preserves previously published object references");
    user.reset();
    check(global.live_user_bytes() == 0,
          "partially constructed factory state cannot prevent complete User reclamation");
}

static void original_contracts() {
    active_root_is_typed_and_fails_closed();
    factory_exhaustion_preserves_existing_objects();
    GlobalContext global;
    {
        auto user = global.make_user_context();
        diagnostic_sql_log_is_enabled_by_default_with_independent_switches(*user);
        disabled_sql_log_rejects_executor_metadata_before_recording(*user);
    }
    Ref<SqlLogEntry> retained_log;
    Ref<EntityRuntimeState> retained_state;
    {
        auto user = global.make_user_context();
        auto entry = default_sql_telemetry_retains_expanded_masked_sql_not_raw_values(*user);
        retained_log = user->make_global<SqlLogEntry>(entry);
        auto state = reused_user_context_returns_independent_mutation_ledgers(*user);
        retained_state = user->make_global<EntityRuntimeState>(state);
    }
    {
        auto user = global.make_user_context();
        metadata_log_retains_structured_intent_sql_forms_and_multilevel_trace(*user);
        user->close();
        bool closed = false;
        try {
            (void)user->entity_runtime_state();
        } catch (const ReferenceError& error) {
            closed = error.kind() == ReferenceError::Kind::Closed;
        }
        check(closed, "the new public factory rejects closed Contexts before touching the anchor");
    }
    check(global.live_user_bytes() == 0,
          "actual source User deletions return all log/anchor/ledger blocks");
    auto next = global.make_user_context();
    const auto& entry = next->get(next->rebind_global(retained_log));
    const auto& state = next->get(next->rebind_global(retained_state));
    EntityKey key(std::allocator_arg,
                  Allocator(next->memory_resource()),
                  "School",
                  Value(next->memory_resource(), std::uint64_t(1)));
    check(entry.params.size() == 1 &&
              entry.params[0] == Value(next->memory_resource(), "[REDACTED]") &&
              state.changed_field_names(key).size() == 1 && state.resolve_entity<String>(1) &&
              *state.resolve_entity<String>(1) == "immutable School graph payload",
          "explicit Global log and graph/ledger copies retain complete data after User deletion");
}

int main() {
    setenv("TEAQL_LOG_ENDPOINT", "off", 1);
    unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        original_contracts();
        std::pmr::set_default_resource(previous);
        std::cout << "Original six Context contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
