#pragma once
#include <teaql/sql/dialect.hpp>
#include <teaql/data_service/executor.hpp>

namespace teaql::detail {
bool select_plan_matches(const SelectQuery&,const SelectQuery&);
SelectQuery select_plan_key(Memory,const SelectQuery&);
Vector<Value> collect_select_params(Memory,const EntityDescriptor&,const SelectQuery&,bool large_in_uses_array_param);
const Value::List* partition_probe_values(const SelectQuery&);
std::optional<SelectQuery> scalar_partition_probe_query(Memory,const SelectQuery&,const Value&);
struct CachedSelectPlan {
    using allocator_type=Allocator;
    EntityDescriptor descriptor;
    SelectQuery key;
    String sql;
    SqlLogContext log;
    CachedSelectPlan(std::allocator_arg_t,Allocator a,const EntityDescriptor& descriptor,SelectQuery key,const CompiledQuery& compiled)
        :descriptor(std::allocator_arg,a,descriptor),key(std::allocator_arg,a,std::move(key)),sql(compiled.sql,a.resource()),log(std::allocator_arg,a,compiled.log_context) {}
};
// Explicit Context-owned caches; never a singleton or ambient process cache.
// The 256-entry bound is logical. Arena bytes are reclaimed at scope close.
class SqlExecutorState {
public:
    using allocator_type=Allocator;
    SqlExecutorState(std::allocator_arg_t,Allocator a):descriptors_(a.resource()),plans_(a.resource()) {}
    const EntityDescriptor* entity_descriptor(const SchemaProvider&,std::string_view) const;
    CompiledQuery compile_select_cached(Memory,const SqlDialect&,const EntityDescriptor&,const SelectQuery&) const;
    Memory memory_resource() const{return plans_.get_allocator().resource();}
    std::size_t cached_plans() const{return plans_.size();}
private:
    mutable std::pmr::map<String,EntityDescriptor,std::less<>> descriptors_;
    mutable Vector<ScopedBox<CachedSelectPlan>> plans_;
};
}
