#include <teaql/sql/transport.hpp>
#include <teaql/sql/error.hpp>
#include <teaql/runtime/context.hpp>
#include <atomic>
#include <thread>
#include <iostream>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}
enum class Error { Fetch, Execute, Begin, Commit, Rollback };
using Rows = Vector<CompactRow>;

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T result;

    Ready(std::allocator_arg_t, Allocator a, T&& result)
        : result(detail::move_in(a.resource(), std::move(result))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

template <class T>
auto& ready(Memory r, Result<T, Error> value) {
    return make_operation<Result<T, Error>, Ready<Result<T, Error>>>(r, std::move(value));
}

struct Stats {
    std::atomic<bool> gate{false};
    int starts = 0, finishes = 0, cancels = 0, reads = 0;
};

struct PendingRows {
    Memory resource;
    const CompiledQuery& query;
    Stats& stats;
    bool fail;
    std::thread worker;
    std::atomic<bool> stop{false}, done{false};
    std::int64_t id = 0;

    PendingRows(Memory r, const CompiledQuery& query, Stats& stats, bool fail)
        : resource(r), query(query), stats(stats), fail(fail) {}

    std::optional<Result<Rows, Error>> poll(WakeSignal& wake) {
        if (!worker.joinable()) {
            ++stats.starts;
            worker = std::thread([this, &wake] {
                while (!stats.gate.load()) {
                    stats.gate.wait(false);
                }
                if (!stop.load()) {
                    id = *query.params.at(1).try_i64();
                    ++stats.reads;
                }
                done.store(true);
                wake.notify();
            });
        }
        if (!done.load()) {
            return {};
        }
        if (fail) {
            return Result<Rows, Error>::err(resource, Error::Fetch);
        }
        Rows rows(resource);
        rows.emplace_back();
        rows.back().insert("id", Value(resource, id));
        return Result<Rows, Error>::ok(resource, std::move(rows));
    }

    void finish_and_wait() noexcept {
        if (worker.joinable()) {
            worker.join();
        }
        ++stats.finishes;
    }

    void cancel_and_wait() noexcept {
        stop.store(true);
        stats.gate.store(true);
        stats.gate.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
        ++stats.cancels;
    }
};

struct Tx final : SqlTransportTransaction<Error> {
    int queries = 0;

    explicit Tx(Memory r) : SqlTransportTransaction(r) {}

    DataServiceOperationResult<Rows, Error>&
    fetch_all_compact_sql_transaction(Memory r, const CompiledQuery&) override {
        ++queries;
        return ready(r, Result<Rows, Error>::err(r, Error::Fetch));
    }

    DataServiceOperationResult<std::uint64_t, Error>&
    execute_sql_transaction(Memory r, const CompiledQuery&) override {
        return ready(r, Result<std::uint64_t, Error>::ok(r, 7));
    }

    DataServiceOperationResult<std::monostate, Error>& commit_sql_transaction(Memory r) override {
        return ready(r, Result<std::monostate, Error>::err(r, Error::Commit));
    }

    DataServiceOperationResult<std::monostate, Error>& rollback_sql_transaction(Memory r) override {
        return ready(r, Result<std::monostate, Error>::ok(r, {}));
    }
};

struct Chunks {
    using allocator_type = Allocator;
    Memory resource;
    bool emitted = false;

    Chunks(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    std::optional<std::optional<Result<StreamChunk, Error>>> poll_next(WakeSignal&) {
        if (emitted) {
            return std::optional<Result<StreamChunk, Error>>{};
        }
        emitted = true;
        StreamChunk chunk(std::allocator_arg, Allocator(resource), 0, true);
        return std::optional<Result<StreamChunk, Error>>(
            Result<StreamChunk, Error>::ok(resource, std::move(chunk)));
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct Transport final : StreamingSqlTransport<Error>, SqlTransactionTransport<Error> {
    using allocator_type = Allocator;
    Vector<CompiledQuery> seen;
    Stats& stats;
    bool delayed = false;
    int fail_at = 0;
    std::size_t chunk_size = 0;

    Transport(std::allocator_arg_t, Allocator a, Stats& stats) : seen(a.resource()), stats(stats) {}

    DataServiceOperationResult<Rows, Error>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& query) override {
        seen.emplace_back(query);
        bool fail = fail_at && int(seen.size()) == fail_at;
        if (delayed) {
            return make_resource_operation<Result<Rows, Error>, PendingRows>(
                r, r, query, stats, fail);
        }
        if (fail) {
            return ready(r, Result<Rows, Error>::err(r, Error::Fetch));
        }
        Rows rows(r);
        rows.emplace_back();
        rows.back().insert("id", query.params.at(1));
        return ready(r, Result<Rows, Error>::ok(r, std::move(rows)));
    }

    DataServiceOperationResult<std::uint64_t, Error>& execute_sql(Memory r,
                                                                  const CompiledQuery&) override {
        return ready(r, Result<std::uint64_t, Error>::err(r, Error::Execute));
    }

    QueryStream<Error>& stream_sql(Memory r, CompiledQuery, std::size_t n) override {
        chunk_size = n;
        return make_stream<Result<StreamChunk, Error>, Chunks>(r);
    }

    DataServiceOperationResult<SqlTransactionHandle<Error>, Error>& begin_sql(Memory r) override {
        auto& tx = detail::async_arena(r).make<Tx>(r);
        return ready(r,
                     Result<SqlTransactionHandle<Error>, Error>::ok(
                         r, SqlTransactionHandle<Error>(std::allocator_arg, Allocator(r), tx)));
    }
};

static CompiledQuery& template_query(UserContext& user) {
    auto& q = user.make<CompiledQuery>();
    q.sql = "SELECT ? AS fixed, ? AS id";
    q.params.emplace_back(99);
    q.params.emplace_back(0);
    q.comment.emplace("source comment kept across repeated fetches", user.memory_resource());
    q.log_context.parameter_policies = {SqlParameterLogPolicy::Credential,
                                        SqlParameterLogPolicy::Masked};
    q.log_context.generated_sql = true;
    return q;
}

static void repeated() {
    Stats stats;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& transport = global.make<Transport>(stats);
    auto& q = template_query(*user);
    Vector<Value> ids(r);
    for (int n : {3, 1, 3}) {
        ids.emplace_back(n);
    }
    auto& operation = transport.fetch_repeated_compact_sql(r, q, 1, ids);
    check(transport.seen.empty(), "default repeated fetch is lazy");
    auto& result = operation.wait();
    check(result && result.value().size() == 3, "all input values append their rows");
    for (std::size_t i = 0; i < ids.size(); ++i) {
        check(*result.value()[i].get("id") == ids[i], "input order and duplicate values preserved");
        auto& seen = transport.seen[i];
        check(seen.params[0] == q.params[0] && seen.comment == q.comment &&
                  seen.log_context == q.log_context && seen.sql == q.sql,
              "only designated template parameter changes");
    }
    check(*q.params[1].try_i64() == 0, "template input is unchanged");
    auto& empty = transport.fetch_repeated_compact_sql(r, q, 900, {});
    check(empty.wait().value().empty() && transport.seen.size() == 3,
          "empty repeat does not validate unused parameter index or perform I/O");
    auto& invalid = transport.fetch_repeated_compact_sql(r, q, 900, ids);
    bool threw = false;
    try {
        invalid.wait();
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw && transport.seen.size() == 3,
          "invalid parameter index fails before any transport call");
    transport.fail_at = 5;
    auto& failed = transport.fetch_repeated_compact_sql(r, q, 1, ids);
    check(failed.wait().error() == Error::Fetch && transport.seen.size() == 5,
          "first transport error stops later iterations and discards partial result");
    auto& stream = transport.stream_sql(r, detail::clone_in(r, q), 17);
    check(stream.wait_next()->value().is_last && !stream.wait_next() && transport.chunk_size == 17,
          "stream transport remains separate and preserves chunk request");
}

static void asynchronous_composition() {
    Stats stats;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& transport = global.make<Transport>(stats);
    transport.delayed = true;
    auto& q = template_query(*user);
    auto& values = user->make<Vector<Value>>();
    values.emplace_back(5);
    values.emplace_back(6);
    auto& operation = transport.fetch_repeated_compact_sql(r, q, 1, values);
    check(!operation.poll() && stats.starts == 1,
          "outer poll waits for pending child without blocking");
    stats.gate.store(true);
    stats.gate.notify_all();
    auto& result = operation.wait();
    check(result.value().size() == 2 && stats.finishes == 2 && stats.cancels == 0 &&
              stats.reads == 2,
          "native child wake propagates to outer wait across iterations");
    stats.gate.store(false);
    auto& abandoned = transport.fetch_repeated_compact_sql(r, q, 1, values);
    check(!abandoned.poll(), "next child pending");
    abandoned.cancel();
    check(stats.cancels == 1 && abandoned.status() == AsyncStatus::Cancelled,
          "explicit abandonment cancels and joins pending child once");
    stats.gate.store(false);
    auto& closing = transport.fetch_repeated_compact_sql(r, q, 1, values);
    check(!closing.poll(), "context-close child pending");
    user.reset();
    check(stats.cancels == 2 && stats.finishes == 2,
          "context close quiesces children before destroying borrowed template memory");
}

static void transaction_contract() {
    Stats stats;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& transport = global.make<Transport>(stats);
    auto& q = template_query(*user);
    const auto& handle = transport.begin_sql(r).wait().value();
    auto& tx = handle.get();
    check(tx.execute_sql(r, q).wait().value() == 7,
          "associated SQL transaction exposes raw execution");
    bool cross = false;
    try {
        (void)detail::clone_in(global.memory_resource(), handle);
    } catch (const std::invalid_argument&) {
        cross = true;
    }
    check(cross, "SQL transaction handle cannot cross owning domain");
    bool wrong = false;
    try {
        tx.commit_sql(global.memory_resource());
    } catch (const std::invalid_argument&) {
        wrong = true;
    }
    check(wrong && !tx.consumed(), "wrong-domain commit cannot consume handle");
    auto& pending = tx.execute_sql(r, q);
    bool borrowed = false;
    try {
        tx.commit_sql(r);
    } catch (const std::logic_error&) {
        borrowed = true;
    }
    check(borrowed && !tx.consumed(), "pending SQL operation prevents terminal consumption");
    pending.cancel();
    auto& commit = tx.commit_sql(r);
    check(tx.consumed() && commit.status() == AsyncStatus::Pending,
          "commit consumes transaction before polling");
    int rejected = 0;
    try {
        tx.execute_sql(r, q);
    } catch (const std::logic_error&) {
        ++rejected;
    }
    try {
        tx.fetch_all_compact_sql(r, q);
    } catch (const std::logic_error&) {
        ++rejected;
    }
    try {
        tx.fetch_repeated_compact_sql(r, q, 1, {});
    } catch (const std::logic_error&) {
        ++rejected;
    }
    try {
        tx.rollback_sql(r);
    } catch (const std::logic_error&) {
        ++rejected;
    }
    check(rejected == 4 && commit.wait().error() == Error::Commit,
          "terminal failure keeps every SQL entry point consumed");
    check(transport.begin_sql(r).wait().value().get().rollback_sql(r).wait().is_ok(),
          "rollback remains independent terminal operation");
}

static void forwarding_contract() {
    WakeSignal child, parent, grandparent;
    WakeSignal::Forward a, b, illegal;
    a.attach(child, parent);
    b.attach(parent, grandparent);
    auto p = parent.ticket(), g = grandparent.ticket();
    std::thread worker([&] {
        child.notify();
    });
    worker.join();
    check(parent.ticket() != p && grandparent.ticket() != g,
          "wake forwarding composes across multiple levels");
    bool cycle = false;
    try {
        illegal.attach(grandparent, child);
    } catch (const std::logic_error&) {
        cycle = true;
    }
    check(cycle, "wake cycles rejected before native locks could deadlock");
    bool shared = false;
    try {
        illegal.attach(child, grandparent);
    } catch (const std::logic_error&) {
        shared = true;
    }
    check(shared, "one pending child has one awaiting parent");
    a.reset();
    p = parent.ticket();
    child.notify();
    check(parent.ticket() == p, "detached parent cannot receive late child notifications");
    WakeSignal source;
    std::atomic<bool> started = false, stop = false;
    std::thread notifier;
    {
        WakeSignal transient;
        WakeSignal::Forward link;
        link.attach(source, transient);
        notifier = std::thread([&] {
            source.notify();
            started.store(true);
            started.notify_all();
            while (!stop.load()) {
                source.notify();
            }
        });
        started.wait(false);
        link.reset();
    } // The native thread keeps notifying after its former parent is destroyed.
    source.notify();
    stop.store(true);
    notifier.join();
}

static void retained_repeat_inputs() {
    Stats stats;
    GlobalContext global;
    auto input = global.make_user_context();
    auto output = global.make_user_context();
    auto r = output->memory_resource();
    auto& transport = global.make<Transport>(stats);
    auto& q = template_query(*input);
    auto& values = input->make<Vector<Value>>();
    {
        String long_text(4096, 'x', input->memory_resource());
        values.emplace_back(long_text);
    }
    auto& operation = transport.fetch_repeated_compact_sql(r, q, 1, values);
    input.reset();
    const auto& rows = operation.wait().value();
    check(rows.size() == 1 && rows[0].get("id")->try_text()->size() == 4096 &&
              rows[0].memory_resource() == r,
          "repeat inputs are explicitly retained in the destination domain before source close");
}

static void error_domains() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = global.memory_resource(), u = user->memory_resource();
    using E = SqlExecutorError<String>;
    auto& compile = user->make<E>(E::compile(u,
                                             SqlCompileError(std::allocator_arg,
                                                             Allocator(u),
                                                             SqlCompileError::Code::UnknownField,
                                                             "a long unknown property name")));
    auto& transport =
        user->make<E>(E::transport(u, String("native transport error with a long payload", u)));
    auto& persisted =
        user->make<E>(E::persisted_record(u, "expected record was absent after mutation"));
    auto& retained_compile = global.make<E>(compile);
    auto& retained_transport = global.make<E>(transport);
    auto& retained_persisted = global.make<E>(persisted);
    user.reset();
    check(retained_compile.to_string(r).starts_with("SQL compile error: ") &&
              retained_compile.compile_error().name == "a long unknown property name",
          "compile error keeps structured payload and display prefix");
    check(retained_transport.to_string(r) ==
                  "Transport error: native transport error with a long payload" &&
              retained_transport.transport_error().get_allocator().resource() == r,
          "typed transport payload is explicitly deep copied");
    check(retained_persisted.to_string(r) ==
              "Persisted record error: expected record was absent after mutation",
          "persisted-record error remains independent");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        repeated();
        asynchronous_composition();
        transaction_contract();
        forwarding_contract();
        retained_repeat_inputs();
        error_domains();
        std::pmr::set_default_resource(previous);
        std::cout << "SQL transport contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
