#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "../fixture_value.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static TraceChain nodes(Memory owner, const Json& input) {
    constexpr std::string_view names[]{"Operation",
                                       "Request",
                                       "Relation",
                                       "Entity",
                                       "Provider",
                                       "Sql",
                                       "Comment",
                                       "Purpose",
                                       "AuditReason"};
    TraceChain result(owner);
    for (const auto& node : detail::model_array(input, "trace nodes")) {
        auto kind = text(field(node, "kind"));
        auto found = std::find(std::begin(names), std::end(names), kind);
        check(found != std::end(names), "unknown source fixture trace kind");
        std::optional<std::uint64_t> id;
        const auto& value = field(node, "entityId");
        if (!std::holds_alternative<std::monostate>(value.data)) {
            id = detail::model_integer<std::uint64_t>(value, "trace entity ID");
        }
        result.emplace_back(TraceKind(found - std::begin(names)),
                            text(field(node, "name")),
                            text(field(node, "detail")),
                            id);
    }
    return result;
}

static ExecutionMetadata metadata(Memory owner, DataServiceOperation operation) {
    ExecutionMetadata result(std::allocator_arg, Allocator(owner), operation);
    result.backend = "sqlite";
    result.parameterized_query.emplace("SELECT 1", owner);
    result.sql_log.generated_sql = true;
    result.sql_log.database_kind.emplace("sqlite", owner);
    return result;
}

static void matches_intent(const std::optional<String>& actual, const Json& expected) {
    if (std::holds_alternative<std::monostate>(expected.data)) {
        check(!actual, "source vector requires absent intent");
    } else {
        check(actual && *actual == text(expected), "source vector intent differs");
    }
}

static void shared_vectors(UserContext& user_context, const Json& fixture) {
    check(text(field(fixture, "contract")) == "teaql.sql-trace-path.v1",
          "unexpected source vector contract");
    const auto& cases = detail::model_array(field(fixture, "cases"), "source vectors");
    check(cases.size() == 12, "complete twelve-vector source fixture required");
    constexpr std::string_view operations[]{"select", "insert", "update", "delete", "recover"};
    constexpr DataServiceOperation kinds[]{DataServiceOperation::Query,
                                           DataServiceOperation::Insert,
                                           DataServiceOperation::Update,
                                           DataServiceOperation::Delete,
                                           DataServiceOperation::Recover};
    auto owner = user_context.memory_resource();
    std::pmr::set<String> ids(owner);
    for (const auto& item : cases) {
        check(ids.emplace(text(field(item, "id"))).second, "duplicate source vector ID");
        auto operation = text(field(item, "operation"));
        auto found = std::find(std::begin(operations), std::end(operations), operation);
        check(found != std::end(operations), "unknown source vector operation");
        auto input = metadata(owner, kinds[found - std::begin(operations)]);
        input.backend = text(field(item, "backend"));
        input.trace_chain = nodes(owner, field(item, "source"));
        auto before = detail::clone_in(owner, input.trace_chain);
        auto expected = nodes(owner, field(item, "expectedPath"));
        user_context.clear_sql_logs();
        user_context.record_metadata_log(input);
        auto logs = user_context.sql_logs();
        check(logs.size() == 1 && logs[0].trace_path == expected,
              "source vector canonical path differs through Context capture");
        const auto& intent = field(item, "expectedIntent");
        matches_intent(logs[0].comment, field(intent, "comment"));
        matches_intent(logs[0].purpose, field(intent, "purpose"));
        matches_intent(logs[0].audit_reason, field(intent, "auditReason"));
        check(input.trace_chain == before, "capture changed source trace nodes");
        input.trace_chain = detail::clone_in(owner, logs[0].trace_path);
        user_context.clear_sql_logs();
        user_context.record_metadata_log(input);
        check(user_context.sql_logs()[0].trace_path == expected,
              "source vector canonicalization is not idempotent");
    }
}

static void request_owned_reason(UserContext& user_context) {
    auto owner = user_context.memory_resource();
    TraceChain lineage(owner);
    lineage.emplace_back(TraceKind::AuditReason, "Order", "submit order", 100);
    lineage.emplace_back(TraceKind::AuditReason, "Payment", "authorize payment", 201);
    for (auto operation : {DataServiceOperation::Insert,
                           DataServiceOperation::Update,
                           DataServiceOperation::Delete,
                           DataServiceOperation::Recover,
                           DataServiceOperation::Query}) {
        auto input = metadata(owner, operation);
        input.comment.emplace("submit order", owner);
        input.trace_chain = detail::clone_in(owner, lineage);
        user_context.clear_sql_logs();
        user_context.record_metadata_log(input);
        auto logs = user_context.sql_logs();
        check(logs.size() == 1 && logs[0].audit_reason == "submit order",
              "executed request lost its root reason");
        check(input.trace_chain == lineage, "root projection changed per-entity responsibility");
        for (const auto& node : logs[0].trace_path) {
            check(node.kind != TraceKind::AuditReason, "intent escaped into the canonical path");
        }
    }
    auto independent = metadata(owner, DataServiceOperation::Query);
    independent.comment.emplace("independent query", owner);
    independent.trace_chain.emplace_back(TraceKind::Comment, "Order", "independent query");
    user_context.clear_sql_logs();
    user_context.record_metadata_log(independent);
    check(!user_context.sql_logs()[0].audit_reason, "request intent leaked through Context reuse");
}

int main(int argc, char** argv) {
    try {
        check(argc == 2, "source trace fixture path required");
        check(setenv("TEAQL_LOG_ENDPOINT", "off", 1) == 0, "disable diagnostic publication");
        check(unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS") == 0,
              "disable plaintext permission");
        std::ifstream input(argv[1]);
        check(input.good(), "source trace fixture is unavailable");
        std::string text(std::istreambuf_iterator<char>{input}, {});
        auto* prior = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto fixture = Json::parse(user_context->memory_resource(), text);
                check(bool(fixture), "source trace fixture is invalid JSON");
                shared_vectors(*user_context, *fixture);
                request_owned_reason(*user_context);
            }
            user_context.reset();
            check(global.live_user_bytes() == 0, "trace capture retained User storage");
        }
        std::pmr::set_default_resource(prior);
        std::cout << "Twelve original trace vectors and request-owned audit reason passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
