#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/id.hpp>
#include <teaql/provider_sqlite/business_id.hpp>
#include <teaql/runtime/id_space.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/provider_sqlite/detail/schema.hpp>
#include <teaql/provider_sqlite/detail/bindings.hpp>
#include <teaql/provider_sqlite/detail/decoding.hpp>
#include <teaql/core/detail/text.hpp>
#include <sqlite3.h>
#include <algorithm>
#include <charconv>
#include <thread>

namespace teaql::detail {
namespace {
constexpr std::size_t cache_capacity = 64;

template <class T>
void append_number(String& out, T value) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), value).ptr;
    out.append(buffer, end);
}

MutationExecutorError native_error(Memory r, sqlite3* db, int code) {
    // A null statement can report MISUSE without updating the connection's
    // last error. Match rusqlite's error_from_handle instead of using stale state.
    const auto* message =
        (!db || sqlite3_errcode(db) != code) ? sqlite3_errstr(code) : sqlite3_errmsg(db);
    return MutationExecutorError::sqlite(r, code, message);
}

MutationExecutorError prepare_error(Memory r, sqlite3* db, int code, std::string_view sql) {
#if SQLITE_VERSION_NUMBER >= 3038000
    if ((code & 0xff) == SQLITE_ERROR) {
        if (auto offset = sqlite3_error_offset(db); offset >= 0) {
            String message(sqlite3_errmsg(db), r);
            message += " in ";
            message += sql;
            message += " at offset ";
            append_number(message, offset);
            return MutationExecutorError::sqlite(r, 0, message);
        }
    }
#else
    (void)sql;
#endif
    return native_error(r, db, code);
}

struct StatementEntry {
    using allocator_type = Allocator;
    String key;
    sqlite3_stmt* statement = nullptr;

    StatementEntry(std::allocator_arg_t, Allocator a) : key(a.resource()) {}

    StatementEntry(std::allocator_arg_t, Allocator a, StatementEntry&& other)
        : key(std::move(other.key), a.resource()),
          statement(std::exchange(other.statement, nullptr)) {}

    StatementEntry(const StatementEntry&) = delete;
};

struct LayoutEntry {
    using allocator_type = Allocator;
    String key;
    const SqliteRowLayout* layout = nullptr;

    LayoutEntry(std::allocator_arg_t, Allocator a) : key(a.resource()) {}

    LayoutEntry(std::allocator_arg_t, Allocator a, LayoutEntry&& other)
        : key(std::move(other.key), a.resource()), layout(other.layout) {}

    LayoutEntry(const LayoutEntry&) = delete;
};
}
struct SqliteLease;

struct SqliteConnectionState {
    using allocator_type = Allocator;
    Memory resource;
    sqlite3* db = nullptr;
    std::thread::id owner = std::this_thread::get_id();
    Vector<StatementEntry> statements;
    // MRU at the front, idle statements only. Capacity is fixed after creation.
    Vector<std::size_t> recency;
    SqliteLease* active = nullptr;
    SqliteLease* first = nullptr;
    SqliteLease* last = nullptr;
    bool poisoned = false;
    // Identity only, never an owning pointer. Native autocommit invalidates the
    // identity so an old consumed handle cannot roll back a later transaction.
    const void* transaction_owner = nullptr;

    SqliteConnectionState(std::allocator_arg_t, Allocator a, std::string_view filename)
        : resource(a.resource()), statements(resource), recency(resource) {
        statements.reserve(cache_capacity);
        for (std::size_t i = 0; i < cache_capacity; ++i) {
            statements.emplace_back();
        }
        recency.reserve(cache_capacity);
        // Resource construction failures are native C++ exceptions, as opposed
        // to Context-borrowed provider results. They must survive stack unwinding
        // that destroys the Context which attempted to create the connection.
        if (filename.find('\0') != filename.npos) {
            throw std::invalid_argument("SQLite filename contains a NUL byte");
        }
        String path(filename, resource);
        auto code = sqlite3_open_v2(path.c_str(),
                                    &db,
                                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI |
                                        SQLITE_OPEN_NOMUTEX,
                                    nullptr);
        if (code != SQLITE_OK) {
            try {
                throw std::runtime_error(db ? sqlite3_errmsg(db)
                                            : "SQLite connection allocation failed");
            } catch (...) {
                sqlite3_close(db);
                db = nullptr;
                throw;
            }
        }
        sqlite3_extended_result_codes(db, 1);
        // rusqlite's default connection busy timeout.
        sqlite3_busy_timeout(db, 5000);
    }

    ~SqliteConnectionState() {
        if (active || first) {
            std::terminate();
        }
        flush();
        if (db && sqlite3_close(db) != SQLITE_OK) {
            std::terminate();
        }
    }

    void require_owner() const {
        if (owner != std::this_thread::get_id()) {
            throw std::logic_error("SQLite Context resources are owner-thread confined");
        }
    }

    void require_borrow(Memory destination) const {
        require_owner();
        if (!async_arena(resource).can_lend_to(async_arena(destination))) {
            throw std::invalid_argument(
                "SQLite work cannot retain a connection from a shorter or unrelated Context");
        }
    }

    void flush() noexcept {
        for (auto& entry : statements) {
            sqlite3_finalize(entry.statement);
            entry.statement = nullptr;
        }
        recency.clear();
    }

    void prepare(Memory r,
                 std::string_view sql,
                 bool cached,
                 sqlite3_stmt*& statement,
                 StatementEntry*& entry) {
        require_owner();
        if (cached) {
            trim(sql);
        }
        if (cached) {
            for (auto p = recency.begin(); p != recency.end(); ++p) {
                if (statements[*p].key == sql) {
                    entry = &statements[*p];
                    statement = std::exchange(entry->statement, nullptr);
                    recency.erase(p);
                    return;
                }
            }
        }
        if (sql.size() > INT32_MAX) {
            throw MutationExecutorError::sqlite(r, SQLITE_TOOBIG, "string or blob too big");
        }
        const char* tail = nullptr;
        auto code = sqlite3_prepare_v3(db,
                                       sql.data(),
                                       int(sql.size()),
                                       cached ? SQLITE_PREPARE_PERSISTENT : 0,
                                       &statement,
                                       &tail);
        if (code != SQLITE_OK) {
            throw prepare_error(r, db, code, sql);
        }
        // rusqlite prepares trailing SQL before reporting MultipleStatement;
        // trailing syntax/schema errors therefore take precedence.
        try {
            auto rest = sql;
            bool extra = false;
            while (tail && tail > rest.data() && tail < rest.data() + rest.size()) {
                rest.remove_prefix(std::size_t(tail - rest.data()));
                sqlite3_stmt* next = nullptr;
                code = sqlite3_prepare_v3(db, rest.data(), int(rest.size()), 0, &next, &tail);
                if (code != SQLITE_OK) {
                    throw prepare_error(r, db, code, rest);
                }
                extra |= next != nullptr;
                sqlite3_finalize(next);
            }
            if (extra) {
                throw MutationExecutorError::sqlite(r, 0, "Multiple statements provided");
            }
            if (cached && statement) {
                auto slot = std::find_if(statements.begin(), statements.end(), [](const auto& e) {
                    return !e.statement;
                });
                if (slot == statements.end()) {
                    auto index = recency.back();
                    recency.pop_back();
                    slot = statements.begin() + std::ptrdiff_t(index);
                    sqlite3_finalize(slot->statement);
                    slot->statement = nullptr;
                }
                slot->key = sql;
                entry = &*slot;
            }
        } catch (...) {
            sqlite3_finalize(statement);
            statement = nullptr;
            throw;
        }
    }

