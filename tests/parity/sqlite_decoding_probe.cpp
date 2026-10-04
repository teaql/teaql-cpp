#include <teaql/provider_sqlite/detail/decoding.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "../provider_sqlite_fixture.hpp"
#include "fixture_value.hpp"
#include <cmath>
#include <iostream>
using namespace teaql;
static Json describe(Memory r,const Value& v) {
    auto out=detail::model_json(r);
    constexpr std::string_view kinds[]={"null","bool","i64","u64","f64","text","typed_null","list","object","decimal","json","date","timestamp"};
    detail::model_put(out,"kind",kinds[v.data.index()]);
    std::visit([&](const auto& item){
        using T=std::decay_t<decltype(item)>;
        if constexpr(std::is_same_v<T,std::int64_t> || std::is_same_v<T,Timestamp>) {
            char buffer[32];std::int64_t n;
            if constexpr(std::is_same_v<T,Timestamp>)n=item.as_millis();else n=item;
            auto end=std::to_chars(buffer,buffer+sizeof(buffer),n).ptr;detail::model_put(out,"value",std::string_view(buffer,end));
        }else if constexpr(std::is_same_v<T,Decimal> || std::is_same_v<T,Date>)detail::model_put(out,"value",item.to_string(r));
        else if constexpr(std::is_same_v<T,double>){if(std::isinf(item))detail::model_put(out,"value",item<0?"-inf":"inf");else detail::model_put(out,"value",item);}
        else if constexpr(std::is_same_v<T,Json> || std::is_same_v<T,String> || std::is_same_v<T,bool>)detail::model_put(out,"value",item);
    },v.data);return out;
}
static Json evaluate(UserContext& user,const Json& input) {
    auto r=user.memory_resource();sqlite_fixture::Connection connection;
    String create("CREATE TABLE data(v ",r);
    if(auto* declaration=optional(input,"decl"))if(std::holds_alternative<String>(declaration->data)){create+='"';for(char c:text(*declaration)){create+=c;if(c=='"')create+='"';}create+='"';}
    create+="); INSERT INTO data VALUES(";create+=text(field(input,"sql_value"));create+=");";connection.execute(create.c_str());
    String sql(optional(input,"query")?text(field(input,"query")):std::string_view("SELECT v FROM data"),r);
    sqlite_fixture::Statement statement(connection,sql.c_str());
    auto& layout=user.make<detail::SqliteRowLayout>(*statement.handle);
    Json::Array rows(r);
    while(sqlite3_step(statement.handle)==SQLITE_ROW){
        auto decoded=detail::decode_sqlite_row(r,*statement.handle,layout);
        if(!decoded){auto out=detail::model_json(r);detail::model_put(out,"error",decoded.error().to_string(r));detail::model_put(out,"kind",decoded.error().kind()==MutationExecutorError::Kind::Bind?"Bind":decoded.error().kind()==MutationExecutorError::Kind::UnsupportedColumnType?"UnsupportedColumnType":"Sqlite");return out;}
        Json::Array keys(r),values(r);for(const auto& key:decoded.value().keys())keys.emplace_back(key);for(const auto& v:decoded.value().values())values.emplace_back(describe(r,v));
        auto row=detail::model_json(r);detail::model_put(row,"keys",keys);detail::model_put(row,"values",values);rows.emplace_back(std::move(row));
    }
    auto out=detail::model_json(r);detail::model_put(out,"rows",rows);return out;
}
int main(){GlobalContext global;std::string line;while(std::getline(std::cin,line)){auto user=global.make_user_context();auto r=user->memory_resource();auto input=Json::parse(r,line);if(!input)throw std::runtime_error("invalid fixture JSON");std::cout<<evaluate(*user,*input).dump(r)<<'\n';}}
