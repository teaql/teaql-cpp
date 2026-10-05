#include <teaql/data_service/types.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct NoDefaultAllocator {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~NoDefaultAllocator() {
        std::pmr::set_default_resource(previous);
    }
};

static bool contains(const Vector<String>& values, std::string_view text) {
    return std::any_of(values.begin(), values.end(), [&](const auto& value) {
        return value == text;
    });
}

static void intent() {
    GlobalContext global;
    auto r = global.memory_resource();
    SqlLogContext source(r);
    check(source.parameter_policies.empty() && !source.generated_sql && !source.execution_outcome,
          "default log context has no invented provenance");
    check(is_credential_log_name("ACCESS_令-TOKEN") &&
              is_credential_log_name("prefix-PaSs_WoRd-suffix") &&
              !is_credential_log_name("monkey"),
          "credential classifier ignores non-ASCII and punctuation");
    Vector<Value> params(r);
    params.emplace_back("ordinary sensitive value");
    params.emplace_back("credential value");
    source.parameter_policies = {SqlParameterLogPolicy::Masked, SqlParameterLogPolicy::Credential};
    source.generated_sql = true;
    auto redactions = SqlIntentRedactions::from_bindings(r, source, params, "SELECT ?");
    Vector<String> safe(r), debug(r);
    redactions.extend_secrets(false, safe);
    redactions.extend_secrets(true, debug);
    check(safe.size() == 2 && debug.size() == 1 && debug[0] == "credential value",
          "debug can reveal masked values but never credentials");
    source.parameter_policies = {SqlParameterLogPolicy::Plain};
    auto unknown = SqlIntentRedactions::from_bindings(r, source, params, "SELECT ?");
    debug.clear();
    unknown.extend_secrets(true, debug);
    check(debug.size() == 2, "binding-policy count mismatch invalidates all policies");
    auto& q = global.make<SelectQuery>("Root");
    auto& child = global.make<SelectQuery>("Child");
    child.filter(Expr::eq(r, "name", "SUBQUERY-SECRET"));
    auto& descriptor = global.make<EntityDescriptor>("Child");
    q.filter(Expr::eq(r, "name", "FILTER-SECRET")
                 .and_expr(Expr::in_subquery(r, "id", descriptor, child, "id")));
    q.project_expr("value", Expr::value(r, "PROJECTION-SECRET"))
        .having(Expr::eq(r, "name", "HAVING-SECRET"))
        .search_with_text("SEARCH-SECRET");
    auto& sibling = global.make<SelectQuery>("Child");
    sibling.filter(Expr::eq(r, "name", "UNRELATED-SIBLING"));
    q.relation_query("siblings", sibling);
    auto collected = SqlIntentRedactions::from_unclassified_query(r, q);
    debug.clear();
    collected.extend_secrets(true, debug);
    for (auto text : {"FILTER-SECRET",
                      "SUBQUERY-SECRET",
                      "PROJECTION-SECRET",
                      "HAVING-SECRET",
                      "SEARCH-SECRET"}) {
        check(contains(debug, text), "fallback visits statement expressions and SQL subqueries");
    }
    check(!contains(debug, "UNRELATED-SIBLING") &&
              collected.debug(r).find("SECRET") == String::npos,
          "fallback excludes separately executed relations and debug hides raw values");
    collected.capture_target_id(Value(r, std::uint64_t(12345)));
    collected.capture_target_id(Value(r, std::uint64_t(12345)));
    debug.clear();
    collected.extend_secrets(true, debug);
    check(std::count(debug.begin(), debug.end(), String("12345", r)) == 1,
          "target IDs forced and deduplicated");
    collected.extend(collected);
    collected.clear();
    check(collected.is_empty(), "clear removes execution-local intent");
    check(as_str(SqlExecutionOutcome::Cancelled) == "cancelled",
          "cancelled means statement completion");
}

