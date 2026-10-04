#include <teaql/runtime/context.hpp>
#include <teaql/runtime/logging.hpp>
#include <teaql/runtime/log_formatter.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace teaql;
static void opt(std::optional<String>& to,Memory r,const Json& j){if(std::holds_alternative<String>(j.data))to.emplace(text(j),r);}
static Json evaluate(Memory r,const Json& j){
    SqlLogEntry e(std::allocator_arg,Allocator(r),SqlLogOperation(number<unsigned>(field(j,"operation"))));
    auto& c=e.log_context;opt(c.database_kind,r,field(j,"database"));opt(c.mode,r,field(j,"mode"));opt(c.omission_reason,r,field(j,"omission"));c.generated_sql=std::get<bool>(field(j,"generated").data);
    for(const auto& p:std::get<Json::Array>(field(j,"policies").data))c.parameter_policies.push_back(SqlParameterLogPolicy(number<unsigned>(p)));
    for(const auto& p:std::get<Json::Array>(field(j,"masked").data))c.masked_parameters.push_back(std::get<bool>(p.data));
    if(auto& outcome=field(j,"outcome");std::holds_alternative<String>(outcome.data))c.execution_outcome=SqlExecutionOutcome(number<unsigned>(outcome));
    e.sql=text(field(j,"sql"));e.debug_sql=text(field(j,"debug"));e.pretty_sql=text(field(j,"pretty"));for(const auto& v:std::get<Json::Array>(field(j,"params").data))e.params.emplace_back(value(r,v));
    opt(e.comment,r,field(j,"comment"));opt(e.purpose,r,field(j,"purpose"));opt(e.audit_reason,r,field(j,"reason"));opt(e.result_type,r,field(j,"type"));e.result_summary=text(field(j,"summary"));
    for(const auto& n:std::get<Json::Array>(field(j,"trace").data))e.trace_path.emplace_back(TraceKind(number<unsigned>(field(n,"kind"))),text(field(n,"entity")),text(field(n,"comment")),std::uint64_t(1001));
    e.started_at=SqlLogEntry::SystemTime(std::chrono::nanoseconds(number<std::int64_t>(field(j,"start"))));e.ended_at=SqlLogEntry::SystemTime(std::chrono::nanoseconds(number<std::int64_t>(field(j,"end"))));e.elapsed={number<std::uint64_t>(field(j,"seconds")),number<std::uint32_t>(field(j,"nanos"))};
    if(std::holds_alternative<String>(field(j,"count").data))e.result_count=number<std::size_t>(field(j,"count"));if(std::holds_alternative<String>(field(j,"affected").data))e.affected_rows=number<std::uint64_t>(field(j,"affected"));
    for(const auto& v:std::get<Json::Array>(field(j,"intent_ids").data))c.intent_redactions.capture_target_id(value(r,v));
    if(std::get<bool>(field(j,"intent_bindings").data))c.intent_redactions.extend(SqlIntentRedactions::from_bindings(r,c,e.params,e.sql));
    if(auto* format=optional(j,"formatter");format && std::get<bool>(format->data)){HumanReaderFormatter human;DebugReaderFormatter debug;auto out=detail::model_json(r);auto trace=std::span<const TraceNode>(e.trace_path);if(auto* empty=optional(j,"empty_trace");empty && std::get<bool>(empty->data))trace={};auto text=human.format_sql_log(r,trace,e);text.erase(0,text.find("]-")+1);detail::model_put(out,"human",text);detail::model_put(out,"debug",debug.format_sql_log(r,trace,e));return out;}
    auto original=detail::debug_sql_log(r,e);auto out=detail::model_json(r);detail::model_put(out,"original",original);Json::Array projections(r);
    for(const auto& step:std::get<Json::Array>(field(j,"steps").data)){
        auto* patch=optional(step,"patch");if(patch){auto name=text(field(*patch,"name"));const auto& v=field(*patch,"value");if(name=="sql")e.sql=text(v);else if(name=="debug")e.debug_sql=text(v);else if(name=="pretty")e.pretty_sql=text(v);else if(name=="comment")e.comment.emplace(text(v),r);else if(name=="summary")e.result_summary=text(v);else if(name=="mode")e.log_context.mode.emplace(text(v),r);else if(name=="param")e.params[0]=value(r,v);else if(name=="start")e.started_at+=std::chrono::nanoseconds(1);else if(name=="count")e.result_count=9;else if(name=="intent")e.log_context.intent_redactions.capture_target_id(value(r,v));else if(name=="policy")e.log_context.parameter_policies[0]=SqlParameterLogPolicy(number<unsigned>(v));else if(name=="masked")e.log_context.masked_parameters[0]=std::get<bool>(v.data);else if(name=="trace")e.trace_path[0].comment=text(v);else if(name=="clear_state")e.log_context.projection_state=SqlProjectionState(r);}
        auto before=detail::debug_sql_log(r,e);auto projected=detail::sql_entry_projection(r,e,std::get<bool>(field(step,"allow").data));if(detail::debug_sql_log(r,e)!=before)throw std::runtime_error("projection mutated source");e=std::move(projected);projections.emplace_back(detail::debug_sql_log(r,e));
    }
    detail::model_put(out,"projections",projections);return out;
}
int main(){GlobalContext global;std::string line;while(std::getline(std::cin,line)){auto user=global.make_user_context();auto r=user->memory_resource();auto j=Json::parse(r,line);if(!j)throw std::runtime_error("invalid input");std::cout<<evaluate(r,*j).dump(r)<<'\n';}}
