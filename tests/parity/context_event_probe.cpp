#include <teaql/runtime/detail/governance_scope.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <chrono>
#include <iostream>
using namespace teaql;
using Outcome=Result<std::monostate,RuntimeError>;
static bool flag(const Json& j,std::string_view key){return std::get<bool>(field(j,key).data);}
struct State{
    using allocator_type=Allocator;Memory r;Json input;Vector<String> notes;std::optional<RawAuditEvent> raw;std::optional<SafeAuditEvent> safe;String outcome;bool unchanged=false;
    State(std::allocator_arg_t,Allocator a,const Json& j):r(a.resource()),input(r,j),notes(r),outcome(r){}
    void note(std::string_view s){notes.emplace_back(s);}
};
struct Raw final:RawAuditEventSink{State& s;explicit Raw(State& s):s(s){}Outcome on_event(Memory r,const UserContext&,const RawAuditEvent& e) const override{s.note("raw");s.raw=detail::clone_in(s.r,e);auto behavior=text(field(s.input,"raw"));if(behavior=="throw")throw std::runtime_error("sink panic");if(behavior=="error")return Outcome::err(r,RuntimeError::event(r,"raw failed"));return Outcome::ok(r,{});}};
struct Safe final:SafeAuditEventSink{State& s;explicit Safe(State& s):s(s){}Outcome on_safe_event(Memory r,const UserContext&,const SafeAuditEvent& e) const override{s.note("safe");s.safe=detail::clone_in(s.r,e);auto behavior=text(field(s.input,"safe"));if(behavior=="throw")throw std::runtime_error("sink panic");if(behavior=="error")return Outcome::err(r,RuntimeError::event(r,"safe failed"));return Outcome::ok(r,{});}};
struct Scope final:RuntimeTelemetryScope{State& s;explicit Scope(State& s):s(s){}~Scope(){s.note("drop");}void success(const RuntimeAttributes&) override{s.note("success");}void failure(std::string_view type) override{s.note(type);}};
struct Adapter final:RuntimeTelemetry{using allocator_type=Allocator;Memory r;State& s;Adapter(std::allocator_arg_t,Allocator a,State& s):r(a.resource()),s(s){}Memory memory_resource() const noexcept override{return r;}ContextBox<RuntimeTelemetryScope>& start(Memory target,const RuntimeOperation& op) const override{if(op.family!="audit" || op.name!="Order.event")throw std::logic_error("operation labels");s.note("start");return detail::resource_arena(target).make_resource<ContextBox<RuntimeTelemetryScope>>(std::in_place_type<Scope>,s);}};
static MutationGovernanceSnapshot snapshot(Memory r,std::string_view name){return MutationGovernanceSnapshot(std::allocator_arg,Allocator(r),name,"request",MutationPolicySource::Customer,MutationPolicyApprovalStatus::Approved);}
struct Send{
    using allocator_type=Allocator;const UserContext& c;State& s;
    Send(std::allocator_arg_t,Allocator,const UserContext& c,State& s):c(c),s(s){}
    std::optional<Outcome> poll(WakeSignal&){auto r=c.memory_resource();const auto& j=s.input;Record values(r);values.emplace(String("name",r),Value(r,"private name"));values.emplace(String("version",r),Value(r,7));auto e=RawAuditEvent::created(r,"Order",values);e.kind=static_cast<RawAuditEventKind>(std::get<std::uint64_t>(field(j,"kind").data));
        auto version=text(field(j,"version"));if(version=="missing")e.new_values.reset();else if(version=="empty")e.new_values->clear();else if(version=="null")e.new_values->insert_or_assign(String("version",r),Value(r));else if(version=="text")e.new_values->insert_or_assign(String("version",r),Value(r,"bad"));
        if(!std::holds_alternative<std::monostate>(field(j,"comment").data))e.trace_chain.emplace_back(TraceKind::Comment,"Order",text(field(j,"comment")));e.mutation_governance=snapshot(r,"forged");if(flag(j,"identity"))e.bootstrap_audit.emplace(std::allocator_arg,Allocator(r),"caller","caller","caller",99,1);
        auto result=c.send_event(e);s.unchanged=e.mutation_governance->execution_id=="forged";return result;
    }
    void finish_and_wait() noexcept{}void cancel_and_wait() noexcept{}
};
static AsyncOperation<Outcome>& send_operation(const UserContext& c,State& s){auto& child=make_operation<Outcome,Send>(c.memory_resource(),c,s);if(flag(s.input,"scope")){auto g=snapshot(c.memory_resource(),"accepted");return detail::with_mutation_governance(c,g,child);}return child;}
struct Ready{Memory r;std::optional<Outcome> poll(WakeSignal&){return Outcome::ok(r,{});}void finish_and_wait() noexcept{}void cancel_and_wait() noexcept{}};
struct Provider final:runtime::SchemaProvider{AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& c,const SchemaInvocation&) const override{return make_operation<Outcome,Ready>(c.memory_resource(),Ready{c.memory_resource()});}};
static GeneratedSchemaBootstrapFuture& bootstrap(const UserContext& c){auto& s=const_cast<State&>(c.require_resource<State>().value().get());return send_operation(c,s);}
static Json evaluate(GlobalContext& global,const Json& j){auto user=global.make_user_context();auto r=user->memory_resource();auto& s=global.make<State>(j);user->insert_resource(global,s);auto& adapter=global.make<Adapter>(s);user->set_runtime_telemetry(global,adapter);
    if(text(field(j,"raw"))!="none"){auto& sink=global.make_resource<Raw>(s);user->set_event_sink(global,sink);}if(text(field(j,"safe"))!="none"){auto& sink=global.make_resource<Safe>(s);user->set_custom_event_sink(global,sink);}
    if(flag(j,"bootstrap")){auto& module=user->make<RuntimeModule>();if(text(field(j,"raw"))!="none"){auto& sink=global.make_resource<Raw>(s);module.event_sink(global,sink);}module.generated_schema_bootstrap(bootstrap);module.apply_to(*user);auto& provider=global.make_resource<Provider>();user->set_schema_provider(global,provider);}
    if(flag(j,"metadata")){auto& metadata=user->make<InMemoryMetadataStore>();EntityDescriptor d(std::allocator_arg,Allocator(r),"Order");d.audit_mask_fields.emplace_back("name");d.audit_value_max_len=3;metadata.register_entity(std::move(d));user->set_metadata(*user,metadata);}
    auto before=std::chrono::system_clock::now();try{auto& op=flag(j,"bootstrap")?user->ensure_schema():send_operation(*user,s);auto& result=op.wait();s.outcome=result?"ok":result.error().message();}catch(const std::runtime_error&){s.outcome="panic";s.unchanged=true;}auto after=std::chrono::system_clock::now();
    auto out=detail::model_json(r);Json::Array notes(r);for(auto& n:s.notes)notes.emplace_back(n);detail::model_put(out,"notes",notes);detail::model_put(out,"outcome",s.outcome);detail::model_put(out,"outside",!detail::current_mutation_governance(*user));detail::model_put(out,"unchanged",s.unchanged);
    Json raw(r);if(s.raw){raw=detail::model_json(r);if(s.raw->mutation_governance)detail::model_put(raw,"governance",s.raw->mutation_governance->execution_id);else detail::model_put(raw,"governance",Json(r));Json b(r);if(s.raw->bootstrap_audit){const auto& v=*s.raw->bootstrap_audit;b=detail::model_json(r);detail::model_put(b,"actor",v.actor);detail::model_put(b,"category",v.category);detail::model_put(b,"reason",v.reason);detail::model_put(b,"version",v.resulting_version?Json(r,*v.resulting_version):Json(r));bool valid=v.actor=="caller"?v.occurred_at_millis==1:v.occurred_at_millis>=std::chrono::duration_cast<std::chrono::milliseconds>(before.time_since_epoch()).count() && v.occurred_at_millis<=std::chrono::duration_cast<std::chrono::milliseconds>(after.time_since_epoch()).count();detail::model_put(b,"time_valid",valid);}detail::model_put(raw,"bootstrap",b);}detail::model_put(out,"raw",raw);
    Json safe(r);if(s.safe){safe=detail::model_json(r);if(s.safe->mutation_governance)detail::model_put(safe,"governance",s.safe->mutation_governance->execution_id);else detail::model_put(safe,"governance",Json(r));Json::Array fields(r);for(const auto& f:s.safe->fields){auto item=detail::model_json(r);detail::model_put(item,"name",f.name);detail::model_put(item,"value",f.value?Json(r,*f.value):Json(r));detail::model_put(item,"masked",f.masked);detail::model_put(item,"truncated",f.truncated);fields.emplace_back(std::move(item));}detail::model_put(safe,"fields",fields);}detail::model_put(out,"safe",safe);return Json(global.memory_resource(),out);
}
int main(){setenv("TEAQL_LOG_ENDPOINT","off",1);unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");std::string line;while(std::getline(std::cin,line)){GlobalContext global;auto r=global.memory_resource();auto j=Json::parse(r,line);std::cout<<evaluate(global,*j).dump(r)<<'\n';}}
