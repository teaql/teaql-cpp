#include "sql_transaction_fixture.hpp"
#include <thread>
#include <atomic>

struct StreamingTransport : StreamingSqlTransport<String> {
    using allocator_type = Allocator;
    Memory resource;
    bool async = false, pending = false, panic = false, last_first = false;
    unsigned chunks = 3, created = 0, closed = 0;
    int error_at = -1;
    std::size_t requested_size = 99;

    StreamingTransport(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    struct Cursor {
        using allocator_type = Allocator;
        Memory resource;
        StreamingTransport& owner;
        CompiledQuery query;
        unsigned index = 0;
        bool spawned = false, closed = false;
        std::atomic<bool> ready = false;
        std::thread worker;

        Cursor(std::allocator_arg_t, Allocator a, StreamingTransport& owner, CompiledQuery query)
            : resource(a.resource()), owner(owner), query(std::allocator_arg, a, std::move(query)) {
        }

        std::optional<std::optional<Result<StreamChunk, String>>> poll_next(WakeSignal& wake) {
            using Item = Result<StreamChunk, String>;
            if (owner.pending) {
                return {};
            }
            if (owner.async && !spawned) {
                worker = std::thread([this, &wake] {
                    ready.store(true);
                    wake.notify();
                });
                spawned = true;
                return {};
            }
            if (owner.async && !ready.load()) {
                return {};
            }
            if (owner.panic) {
                throw std::runtime_error("cursor panic");
            }
            if (int(index) == owner.error_at) {
                ++index;
                return std::optional<Item>(Item::err(resource, String("cursor failure", resource)));
            }
            if (index >= owner.chunks) {
                return std::optional<Item>{};
            }
            StreamChunk chunk(std::allocator_arg,
                              Allocator(resource),
                              index,
                              owner.last_first || index + 1 == owner.chunks);
            for (unsigned n = 0; n < 2; ++n) {
                chunk.rows.emplace_back();
                chunk.rows.back().insert("id", Value(resource, std::uint64_t(index * 2 + n)));
            }
            ++index;
            return std::optional<Item>(Item::ok(resource, std::move(chunk)));
        }

        void finish_and_wait() noexcept {
            close();
        }

        void cancel_and_wait() noexcept {
            close();
        }

        void close() noexcept {
            if (worker.joinable()) {
                worker.join();
            }
            if (!closed) {
                closed = true;
                ++owner.closed;
            }
        }
    };

    QueryStream<String>& stream_sql(Memory r, CompiledQuery query, std::size_t size) override {
        ++created;
        requested_size = size;
        return make_resource_stream<Result<StreamChunk, String>, Cursor>(
            r, *this, std::move(query));
    }

    DataServiceOperationResult<Rows, String>& fetch_all_compact_sql(Memory,
                                                                    const CompiledQuery&) override {
        throw std::logic_error("native stream must use cursor SPI");
    }

    DataServiceOperationResult<std::uint64_t, String>& execute_sql(Memory,
                                                                   const CompiledQuery&) override {
        throw std::logic_error("unused stream fixture");
    }
};

static_assert(std::derived_from<SqlDataServiceExecutor<String, StreamingTransport>,
                                StreamQueryExecutor<SqlExecutorError<String>>>);
static_assert(!std::derived_from<SqlDataServiceExecutor<String>,
                                 StreamQueryExecutor<SqlExecutorError<String>>>);

static void native_lifecycle() {
    GlobalContext global;
    auto& e = model(global);
    auto& schema = global.make_resource<Schema>(e);
    auto& d = global.make_resource<SqliteDialect>();
    auto& transport = global.make<StreamingTransport>();
    auto& cache = global.make<SqlExecutorState>();
    auto& executor = global.make<SqlDataServiceExecutor<String, StreamingTransport>>(
        d, transport, schema, cache);
    auto& events = global.make<Vector<ExecutionMetadata>>();
    auto run = [&](bool capture, bool sink_throws = false) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& observer = user->make<ExecutionObserver>([&](ExecutionMetadata m) {
            check(transport.closed == transport.created, "cursor closes and joins before sink");
            events.emplace_back(std::move(m));
            if (sink_throws) {
                throw 1;
            }
        });
        auto& stream = executor.query_stream_observed(r, request(r, capture), 0, observer);
        check(transport.requested_size == 0, "native provider receives chunk size unchanged");
        const auto* first = stream.wait_next();
        check(first && *first && first->value().chunk_index == 0, "first native chunk");
        check(first->value().rows[0].get("id") && first->value().rows.size() == 2,
              "chunk carries Context-owned rows");
        while (stream.wait_next()) {
        }
        check(first->value().rows.size() == 2, "earlier chunks survive later polls within Context");
    };
    run(true);
    check(events.size() == 1 && events[0].result_count == 6 &&
              events[0].sql_log.execution_outcome == SqlExecutionOutcome::Success,
          "one terminal success counts delivered native rows");
    run(false);
    check(events.size() == 1, "metadata opt-out does not invoke observer");
    transport.async = true;
    run(true, true);
    check(events.size() == 2,
          "worker wake crosses both stream adapters and sink exceptions are suppressed");
    transport.async = false;
    transport.last_first = true;
    run(true);
    check(events.back().result_count == 2,
          "observed final chunk closes source immediately without reading extra chunks");
    transport.last_first = false;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& observer = user->make<ExecutionObserver>([&](ExecutionMetadata m) {
            events.emplace_back(std::move(m));
        });
        transport.error_at = 1;
        auto& stream = executor.query_stream_observed(r, request(r), 1, observer);
        check(bool(*stream.wait_next()), "row before error");
        auto* error = stream.wait_next();
        check(error && !*error &&
                  error->error().kind() == SqlExecutorError<String>::Kind::Transport,
              "stream keeps typed provider error");
        check(!stream.wait_next() && events.back().result_count == 2 &&
                  events.back().sql_log.execution_outcome == SqlExecutionOutcome::Failure,
              "observed error terminates once after prior rows");
    }
    transport.error_at = -1;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& observer = user->make<ExecutionObserver>([&](ExecutionMetadata m) {
            events.emplace_back(std::move(m));
        });
        transport.panic = true;
        auto& stream = executor.query_stream_observed(r, request(r), 1, observer);
        bool threw = false;
        try {
            stream.wait_next();
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw && events.back().sql_log.execution_outcome == SqlExecutionOutcome::Failure,
              "cursor unwind reports failure, not cancellation");
    }
    transport.panic = false;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& observer = user->make<ExecutionObserver>([&](ExecutionMetadata m) {
            check(transport.closed == transport.created, "close joins native source first");
            events.emplace_back(std::move(m));
        });
        transport.pending = true;
        auto& stream = executor.query_stream_observed(r, request(r), 1, observer);
        check(stream.poll_next().kind ==
                  StreamPoll<Result<StreamChunk, SqlExecutorError<String>>>::Kind::Pending,
              "pending cursor");
        user.reset();
        check(events.back().result_count == 0 &&
                  events.back().sql_log.execution_outcome == SqlExecutionOutcome::Cancelled,
              "Context close publishes cancellation while sink data lives");
    }
    transport.pending = false;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto count = events.size();
        auto created = transport.created;
        auto& observer = user->make<ExecutionObserver>([&](ExecutionMetadata m) {
            events.emplace_back(std::move(m));
        });
        SelectQuery q(std::allocator_arg, Allocator(r), "Unknown");
        auto& stream = executor.query_stream_observed(
            r, QueryRequest(std::allocator_arg, Allocator(r), q, true, true), 1, observer);
        check(!*stream.wait_next() && !stream.wait_next() && transport.created == created &&
                  events.size() == count,
              "compile failure is a single error item without a cursor or invented diagnostic");
    }
}

