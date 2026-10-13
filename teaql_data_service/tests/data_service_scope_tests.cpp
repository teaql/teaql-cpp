#include <teaql/data_service.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void retained_model_diagnostics() {
    GlobalContext global;
    Ref<ExecutionMetadata> retained;
    Ref<String> expected;
    Ref<QueryRequest> query_copy;
    Ref<String> query_expected;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto source = user->make_ref<ExecutionMetadata>(DataServiceOperation::Batch);
        auto& metadata = user->get(source);
        metadata.backend.assign(2048, 'b');
        metadata.started_at = ExecutionMetadata::SystemTime::min();
        metadata.ended_at = ExecutionMetadata::SystemTime::max();
        metadata.affected_rows = UINT64_MAX;
        metadata.result_count = UINT32_MAX;
        metadata.comment.emplace(2048, 'c', owner);
        metadata.backend_request_id.emplace(2048, 'i', owner);
        metadata.parameterized_query.emplace("SELECT ?", owner);
        metadata.debug_query.emplace("raw diagnostic", owner);
        metadata.params.emplace_back("raw binding");
        metadata.statements.emplace_back(DataServiceOperation::Insert);
        metadata.statements.back().params.emplace_back("nested binding");
        metadata.sql_log.projection_state =
            SqlProjectionState::make<String>(owner, "PRIVATE-PROJECTION-PAYLOAD");
        metadata.sql_log.intent_redactions.capture_target_id(Value(owner, "PRIVATE-INTENT"));
        auto diagnostic = debug_string(owner, metadata);
        check(diagnostic.find("PRIVATE-PROJECTION-PAYLOAD") == String::npos &&
                  diagnostic.find("PRIVATE-INTENT") == String::npos &&
                  diagnostic.find("raw binding") != String::npos,
              "derived raw metadata exposes bindings and preserves private log placeholders");
        retained = user->make_global<ExecutionMetadata>(metadata);
        expected = user->make_global<String>(diagnostic);
        auto& query = user->make<SelectQuery>("Edge");
        query.comment("query comment remains distinct");
        auto intent = QueryIntent::create(owner, "request comment", "request purpose");
        auto& request = user->make<QueryRequest>(query, intent.value(), true, false);
        request.trace_chain.emplace_back("Edge", "trace comment");
        query_copy = user->make_global<QueryRequest>(request);
        query_expected = user->make_global<String>(debug_string(owner, request));
    }
    check(global.live_user_bytes() == 0, "actual User deletion returns every source arena block");
    auto next = global.make_user_context();
    const auto& metadata = next->get(next->rebind_global(retained));
    const auto& reference = next->get(next->rebind_global(expected));
    check(debug_string(next->memory_resource(), metadata) == reference &&
              metadata.backend.get_allocator().resource() == global.memory_resource() &&
              metadata.statements.front().params.front().memory_resource() ==
                  global.memory_resource(),
          "all retained metadata and nested bindings use Global after source User deletion");
    const auto& request = next->get(next->rebind_global(query_copy));
    String formatted(next->memory_resource());
    DebugWriter writer(formatted, {});
    data_service::write_debug(writer, request);
    check(formatted == next->get(next->rebind_global(query_expected)) &&
              request.comment() == "request comment" &&
              request.query.comment_text == "query comment remains distinct",
          "qualified data-service facade retains request/query/trace responsibilities");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        retained_model_diagnostics();
        std::pmr::set_default_resource(previous);
        std::cout << "Data-service scoped diagnostics passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
