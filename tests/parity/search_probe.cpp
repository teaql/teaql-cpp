#include <teaql/core/dynamic_search.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
using namespace teaql::dynamic_search;
const Json& get(const Json& value,std::string_view key){return std::get<Json::Object>(value.data).find(key)->second;}
std::string_view text(const Json& value){return std::get<String>(value.data);}
Json evaluate(Memory r,const Json& input){
    SearchModels models(r);
    for(const auto& [name,model]:std::get<Json::Object>(get(input,"models").data)){
        auto [entry,inserted]=models.try_emplace(name);(void)inserted;
        for(const auto& [field,kind]:std::get<Json::Object>(get(model,"fields").data))entry->second.fields.emplace(field,text(kind));
        for(const auto& [relation,target]:std::get<Json::Object>(get(model,"relations").data))entry->second.relations.emplace(relation,text(target));
    }
    Json emitted(r);emitted.data=Json::Array(r);
    auto capture=[&](const DynamicSearchWarning& warning){std::get<Json::Array>(emitted.data).emplace_back(warning.to_json(r));};WarningSink sink(capture);
    auto result=normalize_dynamic_search(r,text(get(input,"source")),text(get(input,"entity")),models,std::get<std::uint64_t>(get(input,"maxClauses").data),&sink);
    Json output(r);Json::Object object(r);object.emplace("emitted",std::move(emitted));
    if(!result)object.emplace("error",Json(r,result.error().message));
    else {
        Json body(r);Json::Object fields(r);Json filters(r),orders(r),warnings(r);filters.data=Json::Array(r);orders.data=Json::Array(r);warnings.data=Json::Array(r);
        for(const auto& filter:result.value().filters){Json j(r);Json::Object o(r);o.emplace("fieldPath",Json(r,filter.field_path));o.emplace("operator",Json(r,filter.operator_name));o.emplace("value",Json(r,filter.value));j.data=std::move(o);std::get<Json::Array>(filters.data).emplace_back(std::move(j));}
        for(const auto& order:result.value().orders){Json j(r);Json::Object o(r);o.emplace("fieldPath",Json(r,order.field_path));o.emplace("descending",Json(r,order.descending));j.data=std::move(o);std::get<Json::Array>(orders.data).emplace_back(std::move(j));}
        for(const auto& warning:result.value().warnings)std::get<Json::Array>(warnings.data).emplace_back(warning.to_json(r));
        fields.emplace("filters",std::move(filters));fields.emplace("orders",std::move(orders));fields.emplace("warnings",std::move(warnings));body.data=std::move(fields);object.emplace("result",std::move(body));
    }
    output.data=std::move(object);return output;
}
int main(){GlobalContext global;std::string line;while(std::getline(std::cin,line)){auto user=global.make_user_context();auto r=user->memory_resource();auto input=Json::parse(r,line);if(!input)throw std::runtime_error("Invalid search fixture");std::cout<<evaluate(r,*input).dump(r)<<'\n';}}