    void release_statement(sqlite3_stmt*& statement, StatementEntry*& entry) noexcept {
        if (!statement) {
            return;
        }
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
        if (entry) {
            entry->statement = statement;
            recency.insert(recency.begin(), std::size_t(entry - statements.data()));
            entry = nullptr;
        } else {
            sqlite3_finalize(statement);
        }
        statement = nullptr;
    }
};

// Intrusive, allocation-free wait queue. SQLite calls run on the owner thread;
// the lease remains held between stream polls just like Rust's guarded cursor.
struct SqliteLease {
    Memory resource;
    SqliteConnectionState& connection;
    SqliteLease* previous = nullptr;
    SqliteLease* next = nullptr;
    WakeSignal* wake = nullptr;
    bool queued = false, owned = false;

    SqliteLease(Memory r, SqliteConnectionState& connection)
        : resource(r), connection(connection) {}

    ~SqliteLease() {
        release();
    }

    void unlink() noexcept {
        if (!queued) {
            return;
        }
        if (previous) {
            previous->next = next;
        } else {
            connection.first = next;
        }
        if (next) {
            next->previous = previous;
        } else {
            connection.last = previous;
        }
        previous = next = nullptr;
        queued = false;
    }

    bool acquire(WakeSignal& signal) {
        connection.require_owner();
        if (owned) {
            return true;
        }
        wake = &signal;
        if (connection.poisoned) {
            release();
            throw MutationExecutorError::lock(resource,
                                              "poisoned lock: another task failed inside");
        }
        if (!connection.active && (!connection.first || connection.first == this)) {
            unlink();
            connection.active = this;
            owned = true;
            return true;
        }
        if (!queued) {
            previous = connection.last;
            if (previous) {
                previous->next = this;
            } else {
                connection.first = this;
            }
            connection.last = this;
            queued = true;
        }
        return false;
    }

    void release() noexcept {
        if (!owned && !queued) {
            return;
        }
        unlink();
        if (owned) {
            connection.active = nullptr;
            owned = false;
        }
        wake = nullptr;
        if (!connection.active && connection.first) {
            connection.first->wake->notify();
        }
    }
};

struct SqliteProviderState {
    using allocator_type = Allocator;
    Memory resource;
    SqliteConnectionState& connection;
    Vector<LayoutEntry> layouts;
    std::size_t count = 0;

    SqliteProviderState(std::allocator_arg_t, Allocator a, SqliteConnectionState& connection)
        : resource(a.resource()), connection(connection), layouts(resource) {
        if (resource != connection.resource) {
            throw std::invalid_argument(
                "SQLite provider and connection must share their owning Context");
        }
        layouts.reserve(cache_capacity);
        for (std::size_t i = 0; i < cache_capacity; ++i) {
            layouts.emplace_back();
        }
    }

    const SqliteRowLayout&
    layout(Memory destination, std::string_view sql, sqlite3_stmt* statement) {
        const SqliteRowLayout* found = nullptr;
        for (std::size_t i = 0; i < count; ++i) {
            if (layouts[i].key == sql) {
                found = layouts[i].layout;
                break;
            }
        }
        if (!found) {
            auto& arena = async_arena(resource);
            found = statement ? &arena.make<SqliteRowLayout>(*statement)
                              : &arena.make<SqliteRowLayout>();
            if (count == cache_capacity) {
                count = 0;
            }
            auto& entry = layouts[count];
            entry.key = sql;
            entry.layout = found;
            ++count;
        }
        if (destination == resource) {
            return *found;
        }
        return async_arena(destination).make<SqliteRowLayout>(*found);
    }
};

struct SqliteSchemaCall {
    using allocator_type = Allocator;
    Memory resource;
    const UserContext& context;
    const SqliteDialect* dialect = nullptr;
    SqliteProviderState* state = nullptr;
    Vector<const EntityDescriptor*> entities;
    std::optional<SqliteLease> lease;

    SqliteSchemaCall(std::allocator_arg_t, Allocator a, const UserContext& context)
        : resource(a.resource()), context(context), entities(resource) {}

    std::optional<RuntimeSchemaResult> poll(WakeSignal& wake) {
        try {
            if (!lease) {
                dialect = context.get_resource<SqliteDialect>();
                if (!dialect) {
                    throw MutationExecutorError::bind(resource,
                                                      "missing typed resource: SqliteDialect");
                }
                auto* executor = context.get_resource<SqliteMutationExecutor>();
                if (!executor) {
                    throw MutationExecutorError::bind(
                        resource, "missing typed resource: SqliteMutationExecutor");
                }
                entities = context.all_entities();
                state = &executor->state_;
                state->connection.require_borrow(resource);
                lease.emplace(resource, state->connection);
            }
            if (!lease->acquire(wake)) {
                return {};
            }
            ensure_sqlite_physical_schema(
                resource, *state->connection.db, *dialect, entities, context);
            lease->release();
            state->connection.flush();
            state->count = 0;
            if (!context.initial_graphs().empty() || !context.root_graphs().empty()) {
                throw MutationExecutorError::bind(
                    resource,
                    "generated root/constant bootstrap must use the typed RuntimeModule callback");
            }
            return RuntimeSchemaResult::ok(resource, {});
        } catch (MutationExecutorError& e) {
            return RuntimeSchemaResult::err(resource,
                                            RuntimeError::schema(resource, e.to_string(resource)));
        } catch (...) {
            if (lease && lease->owned) {
                state->connection.poisoned = true;
            }
            throw;
        }
    }

