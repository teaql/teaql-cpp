#include <teaql/runtime/sqlite.hpp>
#include <sqlite3.h>
#include <algorithm>
#include <climits>
#include <charconv>
#include <cmath>
#include <limits>

namespace teaql {
namespace {
using Clock = std::chrono::steady_clock;
struct Statement {
    sqlite3_stmt* stmt = nullptr;
    Statement(sqlite3* db, const CompiledQuery& query) {
        if (query.sql.size() >= std::size_t(INT_MAX)) throw std::length_error("SQL too large");
        int rc = sqlite3_prepare_v2(db, query.sql.c_str(), int(query.sql.size()), &stmt, nullptr);
        if (rc != SQLITE_OK) { sqlite3_finalize(stmt); stmt = nullptr; throw SqliteError(rc); }
        try {
            if (sqlite3_bind_parameter_count(stmt) != int(query.params.size())) throw std::invalid_argument("Parameter count mismatch");
            for (std::size_t i = 0; i < query.params.size(); ++i) {
                const int slot = int(i + 1);
                std::visit([&](const auto& value) {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::same_as<T, std::monostate> || std::same_as<T, TypedNull>) rc = sqlite3_bind_null(stmt, slot);
                    else if constexpr (std::same_as<T, String>) rc = sqlite3_bind_text64(stmt, slot, value.data(), value.size(), SQLITE_TRANSIENT, SQLITE_UTF8);
                    else if constexpr (std::same_as<T, double>) {
                        if (!std::isfinite(value)) throw std::invalid_argument("Non-finite SQL value");
                        rc = sqlite3_bind_double(stmt, slot, value);
                    } else {
                        if constexpr (std::same_as<T, std::uint64_t>) if (value > std::uint64_t(INT64_MAX)) throw std::overflow_error("SQLite integer out of range");
                        rc = sqlite3_bind_int64(stmt, slot, sqlite3_int64(value));
                    }
                }, query.params[i].data);
                if (rc != SQLITE_OK) throw SqliteError(rc);
            }
        } catch (...) { sqlite3_finalize(stmt); stmt = nullptr; throw; }
    }
    ~Statement() { sqlite3_finalize(stmt); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
};
std::string_view intent(const TraceChain& trace, TraceKind kind) {
    for (auto it = trace.rbegin(); it != trace.rend(); ++it) if (it->kind == kind) return it->comment;
    return {};
}
bool blank(std::string_view text) { return text.find_first_not_of(" \t\r\n") == std::string_view::npos; }
void scrub(String& text, std::string_view secret) {
    if (secret.empty()) return;
    std::size_t pos = 0;
    while ((pos = text.find(secret, pos)) != String::npos) { text.replace(pos, secret.size(), "[REDACTED]"); pos += 10; }
}
String scalar_text(Memory r, const Value& value) {
    return std::visit([&](const auto& v) -> String {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::same_as<T, String>) return String(v, r);
        else if constexpr (std::same_as<T, std::monostate> || std::same_as<T, TypedNull>) return String(r);
        else if constexpr (std::same_as<T, bool>) return String(v ? "true" : "false", r);
        else { char buffer[64]; auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), v); if (error != std::errc{}) throw std::runtime_error("Cannot format scalar"); return String(buffer, end, r); }
    }, value.data);
}
void exec_control(sqlite3* db, const char* sql) {
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) throw SqliteError(rc);
}
}
SqliteSession::SqliteSession(UserContext& context, std::span<const EntityDescriptor* const> descriptors, const char* path)
    : context_(context), descriptors_(descriptors.begin(), descriptors.end(), context.memory_resource()), logs_(context.memory_resource()) {
    if (context.closed()) throw std::logic_error("UserContext is closed");
    int rc = sqlite3_open_v2(path, &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr);
    if (rc != SQLITE_OK) { sqlite3_close(db_); db_ = nullptr; throw SqliteError(rc); }
    sqlite3_extended_result_codes(db_, 1);
}
SqliteSession::~SqliteSession() noexcept { rollback(); if (db_) sqlite3_close_v2(db_); }
const EntityDescriptor& SqliteSession::descriptor(std::string_view name) const {
    for (const auto* entity : descriptors_) if (entity->name == name) return *entity;
    throw std::invalid_argument("Unknown entity");
}
void SqliteSession::log(const CompiledQuery& query, SqlLogOperation operation, SqlExecutionOutcome outcome,
                       std::uint64_t count, Clock::time_point start, std::string_view purpose,
                       std::string_view comment, std::string_view audit) {
    if (!logging_) return;
    auto r = context_.memory_resource();
    logs_.emplace_back(r, operation, outcome, count, Clock::now() - start);
    auto& entry = logs_.back(); entry.sql = query.sql; entry.purpose = purpose; entry.comment = comment; entry.audit_reason = audit;
    Vector<String> secrets(r);
    for (std::size_t i = 0; i < query.params.size(); ++i) {
        auto p = query.parameter_policies.size() == query.params.size() ? query.parameter_policies[i] : SqlParameterLogPolicy::Unknown;
        if (p == SqlParameterLogPolicy::Plain || query.params[i].is_null()) entry.params.emplace_back(query.params[i]);
        else {
            secrets.push_back(scalar_text(r, query.params[i]));
            entry.params.emplace_back("[REDACTED]");
        }
    }
    std::sort(secrets.begin(), secrets.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });
    for (const auto& secret : secrets) { scrub(entry.purpose, secret); scrub(entry.comment, secret); scrub(entry.audit_reason, secret); }
}
void SqliteSession::ensure_schema() {
    for (const auto* entity : descriptors_) {
        const auto& compiled = SqliteDialect::create_table(context_, *entity); const auto start = Clock::now();
        try { Statement statement(db_, compiled); int rc = sqlite3_step(statement.stmt); if (rc != SQLITE_DONE) throw SqliteError(rc); }
        catch (...) { log(compiled, SqlLogOperation::Schema, SqlExecutionOutcome::Failure, 0, start); throw; }
        log(compiled, SqlLogOperation::Schema, SqlExecutionOutcome::Success, 0, start);
    }
}
Rows& SqliteSession::query(const SelectQuery& query) {
    auto purpose = intent(query.trace_chain, TraceKind::Purpose);
    if (blank(purpose) || blank(query.comment_text)) throw std::invalid_argument("Query requires purpose and comment");
    auto& entity = descriptor(query.entity);
    auto slice = query.prepare_for_list();
    if (*slice.limit > std::uint64_t(INT64_MAX) || slice.offset > std::uint64_t(INT64_MAX)) throw std::overflow_error("SQLite pagination out of range");
    const auto& compiled = SqliteDialect::select(context_, entity, query, slice);
    auto& rows = context_.make<Rows>(); const auto start = Clock::now();
    try {
        Statement statement(db_, compiled); int rc;
        while ((rc = sqlite3_step(statement.stmt)) == SQLITE_ROW) {
            rows.emplace_back(); auto& row = rows.back(); auto r = context_.memory_resource();
            for (int i = 0; i < sqlite3_column_count(statement.stmt); ++i) {
                auto name = std::string_view(sqlite3_column_name(statement.stmt, i));
                const auto& property = entity.property_by_name(name); Value value(r);
                switch (sqlite3_column_type(statement.stmt, i)) {
                case SQLITE_NULL: break;
                case SQLITE_INTEGER: {
                    const auto n = sqlite3_column_int64(statement.stmt, i);
                    if (property.data_type == DataType::U64) { if (n < 0) throw std::overflow_error("Negative unsigned database value"); value = Value(r, std::uint64_t(n)); }
                    else if (property.data_type == DataType::Bool) value = Value(r, n != 0);
                    else value = Value(r, std::int64_t(n));
                    break;
                }
                case SQLITE_FLOAT: value = Value(r, sqlite3_column_double(statement.stmt, i)); break;
                case SQLITE_TEXT: value = Value(r, std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(statement.stmt, i)), std::size_t(sqlite3_column_bytes(statement.stmt, i)))); break;
                default: throw std::invalid_argument("Unsupported SQLite result type");
                }
                row.emplace(String(name, r), std::move(value));
            }
        }
        if (rc != SQLITE_DONE) throw SqliteError(rc);
    } catch (...) { log(compiled, SqlLogOperation::Select, SqlExecutionOutcome::Failure, rows.size(), start, purpose, query.comment_text); throw; }
    log(compiled, SqlLogOperation::Select, SqlExecutionOutcome::Success, rows.size(), start, purpose, query.comment_text);
    return rows;
}
std::uint64_t SqliteSession::mutate(MutationKind kind, const MutationCommand& cmd) {
    auto audit = intent(cmd.trace_chain, TraceKind::AuditReason);
    if (blank(audit)) throw std::invalid_argument("Mutation requires audit_as");
    const auto& compiled = SqliteDialect::mutation(context_, descriptor(cmd.entity), kind, cmd);
    SqlLogOperation operation = SqlLogOperation::Insert;
    switch (kind) { case MutationKind::Insert: break; case MutationKind::Update: operation = SqlLogOperation::Update; break;
        case MutationKind::Delete: operation = SqlLogOperation::Delete; break; case MutationKind::Recover: operation = SqlLogOperation::Recover; break; }
    const auto start = Clock::now(); std::uint64_t affected;
    try {
        Statement statement(db_, compiled); int rc = sqlite3_step(statement.stmt); if (rc != SQLITE_DONE) throw SqliteError(rc);
        affected = std::uint64_t(sqlite3_changes64(db_));
    } catch (...) { log(compiled, operation, SqlExecutionOutcome::Failure, 0, start, {}, {}, audit); throw; }
    log(compiled, operation, SqlExecutionOutcome::Success, affected, start, {}, {}, audit);
    if (kind != MutationKind::Insert && cmd.expected_version_value && affected == 0) throw OptimisticLockError();
    return affected;
}
void SqliteSession::begin() {
    if (in_transaction_) throw std::logic_error("Nested transaction unsupported");
    exec_control(db_, "BEGIN IMMEDIATE"); in_transaction_ = true;
}
void SqliteSession::commit() { exec_control(db_, "COMMIT"); in_transaction_ = false; }
void SqliteSession::rollback() noexcept {
    if (in_transaction_) { sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr); in_transaction_ = false; }
}
}
