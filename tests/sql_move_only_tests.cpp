#include "move_only_error_fixture.hpp"
#include <teaql/sql/executor.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Counts {
    unsigned produced = 0, destroyed = 0, finished = 0, cancelled = 0;
};

template <class T>
struct ReadyFailure {
    using allocator_type = Allocator;
    Memory owner;
    Counts& counts;
    String label;

    ReadyFailure(std::allocator_arg_t, Allocator a, Counts& counts, std::string_view label)
        : owner(a.resource()), counts(counts), label(label, owner) {}

    std::optional<Result<T, MoveOnlyError>> poll(WakeSignal&) {
        ++counts.produced;
        return Result<T, MoveOnlyError>::err(
            owner, MoveOnlyError(std::allocator_arg, Allocator(owner), label, counts.destroyed));
    }

    void finish_and_wait() noexcept {
        ++counts.finished;
    }

    void cancel_and_wait() noexcept {
        ++counts.cancelled;
    }
};

template <class T>
auto& failed(Memory owner, Counts& counts, std::string_view label) {
    return make_operation<Result<T, MoveOnlyError>, ReadyFailure<T>>(owner, counts, label);
}

struct OneFailure {
    using allocator_type = Allocator;
    Memory owner;
    Counts& counts;
    bool emitted = false;

    OneFailure(std::allocator_arg_t, Allocator a, Counts& counts)
        : owner(a.resource()), counts(counts) {}

    std::optional<std::optional<Result<StreamChunk, MoveOnlyError>>> poll_next(WakeSignal&) {
        if (emitted) {
            return std::optional<Result<StreamChunk, MoveOnlyError>>{};
        }
        emitted = true;
        ++counts.produced;
        return std::optional<Result<StreamChunk, MoveOnlyError>>(
            Result<StreamChunk, MoveOnlyError>::err(
                owner,
                MoveOnlyError(
                    std::allocator_arg, Allocator(owner), "native stream", counts.destroyed)));
    }

    void finish_and_wait() noexcept {
        ++counts.finished;
    }

    void cancel_and_wait() noexcept {
        ++counts.cancelled;
    }
};

struct Dialect final : SqlDialect {
    DatabaseKind kind() const override {
        return DatabaseKind::Sqlite;
    }

    String quote_ident(Memory owner, std::string_view name) const override {
        return quote_identifier_if_needed(owner, name, '"');
    }

    String placeholder(Memory owner, std::size_t) const override {
        return String("?", owner);
    }
};

struct Schema final : SchemaProvider {
    const EntityDescriptor& descriptor;

    explicit Schema(const EntityDescriptor& descriptor) : descriptor(descriptor) {}

    const EntityDescriptor* get_entity(std::string_view name) const override {
        return name == "Record" ? &descriptor : nullptr;
    }
};

struct NativeTransaction final : SqlTransportTransaction<MoveOnlyError> {
    Counts& counts;

    NativeTransaction(Memory owner, Counts& counts)
        : SqlTransportTransaction(owner), counts(counts) {}

    DataServiceOperationResult<Rows, MoveOnlyError>&
    fetch_all_compact_sql_transaction(Memory owner, const CompiledQuery&) override {
        return failed<Rows>(owner, counts, "native transaction query");
    }

    DataServiceOperationResult<std::uint64_t, MoveOnlyError>&
    execute_sql_transaction(Memory owner, const CompiledQuery&) override {
        return failed<std::uint64_t>(owner, counts, "native transaction execute");
    }

    DataServiceOperationResult<std::monostate, MoveOnlyError>&
    commit_sql_transaction(Memory owner) override {
        return failed<std::monostate>(owner, counts, "native commit");
    }

    DataServiceOperationResult<std::monostate, MoveOnlyError>&
    rollback_sql_transaction(Memory owner) override {
        return failed<std::monostate>(owner, counts, "native rollback");
    }

    void cancel_sql_transaction() noexcept override {}
};

struct ReadyTransaction {
    using allocator_type = Allocator;
    Memory owner;
    NativeTransaction& native;

