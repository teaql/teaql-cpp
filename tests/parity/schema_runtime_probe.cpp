#include <teaql/runtime/context.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;
struct Shared{
    using allocator_type=Allocator;Json config;Vector<String> calls;
    Shared(std::allocator_arg_t,Allocator a,const Json& j):config(std::allocator_arg,a,j),calls(a.resource()){}
    void note(const UserContext& context,std::string_view name){String entry(name,calls.get_allocator().resource());entry+=detail::is_generated_schema_bootstrap(context)?":bootstrap":":normal";calls.push_back(std::move(entry));}
};
static Shared& state(const UserContext& context){return const_cast<Shared&>(context.require_resource<Shared>().value().get());}
struct Step{
    using allocator_type=Allocator;Memory r;const UserContext& context;String name;std::uint64_t remaining;bool fail;
    Step(std::allocator_arg_t,Allocator a,const UserContext& context,std::string_view name,std::uint64_t remaining,bool fail):r(a.resource()),context(context),name(name,r),remaining(remaining),fail(fail){}
    void note(std::string_view suffix) const{String message(name,r);message+=suffix;state(context).note(context,message);}
    std::optional<RuntimeSchemaResult> poll(WakeSignal& wake){note(".poll");if(remaining){--remaining;wake.notify();return {};}if(fail){String message(name,r);message+=" failed";return RuntimeSchemaResult::err(r,RuntimeError::schema(r,message));}return RuntimeSchemaResult::ok(r,{});}
    void finish_and_wait() noexcept{note(".finish");}void cancel_and_wait() noexcept{note(".cancel");}
};
struct Provider:runtime::SchemaProvider{
    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& context,const SchemaInvocation&) const override{auto& shared=state(context);shared.note(context,"provider.create");return make_operation<RuntimeSchemaResult,Step>(context.memory_resource(),context,"provider",std::get<std::uint64_t>(field(shared.config,"provider_delay").data),text(field(shared.config,"fail"))=="provider");}
};
struct Business:BusinessIdAllocator,BusinessIdSchemaContributor{
    Result<BusinessIdAllocation,BusinessIdError> allocate(Memory r,const BusinessIdPlan&) const override{return Result<BusinessIdAllocation,BusinessIdError>::err(r,BusinessIdError(std::allocator_arg,Allocator(r),BusinessIdErrorCode::Allocation,"unused"));}
    Result<std::monostate,BusinessIdError> ensure_schema(Memory r,const UserContext& context) const override{auto& shared=state(context);shared.note(context,"business");using R=Result<std::monostate,BusinessIdError>;if(text(field(shared.config,"fail"))=="business")return R::err(r,BusinessIdError(std::allocator_arg,Allocator(r),BusinessIdErrorCode::Allocation,"business failed"));return R::ok(r,{});}
};
static GeneratedSchemaBootstrapFuture& bootstrap(const UserContext& context,unsigned index){auto& shared=state(context);auto r=context.memory_resource();char digit=char('0'+index);String name("callback",r);name+=digit;String create(name,r);create+=".create";shared.note(context,create);auto& delays=std::get<Json::Array>(field(shared.config,"delays").data);return make_operation<RuntimeSchemaResult,Step>(r,context,name,std::get<std::uint64_t>(delays.at(index).data),text(field(shared.config,"fail"))==std::string_view(&digit,1));}
static GeneratedSchemaBootstrapFuture& first(const UserContext& c){return bootstrap(c,0);}static GeneratedSchemaBootstrapFuture& second(const UserContext& c){return bootstrap(c,1);}static GeneratedSchemaBootstrapFuture& third(const UserContext& c){return bootstrap(c,2);}
static Json evaluate(UserContext& context,const Json& j){auto r=context.memory_resource();auto& shared=context.make<Shared>(j);context.insert_resource(context,shared);auto& module=context.make<RuntimeModule>();GeneratedSchemaBootstrap callbacks[]{first,second,third};for(std::size_t i=0;i<std::get<Json::Array>(field(j,"delays").data).size();++i)module.generated_schema_bootstrap(callbacks[i]);module.apply_to(context);if(std::get<bool>(field(j,"provider").data)){auto& provider=context.make_resource<Provider>();context.set_schema_provider(context,provider);}if(std::get<bool>(field(j,"business").data)){auto& business=context.make_resource<Business>();context.set_business_id_infrastructure(context,business);}
 shared.note(context,"before");auto& operation=context.ensure_schema();auto outcome=detail::model_json(r);detail::model_put(outcome,"cancelled",true);for(std::uint64_t i=0;i<std::get<std::uint64_t>(field(j,"polls").data);++i){auto* result=operation.poll();if(!result){shared.note(context,"outside");continue;}outcome=detail::model_json(r);if(*result)detail::model_put(outcome,"ok",true);else{detail::model_put(outcome,"message",result->error().message());detail::model_put(outcome,"display",result->error().to_string(r));}break;}operation.cancel();shared.note(context,"after");auto out=detail::model_json(r);Json::Array calls(r);for(const auto& call:shared.calls)calls.emplace_back(call);detail::model_put(out,"calls",calls);detail::model_put(out,"result",outcome);return out;
}
int main(){GlobalContext global;std::string line;while(std::getline(std::cin,line)){auto context=global.make_user_context();auto r=context->memory_resource();auto j=Json::parse(r,line);if(!j)throw std::runtime_error("invalid fixture JSON");std::cout<<evaluate(*context,*j).dump(r)<<'\n';}}
