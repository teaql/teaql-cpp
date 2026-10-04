#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
namespace teaql {
namespace detail {
bool is_generated_schema_bootstrap(const UserContext& context){context.ensure_open();return context.global_.generated_schema_bootstrap_;}
class SchemaOperation {
public:
    using allocator_type=Allocator;
    SchemaOperation(std::allocator_arg_t,Allocator a,const UserContext& context):resource_(a.resource()),context_(context){++context_.pending_schema_operations_;}
    std::optional<RuntimeSchemaResult> poll(WakeSignal& wake){
        if(phase_==Phase::Provider){
            if(!child_){
                if(!context_.schema_provider_)return RuntimeSchemaResult::err(resource_,RuntimeError::schema(resource_,"missing schema provider"));
                child_=&context_.schema_provider_->ensure_schema(context_,invocation_);check_child();
            }
            auto result=poll_child(wake);if(!result)return {};
            if(!*result)return result;
            phase_=Phase::Business;
        }
        if(phase_==Phase::Business){
            if(auto* service=context_.get_resource<BusinessIdSchemaService>()){
                auto result=service->ensure_schema(resource_,context_);
                if(!result)return RuntimeSchemaResult::err(resource_,RuntimeError::schema(resource_,result.error().to_string(resource_)));
            }
            phase_=Phase::Bootstrap;
        }
        Scope scope(context_.global_.generated_schema_bootstrap_);
        while(context_.generated_schema_bootstraps_ && index_<context_.generated_schema_bootstraps_->size()){
            if(!child_){child_=&context_.generated_schema_bootstraps_->at(index_)(context_);check_child();}
            auto result=poll_child(wake);if(!result){
                // Children register after the outer operation. Run this parent
                // first at Context close so all descendant cancellation happens
                // inside the same dynamic scope as Rust TaskLocalFuture::drop.
                closing_.attach(resource_arena(resource_),this,[](void* p) noexcept{static_cast<SchemaOperation*>(p)->cancel_and_wait();});
                return {};
            }
            if(!*result)return result;
            ++index_;
        }
        return RuntimeSchemaResult::ok(resource_,{});
    }
    void finish_and_wait() noexcept{closing_.reset();forwarding_.reset();child_=nullptr;release_borrow();}
    void cancel_and_wait() noexcept{
        closing_.reset();forwarding_.reset();
        if(child_){if(phase_==Phase::Bootstrap){Scope scope(context_.global_.generated_schema_bootstrap_);child_->cancel();}else child_->cancel();child_=nullptr;}
        release_borrow();
    }
private:
    struct Scope {bool& active;bool previous;explicit Scope(bool& active):active(active),previous(active){active=true;}~Scope(){active=previous;}};
    enum class Phase{Provider,Business,Bootstrap};
    void check_child(){if(child_->memory_resource()!=resource_){child_=nullptr;throw std::invalid_argument("Schema operation must belong to its UserContext");}}
    std::optional<RuntimeSchemaResult> poll_child(WakeSignal& wake){
        forwarding_.attach(child_->wake_signal(),wake);auto* result=child_->poll();if(!result)return {};
        auto copy=clone_in(resource_,*result);forwarding_.reset();child_=nullptr;return copy;
    }
    void release_borrow() noexcept{if(borrowed_){--context_.pending_schema_operations_;borrowed_=false;}}
    bool borrowed_=true;
    SchemaInvocation invocation_;
    Memory resource_;
    const UserContext& context_;
    Phase phase_=Phase::Provider;
    AsyncOperation<RuntimeSchemaResult>* child_=nullptr;
    std::size_t index_=0;
    WakeSignal::Forward forwarding_;
    Arena::Quiescence closing_;
};
}
AsyncOperation<RuntimeSchemaResult>& UserContext::ensure_schema() const {
    ensure_open();return make_operation<RuntimeSchemaResult,detail::SchemaOperation>(memory_resource(),*this);
}
void RuntimeModule::apply_to(UserContext& context) const {
    context.ensure_mutable();
    auto& metadata_copy=context.make<InMemoryMetadataStore>(metadata);
    auto& entities_copy=context.make<InMemoryEntityRegistry>(entities_);
    auto& behaviors=context.make<InMemoryEntityDataServiceBehaviorRegistry>(behaviors_);
    auto& decoders=context.make<InMemoryEntityGraphDecoderRegistry>(graph_decoders_);
    auto& sinks=context.make<InMemoryRawAuditEventSink>(event_sinks_);
    auto& callbacks=context.make<Vector<GeneratedSchemaBootstrap>>(bootstraps_);
    auto& initial=context.make<Vector<GraphNode>>(initial_graphs_);
    auto& roots=context.make<Vector<GraphNode>>(root_graphs_);
    context.set_metadata(context,metadata_copy);context.set_entity_registry(context,entities_copy);
    context.set_event_sink(context,sinks);
    context.set_entity_data_service_behavior_registry(context,behaviors);
    context.graph_decoders_=&decoders;
    context.initial_graphs_=&initial;context.root_graphs_=&roots;
    context.generated_schema_bootstraps_=&callbacks;
}
std::unique_ptr<UserContext> RuntimeModule::into_context(GlobalContext& global) const {
    auto context=global.make_user_context();apply_to(*context);return context;
}
}
