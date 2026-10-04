#pragma once
#include <teaql/memory/arena.hpp>
#include <teaql/runtime/resources.hpp>
#include <teaql/runtime/registry.hpp>
#include <teaql/runtime/behavior.hpp>
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/business_clock.hpp>
#include <teaql/runtime/business_id.hpp>
#include <teaql/runtime/id.hpp>
#include <teaql/runtime/schema.hpp>
#include <teaql/runtime/logging.hpp>
#include <teaql/runtime/detail/context_identity.hpp>
#include <exception>

namespace teaql {
class UserContext;
class RuntimeModule;
class ContinuousPageCursorStore;
class IdSetStore;
template<class E> class EntityDataService;
class InMemoryEntityGraphDecoderRegistry;
class LogFormatterFactory;
class LogManager;
class RuntimeTelemetry;
struct RuntimeOperation;
class FailOpenRuntimeTelemetryScope;
class RawAuditEventSink;
class SafeAuditEventSink;
struct RawAuditEvent;
struct MutationGovernanceSnapshot;
struct GraphNode;
namespace detail { class GovernanceActivation; struct ContextEntityAccess; struct ContextPaginationAccess; class IdSetBuildRegistry; }

// Reject common accidental off-arena containers (std::string/vector).
// Foreign RAII wrappers use the explicitly named make_resource() entry point.
template<class T>
concept ArenaObject = std::uses_allocator_v<T, std::pmr::polymorphic_allocator<T>>
                   || std::is_trivially_destructible_v<T>;

// Application-owned global lifetime; never a hidden singleton. Outlives every UserContext.
class GlobalContext final {
public:
    explicit GlobalContext(std::pmr::memory_resource* upstream = std::pmr::new_delete_resource(),
                           std::size_t initial_block_size = 4096)
        : upstream_(upstream), memory_(initial_block_size, upstream, nullptr, *this) {}
    GlobalContext(const GlobalContext&) = delete;
    GlobalContext& operator=(const GlobalContext&) = delete;
    GlobalContext(GlobalContext&&) = delete;
    GlobalContext& operator=(GlobalContext&&) = delete;
    ~GlobalContext() {
        // Detect lifetime inversion before freeing global metadata still borrowed by a context.
        if (live_contexts_ != 0) std::terminate();
        memory_.close();
    }

    template<ArenaObject T, class... Args> T& make(Args&&... args) {
        return memory_.make<T>(std::forward<Args>(args)...);
    }
    // Lifecycle-owned external resources; internal allocations remain the wrapper's responsibility.
    template<class T, class... Args> T& make_resource(Args&&... args) {
        return memory_.make_resource<T>(std::forward<Args>(args)...);
    }
    std::unique_ptr<UserContext> make_user_context(std::size_t initial_block_size = 4096);
    std::pmr::memory_resource* memory_resource() noexcept { return &memory_; }
    AllocationStats allocation_stats() const noexcept { return memory_.stats(); }
    LogFormatterFactory& log_formatter_factory();
    LogManager& log_manager();
private:
    friend class UserContext;
    friend class detail::SchemaOperation;
    friend class detail::GovernanceActivation;
    friend struct detail::ContextPaginationAccess;
    // Borrowed only while an owner-thread operation is being polled/cancelled.
    const MutationGovernanceSnapshot* active_mutation_governance_=nullptr;
    friend bool detail::is_generated_schema_bootstrap(const UserContext&);
    bool generated_schema_bootstrap_=false;
    std::pmr::memory_resource* upstream_;
    detail::ContextArena memory_;
    std::size_t live_contexts_ = 0;
    LogFormatterFactory* log_formatter_factory_=nullptr;
    LogManager* log_manager_=nullptr;
    AtomicCounterIdGenerator* local_id_generator_=nullptr;
    detail::IdSetBuildRegistry* id_set_build_registry_=nullptr;
    AtomicCounterIdGenerator& local_id_generator(){if(!local_id_generator_)local_id_generator_=&make<AtomicCounterIdGenerator>();return *local_id_generator_;}
};

// Thread-confined owner; registered native work is quiesced before memory teardown.
class UserContext final {
public:
    explicit UserContext(GlobalContext& global, std::size_t initial_block_size = 4096)
        : global_(global), memory_(initial_block_size, global.upstream_, &global.memory_, global) {
        if(global.memory_.closed() || global.memory_.closing()) throw std::logic_error("GlobalContext is closing");
        user_identifier_=&memory_.make<String>(detail::default_user_identifier(&memory_));
        ++global_.live_contexts_;
    }
    UserContext(const UserContext&) = delete;
    UserContext& operator=(const UserContext&) = delete;
    UserContext(UserContext&&) = delete;
    UserContext& operator=(UserContext&&) = delete;
    ~UserContext() { close(); --global_.live_contexts_; }

