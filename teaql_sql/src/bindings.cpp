#include <teaql/sql/bindings.hpp>

namespace teaql {
namespace {
template <class T>
void append_binding(Vector<Value>& values,
                    Vector<SqlParameterLogPolicy>& policies,
                    T&& value,
                    SqlParameterLogPolicy policy) {
    // Keep amortized vector growth and restore logical alignment if cloning the
    // value fails. Scalar policy rollback cannot allocate or throw.
    policies.push_back(policy);
    try {
        values.emplace_back(std::forward<T>(value));
    } catch (...) {
        policies.pop_back();
        throw;
    }
}
}

void SqlBindings::append(const Value& value, SqlParameterLogPolicy policy) {
    append_binding(values_, policies_, value, policy);
}

void SqlBindings::append(Value&& value, SqlParameterLogPolicy policy) {
    append_binding(values_, policies_, std::move(value), policy);
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
    return rebind_log_context(r, result);
}

SqlLogContext SqlBindings::rebind_log_context(Memory r, const SqlLogContext& source) const {
    auto result = detail::clone_in(r, source);
    SqlLogContext original_source(r);
    original_source.generated_sql = source.generated_sql;
    Vector<Value> values(r);
    for (const auto& [index, value] : originals_) {
        values.emplace_back(value);
        original_source.parameter_policies.push_back(index < source.parameter_policies.size()
                                                         ? source.parameter_policies[index]
                                                         : SqlParameterLogPolicy::Unknown);
    }
    result.intent_redactions = SqlIntentRedactions::from_bindings(r, original_source, values, {});
    return result;
}

namespace detail {
SqlParameterLogPolicy sql_field_policy(const EntityDescriptor& entity, std::string_view field) {
    return SqlIntentRedactions::field_policy(entity, field);
}

std::optional<SqlParameterLogPolicy> combine_sql_policy(std::optional<SqlParameterLogPolicy> a,
                                                        std::optional<SqlParameterLogPolicy> b) {
    if (!a) {
        return b;
    }
    if (!b) {
        return a;
    }
    using P = SqlParameterLogPolicy;
    for (auto policy : {P::Credential, P::Unknown, P::Masked}) {
        if (a == policy || b == policy) {
            return policy;
        }
    }
    return P::Plain;
}

std::optional<SqlParameterLogPolicy> sql_expression_policy(const EntityDescriptor& entity,
                                                           const Expr& expr) {
    using K = Expr::Kind;
    if (expr.kind == K::Column) {
        return sql_field_policy(entity, expr.name);
    }
    if (expr.kind == K::Value || expr.kind == K::LikePattern || expr.kind == K::And ||
        expr.kind == K::Or || expr.kind == K::Not) {
        return {};
    }
    std::optional<SqlParameterLogPolicy> result;
    // SubQuery args contain only its left expression; its entity/query live in
    // subquery_data, so their field policies cannot leak into this scalar scope.
    for (const auto& arg : expr.args) {
        result = combine_sql_policy(result, sql_expression_policy(entity, arg));
    }
    return result;
}
}
}
