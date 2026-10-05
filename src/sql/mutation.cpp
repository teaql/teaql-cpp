#include <teaql/sql/dialect.hpp>
#include <limits>

namespace teaql {
namespace {
[[noreturn]] void fail(Memory r, SqlCompileError::Code code, std::string_view name) {
    throw SqlCompileError(std::allocator_arg, Allocator(r), code, name);
}

const PropertyDescriptor& require_id(Memory r, const EntityDescriptor& entity) {
    if (auto* p = entity.id_property()) {
        return *p;
    }
    fail(r, SqlCompileError::Code::MissingIdProperty, entity.name);
}

const PropertyDescriptor& require_version(Memory r, const EntityDescriptor& entity) {
    if (auto* p = entity.version_property()) {
        return *p;
    }
    fail(r, SqlCompileError::Code::MissingVersionProperty, entity.name);
}

std::int64_t next_version(std::int64_t value) {
    // Rust's checked builds panic here. Never invoke C++ signed-overflow UB or
    // invent a SqlCompileError variant for this arithmetic precondition.
    if (value == std::numeric_limits<std::int64_t>::max()) {
        throw std::overflow_error("mutation version increment overflow");
    }
    return value + 1;
}

std::int64_t negate_version(std::int64_t value) {
    if (value == std::numeric_limits<std::int64_t>::min()) {
        throw std::overflow_error("mutation version negation overflow");
    }
    return -value;
}

String join(Memory r, const Vector<String>& parts, std::string_view separator) {
    String out(r);
    bool first = true;
    for (const auto& part : parts) {
        if (!first) {
            out += separator;
        }
        first = false;
        out += part;
    }
    return out;
}

void push_typed(SqlBindings& params,
                const EntityDescriptor& entity,
                const PropertyDescriptor& property,
                const Value* value) {
    // An explicitly TypedNull retains its own type. Only bare/missing Null
    // acquires the destination property's type, as in the source compiler.
    if (!value || std::holds_alternative<std::monostate>(value->data)) {
        params.push_field(
            entity, property.name, Value(params.memory_resource(), TypedNull{property.data_type}));
    } else {
        params.push_field(entity, property.name, *value);
    }
}

const Value* lookup(const MutationValues& values, std::string_view name) {
    auto p = values.fields.find(name);
    return p == values.fields.end() ? nullptr : &p->second;
}

String equal_binding(const SqlDialect& d,
                     Memory r,
                     const PropertyDescriptor& property,
                     std::size_t index) {
    auto out = d.quote_ident(r, property.column_name);
    out += " = ";
    out += d.placeholder(r, index);
    return out;
}

CompiledQuery compiled(Memory r, const SqlDialect& d, String sql, SqlBindings& params) {
    CompiledQuery out(std::allocator_arg, Allocator(r));
    out.sql = std::move(sql);
    out.log_context = params.log_context(r, d.kind());
    out.params = std::move(params).into_values();
    return out;
}

String update_sql(const SqlDialect& d,
                  Memory r,
                  const EntityDescriptor& entity,
                  const Vector<String>& assignments,
                  const Vector<String>& predicates) {
    String out("UPDATE ", r);
    out += d.quote_ident(r, entity.table_name);
    out += " SET ";
    out += join(r, assignments, ", ");
    out += " WHERE ";
    out += join(r, predicates, " AND ");
    return out;
}
}

CompiledQuery SqlDialect::compile_insert(Memory r,
                                         const EntityDescriptor& entity,
                                         const InsertCommand& command) const {
    Vector<String> columns(r), placeholders(r);
    SqlBindings params(r);
    for (const auto& property : entity.properties) {
        if (auto* value = lookup(command.values, property.name)) {
            columns.emplace_back(quote_ident(r, property.column_name));
            push_typed(params, entity, property, value);
            placeholders.emplace_back(placeholder(r, params.size()));
        }
    }
    if (columns.empty()) {
        fail(r, SqlCompileError::Code::EmptyMutation, "insert");
    }
    String sql("INSERT INTO ", r);
    sql += quote_ident(r, entity.table_name);
    sql += " (";
    sql += join(r, columns, ", ");
    sql += ") VALUES (";
    sql += join(r, placeholders, ", ");
    sql += ')';
    return compiled(r, *this, std::move(sql), params);
}

CompiledQuery SqlDialect::compile_batch_insert(Memory r,
                                               const EntityDescriptor& entity,
                                               const BatchInsertCommand& command) const {
    if (command.batch_values.empty()) {
        fail(r, SqlCompileError::Code::EmptyMutation, "batch_insert");
    }
    Vector<const PropertyDescriptor*> columns(r);
    Vector<String> names(r), rows(r);
    SqlBindings params(r);
    // The first record alone determines the batch shape; later absent fields
    // become typed nulls and later extra fields do not extend that shape.
    for (const auto& property : entity.properties) {
        if (lookup(command.batch_values.front(), property.name)) {
            columns.push_back(&property);
        }
    }
    if (columns.empty()) {
        fail(r, SqlCompileError::Code::EmptyMutation, "batch_insert");
    }
    for (auto* property : columns) {
        names.emplace_back(quote_ident(r, property->column_name));
    }
    for (const auto& record : command.batch_values) {
        Vector<String> placeholders(r);
        for (auto* property : columns) {
            push_typed(params, entity, *property, lookup(record, property->name));
            placeholders.emplace_back(placeholder(r, params.size()));
        }
        String row("(", r);
        row += join(r, placeholders, ", ");
        row += ')';
        rows.emplace_back(std::move(row));
    }
    String sql("INSERT INTO ", r);
    sql += quote_ident(r, entity.table_name);
    sql += " (";
    sql += join(r, names, ", ");
    sql += ") VALUES ";
    sql += join(r, rows, ", ");
    return compiled(r, *this, std::move(sql), params);
}

CompiledQuery SqlDialect::compile_update(Memory r,
                                         const EntityDescriptor& entity,
                                         const UpdateCommand& command) const {
    return compile_update_with_guard(r, entity, command, nullptr);
}

CompiledQuery SqlDialect::compile_guarded_update(Memory r,
                                                 const EntityDescriptor& entity,
                                                 const UpdateCommand& command,
                                                 const Expr& guard) const {
    return compile_update_with_guard(r, entity, command, &guard);
}

CompiledQuery SqlDialect::compile_update_with_guard(Memory r,
                                                    const EntityDescriptor& entity,
                                                    const UpdateCommand& command,
                                                    const Expr* guard) const {
    const auto& id = require_id(r, entity);
    Vector<String> assignments(r), predicates(r);
    SqlBindings params(r);
    for (const auto& property : entity.properties) {
        if (property.is_id || (property.is_version && command.expected_version_value)) {
            continue;
        }
        if (auto* value = lookup(command.values, property.name)) {
            push_typed(params, entity, property, value);
            assignments.emplace_back(equal_binding(*this, r, property, params.size()));
        }
    }
    if (auto expected = command.expected_version_value) {
        const auto& version = require_version(r, entity);
        params.push_field(entity, version.name, Value(r, next_version(*expected)));
        assignments.emplace_back(equal_binding(*this, r, version, params.size()));
    }
    if (assignments.empty()) {
        fail(r, SqlCompileError::Code::EmptyMutation, "update");
    }
    params.push_field(entity, id.name, command.id);
    predicates.emplace_back(equal_binding(*this, r, id, params.size()));
    if (auto expected = command.expected_version_value) {
        const auto& version = require_version(r, entity);
        params.push_field(entity, version.name, Value(r, *expected));
        predicates.emplace_back(equal_binding(*this, r, version, params.size()));
    }
    if (guard) {
        predicates.emplace_back(compile_expr(r, entity, *guard, params));
    }
    return compiled(r, *this, update_sql(*this, r, entity, assignments, predicates), params);
}

CompiledQuery SqlDialect::compile_batch_update(Memory r,
                                               const EntityDescriptor& entity,
                                               const BatchUpdateCommand& command) const {
    if (command.batch_values.empty()) {
        fail(r, SqlCompileError::Code::EmptyMutation, "batch_update");
    }
    const auto& id = require_id(r, entity);
    SqlBindings params(r);
    Vector<String> assignments(r), predicates(r);
    auto case_start = [&] {
        String out("CASE ", r);
        out += quote_ident(r, id.column_name);
        return out;
    };
    auto case_arm = [&](String& sql,
                        const Value& key,
                        const PropertyDescriptor& property,
                        const Value* value,
                        bool typed) {
        params.push_field(entity, id.name, key);
        auto id_placeholder = placeholder(r, params.size());
        if (typed) {
            push_typed(params, entity, property, value);
        } else {
            params.push_field(entity, property.name, *value);
        }
        auto value_placeholder = placeholder(r, params.size());
        sql += " WHEN ";
        sql += id_placeholder;
        sql += " THEN ";
        sql += value_placeholder;
    };
    auto case_end = [&](String& sql, const PropertyDescriptor& property) {
        sql += " ELSE ";
        sql += quote_ident(r, property.column_name);
        sql += " END";
    };
    auto assignment = [&](const PropertyDescriptor& property, String body) {
        auto out = quote_ident(r, property.column_name);
        out += " = ";
        out += body;
        return out;
    };
    for (const auto& field : command.update_fields) {
        auto* property = entity.property_by_name(field);
        if (!property) {
            fail(r, SqlCompileError::Code::UnknownField, field);
        }
        auto body = case_start();
        for (std::size_t i = 0; i < command.batch_values.size(); ++i) {
            case_arm(body,
                     command.batch_ids.at(i),
                     *property,
                     lookup(command.batch_values[i], field),
                     true);
        }
        case_end(body, *property);
        assignments.emplace_back(assignment(*property, std::move(body)));
    }
    bool has_versions = false;
    if (auto* version = entity.version_property()) {
        auto body = case_start();
        for (std::size_t i = 0; i < command.batch_expected_versions.size(); ++i) {
            if (auto expected = command.batch_expected_versions[i]) {
                has_versions = true;
                // Preserve the source's order: bind ID before incrementing version.
                const auto& key = command.batch_ids.at(i);
                params.push_field(entity, id.name, key);
                auto id_placeholder = placeholder(r, params.size());
                params.push_field(entity, version->name, Value(r, next_version(*expected)));
                auto value_placeholder = placeholder(r, params.size());
                body += " WHEN ";
                body += id_placeholder;
                body += " THEN ";
                body += value_placeholder;
            }
        }
        if (has_versions) {
            case_end(body, *version);
            assignments.emplace_back(assignment(*version, std::move(body)));
        }
    }
    if (assignments.empty()) {
        fail(r, SqlCompileError::Code::EmptyMutation, "batch_update");
    }
    Vector<String> placeholders(r);
    for (const auto& key : command.batch_ids) {
        params.push_field(entity, id.name, key);
        placeholders.emplace_back(placeholder(r, params.size()));
    }
    auto in = quote_ident(r, id.column_name);
    in += " IN (";
    in += join(r, placeholders, ", ");
    in += ')';
    predicates.emplace_back(std::move(in));
    if (has_versions) {
        const auto& version = *entity.version_property();
        auto body = case_start();
        for (std::size_t i = 0; i < command.batch_expected_versions.size(); ++i) {
            if (auto expected = command.batch_expected_versions[i]) {
                Value value(r, *expected);
                case_arm(body, command.batch_ids.at(i), version, &value, false);
            }
        }
        case_end(body, version);
        predicates.emplace_back(assignment(version, std::move(body)));
    }
    return compiled(r, *this, update_sql(*this, r, entity, assignments, predicates), params);
}

CompiledQuery SqlDialect::compile_delete(Memory r,
                                         const EntityDescriptor& entity,
                                         const DeleteCommand& command) const {
    return compile_delete_with_guard(r, entity, command, nullptr);
}

CompiledQuery SqlDialect::compile_guarded_delete(Memory r,
                                                 const EntityDescriptor& entity,
                                                 const DeleteCommand& command,
                                                 const Expr& guard) const {
    return compile_delete_with_guard(r, entity, command, &guard);
}

CompiledQuery SqlDialect::compile_delete_with_guard(Memory r,
                                                    const EntityDescriptor& entity,
                                                    const DeleteCommand& command,
                                                    const Expr* guard) const {
    const auto& id = require_id(r, entity);
    SqlBindings params(r);
    Vector<String> predicates(r);
    if (command.soft_delete) {
        const auto& version = require_version(r, entity);
        auto deleted = command.expected_version_value
                           ? negate_version(next_version(*command.expected_version_value))
                           : -1;
        params.push_field(entity, version.name, Value(r, deleted));
        params.push_field(entity, id.name, command.id);
        predicates.emplace_back(equal_binding(*this, r, id, params.size()));
        if (auto expected = command.expected_version_value) {
            params.push_field(entity, version.name, Value(r, *expected));
            predicates.emplace_back(equal_binding(*this, r, version, params.size()));
        }
        if (guard) {
            predicates.emplace_back(compile_expr(r, entity, *guard, params));
        }
        Vector<String> assignments(r);
        assignments.emplace_back(equal_binding(*this, r, version, 1));
        return compiled(r, *this, update_sql(*this, r, entity, assignments, predicates), params);
    }
    params.push_field(entity, id.name, command.id);
    predicates.emplace_back(equal_binding(*this, r, id, params.size()));
    if (auto expected = command.expected_version_value) {
        const auto& version = require_version(r, entity);
        params.push_field(entity, version.name, Value(r, *expected));
        predicates.emplace_back(equal_binding(*this, r, version, params.size()));
    }
    if (guard) {
        predicates.emplace_back(compile_expr(r, entity, *guard, params));
    }
    String sql("DELETE FROM ", r);
    sql += quote_ident(r, entity.table_name);
    sql += " WHERE ";
    sql += join(r, predicates, " AND ");
    return compiled(r, *this, std::move(sql), params);
}

CompiledQuery SqlDialect::compile_recover(Memory r,
                                          const EntityDescriptor& entity,
                                          const RecoverCommand& command) const {
    return compile_recover_with_guard(r, entity, command, nullptr);
}

CompiledQuery SqlDialect::compile_guarded_recover(Memory r,
                                                  const EntityDescriptor& entity,
                                                  const RecoverCommand& command,
                                                  const Expr& guard) const {
    return compile_recover_with_guard(r, entity, command, &guard);
}

CompiledQuery SqlDialect::compile_recover_with_guard(Memory r,
                                                     const EntityDescriptor& entity,
                                                     const RecoverCommand& command,
                                                     const Expr* guard) const {
    if (command.expected_version >= 0) {
        SqlCompileError error(
            std::allocator_arg, Allocator(r), SqlCompileError::Code::InvalidRecoverVersion);
        error.version = command.expected_version;
        throw error;
    }
    const auto& id = require_id(r, entity);
    const auto& version = require_version(r, entity);
    SqlBindings params(r);
    params.push_field(
        entity, version.name, Value(r, next_version(negate_version(command.expected_version))));
    params.push_field(entity, id.name, command.id);
    params.push_field(entity, version.name, Value(r, command.expected_version));
    Vector<String> predicates(r);
    predicates.emplace_back(equal_binding(*this, r, id, 2));
    predicates.emplace_back(equal_binding(*this, r, version, 3));
    if (guard) {
        predicates.emplace_back(compile_expr(r, entity, *guard, params));
    }
    Vector<String> assignments(r);
    assignments.emplace_back(equal_binding(*this, r, version, 1));
    return compiled(r, *this, update_sql(*this, r, entity, assignments, predicates), params);
}
}
