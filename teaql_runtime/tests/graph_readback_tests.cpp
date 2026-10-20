#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/module.hpp>
#include <iostream>
using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct ReadbackExecutor final : QueryExecutor<String>, MutationExecutor<String> {
    bool fail;
    Vector<CompactRow> rows;

    explicit ReadbackExecutor(Memory r, bool fail) : fail(fail), rows(r) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    SqlIntentRedactions query_log_intent(Memory r, const SelectQuery&) const override {
        return SqlIntentRedactions(r);
    }

    struct Work {
        using allocator_type = Allocator;
        Memory r;
        ReadbackExecutor& executor;
        QueryRequest request;
        std::optional<ExecutionObserver> observer;

        Work(std::allocator_arg_t,
             Allocator a,
             ReadbackExecutor& executor,
             QueryRequest request,
             std::optional<ExecutionObserver> observer)
            : r(a.resource()), executor(executor),
              request(std::allocator_arg, a, std::move(request)),
              observer(detail::move_in(r, std::move(observer))) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal&) {
            check(request.comment() == "save fixture 7123", "original mutation reason");
            check(request.query.predicate &&
                      *request.query.predicate == Expr::eq(r, "record_key", std::uint64_t(7123)),
                  "runtime ID predicate");
            auto metadata = ExecutionMetadata::unrecorded_query(r, 0);
            metadata.backend = "readback-spy";
            metadata.parameterized_query.emplace(
                "SELECT record_key FROM fixture WHERE record_key = ?", r);
            metadata.params.emplace_back(std::uint64_t(7123));
            metadata.trace_chain = request.execution_trace_chain();
            metadata.comment.emplace(request.comment(), r);
            metadata.sql_log.generated_sql = true;
            metadata.sql_log.database_kind.emplace("sqlite", r);
            metadata.sql_log.parameter_policies.push_back(SqlParameterLogPolicy::Plain);
            using R = Result<QueryResult, String>;
            if (executor.fail) {
                if (observer) {
                    (*observer)(std::move(metadata));
                }
                return R::err(r, String("simulated readback failure", r));
            }
            QueryResult result(std::allocator_arg, Allocator(r), std::move(metadata));
            for (const auto& row : executor.rows) {
                result.rows.emplace_back(row);
            }
            return R::ok(r, std::move(result));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        return query_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<QueryResult, String>& query_observed(
        Memory r, QueryRequest request, std::optional<ExecutionObserver> observer) override {
        return make_operation<Result<QueryResult, String>, Work>(
            r, *this, std::move(request), std::move(observer));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("readback must not write");
    }
};

static void install(UserContext& user_context, bool full) {
    auto r = user_context.memory_resource();
    auto& module = user_context.make<RuntimeModule>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Fixture");
    descriptor.property("record_key", DataType::U64).id();
    if (full) {
        descriptor.property("revision", DataType::I64).version();
        descriptor.property("note", DataType::Text).column("physical_note");
    }
    module.descriptor(std::move(descriptor)).apply_to(user_context);
}

static void logging_contracts() {
    for (bool logging : {true, false}) {
        for (bool fail : {false, true}) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            install(*user_context, false);
            if (!logging) {
                user_context->disable_sql_log();
            }
            auto& executor = user_context->make_resource<ReadbackExecutor>(r, fail);
            auto& parent = EntityDataService<ReadbackExecutor>::for_executor(
                *user_context, "Fixture", *user_context, executor);
            check(bool(Access::bind_mutation_intent(parent, "save fixture 7123")), "bind reason");
            auto& result = Access::fetch_graph_current_row_internal(
                               parent, "Fixture", "record_key", Value(r, std::uint64_t(7123)), {})
                               .wait();
            check(result.is_err() == fail, "original failure outcome");
            auto logs = user_context->sql_logs();
            check(logs.size() == std::size_t(logging), "exact logging count");
            if (logging) {
                check(logs[0].comment == "save fixture [REDACTED]", "target hidden in intent");
                check(logs[0].params.size() == 1 &&
                          logs[0].params[0] == Value(r, std::uint64_t(7123)),
                      "ID binding remains plain");
            }
            check(!Access::has_intent(parent), "readback privacy does not escape to parent");
            SelectQuery query(std::allocator_arg, Allocator(r), "Fixture");
            auto intent = Access::request_intent(parent, query);
            check(intent && intent.value().comment() == "save fixture 7123",
                  "parent reason retained");
        }
    }
}

static void mapped_column_contracts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    install(*user_context, true);
    for (std::string_view missing : {"", "record_key", "revision", "note"}) {
        auto& executor = user_context->make_resource<ReadbackExecutor>(r, false);
        Record values(r);
        if (missing != "record_key") {
            values.emplace("record_key", Value(r, std::uint64_t(7123)));
        }
        if (missing != "revision") {
            values.emplace("revision", Value(r, std::int64_t(1)));
        }
        if (missing != "note") {
            values.emplace("note", Value(r));
        }
        executor.rows.emplace_back(values);
        auto& service = EntityDataService<ReadbackExecutor>::for_executor(
            *user_context, "Fixture", *user_context, executor);
        check(bool(Access::bind_mutation_intent(service, "save fixture 7123")), "bind reason");
        auto& result = Access::fetch_graph_current_row_internal(
                           service, "Fixture", "record_key", Value(r, std::uint64_t(7123)), {})
                           .wait();
        if (missing.empty()) {
            check(result && result.value() && result.value()->get("note") &&
                      *result.value()->get("note") == Value(r),
                  "loaded NULL retained");
        } else {
            String expected("missing mapped field Fixture.", r);
            expected += missing;
            check(!result && result.error().to_string(r).find(expected) != String::npos,
                  "missing mapped logical column rejected");
        }
    }
}

int main() {
    try {
        logging_contracts();
        mapped_column_contracts();
        std::cout << "READBACK=4 logging outcomes/4 mapped-column outcomes passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
