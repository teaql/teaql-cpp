#pragma once
#include <teaql/runtime/registry.hpp>
#include <teaql/runtime/schema.hpp>
#include <teaql/runtime/event.hpp>
#include <teaql/runtime/graph.hpp>
#include <teaql/runtime/behavior.hpp>
#include <teaql/runtime/entity_graph_decoder.hpp>
namespace teaql {
class GlobalContext;
// Checker and language registrations remain pending. Registries preserve their
// independent responsibilities, including entity-name versus descriptor-name keys.
class RuntimeModule {
public:
    using allocator_type=Allocator;
    InMemoryMetadataStore metadata;
    RuntimeModule(std::allocator_arg_t,Allocator a):metadata(std::allocator_arg,a),entities_(std::allocator_arg,a),behaviors_(std::allocator_arg,a),graph_decoders_(std::allocator_arg,a),event_sinks_(std::allocator_arg,a),initial_graphs_(a.resource()),root_graphs_(a.resource()),bootstraps_(a.resource()){}
    RuntimeModule(std::allocator_arg_t,Allocator a,const RuntimeModule& other):metadata(std::allocator_arg,a,other.metadata),entities_(std::allocator_arg,a,other.entities_),behaviors_(std::allocator_arg,a,other.behaviors_),graph_decoders_(std::allocator_arg,a,other.graph_decoders_),event_sinks_(std::allocator_arg,a,other.event_sinks_),initial_graphs_(other.initial_graphs_,a.resource()),root_graphs_(other.root_graphs_,a.resource()),bootstraps_(other.bootstraps_,a.resource()){}
    RuntimeModule(const RuntimeModule&)=delete;
    Memory memory_resource() const noexcept {return metadata.memory_resource();}
    template<IdentifiableEntity T> RuntimeModule& entity() & {descriptor(T::entity_descriptor(memory_resource()));graph_decoders_.register_entity<T>();return *this;}
    template<IdentifiableEntity T,class Owner,class B> requires std::derived_from<B,EntityDataServiceBehavior>
    RuntimeModule& entity_with_behavior(Owner& owner,B& behavior) & {
        auto value=T::entity_descriptor(memory_resource());
        behaviors_.register_behavior(value.name,owner,behavior);
        descriptor(std::move(value));graph_decoders_.register_entity<T>();return *this;
    }
    template<class Owner,class B> requires std::derived_from<B,EntityDataServiceBehavior>
    RuntimeModule& behavior(std::string_view entity,Owner& owner,B& value) & {behaviors_.register_behavior(entity,owner,value);return *this;}
    RuntimeModule& descriptor(EntityDescriptor value) & {entities_.register_entity(value.name);metadata.register_entity(std::move(value));return *this;}
    RuntimeModule& generated_schema_bootstrap(GeneratedSchemaBootstrap callback) & {if(!callback)throw std::invalid_argument("Schema bootstrap callback is required");bootstraps_.push_back(callback);return *this;}
    template<class Owner,class T> requires std::derived_from<T,RawAuditEventSink>
    RuntimeModule& event_sink(Owner& origin,T& sink) & {event_sinks_.register_sink(origin,sink);return *this;}
    RuntimeModule& initial_graph(const GraphNode& graph) & {initial_graphs_.emplace_back(graph);return *this;}
    RuntimeModule& initial_graphs(std::span<const GraphNode> graphs) & {for(const auto& graph:graphs)initial_graphs_.emplace_back(graph);return *this;}
    RuntimeModule& root_graph(const GraphNode& graph) & {root_graphs_.emplace_back(graph);return *this;}
    RuntimeModule& root_graphs(std::span<const GraphNode> graphs) & {for(const auto& graph:graphs)root_graphs_.emplace_back(graph);return *this;}
    void apply_to(UserContext&) const;
    std::unique_ptr<UserContext> into_context(GlobalContext&) const;
private:
    InMemoryEntityRegistry entities_;
    InMemoryEntityDataServiceBehaviorRegistry behaviors_;
    InMemoryEntityGraphDecoderRegistry graph_decoders_;
    InMemoryRawAuditEventSink event_sinks_;
    Vector<GraphNode> initial_graphs_,root_graphs_;
    Vector<GeneratedSchemaBootstrap> bootstraps_;
};
}
