#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_manager.hpp>
#include <teaql/core/detail/text.hpp>
#include <charconv>

namespace teaql {
namespace {
String number(Memory r, std::uint64_t value) {
    char b[32];
    auto end = std::to_chars(b, b + sizeof(b), value).ptr;
    return String(b, end, r);
}

std::optional<String> trace_value(Memory r, const TraceChain& trace, TraceKind kind) {
    for (auto i = trace.rbegin(); i != trace.rend(); ++i) {
        if (i->kind == kind) {
            return String(i->comment, r);
        }
    }
    return {};
}

bool nonblank(std::string_view s) {
    detail::trim(s);
    return !s.empty();
}

TraceChain canonical_trace(Memory r,
                           SqlLogOperation operation,
                           std::string_view backend,
                           const TraceChain& source) {
    TraceChain out(r);
    auto has = [&](TraceKind kind) {
        return std::any_of(source.begin(), source.end(), [&](const auto& n) {
            return n.kind == kind;
        });
    };
    if (has(TraceKind::Operation) && has(TraceKind::Provider) && has(TraceKind::Sql)) {
        for (const auto& n : source) {
            if (n.kind != TraceKind::Comment && n.kind != TraceKind::Purpose &&
                n.kind != TraceKind::AuditReason) {
                out.emplace_back(n);
            }
        }
        return out;
    }
    std::string_view entity = "unknown";
    for (const auto& n : source) {
        if (nonblank(n.entity_type)) {
            entity = n.entity_type;
            break;
        }
    }
    std::string_view statement = entity;
    if (!is_select(operation)) {
        for (auto i = source.rbegin(); i != source.rend(); ++i) {
            if (i->kind == TraceKind::Entity && nonblank(i->entity_type)) {
                statement = i->entity_type;
                break;
            }
        }
    }
    out.emplace_back(TraceKind::Operation, entity, is_select(operation) ? "query" : "mutation");
    out.emplace_back(is_select(operation) ? TraceKind::Request : TraceKind::Entity, statement, "");
    for (const auto& n : source) {
        if (n.kind == TraceKind::Relation) {
            out.emplace_back(n);
        }
    }
    out.emplace_back(TraceKind::Provider, nonblank(backend) ? backend : "unknown", "");
    constexpr std::string_view names[]{"select", "insert", "update", "delete", "recover"};
    out.emplace_back(TraceKind::Sql, names[unsigned(operation)], "");
    return out;
}

String pretty(Memory r, std::string_view sql) {
    String out(sql, r);
    auto replace = [&](std::string_view from, std::string_view to) {
        std::size_t pos = 0;
        while ((pos = out.find(from, pos)) != String::npos) {
            out.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    for (auto key : {" FROM ",
                     " WHERE ",
                     " GROUP BY ",
                     " HAVING ",
                     " ORDER BY ",
                     " LIMIT ",
                     " OFFSET ",
                     " RETURNING "}) {
        String next("\n", r);
        next += std::string_view(key).substr(1);
        replace(key, next);
    }
    replace(" AND ", "\n  AND ");
    return out;
}

LogDuration elapsed(ExecutionMetadata::SystemTime start, ExecutionMetadata::SystemTime end) {
    if (end < start) {
        return {};
    }
    auto elapsed = end.duration_since(start);
    return {elapsed->seconds, elapsed->nanos};
}
}

Vector<SqlLogEntry> UserContext::sql_logs() const {
    ensure_open();
    auto r = memory_resource();
    return sql_log_entries_ ? Vector<SqlLogEntry>(*sql_log_entries_, r) : Vector<SqlLogEntry>(r);
}

void UserContext::clear_sql_logs() const {
    ensure_open();
    if (sql_log_entries_) {
        sql_log_entries_->clear();
    }
}

void UserContext::record_metadata_log(const ExecutionMetadata& metadata) const {
    ensure_open();
    if (!metadata.statements.empty()) {
        for (const auto& child : metadata.statements) {
            record_metadata_log(child);
        }
        return;
    }
    auto op = SqlLogOperation::Update;
    switch (metadata.operation) {
    case DataServiceOperation::Query:
        op = SqlLogOperation::Select;
        break;
    case DataServiceOperation::Insert:
        op = SqlLogOperation::Insert;
        break;
    case DataServiceOperation::Delete:
        op = SqlLogOperation::Delete;
        break;
    case DataServiceOperation::Recover:
        op = SqlLogOperation::Recover;
        break;
    default:
        break;
    }
    if (!sql_log_options_.enabled_for(op)) {
        return;
    }
    auto r = memory_resource();
    SqlLogEntry sensitive(std::allocator_arg, Allocator(r), op);
    sensitive.log_context = detail::clone_in(r, metadata.sql_log);
    if (!sensitive.log_context.database_kind) {
        sensitive.log_context.database_kind.emplace(metadata.backend, r);
    }
    sensitive.comment = trace_value(r, metadata.trace_chain, TraceKind::Comment);
    if (!sensitive.comment) {
        sensitive.comment = detail::clone_in(r, metadata.comment);
    }
    sensitive.purpose = trace_value(r, metadata.trace_chain, TraceKind::Purpose);
    auto local_reason = trace_value(r, metadata.trace_chain, TraceKind::AuditReason);
    if (op != SqlLogOperation::Select || local_reason) {
        sensitive.audit_reason =
            metadata.comment ? detail::clone_in(r, metadata.comment) : std::move(local_reason);
    }
    auto trace = canonical_trace(r, op, metadata.backend, metadata.trace_chain);
    sensitive.trace_path.swap(trace);
    if (metadata.parameterized_query) {
        sensitive.sql = *metadata.parameterized_query;
    }
    sensitive.params = Vector<Value>(metadata.params, r);
    if (metadata.debug_query) {
        sensitive.debug_sql = *metadata.debug_query;
    }
    sensitive.pretty_sql = pretty(r, sensitive.debug_sql);
    sensitive.started_at = metadata.started_at;
    sensitive.ended_at = metadata.ended_at;
    sensitive.elapsed = elapsed(metadata.started_at, metadata.ended_at);
    sensitive.result_count = metadata.result_count;
    sensitive.affected_rows = metadata.affected_rows;
    if (metadata.result_count) {
        sensitive.result_summary = number(r, *metadata.result_count);
        sensitive.result_summary += " rows returned";
    } else if (metadata.affected_rows) {
        sensitive.result_summary = number(r, *metadata.affected_rows);
        sensitive.result_summary += " rows affected";
    }
    auto safe = detail::sql_entry_projection(r, sensitive, false);
    if (!sql_log_entries_) {
        sql_log_entries_ = &make<Vector<SqlLogEntry>>();
    }
    sql_log_entries_->emplace_back(safe);
    if (auto* entry = resources_ ? resources_->find<UnifiedLogBuffer>() : nullptr) {
        auto& buffer = *static_cast<UnifiedLogBuffer*>(entry->object);
        auto target = buffer.memory_resource();
        LogPayload payload = detail::clone_in(r, safe);
        UnifiedLogEntry item(std::allocator_arg, Allocator(target), metadata.started_at, payload);
        if (auto id = user_identifier()) {
            item.user_identifier.emplace(*id, target);
        }
        for (const auto& n : safe.trace_path) {
            item.trace_chain.emplace_back(n);
        }
        buffer.entries.emplace_back(std::move(item));
    }
    auto& manager = global_.log_manager();
    manager.write_sql_log(r, safe.trace_path, safe);
    manager.write_sensitive_sql_log(r, safe.trace_path, sensitive);
}
}
