#include <teaql/runtime/registry.hpp>

namespace teaql {
namespace {
struct MetadataObserver {
    using allocator_type=Allocator;
    ResourceBorrow<const MetadataStore> store;
    MetadataObserver(std::allocator_arg_t,Allocator a,const MetadataStore& target):store(std::allocator_arg,a,target,detail::resource_arena(target.memory_resource())) {}
    MetadataObserver(std::allocator_arg_t,Allocator a,const MetadataObserver& other):store(std::allocator_arg,a,other.store) {}
    void operator()(ExecutionMetadata metadata) const {store.get().record_metadata_log(metadata);}
};
}
std::optional<ExecutionObserver> MetadataStore::query_diagnostic_observer(Memory r) const {
    if(!capture_execution_metadata())return {};
    return ExecutionObserver(std::allocator_arg,Allocator(r),MetadataObserver(std::allocator_arg,Allocator(r),*this));
}
std::optional<ExecutionObserver> MetadataStore::mutation_diagnostic_observer(Memory r) const {
    return ExecutionObserver(std::allocator_arg,Allocator(r),MetadataObserver(std::allocator_arg,Allocator(r),*this));
}
void InMemoryMetadataStore::register_entity(EntityDescriptor descriptor) {
    auto r=memory_resource();auto found=entities_.find(descriptor.name);
    if(found==entities_.end()){
        // Copy the key before moving the descriptor; argument evaluation order
        // must not make the map key depend on a moved-from name.
        String key(descriptor.name,r);entities_.emplace(std::move(key),std::move(descriptor));
    }else detail::assign_scoped(found->second,std::move(descriptor));
}
Vector<const EntityDescriptor*> InMemoryMetadataStore::all_entities(Memory r) const {
    // The array owns only borrowed pointers. Its allocation cannot extend the
    // lifetime of the descriptors it enumerates.
    auto& owner=detail::resource_arena(memory_resource());
    if(!owner.can_lend_to(detail::resource_arena(r)))throw std::invalid_argument("Metadata enumeration cannot outlive its owning Context");
    Vector<const EntityDescriptor*> out(r);out.reserve(entities_.size());for(const auto& [name,entity]:entities_)out.push_back(&entity);return out;
}
}
