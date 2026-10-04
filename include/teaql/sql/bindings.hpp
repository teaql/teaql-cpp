#pragma once
#include <teaql/sql/types.hpp>
namespace teaql {
namespace detail {
SqlParameterLogPolicy sql_field_policy(const EntityDescriptor&,std::string_view);
std::optional<SqlParameterLogPolicy> combine_sql_policy(std::optional<SqlParameterLogPolicy>,std::optional<SqlParameterLogPolicy>);
std::optional<SqlParameterLogPolicy> sql_expression_policy(const EntityDescriptor&,const Expr&);
class SqlBindingScope;
}
class SqlBindings {
public:
    using allocator_type=Allocator;
    explicit SqlBindings(Memory r) : values_(r),policies_(r) {}
    SqlBindings(std::allocator_arg_t,Allocator a) : SqlBindings(a.resource()) {}
    SqlBindings(std::allocator_arg_t,Allocator a,const SqlBindings& other)
        : values_(other.values_,a.resource()),policies_(other.policies_,a.resource()),scope_(other.scope_),untrusted_sql_(other.untrusted_sql_) {}
    SqlBindings(std::allocator_arg_t,Allocator a,SqlBindings&& other) : SqlBindings(detail::move_to(a.resource(),std::move(other))) {}
    SqlBindings(const SqlBindings&)=delete;
    SqlBindings(SqlBindings&&) noexcept=default;
    SqlBindings& operator=(SqlBindings&& other) {return detail::assign_scoped(*this,std::move(other));}
    void push(const Value& value) {append(value,scope_.value_or(SqlParameterLogPolicy::Unknown));}
    void push_field(const EntityDescriptor& entity,std::string_view field,const Value& value) {append(value,detail::sql_field_policy(entity,field));}
    void mark_untrusted_sql() {untrusted_sql_=true;}
    SqlLogContext log_context(Memory,DatabaseKind) const;
    Vector<Value> into_values() && {policies_.clear();return std::move(values_);}
    std::span<const Value> values() const {return values_;}
    std::size_t size() const {return values_.size();}
    const Value& operator[](std::size_t index) const {return values_[index];}
    auto begin() const {return values_.begin();}
    auto end() const {return values_.end();}
    Memory memory_resource() const {return values_.get_allocator().resource();}
private:
    friend class detail::SqlBindingScope;
    void append(const Value&,SqlParameterLogPolicy);
    Vector<Value> values_;
    Vector<SqlParameterLogPolicy> policies_;
    std::optional<SqlParameterLogPolicy> scope_;
    bool untrusted_sql_=false;
};
namespace detail {
// Compiler-internal state, never supplied by a query caller. Subquery scopes
// replace with None; scalar expression scopes inherit and combine provenance.
class SqlBindingScope {
public:
    SqlBindingScope(SqlBindings& bindings,std::optional<SqlParameterLogPolicy> policy,bool inherit=true)
        : bindings_(bindings),previous_(bindings.scope_) {bindings_.scope_=inherit ? combine_sql_policy(previous_,policy):policy;}
    ~SqlBindingScope() {bindings_.scope_=previous_;}
    SqlBindingScope(const SqlBindingScope&)=delete;
    SqlBindingScope& operator=(const SqlBindingScope&)=delete;
private:
    SqlBindings& bindings_;
    std::optional<SqlParameterLogPolicy> previous_;
};
}
}
