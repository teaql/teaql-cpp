#include <teaql/data_service/model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class T>
static auto retain(UserContext& user, const T& value) {
    auto expected =
        user.make_global<String>(debug_string(user.memory_resource(), value, {.pretty = true}));
    return std::pair{user.make_global<T>(value), expected};
}

template <class T>
static const T& verify(UserContext& user, const std::pair<Ref<T>, Ref<String>>& saved) {
    const auto& value = user.get(user.rebind_global(saved.first));
    check(debug_string(user.memory_resource(), value, {.pretty = true}) ==
              user.get(user.rebind_global(saved.second)),
          "retained complete model diagnostics survive actual source User deletion");
    return value;
}

static void defaults_and_time() {
    GlobalContext global;
    auto owner = global.memory_resource();
    auto metadata = ExecutionMetadata::unrecorded_query(owner, 0);
    check(metadata.operation == DataServiceOperation::Query && metadata.result_count == 0 &&
              metadata.backend.empty() && metadata.statements.empty() && metadata.params.empty() &&
              metadata.trace_chain.empty() && !metadata.affected_rows && !metadata.comment &&
              !metadata.parameterized_query && !metadata.backend_request_id &&
              !metadata.debug_query && metadata.started_at == ExecutionMetadata::SystemTime{} &&
              metadata.ended_at == ExecutionMetadata::SystemTime{} &&
              !metadata.sql_log.generated_sql,
          "unrecorded metadata preserves all source defaults without inventing execution facts");
    auto before = ExecutionMetadata::SystemTime(std::chrono::nanoseconds(-1));
    check(debug_string(owner, before) == "SystemTime { tv_sec: -1, tv_nsec: 999999999 }",
          "negative fractional system times use floored seconds and positive nanoseconds");
    check(debug_string(owner, DataServiceOperation::Schema) == "Schema" &&
              debug_string(owner, SchemaResult{false}) == "SchemaResult { changed: false }",
          "unit operation and schema model diagnostics preserve their source type roles");
}

static void ownership_and_borrows() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto expired = user->make_ref<ExecutionMetadata>(DataServiceOperation::Batch);
    std::pair<Ref<ExecutionMetadata>, Ref<String>> saved_metadata;
    std::pair<Ref<QueryRequest>, Ref<String>> saved_request;
    std::pair<Ref<MutationRequest>, Ref<String>> saved_mutation;
    std::pair<Ref<GuardedMutationRequest>, Ref<String>> saved_guarded;
    std::pair<Ref<QueryResult>, Ref<String>> saved_result;
    std::pair<Ref<MutationResult>, Ref<String>> saved_mutation_result;
    std::pair<Ref<StreamChunk>, Ref<String>> saved_chunk;
    std::pair<Ref<SchemaRequest>, Ref<String>> saved_schema;
    {
        auto& metadata = user->get(expired);
        metadata.backend = String(8192, 'b', owner);
        metadata.started_at = ExecutionMetadata::SystemTime(std::chrono::nanoseconds(-1));
        metadata.ended_at = ExecutionMetadata::SystemTime(std::chrono::nanoseconds(1000000001));
        metadata.params.emplace_back(String(8192, 'p', owner));
        metadata.comment.emplace(String(8192, 'c', owner), owner);
        metadata.statements.emplace_back(DataServiceOperation::Update);
        metadata.statements.back().params.emplace_back(String(8192, 'n', owner));
        metadata.sql_log.projection_state =
            SqlProjectionState::make<String>(owner, String(8192, 's', owner));
        metadata.sql_log.intent_redactions.capture_target_id(Value(owner, "hidden intent"));
        auto& layout = user->make<ColumnLayout>(std::initializer_list<std::string_view>{"payload"});
        Vector<Value> values(owner);
        values.emplace_back(String(8192, 'v', owner));
        auto& row = user->make<CompactRow>(layout, std::move(values));
        auto& query = user->make<SelectQuery>("Device");
        query.filter(Expr::eq(owner, "payload", String(8192, 'q', owner)));
        auto intent = QueryIntent::create(owner, "request-owned comment", String(8192, 'p', owner));
        auto& request = user->make<QueryRequest>(query, intent.value(), true, false);
        auto& insert = user->make<InsertCommand>("Device");
        insert.value("payload", String(8192, 'm', owner)).audit_as("mutation reason");
        auto& mutation = user->make<MutationRequest>(insert, "mutation reason");
        auto& guarded =
            user->make<GuardedMutationRequest>(mutation, Expr::eq(owner, "owner", UINT64_C(7)));
        auto& result = user->make<QueryResult>(metadata);
        result.rows.emplace_back(row);
        auto& mutation_result = user->make<MutationResult>(UINT64_MAX, metadata);
        mutation_result.generated_values.fields.emplace(String("id", owner),
                                                        Value(owner, UINT64_MAX));
        mutation_result.persisted_snapshot.emplace(owner);
        mutation_result.persisted_snapshot->fields.emplace(String("payload", owner),
                                                           Value(owner, String(8192, 'o', owner)));
        auto& chunk = user->make<StreamChunk>(0, true);
        chunk.rows.emplace_back(row);
        auto& schema = user->make<SchemaRequest>(String(8192, 'e', owner));
        saved_metadata = retain(*user, metadata);
        saved_request = retain(*user, request);
        saved_mutation = retain(*user, mutation);
        saved_guarded = retain(*user, guarded);
        saved_result = retain(*user, result);
        saved_mutation_result = retain(*user, mutation_result);
        saved_chunk = retain(*user, chunk);
        saved_schema = retain(*user, schema);
    }
    user->close();
    bool rejected = false;
    try {
        (void)user->get(expired);
    } catch (const ReferenceError& error) {
        rejected = error.kind() == ReferenceError::Kind::Closed;
    }
    check(rejected, "execution metadata references expire at owning Context close");
    user.reset();
    check(global.live_user_bytes() == 0, "every source User block is returned");
    auto next = global.make_user_context();
    auto destination = global.memory_resource();
    const auto& copied_metadata = verify(*next, saved_metadata);
    const auto& copied_request = verify(*next, saved_request);
    const auto& copied_mutation = verify(*next, saved_mutation);
    const auto& copied_guarded = verify(*next, saved_guarded);
    const auto& copied_result = verify(*next, saved_result);
    const auto& copied_mutation_result = verify(*next, saved_mutation_result);
    const auto& copied_chunk = verify(*next, saved_chunk);
    const auto& copied_schema = verify(*next, saved_schema);
    check(
        copied_metadata.params[0].memory_resource() == destination &&
            copied_metadata.statements[0].params[0].memory_resource() == destination &&
            copied_metadata.sql_log.projection_state.get<String>()->get_allocator().resource() ==
                destination &&
            copied_request.intent.memory_resource() == destination &&
            copied_request.purpose().size() == 8192 &&
            copied_request.query.memory_resource() == destination &&
            copied_mutation.memory_resource() == destination &&
            copied_guarded.guard.memory_resource() == destination &&
            copied_result.rows[0].memory_resource() == destination &&
            copied_chunk.rows[0].memory_resource() == destination &&
            copied_mutation_result.persisted_snapshot->memory_resource() == destination &&
            copied_schema.memory_resource() == destination,
        "nested queries, rows, mutations, snapshots and private state retain only the destination domain");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        defaults_and_time();
        ownership_and_borrows();
        std::pmr::set_default_resource(previous);
        std::cout << "Data-service model contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
