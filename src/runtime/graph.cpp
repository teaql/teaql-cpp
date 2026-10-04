#include <teaql/runtime/graph.hpp>
#include <algorithm>
#include <limits>
namespace teaql {
namespace detail {
const TraceScopeToken* clone_trace_scope(Memory r,const TraceScopeToken* token){
    if(!token || token->memory_resource()==r)return token;
    return &resource_arena(r).make<TraceScopeToken>(*token);
}
}
TraceScopeToken::TraceScopeToken(std::allocator_arg_t,Allocator a,const TraceScopeToken* parent,const TraceNode& track,std::uint64_t index)
    :track(std::allocator_arg,a,track),node_index(index),parent_(parent){
    auto& arena=detail::resource_arena(a.resource());
    if(parent && !detail::resource_arena(parent->memory_resource()).can_lend_to(arena))throw std::invalid_argument("Trace parent belongs to a shorter or unrelated Context");
}
TraceScopeToken::TraceScopeToken(std::allocator_arg_t,Allocator a,const TraceScopeToken& other)
    :track(std::allocator_arg,a,other.track),node_index(other.node_index),parent_(detail::clone_trace_scope(a.resource(),other.parent_)){}
TraceChain TraceScopeToken::recover_trace_chain(Memory r) const {
    Vector<const TraceNode*> reversed(r);for(auto* token=this;token;token=token->parent_)if(!token->track.comment.empty())reversed.push_back(&token->track);
    TraceChain result(r);for(auto i=reversed.rbegin();i!=reversed.rend();++i)result.emplace_back(**i);return result;
}
bool TraceScopeToken::operator==(const TraceScopeToken& other) const{return track==other.track && node_index==other.node_index && ((!parent_ && !other.parent_) || (parent_ && other.parent_ && *parent_==*other.parent_));}
GraphMutationPlanItem::GraphMutationPlanItem(std::allocator_arg_t,Allocator a,std::string_view entity,GraphMutationKind kind,const MutationValues& values,std::span<const String> fields,std::uint64_t index,const TraceScopeToken* scope,const std::optional<EntitySnapshot>& old)
    :entity(entity,a.resource()),kind(kind),values(std::allocator_arg,a,values),update_fields(fields.begin(),fields.end(),a.resource()),item_index(index),scope_token(detail::clone_trace_scope(a.resource(),scope)),old_values(detail::clone_in(a.resource(),old)){}
GraphMutationPlanItem::GraphMutationPlanItem(std::allocator_arg_t,Allocator a,const GraphMutationPlanItem& other)
    :GraphMutationPlanItem(std::allocator_arg,a,other.entity,other.kind,other.values,other.update_fields,other.item_index,other.scope_token,other.old_values){}
bool GraphMutationPlanItem::operator==(const GraphMutationPlanItem& other) const{return entity==other.entity && kind==other.kind && values.fields==other.values.fields && update_fields==other.update_fields && item_index==other.item_index && ((!scope_token && !other.scope_token) || (scope_token && other.scope_token && *scope_token==*other.scope_token)) && ((!old_values && !other.old_values) || (old_values && other.old_values && old_values->fields==other.old_values->fields));}
void GraphMutationPlan::push(std::string_view entity,GraphMutationKind kind,const MutationValues& values,std::span<const String> fields,const TraceScopeToken* scope,const std::optional<EntitySnapshot>& old){
    // Match Rust's checked Debug arithmetic and wrapping Release arithmetic.
#ifndef NDEBUG
    if(next_item_index==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("attempt to add with overflow");
#endif
    const auto index=next_item_index++;
    items.emplace_back(entity,kind,values,fields,index,scope,old);
}
void GraphMutationPlan::rebuild_batches(){
    batches.clear();for(const auto& item:items){std::span<const String> fields=item.kind==GraphMutationKind::Update?std::span<const String>(item.update_fields):std::span<const String>{};
        if(!batches.empty()){auto& last=batches.back();if(last.entity==item.entity && last.kind==item.kind && std::ranges::equal(last.update_fields,fields)){last.items.emplace_back(item);continue;}}
        batches.emplace_back(item.entity,item.kind,fields);batches.back().items.emplace_back(item);
    }
}
GraphMutationPlan::Counts GraphMutationPlan::grouped_counts(Memory r) const{Counts result(r);for(const auto& batch:batches)result[{String(batch.entity,r),batch.kind}]+=batch.items.size();return result;}
Vector<String> sorted_update_fields(Memory r,const EntityValues& values,std::span<const String> excluded){Vector<String> result(r);for(const auto& [field,value]:values)if(std::find(excluded.begin(),excluded.end(),field)==excluded.end())result.emplace_back(field);return result;}
TraceChain ScopedCommentNode::to_trace_chain(Memory r) const{Vector<const TraceNode*> reversed(r);for(auto* node=this;node;node=node->parent)if(!node->track.comment.empty())reversed.push_back(&node->track);TraceChain result(r);for(auto i=reversed.rbegin();i!=reversed.rend();++i)result.emplace_back(**i);return result;}
}
