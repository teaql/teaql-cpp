#pragma once
#include <teaql/runtime/detail/data_service.hpp>
#include <teaql/runtime/relation.hpp>
namespace teaql {
namespace detail {
struct EntityDataServiceAccess;
void record_query_metadata(const UserContext&,SqlIntentRedactions*,const ExecutionMetadata&);
}
template<class E> class EntityDataService {
public:
    using allocator_type=Allocator;
    using Error=typename E::Error;
    static_assert(std::derived_from<E,QueryExecutor<Error>> && std::derived_from<E,MutationExecutor<Error>>);
    EntityDataService(std::allocator_arg_t,Allocator a,const UserContext& context,std::string_view entity,const ResourceBorrow<E>& executor)
        :entity_(entity,a.resource()),data_service_(std::allocator_arg,a,context,executor),trace_context_(a.resource()){}
    EntityDataService(std::allocator_arg_t,Allocator a,const EntityDataService& other)
        :entity_(other.entity_,a.resource()),data_service_(std::allocator_arg,a,other.data_service_),trace_context_(other.trace_context_,a.resource()),query_log_intent_(other.query_log_intent_){}
    EntityDataService(const EntityDataService&)=delete;EntityDataService(EntityDataService&&) noexcept=default;
    Memory memory_resource() const{return entity_.get_allocator().resource();}
    template<class Owner> static EntityDataService& for_executor(const UserContext& context,std::string_view entity,Owner& owner,E& executor){
        detail::ContextEntityAccess::ensure_open(context);auto r=context.memory_resource();auto entry=detail::ContextResources::entry(detail::resource_arena(r),owner.memory_resource(),executor);
        ResourceBorrow<E> borrow(std::allocator_arg,Allocator(r),executor,*entry.owner);return context.template make<EntityDataService>(context,entity,borrow);
    }
    EntityDataService& with_trace_context(std::span<const TraceNode> trace) & {TraceChain copy(trace.begin(),trace.end(),memory_resource());trace_context_.swap(copy);return *this;}
    std::string_view entity() const{return entity_;}
    SelectQuery select() const{return SelectQuery(std::allocator_arg,Allocator(memory_resource()),entity_);}
    InsertCommand insert_command() const{return InsertCommand(std::allocator_arg,Allocator(memory_resource()),entity_);}
    template<class T> UpdateCommand update_command(T&& id) const{return UpdateCommand(std::allocator_arg,Allocator(memory_resource()),entity_,std::forward<T>(id));}
    template<class T> DeleteCommand delete_command(T&& id) const{return DeleteCommand(std::allocator_arg,Allocator(memory_resource()),entity_,std::forward<T>(id));}
    template<class T> RecoverCommand recover_command(T&& id,std::int64_t version) const{return RecoverCommand(std::allocator_arg,Allocator(memory_resource()),entity_,std::forward<T>(id),version);}
    Result<UpdateCommand,RuntimeError> prepare_update_command(const UpdateCommand& source) const{
        auto r=memory_resource();using R=Result<UpdateCommand,RuntimeError>;auto command=detail::clone_in(r,source);
        if(auto b=behavior()){auto result=b->get().before_update(r,context(),command);if(!result)return R::err(r,std::move(result).error());}
        if(auto* policy=detail::ContextEntityAccess::request_policy(context())){auto result=policy->enforce_update(r,context(),command);if(!result)return R::err(r,std::move(result).error());}
        return R::ok(r,std::move(command));
    }
    Vector<String> relation_loads() const{auto b=behavior();return b?b->get().relation_loads(memory_resource(),context()):Vector<String>(memory_resource());}
    detail::RelationPlansResult relation_plans() const{return detail::build_relation_plans(context(),entity_,relation_loads());}
    Result<SelectQuery,RuntimeError> relation_query(std::string_view relation,std::span<const CompactRow> parents) const{
        auto r=memory_resource();using R=Result<SelectQuery,RuntimeError>;auto plans=relation_plans();if(!plans)return R::err(r,std::move(plans).error());
        for(const auto& plan:plans.value())if(plan.relation_name==relation)return R::ok(r,detail::query_for_relation_plan(context(),plan,parents,false));
        return R::err(r,RuntimeError::missing_relation(r,entity_,relation));
    }
private:
    friend struct detail::EntityDataServiceAccess;
    String entity_;
    detail::ContextDataService<E> data_service_;
    TraceChain trace_context_;
    // Present only on a private query tree, never on UserContext or a wire query.
    SqlIntentRedactions* query_log_intent_=nullptr;
    const UserContext& context() const{return data_service_.metadata.context();}
    std::optional<EntityBehaviorBorrow> behavior() const{
        if(detail::is_generated_schema_bootstrap(context()))return {};
        return context().entity_data_service_behavior(entity_);
    }
    Result<SelectQuery,RuntimeError> prepare_select_query(const SelectQuery& query) const{return detail::prepare_select_query(context(),trace_context_,query);}
    EntityDataService scoped_data_service(std::string_view entity) const{
        auto r=memory_resource();EntityDataService scoped(std::allocator_arg,Allocator(r),context(),entity,data_service_.executor);
        if(query_log_intent_)scoped.query_log_intent_=&context().template make<SqlIntentRedactions>(*query_log_intent_);
        return scoped;
    }
    EntityDataService query_scoped_service() const{
        auto scoped=scoped_data_service(entity_);TraceChain trace(trace_context_,memory_resource());scoped.trace_context_.swap(trace);
        if(!scoped.query_log_intent_)scoped.query_log_intent_=&context().template make<SqlIntentRedactions>();
        return scoped;
    }
    void record_query_metadata(const ExecutionMetadata& value) const{detail::record_query_metadata(context(),query_log_intent_,value);}
    std::optional<ExecutionObserver> query_diagnostic_observer() const{
        if(!data_service_.metadata.capture_execution_metadata())return {};
        const UserContext* owner=&context();auto* intent=query_log_intent_;
        return ExecutionObserver(std::allocator_arg,Allocator(memory_resource()),[owner,intent](ExecutionMetadata value){detail::record_query_metadata(*owner,intent,value);});
    }
};
template<class E> Result<ResourceBorrow<EntityDataService<E>>,ContextError> UserContext::entity_data_service(std::string_view entity) const{
    auto r=memory_resource();using R=Result<ResourceBorrow<EntityDataService<E>>,ContextError>;
    if(!has_entity_data_service(entity))return R::err(r,ContextError(std::allocator_arg,Allocator(r),ContextError::Kind::MissingEntityDataService,entity));
    if(!detail::ContextEntityAccess::has_metadata(*this))return R::err(r,ContextError(std::allocator_arg,Allocator(r),ContextError::Kind::MissingResource,"metadata"));
    auto executor=detail::ContextEntityAccess::require_executor<E>(*this);if(!executor)return R::err(r,std::move(executor).error());
    auto& service=make<EntityDataService<E>>(*this,entity,executor.value());
    return R::ok(r,ResourceBorrow<EntityDataService<E>>(std::allocator_arg,Allocator(r),service,detail::resource_arena(r)));
}
namespace detail {
struct EntityDataServiceAccess {
    template<class E> static auto prepare_select(const EntityDataService<E>& service,const SelectQuery& query){return service.prepare_select_query(query);}
    template<class E> static auto scoped(const EntityDataService<E>& service,std::string_view entity){return service.scoped_data_service(entity);}
    template<class E> static auto query_scoped(const EntityDataService<E>& service){return service.query_scoped_service();}
    template<class E> static void record(const EntityDataService<E>& service,const ExecutionMetadata& metadata){service.record_query_metadata(metadata);}
    template<class E> static auto observer(const EntityDataService<E>& service){return service.query_diagnostic_observer();}
    template<class E> static auto intent(const EntityDataService<E>& service){return service.query_log_intent_?std::optional<SqlIntentRedactions>(clone_in(service.memory_resource(),*service.query_log_intent_)):std::nullopt;}
    template<class E> static const ContextDataService<E>& context_service(const EntityDataService<E>& service){return service.data_service_;}
    template<class E> static std::span<const TraceNode> trace(const EntityDataService<E>& service){return service.trace_context_;}
    template<class E> static EntityDataService<E> relation_child(const EntityDataService<E>& service,const RelationLoadPlan& plan){
        auto r=service.memory_resource();TraceChain trace(service.trace_context_,r);String comment(plan.parent_entity,r);comment+='.';comment+=plan.relation_name;
        trace.emplace_back(TraceKind::Relation,plan.relation_name,comment);auto child=service.scoped_data_service(plan.target_entity);child.with_trace_context(trace);return child;
    }
    template<class E> static Result<std::optional<FlatRelationPlans>,RuntimeError> flat_relation_plans(const EntityDataService<E>& service,const SelectQuery& query){
        using Plans=FlatRelationPlans;using R=Result<std::optional<Plans>,RuntimeError>;auto r=service.memory_resource();
        auto requested=build_relation_plans_from_loads(service.context(),query.entity,query.relations);if(!requested)return R::err(r,std::move(requested).error());
        auto behaviors=service.relation_plans();if(!behaviors)return R::err(r,std::move(behaviors).error());
        auto supported=[&](auto&& self,const RelationLoadPlan& plan)->bool{if(!ContextEntityAccess::has_entity_graph_decoder(service.context(),plan.target_entity))return false;for(const auto& child:plan.children)if(!self(self,child))return false;return true;};
        for(const auto& plan:requested.value())if(!supported(supported,plan))return R::ok(r,{});
        for(const auto& plan:behaviors.value())if(!supported(supported,plan))return R::ok(r,{});
        return R::ok(r,Plans(std::allocator_arg,Allocator(r),std::move(requested).value(),std::move(behaviors).value()));
    }
};
}
}