    void finish_and_wait() noexcept {
        lease.reset();
    }

    void cancel_and_wait() noexcept {
        lease.reset();
    }
};

struct SqliteCursor {
    Memory resource;
    SqliteProviderState& provider;
    SqliteLease lease;
    sqlite3_stmt* statement = nullptr;
    StatementEntry* entry = nullptr;
    const SqliteRowLayout* layout = nullptr;

    SqliteCursor(Memory r, SqliteProviderState& provider)
        : resource(r), provider(provider), lease(r, provider.connection) {}

    ~SqliteCursor() {
        close();
    }

    void close() noexcept {
        provider.connection.release_statement(statement, entry);
        lease.release();
    }

    void prepare(const CompiledQuery& query, bool cached) {
        auto sql = query.sql_with_comment(resource);
        provider.connection.prepare(resource, sql, cached, statement, entry);
        if (cached) {
            layout = &provider.layout(resource, query.sql, statement);
        }
    }

    void bind(std::span<const SqliteBoundValue> values) {
        if (!statement) {
            if (!values.empty()) {
                throw MutationExecutorError::sqlite(
                    resource, 0, "Wrong number of parameters passed to query. Got 1, needed 0");
            }
            return;
        }
        auto result = bind_statement(resource, *statement, values);
        if (!result) {
            throw std::move(result).error();
        }
    }

    bool next() {
        auto code = sqlite3_step(statement);
        if (sqlite3_get_autocommit(provider.connection.db)) {
            provider.connection.transaction_owner = nullptr;
        }
        if (code == SQLITE_ROW) {
            return true;
        }
        if (code == SQLITE_DONE) {
            return false;
        }
        throw native_error(resource, provider.connection.db, code);
    }

    CompactRow row() {
        auto result = decode_sqlite_row(resource, *statement, *layout);
        if (!result) {
            throw std::move(result).error();
        }
        return std::move(result).value();
    }

    void reset() {
        auto code = sqlite3_reset(statement);
        if (code != SQLITE_OK) {
            throw native_error(resource, provider.connection.db, code);
        }
        sqlite3_clear_bindings(statement);
    }
};

// ID allocation reads native INTEGERs strictly, without Value's declaration or
// text inference. Keep one connection lease across the read/CAS write pair.
struct SqliteIdAccess {
    Memory resource;
    SqliteCursor cursor;
    WakeSignal wake;

    SqliteIdAccess(Memory r, SqliteProviderState& state) : resource(r), cursor(r, state) {
        if (!cursor.lease.acquire(wake)) {
            throw MutationExecutorError::lock(
                r, "connection is borrowed by an open SQLite stream; use the pollable transport");
        }
    }

    void prepare(std::string_view sql, std::span<const Value> values) {
        cursor.provider.connection.release_statement(cursor.statement, cursor.entry);
        CompiledQuery query(std::allocator_arg, Allocator(resource));
        query.sql = sql;
        for (const auto& value : values) {
            query.params.emplace_back(value);
        }
        cursor.prepare(query, false);
        auto bound = bind_values(resource, query.params);
        if (!bound) {
            throw std::move(bound).error();
        }
        cursor.bind(bound.value());
    }

    std::int64_t integer(int index) {
        auto type = sqlite3_column_type(cursor.statement, index);
        if (type != SQLITE_INTEGER) {
            String message("Invalid column type ", resource);
            message += type == SQLITE_FLOAT  ? "Real"
                       : type == SQLITE_TEXT ? "Text"
                       : type == SQLITE_BLOB ? "Blob"
                                             : "Null";
            message += " at index: ";
            append_number(message, index);
            message += ", name: ";
            message += sqlite3_column_name(cursor.statement, index);
            throw MutationExecutorError::sqlite(resource, 0, message);
        }
        return sqlite3_column_int64(cursor.statement, index);
    }

    std::optional<std::int64_t> current(std::string_view sql, std::span<const Value> values) {
        prepare(sql, values);
        if (!cursor.next()) {
            return {};
        }
        return integer(0);
    }

    std::optional<std::pair<std::int64_t, std::int64_t>>
    current_business(std::string_view sql, std::span<const Value> values) {
        prepare(sql, values);
        if (!cursor.next()) {
            return {};
        }
        auto level = integer(0);
        auto version = integer(1);
        return std::pair(level, version);
    }

    std::uint64_t execute(std::string_view sql, std::span<const Value> values) {
        prepare(sql, values);
        if (cursor.next()) {
            throw MutationExecutorError::sqlite(
                resource, 0, "Execute returned results - did you mean to call query?");
        }
        cursor.reset();
        return std::uint64_t(sqlite3_changes64(cursor.provider.connection.db));
    }
};

template <class T>
struct SqliteCall {
    using allocator_type = Allocator;
    using Output = Result<T, MutationExecutorError>;
    Memory resource;
    CompiledQuery query;
    SqliteCursor cursor;
    std::optional<SqliteBindings> bindings;

    SqliteCall(std::allocator_arg_t,
               Allocator a,
               SqliteProviderState& state,
               const CompiledQuery& query)
        : resource(a.resource()), query(std::allocator_arg, a, query), cursor(resource, state) {}

    std::optional<Output> poll(WakeSignal& wake) {
        try {
            if (!bindings) {
                auto result = bind_values(resource, query.params);
                if (!result) {
                    return Output::err(resource, std::move(result).error());
                }
                bindings.emplace(std::move(result).value());
            }
            if (!cursor.lease.acquire(wake)) {
                return {};
            }
            cursor.prepare(query, !std::is_same_v<T, std::uint64_t>);
            cursor.bind(*bindings);
            if constexpr (std::is_same_v<T, std::uint64_t>) {
                if (cursor.next()) {
                    return Output::err(
                        resource,
                        MutationExecutorError::sqlite(
                            resource, 0, "Execute returned results - did you mean to call query?"));
                }
                cursor.reset();
                return Output::ok(resource,
                                  std::uint64_t(sqlite3_changes64(cursor.provider.connection.db)));
            } else {
                T rows(resource);
                while (cursor.next()) {
                    rows.push_back(cursor.row());
                }
                return Output::ok(resource, std::move(rows));
            }
        } catch (MutationExecutorError& error) {
            return Output::err(resource, std::move(error));
        } catch (...) {
            if (cursor.lease.owned) {
                cursor.provider.connection.poisoned = true;
            }
            throw;
        }
    }

    void finish_and_wait() noexcept {
        cursor.close();
    }