static void buffered_transactions() {
    Fixture f;
    f.db.exec("INSERT INTO records VALUES(8,1,'second',2),(9,1,'third',2)");
    auto user = f.global.make_user_context();
    auto r = user->memory_resource();
    auto& tx = f.executor.begin(r).wait().value().get();
    auto& streaming = dynamic_cast<StreamQueryExecutor<SqlExecutorError<String>>&>(tx);
    auto& events = f.global.make<Vector<ExecutionMetadata>>();
    auto& observer = user->make<ExecutionObserver>([&](ExecutionMetadata m) {
        events.emplace_back(std::move(m));
    });
    auto make_request = [&] {
        SelectQuery q(std::allocator_arg, Allocator(r), "Record");
        q.order_asc("id");
        return QueryRequest(std::allocator_arg, Allocator(r), q, false, true);
    };
    auto& stream = streaming.query_stream_observed(r, make_request(), 0, observer);
    check(f.transport.reads == 0, "buffered transaction query starts lazily");
    rejected(
        [&] {
            tx.commit(r);
        },
        "open stream borrows transaction even before first poll");
    auto* first = stream.wait_next();
    check(first->value().rows.size() == 1 && !first->value().is_last && f.transport.reads == 1,
          "transaction zero chunk size clamps to one after one fetch");
    stream.cancel();
    check(events.size() == 1 && events[0].result_count == 1 &&
              events[0].sql_log.execution_outcome == SqlExecutionOutcome::Cancelled,
          "buffered cancellation counts one delivered row rather than entire SQL buffer");
    auto& complete = streaming.query_stream_observed(r, make_request(), 2, observer);
    auto* chunk = complete.wait_next();
    check(chunk->value().rows.size() == 2 && chunk->value().chunk_index == 0,
          "first buffered chunk");
    chunk = complete.wait_next();
    check(chunk->value().rows.size() == 1 && chunk->value().is_last &&
              events.back().result_count == 3,
          "final buffered chunk emits success before Context close");
    check(!complete.wait_next(), "buffered EOF");
    check(bool(tx.commit(r).wait()), "ended/cancelled streams release transaction borrow");
    rejected(
        [&] {
            streaming.query_stream(r, make_request(), 1);
        },
        "consumed transaction cannot create a new stream");
    auto& next = f.executor.begin(r).wait().value().get();
    auto& next_stream = dynamic_cast<StreamQueryExecutor<SqlExecutorError<String>>&>(next);
    f.transport.pause_read = true;
    auto& pending = next_stream.query_stream_observed(r, make_request(), 2, observer);
    check(pending.poll_next().kind ==
              StreamPoll<Result<StreamChunk, SqlExecutorError<String>>>::Kind::Pending,
          "buffer stream waits for SQL fetch");
    rejected(
        [&] {
            next.rollback(r);
        },
        "pending stream prevents rollback");
    pending.cancel();
    check(f.transport.cancelled == 1 && events.back().result_count == 0,
          "cancelling buffered query stops SQL work");
    f.transport.pause_read = false;
    next.rollback(r).wait();
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        native_lifecycle();
        buffered_transactions();
        std::pmr::set_default_resource(previous);
        std::cout << "SQL stream execution contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
