#pragma once
#include <teaql/core/entity.hpp>
#include <teaql/data_service/executor.hpp>
#include <teaql/runtime/resources.hpp>
#include <set>

namespace teaql {
// Runtime metadata is independent of the data-service SchemaProvider SPI.
class MetadataStore {
public:
    virtual ~MetadataStore()=default;
    virtual Memory memory_resource() const noexcept=0;
    virtual const EntityDescriptor* entity(std::string_view name) const=0;
    virtual Vector<const EntityDescriptor*> all_entities(Memory) const=0;
    virtual void record_metadata_log(const ExecutionMetadata&) const {}
    virtual bool capture_query_debug() const {return true;}
    virtual bool capture_execution_metadata() const {return true;}
    virtual std::optional<ExecutionObserver> query_diagnostic_observer(Memory) const;
    virtual std::optional<ExecutionObserver> mutation_diagnostic_observer(Memory) const;
};
class EntityRegistry {
public:
    virtual ~EntityRegistry()=default;
    virtual Memory memory_resource() const noexcept=0;
    virtual bool contains(std::string_view entity) const=0;
};
class InMemoryMetadataStore final:public MetadataStore,public SchemaProvider,public EntityDescriptorStore {
public:
    using allocator_type=Allocator;
    InMemoryMetadataStore(std::allocator_arg_t,Allocator a):entities_(std::less<>{},a.resource()) {}
    InMemoryMetadataStore(std::allocator_arg_t,Allocator a,const InMemoryMetadataStore& other):entities_(other.entities_,a.resource()) {}
    InMemoryMetadataStore(std::allocator_arg_t,Allocator a,InMemoryMetadataStore&& other):InMemoryMetadataStore(detail::move_to(a.resource(),std::move(other))) {}
    InMemoryMetadataStore(const InMemoryMetadataStore&)=delete;
    InMemoryMetadataStore(InMemoryMetadataStore&&) noexcept=default;
    InMemoryMetadataStore& operator=(InMemoryMetadataStore&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept override {return entities_.get_allocator().resource();}
    void register_entity(EntityDescriptor);
    void register_descriptor(EntityDescriptor descriptor) override {register_entity(std::move(descriptor));}
    InMemoryMetadataStore& with_entity(EntityDescriptor descriptor) & {register_entity(std::move(descriptor));return *this;}
    InMemoryMetadataStore&& with_entity(EntityDescriptor descriptor) && {register_entity(std::move(descriptor));return std::move(*this);}
    const EntityDescriptor* entity(std::string_view name) const override {auto found=entities_.find(name);return found==entities_.end()?nullptr:&found->second;}
    Vector<const EntityDescriptor*> all_entities(Memory) const override;
    const EntityDescriptor* get_entity(std::string_view name) const override {
        auto* found=entity(name);
        return found?&detail::resource_arena(memory_resource()).make<EntityDescriptor>(*found):nullptr;
    }
private:
    std::pmr::map<String,EntityDescriptor,std::less<>> entities_;
};
class InMemoryEntityRegistry final:public EntityRegistry {
public:
    using allocator_type=Allocator;
    InMemoryEntityRegistry(std::allocator_arg_t,Allocator a):entities_(std::less<>{},a.resource()) {}
    InMemoryEntityRegistry(std::allocator_arg_t,Allocator a,const InMemoryEntityRegistry& other):entities_(other.entities_,a.resource()) {}
    InMemoryEntityRegistry(std::allocator_arg_t,Allocator a,InMemoryEntityRegistry&& other):InMemoryEntityRegistry(detail::move_to(a.resource(),std::move(other))) {}
    InMemoryEntityRegistry(const InMemoryEntityRegistry&)=delete;
    InMemoryEntityRegistry(InMemoryEntityRegistry&&) noexcept=default;
    InMemoryEntityRegistry& operator=(InMemoryEntityRegistry&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept override {return entities_.get_allocator().resource();}
    void register_entity(std::string_view entity){entities_.emplace(entity);}
    InMemoryEntityRegistry& with_entity(std::string_view entity) & {register_entity(entity);return *this;}
    InMemoryEntityRegistry&& with_entity(std::string_view entity) && {register_entity(entity);return std::move(*this);}
    bool contains(std::string_view entity) const override {return entities_.contains(entity);}
private:
    std::pmr::set<String,std::less<>> entities_;
};
}
