#include <teaql/sql/transport.hpp>
#include <teaql/sql/error.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>
static std::uint64_t integer(const Json& j){return std::get<std::uint64_t>(j.data);}
static Json snapshot(Memory r,const CompiledQuery& q){auto out=detail::model_json(r);detail::model_put(out,"sql",q.sql);detail::model_put(out,"comment",q.comment?Json(r,*q.comment):Json(r));Json::Array values(r),policies(r);for(const auto& v:q.params)values.emplace_back(v.to_json_value(r));for(auto p:q.log_context.parameter_policies)policies.emplace_back(unsigned(p));detail::model_put(out,"params",values);detail::model_put(out,"policies",policies);detail::model_put(out,"generated",q.log_context.generated_sql);return out;}
template<class T> struct Ready {using allocator_type=Allocator;T value;Ready(std::allocator_arg_t,Allocator a,T&& value):value(detail::move_in(a.resource(),std::move(value))){}std::optional<T> poll(WakeSignal&){return std::move(value);}void finish_and_wait() noexcept{}void cancel_and_wait() noexcept{}};
struct Transport:SqlTransport<String>{
    using allocator_type=Allocator;Memory resource;std::size_t index,fail;Vector<std::size_t> counts;Json::Array calls;
    Transport(std::allocator_arg_t,Allocator a,const Json& j):resource(a.resource()),index(integer(field(j,"index"))),fail(integer(field(j,"fail"))),counts(a.resource()),calls(a.resource()){for(const auto& c:std::get<Json::Array>(field(j,"counts").data))counts.push_back(integer(c));}
    DataServiceOperationResult<Rows,String>& fetch_all_compact_sql(Memory r,const CompiledQuery& query) override{
        calls.emplace_back(snapshot(r,query));using R=Result<Rows,String>;
        if(fail==calls.size())return make_operation<R,Ready<R>>(r,R::err(r,String("fetch failed",r)));
        Rows rows(r);auto n=calls.size()<=counts.size()?counts[calls.size()-1]:1;
        for(std::size_t i=0;i<n;++i){rows.emplace_back();rows.back().insert("call",Value(r,calls.size()));rows.back().insert("row",Value(r,i));rows.back().insert("value",query.params[index]);}
        return make_operation<R,Ready<R>>(r,R::ok(r,std::move(rows)));
    }
    DataServiceOperationResult<std::uint64_t,String>& execute_sql(Memory,const CompiledQuery&) override{throw std::logic_error("unused fixture path");}
};
static Json evaluate(UserContext& user,const Json& j){
    auto r=user.memory_resource();auto out=detail::model_json(r);
    if(auto mode=optional(j,"error_kind")){
        auto name=text(field(j,"message"));auto kind=text(*mode);using E=SqlExecutorError<String>;
        auto error=kind=="compile"?E::compile(r,SqlCompileError(std::allocator_arg,Allocator(r),SqlCompileError::Code::UnknownField,name)):kind=="transport"?E::transport(r,String(name,r)):E::persisted_record(r,name);
        detail::model_put(out,"text",error.to_string(r));return out;
    }
    auto& q=user.make<CompiledQuery>();q.sql=text(field(j,"sql"));for(const auto& p:std::get<Json::Array>(field(j,"params").data))q.params.emplace_back(value(r,p));
    for(const auto& p:std::get<Json::Array>(field(j,"policies").data))q.log_context.parameter_policies.push_back(SqlParameterLogPolicy(integer(p)));
    q.log_context.generated_sql=std::get<bool>(field(j,"generated").data);if(auto p=optional(j,"comment"))if(std::holds_alternative<String>(p->data))q.comment.emplace(text(*p),r);
    auto& transport=user.make<Transport>(j);Vector<Value> values(r);for(const auto& v:std::get<Json::Array>(field(j,"values").data))values.emplace_back(value(r,v));
    auto& operation=transport.fetch_repeated_compact_sql(r,q,integer(field(j,"index")),values);detail::model_put(out,"calls_before_poll",transport.calls.size());
    try{const auto& result=operation.wait();if(result){Json::Array rows(r);for(const auto& row:result.value())rows.emplace_back(compact_row_to_json_value(r,row));detail::model_put(out,"rows",rows);}else detail::model_put(out,"error",result.error());}
    catch(const std::out_of_range&){detail::model_put(out,"panic","index");}
    detail::model_put(out,"calls",transport.calls);detail::model_put(out,"original",snapshot(r,q));return out;
}
int main(){GlobalContext global;std::string line;while(std::getline(std::cin,line)){auto user=global.make_user_context();auto r=user->memory_resource();auto j=Json::parse(r,line);if(!j)throw std::runtime_error("invalid fixture");std::cout<<evaluate(*user,*j).dump(r)<<'\n';}}