static void scope_and_models() {
    GlobalContext global;
    auto r = global.memory_resource();
    auto user = global.make_user_context();
    auto u = user->memory_resource();
    auto& insert = user->make<InsertCommand>("Customer");
    insert.value("name", "a long field value living only in the user context").audit_as("create");
    auto& request = user->make<MutationRequest>(insert);
    check(request.comment() == "create" && request.trace_chain().size() == 1,
          "mutation trace uses final trace node");
    insert.trace_chain.emplace_back(TraceKind::Comment, "Customer", "");
    auto& empty = user->make<MutationRequest>(insert);
    check(empty.comment().has_value() && empty.comment()->empty(),
          "Some empty comment differs from absent");
    auto& batch = user->make<MutationRequest::Batch>();
    batch.emplace_back(request);
    batch.emplace_back(empty);
    auto& nested = user->make<MutationRequest>(batch);
    check(nested.trace_chain().empty() && !nested.comment(), "batch keeps per-item traces only");
    auto& retained = global.make<MutationRequest>(nested);
    auto& guarded =
        user->make<GuardedMutationRequest>(request, Expr::eq(u, "owner", std::uint64_t(7)));
    auto& retained_guard = global.make<GuardedMutationRequest>(guarded);
    auto& query = user->make<SelectQuery>("Customer");
    query.comment("query-owned comment");
    auto& query_request = user->make<QueryRequest>(query, true, false);
    check(!query_request.comment && query_request.trace_chain.empty() &&
              query_request.capture_debug_query && !query_request.capture_execution_metadata,
          "request metadata stays distinct from query metadata");
    auto& metadata = user->make<ExecutionMetadata>(DataServiceOperation::Batch);
    metadata.backend = "sqlite";
    metadata.statements.emplace_back(DataServiceOperation::Insert);
    metadata.statements.back().params.emplace_back("a nested binding with user lifetime");
    metadata.sql_log.database_kind.emplace("sqlite", u);
    metadata.sql_log.masked_parameters = {true, false};
    metadata.sql_log.intent_redactions.capture_target_id(
        Value(u, "an inherited target with user-context lifetime"));
    metadata.sql_log.projection_state =
        SqlProjectionState::make<String>(u, "private state with a long context string");
    SqlProjectionState same(std::allocator_arg, Allocator(u), metadata.sql_log.projection_state);
    check(same == metadata.sql_log.projection_state && same.get<String>() && !same.get<int>(),
          "same-domain opaque clone preserves identity and concrete type");
    auto& retained_metadata = global.make<ExecutionMetadata>(metadata);
    check(!(retained_metadata.sql_log.projection_state == same),
          "cross-domain opaque clone has separate identity");
    auto& result = user->make<MutationResult>(1, metadata);
    result.generated_values.fields.emplace(String("id", u), Value(u, std::uint64_t(42)));
    result.persisted_snapshot.emplace(u);
    result.persisted_snapshot->fields.emplace(
        String("name", u), Value(u, "persisted name with explicit context lifetime"));
    auto& retained_result = global.make<MutationResult>(result);
    auto& update = user->make<UpdateCommand>("Customer", std::uint64_t(42));
    update.expected_version(3).audit_as("update");
    update.old_values.emplace(u);
    update.old_values->fields.emplace(String("name", u),
                                      Value(u, "old snapshot stored only in user context"));
    auto& update_request = user->make<MutationRequest>(update);
    auto& retained_update = global.make<MutationRequest>(update_request);
    auto& deletion = user->make<DeleteCommand>("Customer", std::uint64_t(42));
    deletion.hard_delete().expected_version(4).audit_as("delete");
    auto& delete_request = user->make<MutationRequest>(deletion);
    auto& retained_delete = global.make<MutationRequest>(delete_request);
    auto& recovery = user->make<RecoverCommand>("Customer", std::uint64_t(42), -4);
    recovery.audit_as("recover");
    auto& recover_request = user->make<MutationRequest>(recovery);
    auto& retained_recover = global.make<MutationRequest>(recover_request);
    auto& query_result = user->make<QueryResult>(metadata);
    auto& retained_query_result = global.make<QueryResult>(query_result);
    auto& chunk = user->make<StreamChunk>(2, true);
    auto& retained_chunk = global.make<StreamChunk>(chunk);
    auto& schema = user->make<SchemaRequest>("Customer");
    auto& retained_schema = global.make<SchemaRequest>(schema);
    user.reset();
    check(std::get<MutationRequest::Batch>(retained.data)[0].comment() == "create",
          "recursive mutation copy survives source close");
    check(std::get<InsertCommand>(retained_guard.mutation.data)
                      .values.fields.at(String("name", r))
                      .memory_resource() == r &&
              retained_guard.guard.memory_resource() == r,
          "guard and payload clone into destination");
    check(retained_metadata.statements[0].params[0].memory_resource() == r &&
              retained_metadata.sql_log.database_kind->get_allocator().resource() == r,
          "nested execution metadata propagates allocator");
    check(*retained_metadata.sql_log.projection_state.get<String>() ==
              "private state with a long context string",
          "opaque state survives explicit cross-domain clone");
    Vector<String> inherited(r);
    retained_metadata.sql_log.intent_redactions.extend_secrets(true, inherited);
    check(inherited.size() == 1 &&
              inherited[0] == "an inherited target with user-context lifetime" &&
              inherited[0].get_allocator().resource() == r,
          "private redaction strings clone into the destination domain");
    check(retained_result.persisted_snapshot->fields.begin()->second.memory_resource() == r &&
              retained_result.generated_values.memory_resource() == r,
          "provider generated values and authoritative snapshot remain distinct scoped types");
    check(retained_update.comment() == "update" &&
              std::get<UpdateCommand>(retained_update.data).old_values->memory_resource() == r,
          "update preserves old snapshot across domains");
    check(retained_delete.comment() == "delete" &&
              !std::get<DeleteCommand>(retained_delete.data).soft_delete &&
              std::get<DeleteCommand>(retained_delete.data).expected_version_value == 4,
          "delete retains hard/soft and optimistic version intent");
    check(retained_recover.comment() == "recover" &&
              std::get<RecoverCommand>(retained_recover.data).expected_version == -4,
          "recover retains independent signed expected version");
    check(retained_query_result.metadata.backend == "sqlite" &&
              retained_query_result.rows.get_allocator().resource() == r,
          "query result retains independently scoped execution metadata");
    check(retained_chunk.chunk_index == 2 && retained_chunk.is_last &&
              retained_chunk.memory_resource() == r && retained_schema.entity_name == "Customer",
          "stream and schema models copy into destination Context");
    auto unrecorded = ExecutionMetadata::unrecorded_query(r, 4);
    check(unrecorded.backend.empty() && unrecorded.started_at == ExecutionMetadata::SystemTime{} &&
              unrecorded.ended_at == ExecutionMetadata::SystemTime{} &&
              unrecorded.result_count == 4 && unrecorded.statements.empty() &&
              unrecorded.params.empty() && !unrecorded.debug_query,
          "unrecorded query retains count only");
    check(!DataServiceCapabilities{}.query && !DataServiceCapabilities{}.returning,
          "capabilities default to unsupported");
}

int main() {
    try {
        NoDefaultAllocator guard;
        intent();
        scope_and_models();
        std::cout << "Data-service contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