    ReadyTransaction(std::allocator_arg_t, Allocator a, NativeTransaction& native)
        : owner(a.resource()), native(native) {}

    std::optional<Result<SqlTransactionHandle<MoveOnlyError>, MoveOnlyError>> poll(WakeSignal&) {
        return Result<SqlTransactionHandle<MoveOnlyError>, MoveOnlyError>::ok(
            owner,
            SqlTransactionHandle<MoveOnlyError>(std::allocator_arg, Allocator(owner), native));
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {
        native.cancel_sql();
    }
};

struct Transport final : StreamingSqlTransport<MoveOnlyError>,
                         SqlTransactionTransport<MoveOnlyError> {
    Counts& counts;
    bool fail_begin = true;

    explicit Transport(Counts& counts) : counts(counts) {}

    DataServiceOperationResult<Rows, MoveOnlyError>&
    fetch_all_compact_sql(Memory owner, const CompiledQuery&) override {
        return failed<Rows>(owner, counts, "native query");
    }

    DataServiceOperationResult<std::uint64_t, MoveOnlyError>&
    execute_sql(Memory owner, const CompiledQuery&) override {
        return failed<std::uint64_t>(owner, counts, "native execute");
    }

    QueryStream<MoveOnlyError>& stream_sql(Memory owner, CompiledQuery, std::size_t) override {
        return make_stream<Result<StreamChunk, MoveOnlyError>, OneFailure>(owner, counts);
    }

    DataServiceOperationResult<SqlTransactionHandle<MoveOnlyError>, MoveOnlyError>&
    begin_sql(Memory owner) override {
        if (fail_begin) {
            return failed<SqlTransactionHandle<MoveOnlyError>>(owner, counts, "native begin");
        }
        auto& transaction =
            detail::async_arena(owner).make_resource<NativeTransaction>(owner, counts);
        return make_operation<Result<SqlTransactionHandle<MoveOnlyError>, MoveOnlyError>,
                              ReadyTransaction>(owner, transaction);
    }
};

static void explicit_completion_consumption() {
    Counts counts;
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    {
        auto& operation = failed<std::uint64_t>(owner, counts, "explicit operation");
        bool rejected = false;
        try {
            (void)operation.take_result();
        } catch (const std::logic_error&) {
            rejected = true;
        }
        check(rejected && counts.produced == 0,
              "taking pending work neither polls nor consumes it");
        check(operation.wait().error().message == "explicit operation",
              "owned completion is first available as a borrow");
        auto result = operation.take_result();
        check(result.error().message == "explicit operation" && counts.finished == 1,
              "taking transfers the complete result without repeating finish hooks");
        rejected = false;
        try {
            (void)operation.poll();
        } catch (const std::logic_error&) {
            rejected = true;
        }
        check(rejected, "poll rejects a consumed completion without accessing an empty optional");
        rejected = false;
        try {
            (void)operation.take_result();
        } catch (const std::logic_error&) {
            rejected = true;
        }
        check(rejected, "completion can be taken only once");
        auto& stream = make_stream<Result<StreamChunk, MoveOnlyError>, OneFailure>(owner, counts);
        const auto* old = stream.wait_next();
        check(old && old->error().message == "native stream",
              "stream item is delivered before consumption");
        auto item = stream.take_item();
        check(item.error().message == "native stream",
              "stream consumption transfers the complete native error");
        bool retired = false;
        try {
            (void)detail::async_arena(owner).slot_for_address(old);
        } catch (const ReferenceError& error) {
            retired = error.kind() == ReferenceError::Kind::Dead;
        }
        check(retired, "taking retires the original stream item identity");
        rejected = false;
        try {
            (void)stream.take_item();
        } catch (const std::logic_error&) {
            rejected = true;
        }
        check(rejected && !stream.wait_next(),
              "stream item cannot be taken twice and EOF remains distinct");
    }
    user.reset();
    check(counts.destroyed == counts.produced && counts.cancelled == 0 &&
              global.live_user_bytes() == 0,
          "consumed completions have exactly-once cleanup and no repeated cancellation");
}

static void actual_sql_adapters() {
    Counts counts;
    GlobalContext global;
    auto& descriptor = global.make<EntityDescriptor>("Record");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text);
    auto& dialect = global.make_resource<Dialect>();
    auto& schema = global.make_resource<Schema>(descriptor);
    auto& transport = global.make_resource<Transport>(counts);
    auto& cache = global.make<SqlExecutorState>();
    auto& executor = global.make<SqlDataServiceExecutor<MoveOnlyError, Transport>>(
        dialect, transport, schema, cache);
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    {
        auto request = [&] {
            SelectQuery query(std::allocator_arg, Allocator(owner), "Record");
            return QueryRequest(std::allocator_arg, Allocator(owner), query, false, true);
        };
        const auto& query = executor.query(owner, request()).wait();
        check(!query && query.error().transport_error().message == "native query",
              "actual SQL query adapter retains a move-only transport error");
        InsertCommand insert(std::allocator_arg, Allocator(owner), "Record");
        insert.value("id", std::uint64_t(1))
            .value("version", std::int64_t(1))
            .value("name", "first");
        const auto& mutation =
            executor
                .mutate(owner,
                        MutationRequest(std::allocator_arg, Allocator(owner), std::move(insert)))
                .wait();
        check(!mutation && mutation.error().transport_error().message == "native execute",
              "actual observed mutation adapter preserves the typed native error");
        const auto& begin = executor.begin(owner).wait();
        check(!begin && begin.error().transport_error().message == "native begin",
              "actual transaction begin maps noncopyable errors");
        transport.fail_begin = false;
        auto& transaction = executor.begin(owner).wait().value().get();
        const auto& transaction_query = transaction.query(owner, request()).wait();
        check(!transaction_query &&
                  transaction_query.error().transport_error().message == "native transaction query",
              "actual transaction query forwards a noncopyable error");
        const auto& commit = transaction.commit(owner).wait();
        check(!commit && commit.error().transport_error().message == "native commit" &&
                  transaction.consumed(),
              "failed terminal commit retains its error and consumes transaction access");
        auto& rollback_transaction = executor.begin(owner).wait().value().get();
        const auto& rollback = rollback_transaction.rollback(owner).wait();
        check(!rollback && rollback.error().transport_error().message == "native rollback" &&
                  rollback_transaction.consumed(),
              "failed terminal rollback retains its error and consumes transaction access");
        unsigned events = 0;
        auto observer =
            ExecutionObserver(std::allocator_arg, Allocator(owner), [&](ExecutionMetadata value) {
                ++events;
                check(value.sql_log.execution_outcome == SqlExecutionOutcome::Failure,
                      "diagnostic stream retains the actual failure outcome");
            });
        auto& stream = executor.query_stream_observed(owner, request(), 2, observer);
        const auto* item = stream.wait_next();
        check(item && !*item && item->error().transport_error().message == "native stream" &&
                  events == 1,
              "mapped and diagnostic stream adapters forward one concrete native error");
        check(!stream.wait_next() && events == 1,
              "diagnostic stream failure terminates without duplicate output");
        CompiledQuery compiled(std::allocator_arg, Allocator(owner));
        compiled.sql = "SELECT ?";
        compiled.params.emplace_back(std::uint64_t(1));
        Vector<Value> ids(owner);
        ids.emplace_back(std::uint64_t(1));
        ids.emplace_back(std::uint64_t(2));
        const auto& repeated = transport.fetch_repeated_compact_sql(owner, compiled, 0, ids).wait();
        check(!repeated && repeated.error().message == "native query",
              "default repeated fetch stops on its move-only error");
    }
    user.reset();
    check(counts.destroyed == counts.produced && global.live_user_bytes() == 0,
          "real SQL adapter paths destroy every native error once and reclaim all User blocks");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    struct Restore {
        Memory previous;

        ~Restore() {
            std::pmr::set_default_resource(previous);
        }
    } restore{previous};

    try {
        explicit_completion_consumption();
        actual_sql_adapters();
        std::cout << "Move-only SQL completion contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