    void cancel_and_wait() noexcept {
        cursor.close();
    }
};

struct SqliteRepeatedCall {
    using allocator_type = Allocator;
    using Rows = Vector<CompactRow>;
    using Output = Result<Rows, MutationExecutorError>;
    Memory resource;
    CompiledQuery query;
    SqliteCursor cursor;
    std::size_t index;
    Vector<Value> values;

    SqliteRepeatedCall(std::allocator_arg_t,
                       Allocator a,
                       SqliteProviderState& state,
                       const CompiledQuery& query,
                       std::size_t index,
                       std::span<const Value> values)
        : resource(a.resource()), query(std::allocator_arg, a, query), cursor(resource, state),
          index(index), values(values.begin(), values.end(), resource) {}

    std::optional<Output> poll(WakeSignal& wake) {
        try {
            if (!cursor.lease.acquire(wake)) {
                return {};
            }
            cursor.prepare(query, true);
            Rows rows(resource);
            for (const auto& value : values) {
                query.params.at(index) = Value(resource, value);
                auto bindings = bind_values(resource, query.params);
                if (!bindings) {
                    return Output::err(resource, std::move(bindings).error());
                }
                cursor.bind(bindings.value());
                while (cursor.next()) {
                    rows.push_back(cursor.row());
                }
                cursor.reset();
            }
            return Output::ok(resource, std::move(rows));
        } catch (MutationExecutorError& error) {
            return Output::err(resource, std::move(error));
        } catch (...) {
            if (cursor.lease.owned) {
                cursor.provider.connection.poisoned = true;
            }
            throw;
        }
    }

    void finish_and_wait() noexcept {
        cursor.close();
    }

    void cancel_and_wait() noexcept {
        cursor.close();
    }
};

CompiledQuery transaction_query(Memory r, std::string_view sql) {
    CompiledQuery query(std::allocator_arg, Allocator(r));
    query.sql = sql;
    return query;
}

// Rust's associated Tx is a provider clone. This scoped carrier keeps that
// connection/cache borrow while making the consuming terminal API explicit.
struct SqliteNativeTransaction final : SqlTransportTransaction<MutationExecutorError> {
    using allocator_type = Allocator;
    SqliteProviderState& provider;
    Arena::Quiescence quiescence;

    SqliteNativeTransaction(std::allocator_arg_t, Allocator a, SqliteProviderState& provider)
        : SqlTransportTransaction<MutationExecutorError>(a.resource()), provider(provider) {
        quiescence.attach(async_arena(a.resource()), this, [](void* p) noexcept {
            static_cast<SqliteNativeTransaction*>(p)->abandon();
        });
    }

    ~SqliteNativeTransaction() {
        abandon();
    }

    void activate() noexcept {
        provider.connection.transaction_owner = this;
    }

    void abandon() noexcept {
        quiescence.reset();
        auto& connection = provider.connection;
        if (connection.transaction_owner != this) {
            return;
        }
        // Context teardown cannot wait for another owner-thread cursor. Modern
        // SQLite allows ROLLBACK with outstanding statements; they retain their
        // own host memory and will report any resulting native abort themselves.
        auto code = sqlite3_exec(connection.db, "ROLLBACK", nullptr, nullptr, nullptr);
        connection.transaction_owner = nullptr;
        if (code != SQLITE_OK && !sqlite3_get_autocommit(connection.db)) {
            connection.poisoned = true;
        }
    }

    struct Terminal {
        using allocator_type = Allocator;
        using Output = Result<std::monostate, MutationExecutorError>;
        Memory resource;
        SqliteNativeTransaction& owner;
        SqliteCall<std::uint64_t> call;

        Terminal(std::allocator_arg_t, Allocator a, SqliteNativeTransaction& owner, bool commit)
            : resource(a.resource()), owner(owner),
              call(std::allocator_arg,
                   a,
                   owner.provider,
                   transaction_query(resource, commit ? "COMMIT" : "ROLLBACK")) {}

        std::optional<Output> poll(WakeSignal& wake) {
            auto result = call.poll(wake);
            if (!result) {
                return {};
            }
            if (!*result) {
                return Output::err(resource, std::move(*result).error());
            }
            return Output::ok(resource, std::monostate{});
        }

        void finish_and_wait() noexcept {
            call.finish_and_wait();
        }

        void cancel_and_wait() noexcept {
            call.cancel_and_wait();
            owner.abandon();
        }
    };

protected:
    DataServiceOperationResult<Rows, MutationExecutorError>&
    fetch_all_compact_sql_transaction(Memory r, const CompiledQuery& query) override {
        return make_resource_operation<Result<Rows, MutationExecutorError>, SqliteCall<Rows>>(
            r, provider, query);
    }

    DataServiceOperationResult<Rows, MutationExecutorError>&
    fetch_repeated_compact_sql_transaction(Memory r,
                                           const CompiledQuery& query,
                                           std::size_t index,
                                           std::span<const Value> values) override {
        return make_resource_operation<Result<Rows, MutationExecutorError>, SqliteRepeatedCall>(
            r, provider, query, index, values);
    }

    DataServiceOperationResult<std::uint64_t, MutationExecutorError>&
    execute_sql_transaction(Memory r, const CompiledQuery& query) override {
        return make_resource_operation<Result<std::uint64_t, MutationExecutorError>,
                                       SqliteCall<std::uint64_t>>(r, provider, query);
    }

    DataServiceOperationResult<std::monostate, MutationExecutorError>&
    commit_sql_transaction(Memory r) override {
        return make_resource_operation<Result<std::monostate, MutationExecutorError>, Terminal>(
            r, *this, true);
    }

    DataServiceOperationResult<std::monostate, MutationExecutorError>&
    rollback_sql_transaction(Memory r) override {
        return make_resource_operation<Result<std::monostate, MutationExecutorError>, Terminal>(
            r, *this, false);
    }
};

struct SqliteBegin {
    using allocator_type = Allocator;
    using Output = Result<SqlTransactionHandle<MutationExecutorError>, MutationExecutorError>;
    Memory resource;
    SqliteNativeTransaction& transaction;
    SqliteCall<std::uint64_t> call;