    template<ArenaObject T, class... Args> T& make(Args&&... args) const {
        return const_cast<detail::ContextArena&>(memory_).make<T>(std::forward<Args>(args)...);
    }
    template<class T, class... Args> T& make_resource(Args&&... args) const {
        return const_cast<detail::ContextArena&>(memory_).make_resource<T>(std::forward<Args>(args)...);
    }
    GlobalContext& global() noexcept { return global_; }
    std::pmr::memory_resource* memory_resource() const noexcept { return const_cast<detail::ContextArena*>(&memory_); }
    AllocationStats allocation_stats() const noexcept { return memory_.stats(); }
    bool closed() const noexcept { return memory_.closed(); }
    void close() noexcept { memory_.close(); }
    template<class Owner,class T> requires std::derived_from<T,ContinuousPageCursorStore>
    void set_continuous_page_cursor_store(Owner& origin,T& store){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),store);continuous_page_cursor_store_=&store;}
    template<class Owner,class T> requires std::derived_from<T,IdSetStore>
    void set_id_set_store(Owner& origin,T& store){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),store);id_set_store_=&store;}
    std::optional<String> continuous_page_plan()const;
    std::optional<String> continuous_page_cursor_id()const;
    std::optional<String> id_set_plan()const;
    std::optional<std::uint64_t> id_set_count()const;
    // Explicit origin makes registration a checked Context borrow, never a
    // second owner. Allocate the object with origin.make/make_resource first.
    template<class Owner,class T> void insert_resource(Owner& origin,T& resource){resources().insert(memory_,origin.memory_resource(),resource);}
    template<class Owner,class T> void insert_named_resource(std::string_view name,Owner& origin,T& resource){resources().insert_named(memory_,origin.memory_resource(),name,resource);}
    template<class T> T* get_resource(){ensure_open();auto* entry=resources_?resources_->find<T>():nullptr;return entry?static_cast<T*>(entry->object):nullptr;}
    template<class T> const T* get_resource() const {ensure_open();auto* entry=resources_?resources_->find<T>():nullptr;return entry?static_cast<const T*>(entry->object):nullptr;}
    template<class T> T* get_named_resource(std::string_view name){ensure_open();auto* entry=resources_?resources_->find_named<T>(name):nullptr;return entry?static_cast<T*>(entry->object):nullptr;}
    template<class T> const T* get_named_resource(std::string_view name) const {ensure_open();auto* entry=resources_?resources_->find_named<T>(name):nullptr;return entry?static_cast<const T*>(entry->object):nullptr;}
    template<class T> Result<ResourceBorrow<T>,ContextError> require_resource(){ensure_open();return required<T>(resources_?resources_->find<T>():nullptr,ContextError::Kind::MissingTypedResource,detail::resource_type_name<T>());}
    template<class T> Result<ResourceBorrow<const T>,ContextError> require_resource() const {ensure_open();return required<const T>(resources_?resources_->find<T>():nullptr,ContextError::Kind::MissingTypedResource,detail::resource_type_name<T>());}
    template<class T> Result<ResourceBorrow<T>,ContextError> require_named_resource(std::string_view name){ensure_open();return required<T>(resources_?resources_->find_named<T>(name):nullptr,ContextError::Kind::MissingResource,name);}
    template<class T> Result<ResourceBorrow<const T>,ContextError> require_named_resource(std::string_view name) const {ensure_open();return required<const T>(resources_?resources_->find_named<T>(name):nullptr,ContextError::Kind::MissingResource,name);}
    template<class T> void put_local(std::string_view key,T&& value){resources().put_local(key,Value(&memory_,std::forward<T>(value)));}
    const Value* local(std::string_view key) const {ensure_open();return resources_?resources_->local(key):nullptr;}
    std::optional<Value> remove_local(std::string_view key){ensure_mutable();return resources_?resources_->remove_local(key):std::optional<Value>{};}

    template<class Owner,class T> requires std::derived_from<T,RuntimeTelemetry>
    void set_runtime_telemetry(Owner& origin,T& telemetry){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),telemetry);runtime_telemetry_=&telemetry;}
    template<class Owner,class T> requires std::derived_from<T,RuntimeTelemetry>
    UserContext& with_runtime_telemetry(Owner& origin,T& telemetry){set_runtime_telemetry(origin,telemetry);return *this;}
    const RuntimeTelemetry& runtime_telemetry() const;
    bool runtime_telemetry_is_noop() const;
    FailOpenRuntimeTelemetryScope& start_runtime_operation(const RuntimeOperation&) const;

    std::optional<std::string_view> user_identifier() const {ensure_open();return user_identifier_?std::optional<std::string_view>(*user_identifier_):std::nullopt;}
    void set_user_identifier(std::string_view value){set_user_identifier_option(value);}
    void set_user_identifier_option(std::optional<std::string_view> value){ensure_mutable();if(value)user_identifier_=&make<String>(*value);else user_identifier_=nullptr;}
    UserContext& with_user_identifier(std::string_view value){set_user_identifier(value);return *this;}
    UserContext& with_user_identifier_option(std::optional<std::string_view> value){set_user_identifier_option(value);return *this;}
    UserContext& with_sql_log_options(SqlLogOptions value){set_sql_log_options(value);return *this;}
    void set_sql_log_options(SqlLogOptions value){ensure_mutable();sql_log_options_=value;}
    void enable_select_sql_log(){ensure_mutable();sql_log_options_.select=true;}
    void enable_mutation_sql_log(){ensure_mutable();sql_log_options_.mutation=true;}
    void disable_select_sql_log(){ensure_mutable();sql_log_options_.select=false;}
    void disable_mutation_sql_log(){ensure_mutable();sql_log_options_.mutation=false;}
    void enable_all_sql_log(){set_sql_log_options(SqlLogOptions::all());}
    void disable_sql_log(){set_sql_log_options(SqlLogOptions::disabled());clear_sql_logs();}
    SqlLogOptions sql_log_options() const {ensure_open();return sql_log_options_;}
    Vector<SqlLogEntry> sql_logs() const;
    void clear_sql_logs() const;
    // Internal runtime/provider diagnostic bridge, including failure metadata.
    void record_metadata_log(const ExecutionMetadata&) const;

    template<class Owner,class T> requires std::derived_from<T,RawAuditEventSink>
    void set_event_sink(Owner& origin,T& sink){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),sink);event_sink_=&sink;}
    template<class Owner,class T> requires std::derived_from<T,RawAuditEventSink>
    UserContext& with_event_sink(Owner& origin,T& sink){set_event_sink(origin,sink);return *this;}
    template<class Owner,class T> requires std::derived_from<T,SafeAuditEventSink>
    void set_custom_event_sink(Owner& origin,T& sink){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),sink);custom_event_sink_=&sink;}
    template<class Owner,class T> requires std::derived_from<T,SafeAuditEventSink>
    UserContext& with_custom_event_sink(Owner& origin,T& sink){set_custom_event_sink(origin,sink);return *this;}
    Result<std::monostate,RuntimeError> send_event(const RawAuditEvent&) const;
    std::span<const GraphNode> initial_graphs() const;
    std::span<const GraphNode> root_graphs() const;
    void set_initial_graphs(std::span<const GraphNode>);
    void set_root_graphs(std::span<const GraphNode>);

    template<class E> Result<ResourceBorrow<EntityDataService<E>>,ContextError> entity_data_service(std::string_view entity) const;
    void set_entity_graph_decoder_registry(const InMemoryEntityGraphDecoderRegistry&);
    template<class Owner,class T> requires std::derived_from<T,EntityDataServiceBehaviorRegistry>
    void set_entity_data_service_behavior_registry(Owner& origin,T& registry){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),registry);entity_behaviors_=&registry;}
    template<class Owner,class T> requires std::derived_from<T,EntityDataServiceBehaviorRegistry>
    UserContext& with_entity_data_service_behavior_registry(Owner& origin,T& registry){set_entity_data_service_behavior_registry(origin,registry);return *this;}
    std::optional<EntityBehaviorBorrow> entity_data_service_behavior(std::string_view entity) const;
    template<class Owner,class T> requires std::derived_from<T,RequestPolicy>
    void set_request_policy(Owner& origin,T& policy){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),policy);request_policy_=&policy;}
    template<class Owner,class T> requires std::derived_from<T,RequestPolicy>
    UserContext& with_request_policy(Owner& origin,T& policy){set_request_policy(origin,policy);return *this;}
    void clear_request_policy(){ensure_mutable();request_policy_=nullptr;}

    template<class Owner,class T> requires std::derived_from<T,BusinessClock>
    void set_business_clock(Owner& origin,T& clock){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),clock);business_clock_=&clock;}
    template<class Owner,class T> requires std::derived_from<T,BusinessClock>
    UserContext& with_business_clock(Owner& origin,T& clock){set_business_clock(origin,clock);return *this;}
    UtcDateTime business_time() const {ensure_open();return business_clock_?business_clock_->business_time():SystemBusinessClock{}.business_time();}
    Date business_date() const {if(auto* date=get_resource<BusinessDate>())return date->value;return business_clock_?business_clock_->business_date():SystemBusinessClock{}.business_date();}
    template<class Owner> void set_business_id_service(Owner& origin,BusinessIdService& service){insert_resource(origin,service);}
    template<class Owner> UserContext& with_business_id_service(Owner& origin,BusinessIdService& service){set_business_id_service(origin,service);return *this;}
    Result<ResourceBorrow<const BusinessIdService>,ContextError> business_ids() const {return require_resource<BusinessIdService>();}
    template<class Owner,class T> requires std::derived_from<T,BusinessIdKeyProvider>
    Result<std::monostate,ContextError> set_business_id_key_provider(Owner& origin,T& provider){
        ensure_mutable();auto service=business_ids();using R=Result<std::monostate,ContextError>;
        if(!service)return R::err(&memory_,detail::clone_in(&memory_,service.error()));
        auto& replacement=make<BusinessIdService>(service.value().get());replacement.with_key_provider(origin,provider);insert_resource(*this,replacement);return R::ok(&memory_,{});
    }
    template<class Owner,class T> requires std::derived_from<T,BusinessIdAllocator> && std::derived_from<T,BusinessIdSchemaContributor>
    void set_business_id_infrastructure(Owner& origin,T& allocator){
        ensure_mutable();auto& service=make<BusinessIdService>(origin,allocator);auto& schema=make<detail::BusinessIdSchemaService>(origin,allocator);insert_resource(*this,service);insert_resource(*this,schema);
    }

    template<class Owner,class T> requires std::derived_from<T,MetadataStore>
    void set_metadata(Owner& origin,T& metadata){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),metadata);metadata_=&metadata;}
    template<class Owner,class T> requires std::derived_from<T,MetadataStore>
    UserContext& with_metadata(Owner& origin,T& metadata){set_metadata(origin,metadata);return *this;}
    template<class Owner,class T> requires std::derived_from<T,EntityRegistry>
    void set_entity_registry(Owner& origin,T& registry){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),registry);entity_registry_=&registry;}
    template<class Owner,class T> requires std::derived_from<T,EntityRegistry>
    UserContext& with_entity_registry(Owner& origin,T& registry){set_entity_registry(origin,registry);return *this;}
    const EntityDescriptor* entity(std::string_view name) const {ensure_open();return metadata_?metadata_->entity(name):nullptr;}
    Vector<const EntityDescriptor*> all_entities() const {ensure_open();auto r=const_cast<detail::ContextArena*>(&memory_);return metadata_?metadata_->all_entities(r):Vector<const EntityDescriptor*>(r);}
    Result<ResourceBorrow<const EntityDescriptor>,RuntimeError> require_entity(std::string_view name) const {
        using R=Result<ResourceBorrow<const EntityDescriptor>,RuntimeError>;auto* found=entity(name);auto r=const_cast<detail::ContextArena*>(&memory_);
        if(!found)return R::err(r,RuntimeError::missing_entity(r,name));
        return R::ok(r,ResourceBorrow<const EntityDescriptor>(std::allocator_arg,Allocator(r),*found,detail::resource_arena(found->memory_resource())));
    }
    bool has_entity_data_service(std::string_view name) const {ensure_open();return (entity_registry_ && entity_registry_->contains(name)) || entity(name)!=nullptr;}

    template<class Owner,class T> requires std::derived_from<T,runtime::SchemaProvider>
    void set_schema_provider(Owner& origin,T& provider){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),provider);schema_provider_=&provider;}
    template<class Owner,class T> requires std::derived_from<T,runtime::SchemaProvider>
    UserContext& with_schema_provider(Owner& origin,T& provider){set_schema_provider(origin,provider);return *this;}
    AsyncOperation<RuntimeSchemaResult>& ensure_schema() const;

    template<class Owner,class T> requires std::derived_from<T,InternalIdGenerator>
    void set_internal_id_generator(Owner& origin,T& generator){ensure_mutable();detail::ContextResources::entry(memory_,origin.memory_resource(),generator);internal_id_generator_=&generator;}
    template<class Owner,class T> requires std::derived_from<T,InternalIdGenerator>
    UserContext& with_internal_id_generator(Owner& origin,T& generator){set_internal_id_generator(origin,generator);return *this;}
    Result<std::optional<std::uint64_t>,RuntimeError> generate_id(std::string_view entity) const {
        ensure_open();auto r=const_cast<detail::ContextArena*>(&memory_);using R=Result<std::optional<std::uint64_t>,RuntimeError>;
        if(!internal_id_generator_)return R::ok(r,{});
        auto id=internal_id_generator_->generate_id(r,entity);if(!id)return R::err(r,std::move(id).error());return R::ok(r,id.value());
    }
    Result<std::uint64_t,RuntimeError> next_id(std::string_view entity) const {
        auto generated=generate_id(entity);auto r=const_cast<detail::ContextArena*>(&memory_);using R=Result<std::uint64_t,RuntimeError>;
        if(!generated)return R::err(r,std::move(generated).error());
        if(generated.value())return R::ok(r,*generated.value());
        return global_.local_id_generator().generate_id(r,entity);
    }
    template<Entity E> Result<std::monostate,RuntimeError> initialize_generated_bootstrap_entity(E& entity,std::string_view entity_name,std::uint64_t fixed_id) const {
        ensure_open();auto r=const_cast<detail::ContextArena*>(&memory_);using R=Result<std::monostate,RuntimeError>;
        if(!internal_id_generator_)return R::err(r,RuntimeError::id_generation(r,"missing internal ID generator"));
        auto reserved=internal_id_generator_->ensure_floor(r,entity_name,fixed_id);if(!reserved)return reserved;
        entity.mark_as_new();return R::ok(r,{});
    }

