#pragma once
#include <teaql/core/trace.hpp>
#include <teaql/runtime/mutation_governance.hpp>
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/resources.hpp>
#include <span>
namespace teaql {
class UserContext;
enum class RawAuditEventKind {Created,Updated,Deleted,Recovered,SchemaCreated,SchemaVerified,FieldAdded,DataSeeded};
struct EntityPropertyChange {
    using allocator_type=Allocator;
    String field;std::optional<Value> old_value,new_value;
    EntityPropertyChange(std::allocator_arg_t,Allocator a,std::string_view field,const std::optional<Value>& old_value={},const std::optional<Value>& new_value={}):field(field,a.resource()),old_value(detail::clone_in(a.resource(),old_value)),new_value(detail::clone_in(a.resource(),new_value)){}
    EntityPropertyChange(std::allocator_arg_t,Allocator a,const EntityPropertyChange& other):EntityPropertyChange(std::allocator_arg,a,other.field,other.old_value,other.new_value){}
    EntityPropertyChange(std::allocator_arg_t,Allocator a,EntityPropertyChange&& other):EntityPropertyChange(detail::move_to(a.resource(),std::move(other))){}
    EntityPropertyChange(const EntityPropertyChange&)=delete;EntityPropertyChange(EntityPropertyChange&&) noexcept=default;
    EntityPropertyChange& operator=(EntityPropertyChange&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept{return field.get_allocator().resource();}
    bool operator==(const EntityPropertyChange&) const=default;
};
struct BootstrapAuditIdentity {
    using allocator_type=Allocator;
    String actor,category,reason;std::optional<std::int64_t> resulting_version;std::uint64_t occurred_at_millis;
    BootstrapAuditIdentity(std::allocator_arg_t,Allocator a,std::string_view actor,std::string_view category,std::string_view reason,std::optional<std::int64_t> version,std::uint64_t at):actor(actor,a.resource()),category(category,a.resource()),reason(reason,a.resource()),resulting_version(version),occurred_at_millis(at){}
    BootstrapAuditIdentity(std::allocator_arg_t,Allocator a,const BootstrapAuditIdentity& other):BootstrapAuditIdentity(std::allocator_arg,a,other.actor,other.category,other.reason,other.resulting_version,other.occurred_at_millis){}
    BootstrapAuditIdentity(std::allocator_arg_t,Allocator a,BootstrapAuditIdentity&& other):BootstrapAuditIdentity(detail::move_to(a.resource(),std::move(other))){}
    BootstrapAuditIdentity(const BootstrapAuditIdentity&)=delete;BootstrapAuditIdentity(BootstrapAuditIdentity&&) noexcept=default;
    BootstrapAuditIdentity& operator=(BootstrapAuditIdentity&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept{return actor.get_allocator().resource();}
    bool operator==(const BootstrapAuditIdentity&) const=default;
};
struct SafeAuditField {
    using allocator_type=Allocator;
    String name;std::optional<String> value;bool masked=false,truncated=false;
    std::optional<std::size_t> raw_length,output_length;
    std::optional<String> mask_reason,truncate_reason;
    SafeAuditField(std::allocator_arg_t,Allocator a,std::string_view name):name(name,a.resource()){}
    SafeAuditField(std::allocator_arg_t,Allocator a,const SafeAuditField& other):name(other.name,a.resource()),value(detail::clone_in(a.resource(),other.value)),masked(other.masked),truncated(other.truncated),raw_length(other.raw_length),output_length(other.output_length),mask_reason(detail::clone_in(a.resource(),other.mask_reason)),truncate_reason(detail::clone_in(a.resource(),other.truncate_reason)){}
    SafeAuditField(std::allocator_arg_t,Allocator a,SafeAuditField&& other):SafeAuditField(detail::move_to(a.resource(),std::move(other))){}
    SafeAuditField(const SafeAuditField&)=delete;SafeAuditField(SafeAuditField&&) noexcept=default;
    SafeAuditField& operator=(SafeAuditField&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept{return name.get_allocator().resource();}
    bool operator==(const SafeAuditField&) const=default;
};
struct SafeAuditEvent {
    using allocator_type=Allocator;
    RawAuditEventKind kind;String entity;Vector<SafeAuditField> fields;TraceChain trace_chain;
    std::optional<MutationGovernanceSnapshot> mutation_governance;
    SafeAuditEvent(std::allocator_arg_t,Allocator a,RawAuditEventKind kind,std::string_view entity):kind(kind),entity(entity,a.resource()),fields(a.resource()),trace_chain(a.resource()){}
    SafeAuditEvent(std::allocator_arg_t,Allocator a,const SafeAuditEvent& other):kind(other.kind),entity(other.entity,a.resource()),fields(other.fields,a.resource()),trace_chain(other.trace_chain,a.resource()),mutation_governance(detail::clone_in(a.resource(),other.mutation_governance)){}
    SafeAuditEvent(std::allocator_arg_t,Allocator a,SafeAuditEvent&& other):SafeAuditEvent(detail::move_to(a.resource(),std::move(other))){}
    SafeAuditEvent(const SafeAuditEvent&)=delete;SafeAuditEvent(SafeAuditEvent&&) noexcept=default;
    SafeAuditEvent& operator=(SafeAuditEvent&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept{return entity.get_allocator().resource();}
    bool operator==(const SafeAuditEvent&) const=default;
};
struct RawAuditEvent {
    using allocator_type=Allocator;
    RawAuditEventKind kind;String entity;Record values;Vector<String> updated_fields;
    std::optional<Record> old_values,new_values;Vector<EntityPropertyChange> changes;TraceChain trace_chain;
    std::optional<BootstrapAuditIdentity> bootstrap_audit;
    std::optional<MutationGovernanceSnapshot> mutation_governance;
    RawAuditEvent(std::allocator_arg_t,Allocator a,RawAuditEventKind kind,std::string_view entity):kind(kind),entity(entity,a.resource()),values(a.resource()),updated_fields(a.resource()),changes(a.resource()),trace_chain(a.resource()){}
    RawAuditEvent(std::allocator_arg_t,Allocator a,const RawAuditEvent& other):kind(other.kind),entity(other.entity,a.resource()),values(other.values,a.resource()),updated_fields(other.updated_fields,a.resource()),old_values(detail::clone_in(a.resource(),other.old_values)),new_values(detail::clone_in(a.resource(),other.new_values)),changes(other.changes,a.resource()),trace_chain(other.trace_chain,a.resource()),bootstrap_audit(detail::clone_in(a.resource(),other.bootstrap_audit)),mutation_governance(detail::clone_in(a.resource(),other.mutation_governance)){}
    RawAuditEvent(std::allocator_arg_t,Allocator a,RawAuditEvent&& other):RawAuditEvent(detail::move_to(a.resource(),std::move(other))){}
    RawAuditEvent(const RawAuditEvent&)=delete;RawAuditEvent(RawAuditEvent&&) noexcept=default;
    RawAuditEvent& operator=(RawAuditEvent&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept{return entity.get_allocator().resource();}
    bool operator==(const RawAuditEvent&) const=default;
    static RawAuditEvent created(Memory,std::string_view,const Record&);
    static RawAuditEvent updated(Memory,std::string_view,const Record&);
    static RawAuditEvent updated_with_old_values(Memory,std::string_view,const Record&,const std::optional<Record>&,const Record&,std::span<const String>);
    static RawAuditEvent deleted(Memory,std::string_view,const Value&,std::optional<std::int64_t>);
    static RawAuditEvent deleted_with_old_values(Memory,std::string_view,const Value&,std::optional<std::int64_t>,const std::optional<Record>&);
    static RawAuditEvent recovered(Memory,std::string_view,const Value&,std::int64_t);
    static RawAuditEvent recovered_with_old_values(Memory,std::string_view,const Value&,std::int64_t,const std::optional<Record>&);
    static RawAuditEvent schema_created(Memory,std::string_view,std::string_view,std::size_t);
    static RawAuditEvent schema_verified(Memory,std::string_view,std::string_view,std::size_t);
    static RawAuditEvent field_added(Memory,std::string_view,std::string_view,std::string_view);
    static RawAuditEvent data_seeded(Memory,std::string_view,std::string_view,std::size_t,std::size_t);
    SafeAuditEvent build_safe_event(Memory,std::span<const String>,std::optional<std::size_t>) const;
};
std::pair<String,bool> limit_audit_value(Memory,std::string_view,std::size_t);
SafeAuditField build_safe_audit_field(Memory,std::string_view,std::optional<std::string_view>,std::span<const String>,std::optional<std::size_t>);
class RawAuditEventSink {
public:virtual ~RawAuditEventSink()=default;
    virtual Result<std::monostate,RuntimeError> on_event(Memory,const UserContext&,const RawAuditEvent&) const=0;
};
class SafeAuditEventSink {
public:virtual ~SafeAuditEventSink()=default;
    virtual Result<std::monostate,RuntimeError> on_safe_event(Memory,const UserContext&,const SafeAuditEvent&) const=0;
};
class InMemoryRawAuditEventSink final:public RawAuditEventSink {
public:
    using allocator_type=Allocator;
    InMemoryRawAuditEventSink(std::allocator_arg_t,Allocator a):sinks_(a.resource()){}
    InMemoryRawAuditEventSink(std::allocator_arg_t,Allocator a,const InMemoryRawAuditEventSink& other):sinks_(other.sinks_,a.resource()){}
    InMemoryRawAuditEventSink(const InMemoryRawAuditEventSink&)=delete;
    template<class Owner,class T> requires std::derived_from<T,RawAuditEventSink>
    void register_sink(Owner& owner,T& sink){auto& arena=detail::resource_arena(memory_resource());auto entry=detail::ContextResources::entry(arena,owner.memory_resource(),sink);sinks_.emplace_back(static_cast<const RawAuditEventSink&>(sink),*entry.owner);}
    template<class Owner,class T> requires std::derived_from<T,RawAuditEventSink>
    InMemoryRawAuditEventSink& with_sink(Owner& owner,T& sink){register_sink(owner,sink);return *this;}
    Result<std::monostate,RuntimeError> on_event(Memory,const UserContext&,const RawAuditEvent&) const override;
    Memory memory_resource() const noexcept{return sinks_.get_allocator().resource();}
private:Vector<ResourceBorrow<const RawAuditEventSink>> sinks_;
};
}
