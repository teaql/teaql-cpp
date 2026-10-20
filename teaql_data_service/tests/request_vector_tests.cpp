#include <teaql/runtime/context.hpp>
#include <teaql/data_service/model_debug.hpp>
#include "../../tests/parity/fixture_value.hpp"
#include <fstream>
#include <iostream>

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static std::optional<std::string_view> option_text(const Json& input, std::string_view name) {
    const auto* item = optional(input, name);
    if (item && std::holds_alternative<String>(item->data)) {
        return text(*item);
    }
    return {};
}

static TraceChain source_trace(Memory memory, const Json& input) {
    TraceChain result(memory);
    if (const auto* trace = optional(input, "trace")) {
        for (const auto& node : std::get<Json::Array>(trace->data)) {
            auto name = text(field(node, "kind"));
            auto kind = name == "Comment"       ? TraceKind::Comment
                        : name == "Purpose"     ? TraceKind::Purpose
                        : name == "AuditReason" ? TraceKind::AuditReason
                        : name == "Entity"      ? TraceKind::Entity
                        : name == "Provider"    ? TraceKind::Provider
                        : name == "Sql"         ? TraceKind::Sql
                                                : throw std::runtime_error("Unknown fixture kind");
            result.emplace_back(kind, "Order", option_text(node, "detail").value_or(""));
        }
    }
    return result;
}

static void expected_error(const RequestIntentError& error, const Json& item) {
    const auto& expected = field(item, "error");
    check(error.code() == text(field(expected, "code")) &&
              error.field == text(field(expected, "field")),
          "Original structured request-intent error");
}

static void shared_vectors(UserContext& user_context, const Json& fixture) {
    auto memory = user_context.memory_resource();
    check(text(field(fixture, "contract")) == "teaql.request-intent.v1", "Original contract");
    const auto& cases = std::get<Json::Array>(field(fixture, "cases").data);
    check(cases.size() == 20, "All twenty original vectors");
    for (const auto& item : cases) {
        const auto& input = field(item, "input");
        if (text(field(item, "kind")) == "query") {
            SelectQuery query(std::allocator_arg, Allocator(memory), "Order");
            if (auto comment = option_text(input, "comment")) {
                query.comment(*comment);
            }
            if (auto purpose = option_text(input, "purpose")) {
                query.purpose(*purpose);
            }
            query.trace_chain = source_trace(memory, input);
            auto request = QueryRequest::from_query(memory, query);
            if (optional(item, "error")) {
                check(!request, "Invalid original query rejected");
                expected_error(request.error(), item);
            } else {
                check(bool(request), "Valid original query accepted");
                if (const auto* logging = optional(input, "logging");
                    logging && std::holds_alternative<bool>(logging->data) &&
                    !std::get<bool>(logging->data)) {
                    request.value().capture_execution_metadata = false;
                }
                const auto& expected = field(item, "expected");
                check(request.value().comment() == text(field(expected, "comment")) &&
                          request.value().purpose() == text(field(expected, "purpose")),
                      "Original owned query intent");
            }
        } else {
            check(text(field(item, "kind")) == "mutation", "Original vector kind");
            auto command = [&]() -> MutationCommand {
                if (const auto* children = optional(input, "children")) {
                    MutationCommand::Batch batch(memory);
                    for (const auto& child : std::get<Json::Array>(children->data)) {
                        InsertCommand insert(std::allocator_arg, Allocator(memory), "OrderItem");
                        auto request =
                            MutationRequest::create(memory, insert, text(field(child, "comment")));
                        check(bool(request), "Original valid child request");
                        batch.emplace_back(std::move(request).value());
                    }
                    return MutationCommand(std::allocator_arg, Allocator(memory), batch);
                }
                InsertCommand insert(std::allocator_arg, Allocator(memory), "Order");
                insert.trace_chain = source_trace(memory, input);
                return MutationCommand(std::allocator_arg, Allocator(memory), insert);
            }();
            auto intent = MutationIntent::from_optional(memory, option_text(input, "comment"));
            if (optional(item, "error")) {
                check(!intent, "Invalid original mutation rejected");
                expected_error(intent.error(), item);
            } else {
                check(bool(intent), "Valid original mutation accepted");
                MutationRequest request(
                    std::allocator_arg, Allocator(memory), command, intent.value());
                check(request.comment() == text(field(field(item, "expected"), "comment")),
                      "Original owned batch/standalone mutation reason");
            }
        }
    }
}

