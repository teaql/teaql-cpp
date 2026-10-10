#pragma once
#include <teaql/compatibility/legacy_sqlite_compiler.hpp>
#include <teaql/runtime/context.hpp>
#include <chrono>
#include <span>
#include <stdexcept>

struct sqlite3;

namespace teaql {
using Rows = Vector<Record>;
enum class LegacySqlLogOperation { Schema, Select, Insert, Update, Delete, Recover };

struct LegacySqlLogEntry {
    String sql, purpose, comment, audit_reason;
    Vector<Value> params; // Safe log projection only; never execution bindings.
    LegacySqlLogOperation operation;
    SqlExecutionOutcome outcome;
    std::uint64_t count;
    std::chrono::nanoseconds elapsed;

    LegacySqlLogEntry(Memory r,
                      LegacySqlLogOperation op,
                      SqlExecutionOutcome outcome,
                      std::uint64_t count,
                      std::chrono::nanoseconds elapsed)
        : sql(r), purpose(r), comment(r), audit_reason(r), params(r), operation(op),
          outcome(outcome), count(count), elapsed(elapsed) {}
};

class SqliteError : public std::runtime_error {
public:
    int code;

    explicit SqliteError(int code) : std::runtime_error("SQLite execution failed"), code(code) {}
};

class OptimisticLockError : public std::runtime_error {
public:
    OptimisticLockError() : std::runtime_error("Optimistic lock conflict") {}
};

// Context-owned native SQLite resource. No connection pool, background executor or global state.
class SqliteSession final {
public:
    explicit SqliteSession(UserContext& context,
                           std::span<const EntityDescriptor* const> descriptors,
                           const char* path = ":memory:");
    ~SqliteSession() noexcept;
    SqliteSession(const SqliteSession&) = delete;
    SqliteSession& operator=(const SqliteSession&) = delete;
    void ensure_schema();
    Rows& query(const SelectQuery&);
    std::uint64_t execute(const InsertCommand&);
    std::uint64_t execute(const UpdateCommand&);
    std::uint64_t execute(const DeleteCommand&);
    std::uint64_t execute(const RecoverCommand&);

    const Vector<LegacySqlLogEntry>& sql_logs() const noexcept {
        return logs_;
    }

    void enable_sql_log(bool value) noexcept {
        logging_ = value;
    }

    // Transaction covers DB changes; statement logs still describe statement outcomes.
    template <class F>
    void transaction(F&& body) {
        begin();
        try {
            std::forward<F>(body)();
            commit();
        } catch (...) {
            rollback();
            throw;
        }
    }

private:
    UserContext& context_;
    sqlite3* db_ = nullptr;
    Vector<const EntityDescriptor*> descriptors_;
    Vector<LegacySqlLogEntry> logs_;
    bool logging_ = true, in_transaction_ = false;
    const EntityDescriptor& descriptor(std::string_view) const;
    std::uint64_t execute_compiled(const CompiledQuery&,
                                   LegacySqlLogOperation,
                                   const TraceChain&,
                                   bool check_version);
    void log(const CompiledQuery&,
             LegacySqlLogOperation,
             SqlExecutionOutcome,
             std::uint64_t,
             std::chrono::steady_clock::time_point,
             std::string_view purpose = {},
             std::string_view comment = {},
             std::string_view audit = {});
    void begin();
    void commit();
    void rollback() noexcept;
};
}
