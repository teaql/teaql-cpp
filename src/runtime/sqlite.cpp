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
                    } else if constexpr (std::integral<T>) {
                        if constexpr (std::same_as<T, std::uint64_t>) if (value > std::uint64_t(INT64_MAX)) throw std::overflow_error("SQLite integer out of range");
                        rc = sqlite3_bind_int64(stmt, slot, sqlite3_int64(value));
                    } else throw std::invalid_argument("Unsupported SQLite value binding");
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
        else if constexpr (std::same_as<T, Decimal> || std::same_as<T, Date>) return v.to_string(r);
        else if constexpr (std::same_as<T, Timestamp>) return scalar_text(r, Value(r, v.as_millis()));
        else if constexpr (std::same_as<T, Json>) return v.dump(r);
        else if constexpr (std::is_arithmetic_v<T>) { char buffer[64]; auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), v); if (error != std::errc{}) throw std::runtime_error("Cannot format scalar"); return String(buffer, end, r); }
        else return String(r);
    }, value.data);
}
bool credential_key(Memory r, std::string_view name) {
    String normalized(r);
    for (char c : name) {
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) normalized += c;
    }
    for (auto word : {"password", "passwd", "passphrase", "privatekey", "secret", "accesstoken", "refreshtoken", "idtoken", "apikey", "authorization", "credential", "sessiontoken", "magiclinktoken"})
        if (normalized.find(word) != String::npos) return true;
    return false;
}
bool has_credentials(Memory r, const Value& value) {
    if (const auto* object = value.as_object()) {
        for (const auto& [key, item] : *object)
            if (credential_key(r, key) || has_credentials(r, item)) return true;
    } else if (const auto* list = value.as_list()) {
        for (const auto& item : *list) if (has_credentials(r, item)) return true;
    } else if (const auto* json = std::get_if<Json>(&value.data)) {
        const auto walk = [&](const auto& self, const Json& node) -> bool {
            if (const auto* o = std::get_if<Json::Object>(&node.data)) {
                for (const auto& [k, v] : *o) if (credential_key(r, k) || self(self, v)) return true;
            } else if (const auto* a = std::get_if<Json::Array>(&node.data)) {
                for (const auto& v : *a) if (self(self, v)) return true;
            }
            return false;
        };
        return walk(walk, *json);
    }
    return false;
}
void collect_secrets(Memory r, const Value& value, Vector<String>& secrets) {
    if (const auto* object = value.as_object()) {
        for (const auto& [key, item] : *object) { (void)key; collect_secrets(r, item, secrets); }
    } else if (const auto* list = value.as_list()) {
        for (const auto& item : *list) collect_secrets(r, item, secrets);
    } else if (const auto* json = std::get_if<Json>(&value.data)) {
        const auto walk = [&](const auto& self, const Json& node) -> void {
            if (const auto* o = std::get_if<Json::Object>(&node.data)) {
                for (const auto& [k, v] : *o) { (void)k; self(self, v); }
            } else if (const auto* a = std::get_if<Json::Array>(&node.data)) {
                for (const auto& v : *a) self(self, v);
            } else if (const auto* s = std::get_if<String>(&node.data)) secrets.emplace_back(*s);
            else if (!std::holds_alternative<std::monostate>(node.data)) secrets.push_back(node.dump(r));
        };
        walk(walk, *json);
    } else if (!value.is_null()) secrets.push_back(scalar_text(r, value));
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
void SqliteSession::log(const CompiledQuery& query, LegacySqlLogOperation operation, SqlExecutionOutcome outcome,
                       std::uint64_t count, Clock::time_point start, std::string_view purpose,
                       std::string_view comment, std::string_view audit) {
    if (!logging_) return;
    auto r = context_.memory_resource();
    logs_.emplace_back(r, operation, outcome, count, Clock::now() - start);
    auto& entry = logs_.back(); entry.sql = query.sql; entry.purpose = purpose; entry.comment = comment; entry.audit_reason = audit;
    Vector<String> secrets(r);
    for (std::size_t i = 0; i < query.params.size(); ++i) {
        auto p = query.log_context.parameter_policies.size() == query.params.size() ? query.log_context.parameter_policies[i] : SqlParameterLogPolicy::Unknown;
        if ((p == SqlParameterLogPolicy::Plain && !has_credentials(r, query.params[i])) || query.params[i].is_null()) entry.params.emplace_back(query.params[i]);
        else {
            collect_secrets(r, query.params[i], secrets);
            entry.params.emplace_back("[REDACTED]");
        }
    }
    std::sort(secrets.begin(), secrets.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });
    for (const auto& secret : secrets) { scrub(entry.purpose, secret); scrub(entry.comment, secret); scrub(entry.audit_reason, secret); }
}
void SqliteSession::ensure_schema() {
    for (const auto* entity : descriptors_) {
        const auto& compiled = LegacySqliteCompiler::create_table(context_.memory_resource(), *entity); const auto start = Clock::now();
        try { Statement statement(db_, compiled); int rc = sqlite3_step(statement.stmt); if (rc != SQLITE_DONE) throw SqliteError(rc); }
        catch (...) { log(compiled, LegacySqlLogOperation::Schema, SqlExecutionOutcome::Failure, 0, start); throw; }
        log(compiled, LegacySqlLogOperation::Schema, SqlExecutionOutcome::Success, 0, start);
    }
}
Rows& SqliteSession::query(const SelectQuery& query) {
    auto purpose = intent(query.trace_chain, TraceKind::Purpose);
    if (blank(purpose) || (!query.comment_text || blank(*query.comment_text))) throw std::invalid_argument("Query requires purpose and comment");
    auto& entity = descriptor(query.entity);
    auto prepared = query.prepare_for_list();
    const auto& slice = *prepared.slice;
    if (*slice.limit > std::uint64_t(INT64_MAX) || slice.offset > std::uint64_t(INT64_MAX)) throw std::overflow_error("SQLite pagination out of range");
    const auto& compiled = LegacySqliteCompiler::select(context_.memory_resource(), entity, prepared);
    auto& rows = context_.make<Rows>(); const auto start = Clock::now();
    try {
        Statement statement(db_, compiled); int rc;
        while ((rc = sqlite3_step(statement.stmt)) == SQLITE_ROW) {
            rows.emplace_back(); auto& row = rows.back(); auto r = context_.memory_resource();
            for (int i = 0; i < sqlite3_column_count(statement.stmt); ++i) {
                auto name = std::string_view(sqlite3_column_name(statement.stmt, i));
                const auto& property = entity.require_property(name); Value value(r);
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
    } catch (...) { log(compiled, LegacySqlLogOperation::Select, SqlExecutionOutcome::Failure, rows.size(), start, purpose, *query.comment_text); throw; }
    log(compiled, LegacySqlLogOperation::Select, SqlExecutionOutcome::Success, rows.size(), start, purpose, *query.comment_text);
    return rows;
}
std::uint64_t SqliteSession::execute(const InsertCommand& command) {
    return execute_compiled(LegacySqliteCompiler::compile_insert(context_.memory_resource(), descriptor(command.entity), command),
                            LegacySqlLogOperation::Insert, command.trace_chain, false);
}
std::uint64_t SqliteSession::execute(const UpdateCommand& command) {
    return execute_compiled(LegacySqliteCompiler::compile_update(context_.memory_resource(), descriptor(command.entity), command),
                            LegacySqlLogOperation::Update, command.trace_chain, command.expected_version_value.has_value());
}
std::uint64_t SqliteSession::execute(const DeleteCommand& command) {
    return execute_compiled(LegacySqliteCompiler::compile_delete(context_.memory_resource(), descriptor(command.entity), command),
                            LegacySqlLogOperation::Delete, command.trace_chain, command.expected_version_value.has_value());
}
std::uint64_t SqliteSession::execute(const RecoverCommand& command) {
    return execute_compiled(LegacySqliteCompiler::compile_recover(context_.memory_resource(), descriptor(command.entity), command),
                            LegacySqlLogOperation::Recover, command.trace_chain, true);
}
std::uint64_t SqliteSession::execute_compiled(const CompiledQuery& compiled, LegacySqlLogOperation operation,
                                            const TraceChain& trace, bool check_version) {
    auto audit = intent(trace, TraceKind::AuditReason);
    if (blank(audit)) throw std::invalid_argument("Mutation requires audit_as");
    const auto start = Clock::now(); std::uint64_t affected;
    try {
        Statement statement(db_, compiled); int rc = sqlite3_step(statement.stmt); if (rc != SQLITE_DONE) throw SqliteError(rc);
        affected = std::uint64_t(sqlite3_changes64(db_));
    } catch (...) { log(compiled, operation, SqlExecutionOutcome::Failure, 0, start, {}, {}, audit); throw; }
    log(compiled, operation, SqlExecutionOutcome::Success, affected, start, {}, {}, audit);
    if (check_version && affected == 0) throw OptimisticLockError();
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
