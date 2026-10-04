#include <teaql/runtime/legacy_sqlite_compiler.hpp>
#include <teaql/sql/bindings.hpp>
#include <algorithm>
#include <charconv>
#include <limits>

namespace teaql {
namespace {
String quote(Memory r, std::string_view value) {
    if (value.find('\0') != std::string_view::npos) throw std::invalid_argument("NUL in identifier");
    String out("\"", r);
    for (char c : value) { out += c; if (c == '"') out += c; }
    out += '"'; return out;
}
void number(String& out, std::uint64_t value) {
    char buffer[32]; auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (error != std::errc{}) throw std::invalid_argument("integer conversion failed");
    out.append(buffer, end);
}
SqlParameterLogPolicy policy(Memory, const EntityDescriptor& entity, std::string_view field) {
    return detail::sql_field_policy(entity,field);
}
std::optional<SqlParameterLogPolicy> expr_policy(Memory, const EntityDescriptor& entity, const Expr& expr) {
    return detail::sql_expression_policy(entity,expr);
}
void bind(CompiledQuery& out, const Value& value, SqlParameterLogPolicy p) {
    out.params.emplace_back(value); out.log_context.parameter_policies.push_back(p); out.sql += '?';
}
void field_bind(CompiledQuery& out, const EntityDescriptor& entity, const PropertyDescriptor& p, const Value& value) {
    auto r = out.sql.get_allocator().resource();
    auto v = value.is_null() ? Value(r, TypedNull{p.data_type}) : Value(r, value);
    bind(out, v, policy(r, entity, p.name));
}
void expression(CompiledQuery& out, const EntityDescriptor& entity, const Expr& e,
                std::optional<SqlParameterLogPolicy> inherited = {}) {
    auto r = out.sql.get_allocator().resource();
    auto p = detail::combine_sql_policy(inherited,expr_policy(r,entity,e));
    using K = Expr::Kind;
    if (e.kind == K::Function || e.kind == K::SubQuery)
        throw std::invalid_argument("UnsupportedQueryFeature: expression compilation pending");
    if (e.kind == K::Column) { out.sql += quote(r, entity.require_property(e.name).column_name); return; }
    if (e.kind == K::Value) { bind(out, e.literal, p.value_or(SqlParameterLogPolicy::Unknown)); return; }
    auto arg = [&](std::size_t i) { expression(out, entity, e.args.at(i), p); };
    out.sql += '(';
    if (e.kind == K::Binary) {
        arg(0);
        const bool in = e.op == BinaryOp::In || e.op == BinaryOp::NotIn || e.op == BinaryOp::InLarge || e.op == BinaryOp::NotInLarge;
        if (in) {
            const auto& right = e.args.at(1);
            const auto* values = right.kind == K::Value ? right.literal.as_list() : nullptr;
            if (!values) throw std::invalid_argument("IN requires a list value");
            if (values->empty()) throw std::invalid_argument("EmptyInList");
            out.sql += (e.op == BinaryOp::NotIn || e.op == BinaryOp::NotInLarge) ? " NOT IN (" : " IN (";
            for (std::size_t i = 0; i < values->size(); ++i) { if (i) out.sql += ", "; bind(out, values->at(i), p.value_or(SqlParameterLogPolicy::Unknown)); }
            out.sql += ')';
        } else {
            switch (e.op) {
            case BinaryOp::Eq: out.sql += " = "; break; case BinaryOp::Ne: out.sql += " != "; break;
            case BinaryOp::Gt: out.sql += " > "; break; case BinaryOp::Gte: out.sql += " >= "; break;
            case BinaryOp::Lt: out.sql += " < "; break; case BinaryOp::Lte: out.sql += " <= "; break;
            case BinaryOp::Like: out.sql += " LIKE "; break; case BinaryOp::NotLike: out.sql += " NOT LIKE "; break;
            default: throw std::invalid_argument("InvalidBinaryOp");
            }
            arg(1);
        }
    } else if (e.kind == K::Between) { arg(0); out.sql += " BETWEEN "; arg(1); out.sql += " AND "; arg(2); }
    else if (e.kind == K::IsNull || e.kind == K::IsNotNull) { arg(0); out.sql += e.kind == K::IsNull ? " IS NULL" : " IS NOT NULL"; }
    else if (e.kind == K::Not) { out.sql += "NOT "; arg(0); }
    else {
        if (e.args.empty()) throw std::invalid_argument("Empty logical expression");
        for (std::size_t i = 0; i < e.args.size(); ++i) {
            if (i) out.sql += e.kind == K::And ? " AND " : " OR ";
            // Logical siblings do not inherit each other's field policies.
            expression(out, entity, e.args[i]);
        }
    }
    out.sql += ')';
}
void require_entity(const EntityDescriptor& entity, std::string_view name) {
    if (entity.name != name) throw std::invalid_argument("Entity mismatch");
}
std::int64_t next_version(std::int64_t value) {
    if (value == INT64_MAX) throw std::overflow_error("Version overflow");
    return value + 1;
}
}
CompiledQuery LegacySqliteCompiler::create_table(Memory r, const EntityDescriptor& entity) {
    CompiledQuery out(std::allocator_arg, Allocator(r));
    out.log_context.database_kind.emplace("sqlite",r);out.log_context.generated_sql=true;
    if (entity.properties.empty()) throw std::invalid_argument("Empty entity");
    out.sql = "CREATE TABLE IF NOT EXISTS "; out.sql += quote(r, entity.table_name); out.sql += " (";
    for (std::size_t i = 0; i < entity.properties.size(); ++i) {
        if (i) out.sql += ", ";
        const auto& p = entity.properties[i];
        out.sql += quote(r, p.column_name);
        switch (p.data_type) {
        case DataType::Bool: out.sql += " BOOLEAN"; break;
        case DataType::I64: case DataType::U64: out.sql += " INTEGER"; break;
        case DataType::F64: out.sql += " REAL"; break;
        case DataType::Text: case DataType::LargeText: out.sql += " TEXT"; break;
        default: throw std::invalid_argument("Unsupported data type in initial SQLite kernel");
        }
        if (p.is_id) out.sql += " PRIMARY KEY";
        if (p.is_id || !p.nullable) out.sql += " NOT NULL";
    }
    out.sql += ')'; return out;
}
CompiledQuery LegacySqliteCompiler::select(Memory r, const EntityDescriptor& entity, const SelectQuery& query,
                                    std::optional<Slice> bounded_slice) {
    require_entity(entity, query.entity);
    // Model parity is independent of executor parity. Until the layered SQL
    // compiler/runtime is restored, never silently discard a requested feature.
    if (!query.expr_projection.empty() || query.search_text || query.having_predicate ||
        query.partition_field || query.top_n_probe_parent_threshold_value || !query.aggregates.empty() ||
        !query.group_by_fields.empty() || !query.relations.empty() || query.aggregation_cache ||
        query.raw_sql_text || !query.raw_sql_search_criteria.empty() || !query.dynamic_properties.empty() ||
        !query.raw_projections.empty() || !query.object_group_bys.empty() || !query.child_enhancements.empty() ||
        query.stream_config || query.continuous_page_fetch || query.id_set_pagination)
        throw std::invalid_argument("UnsupportedQueryFeature: layered query execution pending");
    for (const auto& order : query.order_by)
        if (order.expr) throw std::invalid_argument("UnsupportedQueryFeature: expression ordering pending");
    CompiledQuery out(std::allocator_arg, Allocator(r));
    out.log_context.database_kind.emplace("sqlite",r);out.log_context.generated_sql=true;
    out.sql = "SELECT "; bool first = true;
    out.comment=detail::clone_in(r,query.comment_text);
    auto project = [&](const PropertyDescriptor& p) {
        if (!first) { out.sql += ", "; }
        first = false;
        out.sql += quote(r, p.column_name);
        if (p.column_name != p.name) { out.sql += " AS "; out.sql += quote(r, p.name); }
    };
    if (query.projection.empty()) for (const auto& p : entity.properties) project(p);
    else for (std::size_t i = 0; i < query.projection.size(); ++i) {
        if (std::find(query.projection.begin(), query.projection.begin() + i, query.projection[i]) == query.projection.begin() + i)
            project(entity.require_property(query.projection[i]));
    }
    if (first) throw std::invalid_argument("Empty projection");
    out.sql += " FROM "; out.sql += quote(r, entity.table_name);
    if (query.predicate) { out.sql += " WHERE "; expression(out, entity, *query.predicate); }
    if (!query.order_by.empty()) {
        out.sql += " ORDER BY "; bool first_order = true;
        for (const auto& order : query.order_by) {
            if (!first_order) { out.sql += ", "; }
            first_order = false;
            out.sql += quote(r, entity.require_property(order.field).column_name);
            out.sql += order.direction == SortDirection::Asc ? " ASC" : " DESC";
        }
    }
    // Compilation preserves the supplied query; runtime applies materialization limits.
    const auto slice = bounded_slice ? bounded_slice : query.slice;
    if (slice) {
        out.sql += " LIMIT "; if (slice->limit) number(out.sql, *slice->limit); else out.sql += "-1";
        if (slice->offset) { out.sql += " OFFSET "; number(out.sql, slice->offset); }
    }
    return out;
}
namespace {
// A private compilation view, not a core command or an application mutation model.
struct MutationView {
    std::string_view entity;
    const MutationValues* values;
    const Value* id;
    std::optional<std::int64_t> expected_version_value;
    bool soft_delete;
};
CompiledQuery compile_mutation(Memory r, const EntityDescriptor& entity, MutationKind kind, const MutationView& cmd) {
    require_entity(entity, cmd.entity);
    CompiledQuery out(std::allocator_arg, Allocator(r));
    out.log_context.database_kind.emplace("sqlite",r);out.log_context.generated_sql=true;
    if (kind == MutationKind::Insert) {
        if (cmd.values->fields.empty()) throw std::invalid_argument("EmptyMutation");
        out.sql = "INSERT INTO "; out.sql += quote(r, entity.table_name); out.sql += " ("; bool first = true;
        for (const auto& p : entity.properties) if (cmd.values->fields.contains(p.name)) {
            if (!first) { out.sql += ", "; }
            first = false;
            out.sql += quote(r, p.column_name);
        }
        if (first) throw std::invalid_argument("EmptyMutation");
        out.sql += ") VALUES ("; first = true;
        for (const auto& p : entity.properties) if (auto it = cmd.values->fields.find(p.name); it != cmd.values->fields.end()) {
            if (!first) { out.sql += ", "; }
            first = false;
            field_bind(out, entity, p, it->second);
        }
        out.sql += ')'; return out;
    }
    const auto& id = entity.require_id_property();
    const bool hard_delete = kind == MutationKind::Delete && !cmd.soft_delete;
    if (hard_delete) { out.sql = "DELETE FROM "; out.sql += quote(r, entity.table_name); }
    else {
        out.sql = "UPDATE "; out.sql += quote(r, entity.table_name); out.sql += " SET "; bool first = true;
        if (kind == MutationKind::Update) for (const auto& p : entity.properties) {
            if (p.is_id || (p.is_version && cmd.expected_version_value)) continue;
            if (auto it = cmd.values->fields.find(p.name); it != cmd.values->fields.end()) {
                if (!first) { out.sql += ", "; }
                first = false;
                out.sql += quote(r, p.column_name);
                out.sql += " = ";
                field_bind(out, entity, p, it->second);
            }
        }
        if (cmd.expected_version_value || kind != MutationKind::Update) {
            const auto& version = entity.require_version_property(); std::int64_t next = 0;
            if (kind == MutationKind::Recover) {
                if (!cmd.expected_version_value || *cmd.expected_version_value >= 0) throw std::invalid_argument("InvalidRecoverVersion");
                if (*cmd.expected_version_value <= -INT64_MAX) throw std::overflow_error("Version overflow");
                next = -*cmd.expected_version_value + 1;
            } else if (kind == MutationKind::Delete) next = cmd.expected_version_value ? -next_version(*cmd.expected_version_value) : -1;
            else next = next_version(*cmd.expected_version_value);
            if (!first) { out.sql += ", "; }
            first = false;
            out.sql += quote(r, version.column_name);
            out.sql += " = ";
            field_bind(out, entity, version, Value(r, next));
        }
        if (first) throw std::invalid_argument("EmptyMutation");
    }
    out.sql += " WHERE "; out.sql += quote(r, id.column_name); out.sql += " = "; field_bind(out, entity, id, *cmd.id);
    if (cmd.expected_version_value) {
        const auto& p = entity.require_version_property(); out.sql += " AND "; out.sql += quote(r, p.column_name); out.sql += " = "; field_bind(out, entity, p, Value(r, *cmd.expected_version_value));
    }
    return out;
}
} // namespace
CompiledQuery LegacySqliteCompiler::compile_insert(Memory r, const EntityDescriptor& entity, const InsertCommand& command) {
    return compile_mutation(r, entity, MutationKind::Insert, {command.entity, &command.values, nullptr, {}, false});
}
CompiledQuery LegacySqliteCompiler::compile_update(Memory r, const EntityDescriptor& entity, const UpdateCommand& command) {
    return compile_mutation(r, entity, MutationKind::Update, {command.entity, &command.values, &command.id, command.expected_version_value, false});
}
CompiledQuery LegacySqliteCompiler::compile_delete(Memory r, const EntityDescriptor& entity, const DeleteCommand& command) {
    return compile_mutation(r, entity, MutationKind::Delete, {command.entity, nullptr, &command.id, command.expected_version_value, command.soft_delete});
}
CompiledQuery LegacySqliteCompiler::compile_recover(Memory r, const EntityDescriptor& entity, const RecoverCommand& command) {
    return compile_mutation(r, entity, MutationKind::Recover, {command.entity, nullptr, &command.id, command.expected_version, false});
}
}