private:
    friend class RuntimeModule;
    friend struct detail::ContextEntityAccess;
    const InMemoryEntityGraphDecoderRegistry* graph_decoders_=nullptr;
    const EntityDataServiceBehaviorRegistry* entity_behaviors_=nullptr;
    const RequestPolicy* request_policy_=nullptr;
    friend class detail::GovernanceActivation;
    friend class detail::SchemaOperation;
    friend bool detail::is_generated_schema_bootstrap(const UserContext&);
    mutable std::size_t pending_schema_operations_=0;
    runtime::SchemaProvider* schema_provider_=nullptr;
    const Vector<GeneratedSchemaBootstrap>* generated_schema_bootstraps_=nullptr;
    void ensure_mutable() const {ensure_open();if(memory_.closing())throw std::logic_error("Context is closing");if(pending_schema_operations_)throw std::logic_error("Context is borrowed by schema initialization");}
    BusinessClock* business_clock_=nullptr;
    MetadataStore* metadata_=nullptr;
    EntityRegistry* entity_registry_=nullptr;
    InternalIdGenerator* internal_id_generator_=nullptr;
    GlobalContext& global_;
    detail::ContextArena memory_;
    void ensure_open() const {if(memory_.closed())throw std::logic_error("Context is closed");}
    detail::ContextResources& resources(){ensure_mutable();if(!resources_)resources_=&memory_.make<detail::ContextResources>();return *resources_;}
    template<class T> Result<ResourceBorrow<T>,ContextError> required(const detail::RegisteredResource* entry,ContextError::Kind missing,std::string_view name) const {
        auto* r=const_cast<detail::ContextArena*>(&memory_);using Output=Result<ResourceBorrow<T>,ContextError>;
        if(!entry)return Output::err(r,ContextError(std::allocator_arg,Allocator(r),missing,name));
        return Output::ok(r,ResourceBorrow<T>(std::allocator_arg,Allocator(r),*static_cast<T*>(entry->object),*entry->owner));
    }
    detail::ContextResources* resources_=nullptr;
    friend struct detail::ContextPaginationAccess;
    mutable ContinuousPageCursorStore* continuous_page_cursor_store_=nullptr;
    mutable IdSetStore* id_set_store_=nullptr;
    struct PaginationObservation;
    mutable PaginationObservation* pagination_observation_=nullptr;
    mutable const RuntimeTelemetry* runtime_telemetry_=nullptr;
    const RawAuditEventSink* event_sink_=nullptr;
    const SafeAuditEventSink* custom_event_sink_=nullptr;
    const Vector<GraphNode>* initial_graphs_=nullptr;
    const Vector<GraphNode>* root_graphs_=nullptr;
    String* user_identifier_=nullptr;
    SqlLogOptions sql_log_options_;
    mutable Vector<SqlLogEntry>* sql_log_entries_=nullptr;

};

inline std::unique_ptr<UserContext> GlobalContext::make_user_context(std::size_t initial_block_size) {
    return std::make_unique<UserContext>(*this, initial_block_size);
}
}