static void execution_traces(UserContext& user_context) {
    auto memory = user_context.memory_resource();
    TraceChain source(memory);
    source.emplace_back(TraceKind::Comment, "Order", "stale source comment");
    source.emplace_back(TraceKind::Purpose, "Order", "stale source purpose");
    source.emplace_back(TraceKind::Relation, "items", "Order.items");
    SelectQuery query(std::allocator_arg, Allocator(memory), "OrderItem");
    query.trace_chain = detail::clone_in(memory, source);
    auto intent = QueryIntent::create(memory, "load order", "render order");
    check(bool(intent), "Original query intent");
    QueryRequest request(std::allocator_arg, Allocator(memory), query, intent.value());
    request.query.comment("changed builder");
    TraceChain expected(memory);
    expected.emplace_back(TraceKind::Comment, "Order", "load order");
    expected.emplace_back(TraceKind::Purpose, "Order", "render order");
    expected.emplace_back(source[2]);
    check(request.execution_trace_chain() == expected && request.trace_chain == source,
          "Owned query projection preserves inherited route and caller trace");

    InsertCommand insert(std::allocator_arg, Allocator(memory), "Order");
    insert.value("id", std::uint64_t(100));
    auto created = MutationRequest::create(memory, insert, "submit order");
    check(bool(created) && created.value().trace_chain().empty(), "Original bare mutation trace");
    expected.clear();
    expected.emplace_back(TraceKind::AuditReason, "Order", "submit order", 100);
    check(created.value().execution_trace_chain() == expected &&
              created.value().trace_chain().empty(),
          "Owned mutation materializes root identity without mutating source");
    UpdateCommand update(std::allocator_arg, Allocator(memory), "Payment", std::uint64_t(301));
    update.trace_chain.emplace_back(expected[0]);
    update.trace_chain.emplace_back(TraceKind::AuditReason, "Payment", "authorize payment", 301);
    auto changed = MutationRequest::create(memory, update, "submit order");
    check(bool(changed) && changed.value().execution_trace_chain() == update.trace_chain,
          "Parent/child mutation lineage retains one root");
}

static void request_privacy(UserContext& user_context) {
    auto memory = user_context.memory_resource();
    SqlLogContext policy(memory);
    policy.generated_sql = true;
    policy.parameter_policies = {SqlParameterLogPolicy::Masked};
    Vector<Value> operands(memory);
    operands.emplace_back("PRIVATE-ORIGINAL");
    auto source = SqlIntentRedactions::from_bindings(memory, policy, operands, "");
    InsertCommand insert(std::allocator_arg, Allocator(memory), "CustomerOrder");
    auto request = MutationRequest::create(memory, insert, "save graph");
    check(bool(request), "Original privacy request");
    request.value().with_diagnostic_redactions(&source);
    operands[0] = Value(memory, "LATER-VALUE");
    source.extend(SqlIntentRedactions::from_bindings(memory, policy, operands, ""));
    Vector<String> values(memory);
    request.value().diagnostic_redactions()->extend_secrets(false, values);
    check(values.size() == 1 && values[0] == "PRIVATE-ORIGINAL", "Request privacy snapshot");
    check(debug_string(memory, request.value()).find("PRIVATE-ORIGINAL") == String::npos,
          "Request Debug hides private source");
    MutationCommand next_command(
        std::allocator_arg, Allocator(memory), std::get<InsertCommand>(request.value().data));
    auto next = MutationRequest::create(memory, next_command, "independent request");
    check(bool(next) && !next.value().diagnostic_redactions(), "No implicit privacy inheritance");
    values.clear();
    request.value().diagnostic_redactions()->extend_secrets(true, values);
    check(values.empty(), "Original allowed debug view excludes nonforced redactions");
}

int main(int argc, char** argv) {
    try {
        check(argc == 2, "Original fixture path required");
        std::pmr::set_default_resource(std::pmr::null_memory_resource());
        std::ifstream stream(argv[1]);
        check(stream.is_open(), "Read original vector fixture");
        std::string encoded(std::istreambuf_iterator<char>(stream), {});
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto fixture = Json::parse(user_context->memory_resource(), encoded);
        check(fixture.has_value(), "Parse original vector fixture");
        shared_vectors(*user_context, *fixture);
        execution_traces(*user_context);
        request_privacy(*user_context);
        std::cout << "Twenty request vectors, three trace contracts and privacy snapshot passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
