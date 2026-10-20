#include <teaql/runtime/transaction.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;
using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
using Scope = TransactionScope<Executor>;
using Output = Result<std::uint64_t, RuntimeError>;

template <class T>
static void put_optional(Json& output, std::string_view name, const std::optional<T>& value) {
    auto r = output.memory_resource();
    detail::model_put(output, name, value ? Json(r, *value) : Json(r));
}

static Json rows_out(Memory r, std::span<const CompactRow> rows) {
    Json::Array result(r);
    for (const auto& row : rows) {
        auto item = detail::model_json(r);
        for (const auto& [name, value] : row.into_map(r)) {
            detail::model_put(item, name, detail::debug_value(r, value));
        }
        result.emplace_back(std::move(item));
    }
    return Json(r, result);
}

struct Body {
    using allocator_type = Allocator;
    Memory r;
    const Scope& scope;
    Json input;
    Json::Array& steps;
    std::size_t index = 0;
    AsyncOperation<Result<QueryResult, RuntimeError>>* read = nullptr;
    AsyncOperation<Result<MutationResult, RuntimeError>>* write = nullptr;
    WakeSignal::Forward forwarding;

    Body(std::allocator_arg_t,
         Allocator a,
         const Scope& scope,
         const Json& input,
         Json::Array& steps)
        : r(a.resource()), scope(scope), input(r, input), steps(steps) {}

    std::optional<Output> poll(WakeSignal& wake) {
        const auto& actions = std::get<Json::Array>(field(input, "actions").data);
        while (index < actions.size()) {
            auto item = detail::model_json(r);
            auto action = text(actions[index]);
            detail::model_put(item, "kind", action);
            if (action == "query") {
                if (!read) {
                    SelectQuery query(std::allocator_arg, Allocator(r), "Order");
                    query.comment_text.emplace(text(field(input, "comment")), r);
                    query.purpose_text.emplace(text(field(input, "purpose")), r);
                    auto request = QueryRequest::from_query(r, query, true, true);
                    if (!request) {
                        return fail(RuntimeError::request_intent(r, request.error()));
                    }
                    read = &scope.query(std::move(request).value());
                }
                forwarding.attach(read->wake_signal(), wake);
                auto* result = read->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return fail(result->error());
                }
                detail::model_put(item, "rows", rows_out(r, result->value().rows));
            } else {
                if (!write) {
                    UpdateCommand command(
                        std::allocator_arg, Allocator(r), "Order", std::uint64_t(7));
                    command.value("name", "changed");
                    command.expected_version(
                        detail::model_integer<std::int64_t>(field(input, "version"), "version"));
                    auto request = MutationRequest::create(
                        r, std::move(command), text(field(input, "reason")));
                    if (!request) {
                        return fail(RuntimeError::request_intent(r, request.error()));
                    }
                    write = &scope.mutate(std::move(request).value());
                }
                forwarding.attach(write->wake_signal(), wake);
                auto* result = write->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return fail(result->error());
                }
                detail::model_put(item, "affected", result->value().affected_rows);
                auto snapshot = detail::model_json(r);
                if (result->value().persisted_snapshot) {
                    for (const auto& [name, value] : result->value().persisted_snapshot->fields) {
                        detail::model_put(snapshot, name, detail::debug_value(r, value));
                    }
                    detail::model_put(item, "snapshot", snapshot);
                } else {
                    detail::model_put(item, "snapshot", Json(r));
                }
            }
            steps.emplace_back(std::move(item));
            forwarding.reset();
            read = nullptr;
            write = nullptr;
            ++index;
        }
        if (std::get<bool>(field(input, "body_fail").data)) {
            return fail(RuntimeError::event(r, "business failed"));
        }
        return Output::ok(r, 17);
    }

    Output fail(const RuntimeError& error) {
        return Output::err(r, detail::clone_in(r, error));
    }

    void finish_and_wait() noexcept {
        forwarding.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding.reset();
        if (read) {
            read->cancel();
        }
        if (write) {
            write->cancel();
        }
    }
};

