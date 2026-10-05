#include <teaql/sql/bindings.hpp>

namespace teaql {
void SqlBindings::append(const Value& value, SqlParameterLogPolicy policy) {
    // Keep amortized vector growth and restore logical alignment if cloning the
    // value fails. Scalar policy rollback cannot allocate or throw.
    policies_.push_back(policy);
    try {
        values_.emplace_back(value);
    } catch (...) {
        policies_.pop_back();
        throw;
    }
}

SqlLogContext SqlBindings::log_context(Memory r, DatabaseKind kind) const {
    SqlLogContext result(r);
    result.parameter_policies = policies_;
    result.generated_sql = !untrusted_sql_;
    switch (kind) {
    case DatabaseKind::Sqlite:
        result.database_kind.emplace("sqlite", r);
        break;
    case DatabaseKind::PostgreSql:
        result.database_kind.emplace("postgresql", r);
        break;
    case DatabaseKind::MySql:
        result.database_kind.emplace("mysql", r);
        break;
    }
    return result;
}

namespace detail {
SqlParameterLogPolicy sql_field_policy(const EntityDescriptor& entity, std::string_view field) {
    auto* property = entity.property_by_name(field);
    using P = SqlParameterLogPolicy;
    if (!property)
        return P::Unknown;
    if (is_credential_log_name(property->name) || is_credential_log_name(property->column_name))
        return P::Credential;
    if (!entity.audit_mask_fields_declared)
        return P::Unknown;
    for (const auto& masked : entity.audit_mask_fields)
        if (masked == property->name)
            return P::Masked;
    return P::Plain;
}

std::optional<SqlParameterLogPolicy> combine_sql_policy(std::optional<SqlParameterLogPolicy> a,
                                                        std::optional<SqlParameterLogPolicy> b) {
    if (!a)
        return b;
    if (!b)
        return a;
    using P = SqlParameterLogPolicy;
    for (auto policy : {P::Credential, P::Unknown, P::Masked})
        if (a == policy || b == policy)
            return policy;
    return P::Plain;
}

std::optional<SqlParameterLogPolicy> sql_expression_policy(const EntityDescriptor& entity,
                                                           const Expr& expr) {
    using K = Expr::Kind;
    if (expr.kind == K::Column)
        return sql_field_policy(entity, expr.name);
    if (expr.kind == K::Value || expr.kind == K::And || expr.kind == K::Or || expr.kind == K::Not)
        return {};
    std::optional<SqlParameterLogPolicy> result;
    // SubQuery args contain only its left expression; its entity/query live in
    // subquery_data, so their field policies cannot leak into this scalar scope.
    for (const auto& arg : expr.args)
        result = combine_sql_policy(result, sql_expression_policy(entity, arg));
    return result;
}
}
}
