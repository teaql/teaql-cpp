#include <teaql/data_service/model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

struct TraceView {
    std::span<const TraceNode> nodes;
};

static void write_debug(DebugWriter& writer, const TraceView& value) {
    writer.sequence(value.nodes, DefaultDebugElement{});
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    auto name = text(field(input, "text"));
    auto payload = value(owner, field(input, "value"));
    DataServiceCapabilities capabilities{bool(flags & 1),
                                         bool(flags & 2),
                                         bool(flags & 4),
                                         bool(flags & 8),
                                         bool(flags & 16),
                                         bool(flags & 32),
                                         bool(flags & 64),
                                         bool(flags & 128)};
    TraceChain traces(owner);
    if (flags & 1) {
        traces.emplace_back(TraceKind::Comment, "Device", name, UINT64_MAX);
        traces.emplace_back(TraceKind::AuditReason, "Device", "", std::nullopt);
    }
    SelectQuery query(std::allocator_arg, Allocator(owner), "Device");
    query.comment(name);
    query.filter(Expr::eq(owner, "payload", payload));
    auto* comment = std::get_if<String>(&field(input, "comment").data);
    auto intent = QueryIntent::create(
        owner, comment ? std::string_view(*comment) : name, "inspect data-service model fixture");
    if (!intent) {
        Json::Object error(owner), result(owner);
        error.emplace("code", Json(owner, intent.error().code()));
        error.emplace("field", Json(owner, intent.error().field));
        error.emplace("message", Json(owner, intent.error().to_string(owner)));
        result.emplace("intent_error", Json(owner, std::move(error)));
        return Json(owner, std::move(result));
    }
    QueryRequest request(
        std::allocator_arg, Allocator(owner), query, intent.value(), flags & 2, flags & 4);
    request.trace_chain = TraceChain(traces, owner);
    auto count = std::get<std::uint64_t>(field(input, "count").data);
    auto metadata = ExecutionMetadata::unrecorded_query(owner, count);
    Json::Object output(owner);
    output.emplace("unrecorded", formatted_debug(owner, metadata));
    metadata.operation = static_cast<DataServiceOperation>(flags % 7);
    metadata.backend = name;
    metadata.started_at = ExecutionMetadata::SystemTime(
        std::chrono::nanoseconds(number<std::int64_t>(field(input, "start"))));
    metadata.ended_at = ExecutionMetadata::SystemTime(
        std::chrono::nanoseconds(number<std::int64_t>(field(input, "end"))));
    metadata.affected_rows = UINT64_MAX;
    metadata.trace_chain = TraceChain(traces, owner);
    metadata.comment.emplace(request.comment(), owner);
    metadata.backend_request_id.emplace(name, owner);
    metadata.parameterized_query.emplace("SELECT ?", owner);
    metadata.params.emplace_back(payload);
    metadata.debug_query.emplace(name, owner);
    metadata.sql_log.generated_sql = flags & 8;
    metadata.sql_log.parameter_policies = {SqlParameterLogPolicy::Credential};
    metadata.sql_log.masked_parameters = {true, false};
    if (flags & 16) {
        auto child = ExecutionMetadata::unrecorded_query(owner, 3);
        child.backend = "child";
        child.params.emplace_back(payload);
        metadata.statements.emplace_back(std::move(child));
    }
    auto& columns =
        user.make<ColumnLayout>(std::initializer_list<std::string_view>{"payload", "id"});
    Vector<Value> row_values(owner);
    row_values.emplace_back(payload);
    row_values.emplace_back(UINT64_MAX);
    CompactRow row(std::allocator_arg, Allocator(owner), columns, std::move(row_values));
    QueryResult result(std::allocator_arg, Allocator(owner), metadata);
    result.rows.emplace_back(row);
    InsertCommand insert(std::allocator_arg, Allocator(owner), "Device");
    insert.values.fields.emplace(String("payload", owner), Value(owner, payload));
    insert.trace_chain = TraceChain(traces, owner);
    UpdateCommand update(std::allocator_arg, Allocator(owner), "Device", payload);
    update.trace_chain = TraceChain(traces, owner);
    DeleteCommand deletion(std::allocator_arg, Allocator(owner), "Device", payload);
    deletion.trace_chain = TraceChain(traces, owner);
    RecoverCommand recover(std::allocator_arg, Allocator(owner), "Device", payload, -1);
    recover.trace_chain = TraceChain(traces, owner);
    Vector<MutationRequest> requests(owner);
    requests.emplace_back(insert, "inspect data-service mutation fixture");
    requests.emplace_back(update, "inspect data-service mutation fixture");
    requests.emplace_back(deletion, "inspect data-service mutation fixture");
    requests.emplace_back(recover, "inspect data-service mutation fixture");
    MutationRequest::Batch empty(owner);
    requests.emplace_back(empty, "inspect data-service mutation fixture");
    MutationRequest::Batch nested(owner);
    nested.emplace_back(empty, "inspect data-service mutation fixture");
    requests.emplace_back(nested, "inspect data-service mutation fixture");
    GuardedMutationRequest guarded(
        std::allocator_arg, Allocator(owner), requests[0], Expr::eq(owner, "owner", UINT64_C(7)));
    MutationResult mutation_result(std::allocator_arg, Allocator(owner), UINT64_MAX, metadata);
    mutation_result.generated_values.fields.emplace(String("payload", owner),
                                                    Value(owner, payload));
    if (flags & 32) {
        mutation_result.persisted_snapshot.emplace(owner);
        mutation_result.persisted_snapshot->fields.emplace(String("payload", owner),
                                                           Value(owner, payload));
    }
    StreamChunk chunk(std::allocator_arg, Allocator(owner), count, flags & 64);
    chunk.rows.emplace_back(row);
    SchemaRequest schema(std::allocator_arg, Allocator(owner), name);
    SchemaResult schema_result{bool(flags & 128)};
    auto observe = [&](std::string_view key, const auto& item) {
        output.emplace(key, formatted_debug(owner, item));
    };
    observe("capabilities", capabilities);
    observe("default_capabilities", DataServiceCapabilities{});
    observe("request", request);
    observe("request_clone", QueryRequest(std::allocator_arg, Allocator(owner), request));
    observe("metadata", metadata);
    observe("metadata_clone", ExecutionMetadata(std::allocator_arg, Allocator(owner), metadata));
    observe("result", result);
    observe("guarded", guarded);
    observe("mutation_result", mutation_result);
    observe("chunk", chunk);
    observe("schema", schema);
    observe("schema_result", schema_result);
    Json::Array request_outputs(owner), comments(owner), trace_outputs(owner), operations(owner);
    for (const auto& mutation : requests) {
        request_outputs.emplace_back(formatted_debug(owner, mutation));
        auto comment = mutation.comment();
        comments.emplace_back(comment);
        trace_outputs.emplace_back(formatted_debug(owner, TraceView{mutation.trace_chain()}));
    }
    for (unsigned i = 0; i < 7; ++i) {
        operations.emplace_back(formatted_debug(owner, static_cast<DataServiceOperation>(i)));
    }
    output.emplace("requests", Json(owner, std::move(request_outputs)));
    output.emplace("comments", Json(owner, std::move(comments)));
    output.emplace("traces", Json(owner, std::move(trace_outputs)));
    output.emplace("operations", Json(owner, std::move(operations)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("Invalid data-service model fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