static Json evaluate(Memory destination, const Json& input) {
    GlobalContext global;
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    auto r = user_context.memory_resource();
    user_context.set_user_identifier("fixture");
    user_context.set_trace_id("fixture");
    user_context.set_sql_log_options({std::get<bool>(field(input, "select_logs").data),
                                      std::get<bool>(field(input, "mutation_logs").data)});
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Order");
    descriptor.table("orders");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text);
    metadata.register_entity(std::move(descriptor));
    user_context.set_metadata(user_context, metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    auto& compiled = user_context.make<CompiledQuery>();
    for (auto sql : {"CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
                     "INSERT INTO orders VALUES(7,3,'original')"}) {
        compiled.sql = sql;
        if (!transport.execute(r, compiled)) {
            throw std::runtime_error("SQLite transaction fixture setup failed");
        }
    }
    auto& dialect = user_context.make_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<Executor>(dialect, transport, metadata, state);
    user_context.insert_resource(executor);
    Json::Array steps(r);
    const auto& result = user_context
                             .execute_in_transaction<Executor, std::uint64_t>(
                                 [&](const Scope& scope) -> AsyncOperation<Output>& {
                                     return make_operation<Output, Body>(r, scope, input, steps);
                                 })
                             .wait();
    auto output = detail::model_json(r);
    auto outcome = detail::model_json(r);
    if (result) {
        detail::model_put(outcome, "value", result.value());
    } else {
        detail::model_put(outcome, "error", result.error().to_string(r));
    }
    detail::model_put(output, "outcome", outcome);
    detail::model_put(output, "steps", steps);
    compiled.sql = "SELECT id,version,name FROM orders";
    const auto& persisted = transport.fetch_all_compact_sql(r, compiled).wait();
    if (!persisted) {
        throw std::runtime_error("SQLite transaction fixture inspection failed");
    }
    detail::model_put(output, "persisted", rows_out(r, persisted.value()));
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        auto item = detail::model_json(r);
        detail::model_put(item, "context", debug_string(r, entry.log_context));
        detail::model_put(item, "operation", debug_string(r, entry.operation));
        put_optional(item, "comment", entry.comment);
        put_optional(item, "purpose", entry.purpose);
        put_optional(item, "audit_reason", entry.audit_reason);
        Json::Array trace(r);
        for (const auto& node : entry.trace_path) {
            auto frame = detail::model_json(r);
            detail::model_put(frame, "kind", std::uint8_t(node.kind));
            detail::model_put(frame, "entity", node.entity_type);
            put_optional(frame, "id", node.entity_id);
            detail::model_put(frame, "comment", node.comment);
            trace.emplace_back(std::move(frame));
        }
        detail::model_put(item, "trace", trace);
        detail::model_put(item, "sql", entry.sql);
        Json::Array params(r);
        for (const auto& value : entry.params) {
            params.emplace_back(detail::debug_value(r, value));
        }
        detail::model_put(item, "params", params);
        detail::model_put(item, "debug_sql", entry.debug_sql);
        detail::model_put(item, "pretty_sql", entry.pretty_sql);
        put_optional(item, "result_count", entry.result_count);
        put_optional(item, "result_type", entry.result_type);
        put_optional(item, "affected_rows", entry.affected_rows);
        detail::model_put(item, "summary", entry.result_summary);
        logs.emplace_back(std::move(item));
    }
    detail::model_put(output, "logs", logs);
    return Json(destination, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        std::string line;
        while (std::getline(std::cin, line)) {
            Arena output(4096, std::pmr::new_delete_resource());
            auto input = Json::parse(&output, line);
            if (!input) {
                throw std::runtime_error("Invalid transaction fixture JSON");
            }
            std::cout << evaluate(&output, *input).dump(&output) << '\n';
        }
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
    std::pmr::set_default_resource(previous);
}