    SqliteBegin(std::allocator_arg_t, Allocator a, SqliteNativeTransaction& transaction)
        : resource(a.resource()), transaction(transaction),
          call(std::allocator_arg,
               a,
               transaction.provider,
               transaction_query(resource, "BEGIN IMMEDIATE")) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto result = call.poll(wake);
        if (!result) {
            return {};
        }
        if (!*result) {
            return Output::err(resource, std::move(*result).error());
        }
        transaction.activate();
        return Output::ok(resource,
                          SqlTransactionHandle<MutationExecutorError>(
                              std::allocator_arg, Allocator(resource), transaction));
    }

    void finish_and_wait() noexcept {
        call.finish_and_wait();
    }

    void cancel_and_wait() noexcept {
        call.cancel_and_wait();
        transaction.abandon();
    }
};

struct SqliteStream {
    using allocator_type = Allocator;
    using Item = Result<StreamChunk, MutationExecutorError>;
    using Poll = std::optional<std::optional<Item>>;
    Memory resource;
    CompiledQuery query;
    SqliteCursor cursor;
    std::size_t chunk_size, index = 0;
    std::optional<SqliteBindings> bindings;
    bool started = false, terminal = false;

    SqliteStream(std::allocator_arg_t,
                 Allocator a,
                 SqliteProviderState& state,
                 CompiledQuery query,
                 std::size_t size)
        : resource(a.resource()), query(std::allocator_arg, a, std::move(query)),
          cursor(resource, state), chunk_size(size) {}

    Poll poll_next(WakeSignal& wake) {
        if (terminal) {
            cursor.close();
            return std::optional<Item>{};
        }
        try {
            if (!started) {
                if (!bindings) {
                    auto result = bind_values(resource, query.params);
                    if (!result) {
                        terminal = true;
                        return std::optional<Item>(Item::err(resource, std::move(result).error()));
                    }
                    bindings.emplace(std::move(result).value());
                }
                if (!cursor.lease.acquire(wake)) {
                    return {};
                }
                cursor.prepare(query, true);
                cursor.bind(*bindings);
                started = true;
            }
            StreamChunk chunk(std::allocator_arg, Allocator(resource), index, false);
            while (cursor.next()) {
                chunk.rows.push_back(cursor.row());
                if (chunk.rows.size() == chunk_size) {
                    ++index;
                    return std::optional<Item>(Item::ok(resource, std::move(chunk)));
                }
            }
            terminal = true;
            if (chunk.rows.empty()) {
                cursor.close();
                return std::optional<Item>{};
            }
            chunk.is_last = true;
            return std::optional<Item>(Item::ok(resource, std::move(chunk)));
        } catch (MutationExecutorError& error) {
            terminal = true;
            return std::optional<Item>(Item::err(resource, std::move(error)));
        } catch (...) {
            if (cursor.lease.owned) {
                cursor.provider.connection.poisoned = true;
            }
            throw;
        }
    }

    void finish_and_wait() noexcept {
        cursor.close();
    }

    void cancel_and_wait() noexcept {
        cursor.close();
    }
};

struct SqliteErrorStream {
    using allocator_type = Allocator;
    using Item = Result<StreamChunk, MutationExecutorError>;
    Memory resource;
    std::optional<MutationExecutorError> error;

    SqliteErrorStream(std::allocator_arg_t, Allocator a, MutationExecutorError error)
        : resource(a.resource()), error(move_in(resource, std::move(error))) {}

