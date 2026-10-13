#pragma once
#include "sqlite_execution_fixture.hpp"
#include <iostream>

struct TxControls {
    bool pending_begin = false, fail_begin = false, pending_terminal = false, fail_terminal = false;
    unsigned begins = 0, commits = 0, rollbacks = 0, abandoned = 0, cancelled_begin = 0;
};

struct NativeTransaction final : SqlTransportTransaction<String> {
    Database& db;
    Transport& inner;
    TxControls& controls;
    bool active = true;
    Arena::Quiescence close_;

    NativeTransaction(Memory r, Database& db, Transport& inner, TxControls& controls)
        : SqlTransportTransaction<String>(r), db(db), inner(inner), controls(controls) {
        db.exec("BEGIN");
        close_.attach(detail::async_arena(r), this, [](void* p) noexcept {
            static_cast<NativeTransaction*>(p)->close();
        });
    }

    ~NativeTransaction() {
        close();
    }

    void close() noexcept {
        close_.reset();
        if (active) {
            sqlite3_exec(db.handle, "ROLLBACK", nullptr, nullptr, nullptr);
            active = false;
            ++controls.abandoned;
        }
    }

    struct Terminal {
        using allocator_type = Allocator;
        Memory resource;
        NativeTransaction& owner;
        bool commit;

        Terminal(std::allocator_arg_t, Allocator a, NativeTransaction& owner, bool commit)
            : resource(a.resource()), owner(owner), commit(commit) {}

        std::optional<Result<std::monostate, String>> poll(WakeSignal&) {
            if (owner.controls.pending_terminal) {
                return {};
            }
            if (owner.controls.fail_terminal) {
                return Result<std::monostate, String>::err(resource,
                                                           String("terminal failure", resource));
            }
            owner.db.exec(commit ? "COMMIT" : "ROLLBACK");
            owner.active = false;
            if (commit) {
                ++owner.controls.commits;
            } else {
                ++owner.controls.rollbacks;
            }
            return Result<std::monostate, String>::ok(resource, std::monostate{});
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            owner.close();
        }
    };

protected:
    void cancel_sql_transaction() noexcept override {
        close();
    }

    DataServiceOperationResult<Rows, String>&
    fetch_all_compact_sql_transaction(Memory r, const CompiledQuery& q) override {
        return inner.fetch_all_compact_sql(r, q);
    }

    DataServiceOperationResult<std::uint64_t, String>&
    execute_sql_transaction(Memory r, const CompiledQuery& q) override {
        return inner.execute_sql(r, q);
    }

    DataServiceOperationResult<std::monostate, String>& commit_sql_transaction(Memory r) override {
        return make_operation<Result<std::monostate, String>, Terminal>(r, *this, true);
    }

    DataServiceOperationResult<std::monostate, String>&
    rollback_sql_transaction(Memory r) override {
        return make_operation<Result<std::monostate, String>, Terminal>(r, *this, false);
    }
};

struct TransactionTransport final : SqlTransactionTransport<String> {
    using allocator_type = Allocator;
    Database& db;
    Transport& inner;
    TxControls& controls;

    TransactionTransport(
        std::allocator_arg_t, Allocator, Database& db, Transport& inner, TxControls& controls)
        : db(db), inner(inner), controls(controls) {}

    struct Begin {
        using allocator_type = Allocator;
        Memory resource;
        TransactionTransport& owner;

        Begin(std::allocator_arg_t, Allocator a, TransactionTransport& owner)
            : resource(a.resource()), owner(owner) {}

        std::optional<Result<SqlTransactionHandle<String>, String>> poll(WakeSignal&) {
            if (owner.controls.pending_begin) {
                return {};
            }
            ++owner.controls.begins;
            if (owner.controls.fail_begin) {
                return Result<SqlTransactionHandle<String>, String>::err(
                    resource, String("begin failure", resource));
            }
            auto& tx = detail::async_arena(resource).make_resource<NativeTransaction>(
                resource, owner.db, owner.inner, owner.controls);
            return Result<SqlTransactionHandle<String>, String>::ok(
                resource,
                SqlTransactionHandle<String>(std::allocator_arg, Allocator(resource), tx));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {
            ++owner.controls.cancelled_begin;
        }
    };

    DataServiceOperationResult<Rows, String>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& q) override {
        return inner.fetch_all_compact_sql(r, q);
    }

    DataServiceOperationResult<std::uint64_t, String>&
    execute_sql(Memory r, const CompiledQuery& q) override {
        return inner.execute_sql(r, q);
    }

    DataServiceOperationResult<SqlTransactionHandle<String>, String>& begin_sql(Memory r) override {
        return make_operation<Result<SqlTransactionHandle<String>, String>, Begin>(r, *this);
    }
};

using Executor = SqlDataServiceExecutor<String, TransactionTransport>;
static_assert(std::derived_from<Executor, TransactionExecutor<SqlExecutorError<String>>>);
static_assert(!std::derived_from<SqlDataServiceExecutor<String>,
                                 TransactionExecutor<SqlExecutorError<String>>>);

struct Fixture {
    GlobalContext global;
    Database& db = global.make_resource<Database>();
    SqliteDialect& dialect = global.make_resource<SqliteDialect>();
    EntityDescriptor& entity = model(global);
    Schema& schema = global.make_resource<Schema>(entity);
    Transport& transport = global.make<Transport>(db);
    TxControls& controls = global.make_resource<TxControls>();
    TransactionTransport& provider = global.make<TransactionTransport>(db, transport, controls);
    SqlExecutorState& cache = global.make<SqlExecutorState>();
    Executor& executor = global.make<Executor>(dialect, provider, schema, cache);
};

static QueryRequest fixture_query_request(Memory r, bool metadata = true) {
    SelectQuery q(std::allocator_arg, Allocator(r), "Record");
    q.comment("Exercise SQL transaction query");
    q.purpose("Check transaction snapshots and lifecycle");
    q.filter(Expr::eq(r, "id", std::uint64_t(7)));
    return QueryRequest(std::allocator_arg, Allocator(r), q, false, metadata);
}

static std::string_view saved_name(const QueryResult& result) {
    return std::get<String>(result.rows[0].get("name")->data);
}

template <class F>
static void rejected(F f, const char* message) {
    bool rejected = false;
    try {
        f();
    } catch (const std::logic_error&) {
        rejected = true;
    }
    check(rejected, message);
}
