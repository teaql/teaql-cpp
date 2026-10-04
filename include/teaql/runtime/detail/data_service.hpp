#pragma once
#include <teaql/runtime/detail/context_entity.hpp>
#include <teaql/core/list.hpp>
namespace teaql::detail {
// A metadata view delegates to the Context; it does not snapshot the registry.
class UserContextMetadata final:public MetadataStore {
public:
    using allocator_type=Allocator;
    UserContextMetadata(std::allocator_arg_t,Allocator a,const UserContext& context):context_(std::allocator_arg,a,context,resource_arena(context.memory_resource())){}
    UserContextMetadata(std::allocator_arg_t,Allocator a,const UserContextMetadata& other):context_(std::allocator_arg,a,other.context_){}
    UserContextMetadata(const UserContextMetadata&)=delete;UserContextMetadata(UserContextMetadata&&) noexcept=default;
    Memory memory_resource() const noexcept override{return context_.memory_resource();}
    const UserContext& context() const{return context_.get();}
    const EntityDescriptor* entity(std::string_view name) const override{return context().entity(name);}
    Vector<const EntityDescriptor*> all_entities(Memory) const override;
    void record_metadata_log(const ExecutionMetadata& value) const override{context().record_metadata_log(value);}
    bool capture_query_debug() const override{return context().sql_log_options().select;}
    bool capture_execution_metadata() const override{return context().sql_log_options().select;}
    std::optional<ExecutionObserver> query_diagnostic_observer(Memory) const override;
    std::optional<ExecutionObserver> mutation_diagnostic_observer(Memory) const override;
private:ResourceBorrow<const UserContext> context_;
};
std::optional<String> resolve_final_comment(const UserContext&,std::span<const TraceNode>,std::optional<std::string_view>);
// RuntimeDataService is the compatibility adapter for a metadata store and an
// explicit executor. ContextDataService and EntityDataService remain separate.
template<class M,class E> class RuntimeDataService {
public:
    using allocator_type=Allocator;
    using Error=typename E::Error;
    RuntimeDataService(std::allocator_arg_t,Allocator a,const M& metadata,const ResourceBorrow<E>& executor)
        :metadata_(std::allocator_arg,a,metadata,resource_arena(metadata.memory_resource())),executor_(std::allocator_arg,a,executor){}
    RuntimeDataService(std::allocator_arg_t,Allocator a,const RuntimeDataService& other):metadata_(std::allocator_arg,a,other.metadata_),executor_(std::allocator_arg,a,other.executor_){}
    RuntimeDataService(const RuntimeDataService&)=delete;RuntimeDataService(RuntimeDataService&&) noexcept=default;
    Memory memory_resource() const{return metadata_.memory_resource();}
    DataServiceOperationResult<Vector<CompactRow>,DataServiceError<Error>>& fetch_all(const SelectQuery&) const;
    DataServiceOperationResult<SmartList<CompactRow>,DataServiceError<Error>>& fetch_smart_list(const SelectQuery&) const;
    template<Entity T> DataServiceOperationResult<SmartList<T>,DataServiceError<Error>>& fetch_entities(const SelectQuery&) const;
    template<Entity T> auto& fetch_enhanced_entities(const SelectQuery& query) const{return fetch_entities<T>(query);}
private:
    ResourceBorrow<const M> metadata_;
    ResourceBorrow<E> executor_;
};
template<class E> class ContextDataService {
public:
    using allocator_type=Allocator;
    using Error=typename E::Error;
    UserContextMetadata metadata;
    ResourceBorrow<E> executor;
    ContextDataService(std::allocator_arg_t,Allocator a,const UserContext& context,const ResourceBorrow<E>& executor):metadata(std::allocator_arg,a,context),executor(std::allocator_arg,a,executor){}
    ContextDataService(std::allocator_arg_t,Allocator a,const ContextDataService& other):metadata(std::allocator_arg,a,other.metadata),executor(std::allocator_arg,a,other.executor){}
    ContextDataService(const ContextDataService&)=delete;ContextDataService(ContextDataService&&) noexcept=default;
    Memory memory_resource() const{return metadata.memory_resource();}
    RuntimeDataService<UserContextMetadata,E> data_service() const{
        // The asynchronous adapter may outlive this temporary wrapper. Keep its
        // metadata view in the Context and capture that stable view explicitly.
        auto& view=metadata.context().template make<UserContextMetadata>(metadata);
        return RuntimeDataService<UserContextMetadata,E>(std::allocator_arg,Allocator(memory_resource()),view,executor);
    }
    std::optional<String> resolve_final_comment(std::span<const TraceNode> trace,std::optional<std::string_view> comment) const{return detail::resolve_final_comment(metadata.context(),trace,comment);}
    template<class T,class Name> DataServiceOperationResult<T,DataServiceError<Error>>& observe(std::string_view family,Name&& name,std::string_view entity,DataServiceOperationResult<T,DataServiceError<Error>>& work) const;
    DataServiceOperationResult<Vector<CompactRow>,DataServiceError<Error>>& fetch_all(const SelectQuery&) const;
    DataServiceOperationResult<SmartList<CompactRow>,DataServiceError<Error>>& fetch_smart_list(const SelectQuery&) const;
    template<Entity T> DataServiceOperationResult<SmartList<T>,DataServiceError<Error>>& fetch_entities(const SelectQuery&) const;
    template<Entity T> auto& fetch_enhanced_entities(const SelectQuery& query) const{return fetch_entities<T>(query);}
};
}
#include <teaql/runtime/detail/base_query.hpp>
#include <teaql/runtime/detail/context_query.hpp>