    std::optional<std::optional<Item>> poll_next(WakeSignal&) {
        if (!error) {
            return std::optional<Item>{};
        }
        auto item = Item::err(resource, std::move(*error));
        error.reset();
        return std::optional<Item>(std::move(item));
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

template <class T>
Result<T, MutationExecutorError>
synchronous(Memory r, SqliteProviderState& state, const CompiledQuery& query) {
    state.connection.require_owner();
    SqliteCall<T> call(std::allocator_arg, Allocator(r), state, query);
    WakeSignal wake;
    auto result = call.poll(wake);
    call.finish_and_wait();
    if (!result) {
        return Result<T, MutationExecutorError>::err(
            r,
            MutationExecutorError::lock(
                r, "connection is borrowed by an open SQLite stream; use the pollable transport"));
    }
    return std::move(*result);
}
}

namespace teaql {
AsyncOperation<RuntimeSchemaResult>&
SqliteSchemaProvider::ensure_schema(const UserContext& context, const SchemaInvocation&) const {
    return make_resource_operation<RuntimeSchemaResult, detail::SqliteSchemaCall>(
        context.memory_resource(), context);
}

SqliteConnection::SqliteConnection(std::allocator_arg_t, Allocator a, std::string_view filename)
    : state_(detail::async_arena(a.resource())
                 .make_resource<detail::SqliteConnectionState>(filename)) {}

Memory SqliteConnection::memory_resource() const noexcept {
    return state_.resource;
}

sqlite3* SqliteConnection::native_handle() const {
    state_.require_owner();
    return state_.db;
}

std::size_t SqliteConnection::cached_statement_count() const {
    state_.require_owner();
    return state_.recency.size();
}

SqliteMutationExecutor::SqliteMutationExecutor(std::allocator_arg_t,
                                               Allocator a,
                                               SqliteConnection& connection)
    : connection_(connection),
      state_(
          detail::async_arena(a.resource()).make<detail::SqliteProviderState>(connection.state_)) {}

SqliteMutationExecutor::SqliteMutationExecutor(std::allocator_arg_t,
                                               Allocator a,
                                               const SqliteMutationExecutor& other)
    : connection_(other.connection_), state_(other.state_) {
    if (a.resource() != state_.resource) {
        throw std::invalid_argument("SQLite provider clones must remain in their owning Context");
    }
}

Memory SqliteMutationExecutor::memory_resource() const noexcept {
    return state_.resource;
}

DataServiceCapabilities SqliteMutationExecutor::capabilities() const {
    DataServiceCapabilities result;
    result.query = true;
    result.mutation = true;
    result.transaction = true;
    return result;
}

Result<std::uint64_t, MutationExecutorError>
SqliteMutationExecutor::execute(Memory r, const CompiledQuery& query) {
    return detail::synchronous<std::uint64_t>(r, state_, query);
}

Result<SqliteMutationExecutor::Rows, MutationExecutorError>
SqliteMutationExecutor::fetch_all_compact(Memory r, const CompiledQuery& query) {
    return detail::synchronous<Rows>(r, state_, query);
}

Result<Vector<StreamChunk>, MutationExecutorError>
SqliteMutationExecutor::fetch_stream(Memory r, const CompiledQuery& query, std::size_t size) {
    using Output = Result<Vector<StreamChunk>, MutationExecutorError>;
    auto fetched = fetch_all_compact(r, query);
    if (!fetched) {
        return Output::err(r, std::move(fetched).error());
    }
    Vector<StreamChunk> chunks(r);
    StreamChunk current(std::allocator_arg, Allocator(r), 0, false);
    for (auto& row : fetched.value()) {
        current.rows.push_back(std::move(row));
        if (current.rows.size() >= size) {
            chunks.push_back(std::move(current));
            current = StreamChunk(std::allocator_arg, Allocator(r), chunks.size(), false);
        }
    }
    current.is_last = true;
    chunks.push_back(std::move(current));
    return Output::ok(r, std::move(chunks));
}

namespace {
Result<std::monostate, MutationExecutorError>
transaction_command(Memory r, detail::SqliteProviderState& state, std::string_view sql) {
    state.connection.require_owner();
    auto result = detail::synchronous<std::uint64_t>(r, state, detail::transaction_query(r, sql));
    if (!result) {
        return Result<std::monostate, MutationExecutorError>::err(r, std::move(result).error());
    }
    return Result<std::monostate, MutationExecutorError>::ok(r, std::monostate{});
}
}

Result<std::monostate, MutationExecutorError> SqliteMutationExecutor::begin_transaction(Memory r) {
    return transaction_command(r, state_, "BEGIN IMMEDIATE");
}

Result<std::monostate, MutationExecutorError> SqliteMutationExecutor::commit_transaction(Memory r) {
    return transaction_command(r, state_, "COMMIT");
}

Result<std::monostate, MutationExecutorError>
SqliteMutationExecutor::rollback_transaction(Memory r) {
    return transaction_command(r, state_, "ROLLBACK");
}

DataServiceOperationResult<SqlTransactionHandle<MutationExecutorError>, MutationExecutorError>&
SqliteMutationExecutor::begin_sql(Memory r) {
    state_.connection.require_borrow(r);
    auto& transaction =
        detail::async_arena(r).make_resource<detail::SqliteNativeTransaction>(state_);
    return make_resource_operation<
        Result<SqlTransactionHandle<MutationExecutorError>, MutationExecutorError>,
        detail::SqliteBegin>(r, transaction);
}

DataServiceOperationResult<std::uint64_t, MutationExecutorError>&
SqliteMutationExecutor::execute_sql(Memory r, const CompiledQuery& query) {
    state_.connection.require_borrow(r);
    return make_resource_operation<Result<std::uint64_t, MutationExecutorError>,
                                   detail::SqliteCall<std::uint64_t>>(r, state_, query);
}

DataServiceOperationResult<SqliteMutationExecutor::Rows, MutationExecutorError>&
SqliteMutationExecutor::fetch_all_compact_sql(Memory r, const CompiledQuery& query) {
    state_.connection.require_borrow(r);
    return make_resource_operation<Result<Rows, MutationExecutorError>, detail::SqliteCall<Rows>>(
        r, state_, query);
}

DataServiceOperationResult<SqliteMutationExecutor::Rows, MutationExecutorError>&
SqliteMutationExecutor::fetch_repeated_compact_sql(Memory r,
                                                   const CompiledQuery& query,
                                                   std::size_t index,
                                                   std::span<const Value> values) {
    state_.connection.require_borrow(r);
    return make_resource_operation<Result<Rows, MutationExecutorError>, detail::SqliteRepeatedCall>(
        r, state_, query, index, values);
}

QueryStream<MutationExecutorError>&
SqliteMutationExecutor::stream_sql(Memory r, CompiledQuery query, std::size_t size) {
    state_.connection.require_borrow(r);
    return make_resource_stream<Result<StreamChunk, MutationExecutorError>, detail::SqliteStream>(
        r, state_, std::move(query), size);
}

QueryStream<MutationExecutorError>&
SqliteMutationExecutor::query_stream(Memory r, QueryRequest request, std::size_t size) {
    state_.connection.require_borrow(r);
    try {
        SqliteDialect dialect;
        EntityDescriptor entity(std::allocator_arg, Allocator(r), request.query.entity);
        return stream_sql(r, dialect.compile_select(r, entity, request.query), size);
    } catch (SqlCompileError& error) {
        return make_stream<Result<StreamChunk, MutationExecutorError>, detail::SqliteErrorStream>(
            r, MutationExecutorError::sql_compile(r, std::move(error)));
    }
}

Result<std::monostate, MutationExecutorError>
SqliteMutationExecutor::ensure_id_space_table(Memory r, std::string_view table_name) {
    state_.connection.require_owner();
    String sql("CREATE TABLE IF NOT EXISTS ", r);
    sql += quote_identifier_if_needed(r, table_name, '"');
    sql += " (type_name VARCHAR(100) PRIMARY KEY, current_level BIGINT NOT NULL)";
    return transaction_command(r, state_, sql);
}

SqliteIdSpaceGenerator::SqliteIdSpaceGenerator(std::allocator_arg_t,
                                               Allocator a,
                                               SqliteMutationExecutor& executor)
    : executor_(executor), table_name_(DEFAULT_ID_SPACE_TABLE, a.resource()) {
    executor_.state_.connection.require_borrow(a.resource());
}

SqliteIdSpaceGenerator::SqliteIdSpaceGenerator(std::allocator_arg_t,
                                               Allocator a,
                                               const SqliteIdSpaceGenerator& other)
    : SqliteIdSpaceGenerator(std::allocator_arg, a, other.executor_) {
    table_name_ = other.table_name_;
}

namespace {
bool retryable_id_lock(const MutationExecutorError& error) {
    auto code = error.sqlite_extended_code() & 0xff;
    return error.kind() == MutationExecutorError::Kind::Sqlite &&
           (code == SQLITE_BUSY || code == SQLITE_LOCKED);
}

String
id_message(Memory r, std::string_view prefix, std::string_view entity, std::string_view table) {
    String message(prefix, r);
    message += entity;
    message += " in table ";
    message += table;
    return message;
}
}

Result<std::monostate, MutationExecutorError> SqliteIdSpaceGenerator::ensure_table(Memory r) const {
    using Output = Result<std::monostate, MutationExecutorError>;
    for (unsigned attempt = 0; attempt < 100; ++attempt) {
        auto result = executor_.ensure_id_space_table(r, table_name_);
        if (result) {
            return result;
        }
        if (!retryable_id_lock(result.error())) {
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    String message("SQLite ID provider was unable to ensure ID-space table ", r);
    message += table_name_;
    message += " after 100 lock-contention attempts";
    return Output::err(r, MutationExecutorError::bind(r, message));
}

Result<std::uint64_t, MutationExecutorError>
SqliteIdSpaceGenerator::next_id(Memory r, std::string_view name) const {
    using Output = Result<std::uint64_t, MutationExecutorError>;
    executor_.state_.connection.require_owner();
    auto entity = canonical_id_space_entity(r, name);
    auto ensured = ensure_table(r);
    if (!ensured) {
        return Output::err(r, std::move(ensured).error());
    }
    auto table = quote_identifier_if_needed(r, table_name_, '"');
    String select("SELECT current_level FROM ", r);
    select += table;
    select += " WHERE type_name = ?";
    String insert("INSERT INTO ", r);
    insert += table;
    insert += " (type_name, current_level) VALUES (?, 1)";
    String update("UPDATE ", r);
    update += table;
    update += " SET current_level = ? WHERE type_name = ? AND current_level = ?";
    Vector<Value> key(r);
    key.emplace_back(entity);
    for (unsigned attempt = 1; attempt <= 100; ++attempt) {
        try {
            detail::SqliteIdAccess access(r, executor_.state_);
            auto current = access.current(select, key);
            if (current) {
                if (*current == INT64_MAX) {
                    auto message = id_message(
                        r, "SQLite ID provider overflow for ID space ", entity, table_name_);
                    message += " on optimistic-lock attempt ";
                    detail::append_number(message, attempt);
                    return Output::err(r, MutationExecutorError::bind(r, message));
                }
                auto next = *current + 1;
                Vector<Value> params(r);
                params.emplace_back(next);
                params.emplace_back(entity);
                params.emplace_back(*current);
                if (access.execute(update, params) == 1) {
                    if (next < 0) {
                        String message("SQLite ID provider generated id ", r);
                        detail::append_number(message, next);
                        message += " for ID space ";
                        message += entity;
                        message += " in table ";
                        message += table_name_;
                        message += " that cannot be represented as u64";
                        return Output::err(r, MutationExecutorError::bind(r, message));
                    }
                    return Output::ok(r, std::uint64_t(next));
                }
            } else {
                try {
                    auto changed = access.execute(insert, key);
                    if (changed == 1) {
                        return Output::ok(r, std::uint64_t(1));
                    }
                    auto message = id_message(
                        r, "SQLite ID provider insert for ID space ", entity, table_name_);
                    message += " changed ";
                    detail::append_number(message, changed);
                    message += " rows on optimistic-lock attempt ";
                    detail::append_number(message, attempt);
                    return Output::err(r, MutationExecutorError::bind(r, message));
                } catch (MutationExecutorError& error) {
                    if (error.kind() != MutationExecutorError::Kind::Sqlite ||
                        (error.sqlite_extended_code() & 0xff) != SQLITE_CONSTRAINT) {
                        throw;
                    }
                }
            }
        } catch (MutationExecutorError& error) {
            if (!retryable_id_lock(error)) {
                return Output::err(r, std::move(error));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto message =
        id_message(r, "SQLite ID provider was unable to allocate ID space ", entity, table_name_);
    message += " after 100 optimistic-lock attempts";
    return Output::err(r, MutationExecutorError::bind(r, message));
}

Result<std::monostate, MutationExecutorError>
SqliteIdSpaceGenerator::ensure_floor(Memory r, std::string_view name, std::uint64_t floor) const {
    using Output = Result<std::monostate, MutationExecutorError>;
    executor_.state_.connection.require_owner();
    auto entity = canonical_id_space_entity(r, name);
    auto ensured = ensure_table(r);
    if (!ensured) {
        return ensured;
    }
    if (floor > std::uint64_t(INT64_MAX)) {
        String message("SQLite ID provider floor ", r);
        detail::append_number(message, floor);
        message += " for ID space ";
        message += entity;
        message += " in table ";
        message += table_name_;
        message += " exceeds i64";
        return Output::err(r, MutationExecutorError::bind(r, message));
    }
    auto table = quote_identifier_if_needed(r, table_name_, '"');
    String select("SELECT current_level FROM ", r);
    select += table;
    select += " WHERE type_name = ?";
    String insert("INSERT INTO ", r);
    insert += table;
    insert += "(type_name, current_level) VALUES (?, ?)";
    String update("UPDATE ", r);
    update += table;
    update += " SET current_level = ? WHERE type_name = ? AND current_level = ?";
    Vector<Value> key(r);
    key.emplace_back(entity);
    try {
        for (unsigned attempt = 0; attempt < 100; ++attempt) {
            detail::SqliteIdAccess access(r, executor_.state_);
            auto current = access.current(select, key);
            Vector<Value> params(r);
            if (current) {
                if (*current >= std::int64_t(floor)) {
                    return Output::ok(r, std::monostate{});
                }
                params.emplace_back(std::int64_t(floor));
                params.emplace_back(entity);
                params.emplace_back(*current);
                if (access.execute(update, params) == 1) {
                    return Output::ok(r, std::monostate{});
                }
            } else {
                params.emplace_back(entity);
                params.emplace_back(std::int64_t(floor));
                try {
                    if (access.execute(insert, params) == 1) {
                        return Output::ok(r, std::monostate{});
                    }
                } catch (MutationExecutorError& error) {
                    if (error.kind() != MutationExecutorError::Kind::Sqlite ||
                        (error.sqlite_extended_code() & 0xff) != SQLITE_CONSTRAINT) {
                        throw;
                    }
                }
            }
        }
    } catch (MutationExecutorError& error) {
        return Output::err(r, std::move(error));
    }
    auto message = id_message(
        r, "SQLite ID provider was unable to synchronize floor for ID space ", entity, table_name_);
    message += " after 100 optimistic-lock attempts";
    return Output::err(r, MutationExecutorError::bind(r, message));
}

SqliteBusinessIdAllocator::SqliteBusinessIdAllocator(std::allocator_arg_t,
                                                     Allocator a,
                                                     SqliteMutationExecutor& executor)
    : executor_(executor), table_name_(DEFAULT_BUSINESS_ID_SPACE_TABLE, a.resource()) {
    executor_.state_.connection.require_borrow(a.resource());
}

SqliteBusinessIdAllocator::SqliteBusinessIdAllocator(std::allocator_arg_t,
                                                     Allocator a,
                                                     const SqliteBusinessIdAllocator& other)
    : SqliteBusinessIdAllocator(std::allocator_arg, a, other.executor_) {
    table_name_ = other.table_name_;
}

namespace {
BusinessIdError business_sqlite_error(Memory r, const MutationExecutorError& error) {
    return BusinessIdError(
        std::allocator_arg, Allocator(r), BusinessIdErrorCode::Allocation, error.to_string(r));
}

BusinessIdError business_error(Memory r,
                               BusinessIdErrorCode code,
                               std::string_view prefix,
                               std::string_view scope) {
    String message(prefix, r);
    message += scope;
    return BusinessIdError(std::allocator_arg, Allocator(r), code, message);
}
}

Result<std::monostate, BusinessIdError> SqliteBusinessIdAllocator::ensure_schema(Memory r) const {
    using Output = Result<std::monostate, BusinessIdError>;
    executor_.state_.connection.require_owner();
    String sql("CREATE TABLE IF NOT EXISTS ", r);
    sql += quote_identifier_if_needed(r, table_name_, '"');
    sql +=
        " (scope_key VARCHAR(512) PRIMARY KEY, current_level INTEGER NOT NULL, version INTEGER NOT NULL)";
    auto result = transaction_command(r, executor_.state_, sql);
    if (!result) {
        return Output::err(r, business_sqlite_error(r, result.error()));
    }
    return Output::ok(r, std::monostate{});
}

Result<BusinessIdAllocation, BusinessIdError>
SqliteBusinessIdAllocator::allocate(Memory r, const BusinessIdPlan& plan) const {
    using Output = Result<BusinessIdAllocation, BusinessIdError>;
    executor_.state_.connection.require_borrow(r);
    auto valid = plan.validate_allocation_range();
    if (!valid) {
        return Output::err(r, detail::clone_in(r, valid.error()));
    }
    auto table = quote_identifier_if_needed(r, table_name_, '"');
    auto scope = plan.scope.canonical_key(r);
    String select("SELECT current_level, version FROM ", r);
    select += table;
    select += " WHERE scope_key = ?";
    String insert("INSERT INTO ", r);
    insert += table;
    insert += " (scope_key, current_level, version) VALUES (?, ?, 1)";
    String update("UPDATE ", r);
    update += table;
    update += " SET current_level = ?, version = version + 1 WHERE scope_key = ? AND version = ?";
    Vector<Value> key(r);
    key.emplace_back(scope);
    for (unsigned attempt = 1; attempt <= 100; ++attempt) {
        std::optional<BusinessIdError> failure;
        try {
            detail::SqliteIdAccess access(r, executor_.state_);
            // Retain cleanup only for a transaction actually started here. A
            // failed nested BEGIN must never roll back somebody else's save.
            auto& cleanup = detail::async_arena(r).make_resource<detail::SqliteNativeTransaction>(
                executor_.state_);
            access.execute("BEGIN IMMEDIATE", {});
            cleanup.activate();
            std::optional<std::uint64_t> sequence;
            try {
                sequence = [&]() -> std::optional<std::uint64_t> {
                    auto current = access.current_business(select, key);
                    std::uint64_t next;
                    if (current) {
                        if (current->first == INT64_MAX) {
                            throw business_error(r,
                                                 BusinessIdErrorCode::Exhausted,
                                                 "business ID sequence overflow for scope ",
                                                 scope);
                        }
                        auto candidate = current->first + 1;
                        Vector<Value> params(r);
                        params.emplace_back(candidate);
                        params.emplace_back(scope);
                        params.emplace_back(current->second);
                        if (access.execute(update, params) != 1) {
                            return {};
                        }
                        if (candidate < 0) {
                            throw business_error(r,
                                                 BusinessIdErrorCode::Allocation,
                                                 "negative business ID sequence stored by SQLite",
                                                 {});
                        }
                        next = std::uint64_t(candidate);
                    } else {
                        if (plan.initial_sequence > std::uint64_t(INT64_MAX)) {
                            throw business_error(
                                r,
                                BusinessIdErrorCode::InvalidDefinition,
                                "business ID initial sequence exceeds SQLite INTEGER",
                                {});
                        }
                        Vector<Value> params(r);
                        params.emplace_back(scope);
                        params.emplace_back(std::int64_t(plan.initial_sequence));
                        access.execute(insert, params);
                        next = plan.initial_sequence;
                    }
                    if (next > plan.max_sequence) {
                        throw business_error(r,
                                             BusinessIdErrorCode::Exhausted,
                                             "business ID sequence exhausted for scope ",
                                             scope);
                    }
                    return next;
                }();
            } catch (...) {
                try {
                    access.execute("ROLLBACK", {});
                } catch (...) {
                }
                throw;
            }
            // Source commit failures preserve SQLite's transaction state. The
            // Context cleanup above reclaims it if the caller cannot recover.
            access.execute("COMMIT", {});
            if (sequence) {
                return Output::ok(
                    r,
                    BusinessIdAllocation(std::allocator_arg, Allocator(r), plan.scope, *sequence));
            }
        } catch (BusinessIdError& error) {
            failure.emplace(std::move(error));
        } catch (MutationExecutorError& error) {
            failure.emplace(business_sqlite_error(r, error));
        }
        if (failure && failure->message.find("locked") == String::npos &&
            failure->message.find("busy") == String::npos) {
            return Output::err(r, std::move(*failure));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    String message("unable to allocate business ID scope ", r);
    message += scope;
    message += " after 100 attempts";
    return Output::err(
        r,
        BusinessIdError(
            std::allocator_arg, Allocator(r), BusinessIdErrorCode::Allocation, message));
}

void SqliteMutationExecutor::clear_query_caches() {
    state_.connection.require_owner();
    if (state_.connection.active) {
        throw std::logic_error("Cannot clear SQLite caches with an open cursor");
    }
    state_.connection.flush();
    state_.count = 0;
}

std::size_t SqliteMutationExecutor::cached_column_layout_count() const {
    state_.connection.require_owner();
    return state_.count;
}

Result<bool, MutationExecutorError>
SqliteMutationExecutor::table_exists(Memory r, std::string_view table) const {
    using R = Result<bool, MutationExecutorError>;
    try {
        detail::SqliteIdAccess access(r, state_);
        return R::ok(r, detail::sqlite_table_exists(r, *state_.connection.db, table));
    } catch (MutationExecutorError& e) {
        return R::err(r, std::move(e));
    }
}

Result<SqliteMutationExecutor::TableColumns, MutationExecutorError>
SqliteMutationExecutor::table_columns(Memory r, std::string_view table) const {
    using R = Result<TableColumns, MutationExecutorError>;
    try {
        detail::SqliteIdAccess access(r, state_);
        return R::ok(r, detail::sqlite_table_columns(r, *state_.connection.db, table));
    } catch (MutationExecutorError& e) {
        return R::err(r, std::move(e));
    }
}
}
