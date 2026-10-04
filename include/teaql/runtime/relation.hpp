#pragma once
#include <teaql/runtime/error.hpp>
#include <teaql/core/query.hpp>
namespace teaql {
class UserContext;
struct RelationLoadPlan {
    using allocator_type=Allocator;
    String parent_entity,relation_name,path,target_entity,local_key,foreign_key;
    bool many=false;
    std::optional<SelectQuery> query;
    Vector<RelationLoadPlan> children;
    RelationLoadPlan(std::allocator_arg_t,Allocator a,std::string_view parent,const RelationDescriptor& relation)
        :parent_entity(parent,a.resource()),relation_name(relation.name,a.resource()),path(relation.name,a.resource()),target_entity(relation.target_entity,a.resource()),local_key(relation.local_key,a.resource()),foreign_key(relation.foreign_key,a.resource()),many(relation.many),children(a.resource()){}
    RelationLoadPlan(std::allocator_arg_t,Allocator a,const RelationLoadPlan& other)
        :parent_entity(other.parent_entity,a.resource()),relation_name(other.relation_name,a.resource()),path(other.path,a.resource()),target_entity(other.target_entity,a.resource()),local_key(other.local_key,a.resource()),foreign_key(other.foreign_key,a.resource()),many(other.many),query(detail::clone_in(a.resource(),other.query)),children(other.children,a.resource()){}
    RelationLoadPlan(std::allocator_arg_t,Allocator a,RelationLoadPlan&& other):RelationLoadPlan(detail::move_to(a.resource(),std::move(other))){}
    RelationLoadPlan(const RelationLoadPlan&)=delete;RelationLoadPlan(RelationLoadPlan&&) noexcept=default;
    RelationLoadPlan& operator=(RelationLoadPlan&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const{return parent_entity.get_allocator().resource();}
    bool operator==(const RelationLoadPlan&) const=default;
};
namespace detail {
// Allocator-aware counterpart of the source's pair of plan vectors.
struct FlatRelationPlans {
    using allocator_type=Allocator;
    Vector<RelationLoadPlan> query,behavior;
    FlatRelationPlans(std::allocator_arg_t,Allocator a,Vector<RelationLoadPlan> q,Vector<RelationLoadPlan> b):query(std::move(q),a.resource()),behavior(std::move(b),a.resource()){}
    FlatRelationPlans(std::allocator_arg_t,Allocator a,const FlatRelationPlans& other):query(other.query,a.resource()),behavior(other.behavior,a.resource()){}
    FlatRelationPlans(std::allocator_arg_t,Allocator a,FlatRelationPlans&& other):FlatRelationPlans(detail::move_to(a.resource(),std::move(other))){}
    FlatRelationPlans(const FlatRelationPlans&)=delete;FlatRelationPlans(FlatRelationPlans&&) noexcept=default;
    Memory memory_resource() const{return query.get_allocator().resource();}
};
using RelationPlansResult=Result<Vector<RelationLoadPlan>,RuntimeError>;
RelationPlansResult build_relation_plans(const UserContext&,std::string_view,std::span<const String>);
RelationPlansResult build_relation_plans_from_loads(const UserContext&,std::string_view,std::span<const RelationLoad>);
Vector<Value> unique_relation_values(Memory,std::span<const CompactRow>,std::string_view);
String graph_identity_key(Memory,const Value&);
SelectQuery query_for_relation_plan(const UserContext&,const RelationLoadPlan&,std::span<const CompactRow>,bool compact);
void ensure_identity_projection(const UserContext&,SelectQuery&);
Result<SelectQuery,RuntimeError> prepare_select_query(const UserContext&,std::span<const TraceNode>,const SelectQuery&);
}
}
