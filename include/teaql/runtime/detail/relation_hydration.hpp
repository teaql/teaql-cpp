#pragma once
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/detail/relation_execution.hpp>
#include <teaql/runtime/detail/relation_installation.hpp>
namespace teaql::detail {
enum class RelationHydrationMode {Embedded,Flat,CompactFlat};
template<class E> using RelationHydrationResult=Result<std::monostate,DataServiceError<typename E::Error>>;
template<class E> using EntityRelationFetch=ContextFunction<AsyncOperation<RelationRowsResult<E>>& (const EntityDataService<E>&,SelectQuery)>;

// Plans and parent storage are stable Context borrows. A forest owns one plan
// snapshot; descendant nodes borrow it instead of copying each subtree again.
template<class E> class RelationHydrationNode {
    using R=RelationHydrationResult<E>;
public:
    using allocator_type=Allocator;
    RelationHydrationNode(std::allocator_arg_t,Allocator a,const EntityDataService<E>& service,
        std::span<const CompactRow> parents,std::span<CompactRow> mutable_parents,const RelationLoadPlan& plan,
        RelationHydrationMode mode,const EntityRuntimeState* root,EntityGraphBuilder* graph,const EntityRelationFetch<E>& fetch)
        :service_(std::allocator_arg,a,service),parents_(parents),mutable_parents_(mutable_parents),plan_(&plan),mode_(mode),root_(root),graph_(graph),
         fetch_(std::allocator_arg,a,fetch),children_(a.resource()),records_(a.resource()),scratch_(a.resource()){}
    std::optional<R> poll(WakeSignal& wake){
        auto r=service_.memory_resource();
        if(mode_!=RelationHydrationMode::Embedded)return poll_body(wake);
        if(!scope_){
            const auto& cs=EntityDataServiceAccess::context_service(service_);auto capabilities=cs.executor.get().capabilities();
            auto count=unique_relation_values(r,parents_,plan_->local_key).size();
            auto selected=relation_top_n_execution_plan(capabilities,*plan_,count);
            auto operation=relation_top_n_operation(r,*plan_,selected,count);scope_=&cs.metadata.context().start_runtime_operation(operation);
        }
        auto result=scope_->poll_with_context([&]{return poll_body(wake);});
        if(result){
            if(*result){RuntimeAttributes attrs(r);attrs.emplace(String("teaql.result.cardinality",r),std::make_obj_using_allocator<RuntimeAttributeValue>(Allocator(r),std::uint64_t(parents_.size())));scope_->success(attrs);}
            else scope_->failure("relation_load_error");
            scope_->close();
        }
        return result;
    }
    void finish_and_wait() noexcept{forwarding_.reset();rows_operation_=nullptr;descendant_=nullptr;if(scope_)scope_->close();}
    void cancel_and_wait() noexcept{
        forwarding_.reset();if(descendant_){descendant_->cancel();descendant_=nullptr;}if(rows_operation_){rows_operation_->cancel();rows_operation_=nullptr;}if(scope_)scope_->close();
    }
private:
    enum class Phase {Fetch,Descendants,Install};
    const UserContext& context() const{return EntityDataServiceAccess::context_service(service_).metadata.context();}
    std::optional<R> poll_body(WakeSignal& wake){
        auto r=service_.memory_resource();
        if(!child_service_){
            child_service_=&context().template make<EntityDataService<E>>(EntityDataServiceAccess::relation_child(service_,*plan_));
            RelationFetch<E> call(std::allocator_arg,Allocator(r),[this](SelectQuery query)->auto&{return fetch_(*child_service_,std::move(query));});
            rows_operation_=&fetch_relation_rows(EntityDataServiceAccess::context_service(service_),*plan_,parents_,mode_==RelationHydrationMode::CompactFlat,call);
        }
        if(phase_==Phase::Fetch){
            forwarding_.attach(rows_operation_->wake_signal(),wake);auto* result=rows_operation_->poll();if(!result)return {};
            if(!*result)return R::err(r,clone_in(r,result->error()));
            for(const auto& row:result->value())children_.emplace_back(row);
            forwarding_.reset();rows_operation_=nullptr;
            if(mode_!=RelationHydrationMode::CompactFlat)for(auto& child:children_)child.remove(PARTITION_RANK_PROPERTY);
            if(mode_==RelationHydrationMode::Embedded){
                attach_relation_rows(context(),mutable_parents_,*plan_,std::move(children_));
                if(!plan_->children.empty())for(auto& parent:mutable_parents_)if(auto* relation=parent.get_mut(plan_->relation_name)){
                    if(auto* record=std::get_if<Value::Object>(&relation->data))records_.push_back(record);
                    else if(auto* list=std::get_if<Value::List>(&relation->data))for(auto& value:*list)if(auto* record=std::get_if<Value::Object>(&value.data))records_.push_back(record);
                }
            }
            phase_=Phase::Descendants;
        }
        while(phase_==Phase::Descendants){
            if(mode_==RelationHydrationMode::Embedded){if(record_index_==records_.size()){phase_=Phase::Install;break;}}
            else if(plan_index_==plan_->children.size()){phase_=Phase::Install;break;}
            if(!descendant_){
                std::span<const CompactRow> rows=children_;std::span<CompactRow> mutable_rows=children_;
                if(mode_==RelationHydrationMode::Embedded){
                    // Rust mem::take leaves this record empty if a child plan
                    // fails. Restore it only after that particular plan succeeds.
                    Record taken(r);taken.swap(*records_[record_index_]);scratch_.clear();scratch_.emplace_back(taken);rows=scratch_;mutable_rows=scratch_;
                }else if(mode_==RelationHydrationMode::CompactFlat)mutable_rows={};
                descendant_=&make_operation<R,RelationHydrationNode>(r,*child_service_,rows,mutable_rows,plan_->children[plan_index_],mode_,root_,graph_,fetch_);
            }
            forwarding_.attach(descendant_->wake_signal(),wake);auto* result=descendant_->poll();if(!result)return {};
            if(!*result)return R::err(r,clone_in(r,result->error()));
            forwarding_.reset();descendant_=nullptr;++plan_index_;
            if(mode_==RelationHydrationMode::Embedded){
                auto replacement=scratch_.front().into_map(r);records_[record_index_]->swap(replacement);scratch_.clear();
                if(plan_index_==plan_->children.size()){plan_index_=0;++record_index_;}
            }
        }
        if(mode_==RelationHydrationMode::Embedded)return R::ok(r,{});
        auto result=mode_==RelationHydrationMode::CompactFlat?install_compact_flat_relation(context(),parents_,*plan_,std::move(children_),*root_,*graph_):install_flat_relation(context(),mutable_parents_,*plan_,std::move(children_),*root_,*graph_);
        if(!result)return R::err(r,DataServiceError<typename E::Error>::entity(r,std::move(result).error()));
        return R::ok(r,{});
    }
    EntityDataService<E> service_;
    std::span<const CompactRow> parents_;std::span<CompactRow> mutable_parents_;const RelationLoadPlan* plan_;
    RelationHydrationMode mode_;const EntityRuntimeState* root_;EntityGraphBuilder* graph_;EntityRelationFetch<E> fetch_;
    EntityDataService<E>* child_service_=nullptr;Vector<CompactRow> children_;Vector<Record*> records_;Vector<CompactRow> scratch_;
    AsyncOperation<RelationRowsResult<E>>* rows_operation_=nullptr;AsyncOperation<R>* descendant_=nullptr;WakeSignal::Forward forwarding_;
    FailOpenRuntimeTelemetryScope* scope_=nullptr;Phase phase_=Phase::Fetch;std::size_t plan_index_=0,record_index_=0;
};

template<class E> class RelationHydrationForest {
    using R=RelationHydrationResult<E>;
public:
    using allocator_type=Allocator;
    RelationHydrationForest(std::allocator_arg_t,Allocator a,const EntityDataService<E>& service,
        std::span<const CompactRow> parents,std::span<CompactRow> mutable_parents,std::span<const RelationLoadPlan> plans,
        RelationHydrationMode mode,const EntityRuntimeState* root,EntityGraphBuilder* graph,const EntityRelationFetch<E>& fetch)
        :service_(std::allocator_arg,a,service),parents_(parents),mutable_parents_(mutable_parents),plans_(plans.begin(),plans.end(),a.resource()),mode_(mode),root_(root),graph_(graph),fetch_(std::allocator_arg,a,fetch){
        validate_relation_domain(EntityDataServiceAccess::context_service(service_).metadata.context(),parents,root,graph);
        if(mode!=RelationHydrationMode::Embedded && (!root || !graph))throw std::invalid_argument("Identity graph hydration requires a root and graph");
        if(mode!=RelationHydrationMode::CompactFlat && parents.size()!=mutable_parents.size())throw std::invalid_argument("This relation path requires mutable rows");
    }
    std::optional<R> poll(WakeSignal& wake){
        auto r=service_.memory_resource();while(index_<plans_.size()){
            if(!child_)child_=&make_operation<R,RelationHydrationNode<E>>(r,service_,parents_,mutable_parents_,plans_[index_],mode_,root_,graph_,fetch_);
            forwarding_.attach(child_->wake_signal(),wake);auto* result=child_->poll();if(!result)return {};
            if(!*result)return R::err(r,clone_in(r,result->error()));
            forwarding_.reset();child_=nullptr;++index_;
        }return R::ok(r,{});
    }
    void finish_and_wait() noexcept{forwarding_.reset();child_=nullptr;}
    void cancel_and_wait() noexcept{forwarding_.reset();if(child_){child_->cancel();child_=nullptr;}}
private:
    EntityDataService<E> service_;std::span<const CompactRow> parents_;std::span<CompactRow> mutable_parents_;Vector<RelationLoadPlan> plans_;
    RelationHydrationMode mode_;const EntityRuntimeState* root_;EntityGraphBuilder* graph_;EntityRelationFetch<E> fetch_;
    std::size_t index_=0;AsyncOperation<R>* child_=nullptr;WakeSignal::Forward forwarding_;
};
template<class E> auto& enhance_relation_plans(const EntityDataService<E>& service,std::span<CompactRow> parents,std::span<const RelationLoadPlan> plans,const EntityRelationFetch<E>& fetch){
    return make_operation<RelationHydrationResult<E>,RelationHydrationForest<E>>(service.memory_resource(),service,std::span<const CompactRow>(parents),parents,plans,RelationHydrationMode::Embedded,nullptr,nullptr,fetch);
}
template<class E> auto& hydrate_flat_plans(const EntityDataService<E>& service,std::span<CompactRow> parents,std::span<const RelationLoadPlan> plans,const EntityRuntimeState& root,EntityGraphBuilder& graph,const EntityRelationFetch<E>& fetch){
    return make_operation<RelationHydrationResult<E>,RelationHydrationForest<E>>(service.memory_resource(),service,std::span<const CompactRow>(parents),parents,plans,RelationHydrationMode::Flat,&root,&graph,fetch);
}
template<class E> auto& hydrate_compact_flat_plans(const EntityDataService<E>& service,std::span<const CompactRow> parents,std::span<const RelationLoadPlan> plans,const EntityRuntimeState& root,EntityGraphBuilder& graph,const EntityRelationFetch<E>& fetch){
    return make_operation<RelationHydrationResult<E>,RelationHydrationForest<E>>(service.memory_resource(),service,parents,std::span<CompactRow>{},plans,RelationHydrationMode::CompactFlat,&root,&graph,fetch);
}
}
