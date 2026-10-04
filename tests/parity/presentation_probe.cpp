#include <teaql/core/web.hpp>
#include <teaql/core/xls.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
template<class T> Json decode(Memory r,const Json& input) {
    auto result=T::from_json_value(r,input);auto output=detail::model_json(r);
    if(result) detail::model_put(output,"ok",result.value().to_json_value(r));
    else detail::model_put(output,"error",true);
    return output;
}
Json evaluate(Memory r,const Json& input) {
    const auto& object=std::get<Json::Object>(input.data);
    const auto& kind=std::get<String>(object.find("kind")->second.data);const auto& value=object.find("value")->second;
    if(kind=="action_factory") {
        const auto& name=std::get<String>(value.data);WebAction action(std::allocator_arg,Allocator(r));
        if(name=="view") action=WebAction::view_web_action(r);
        else if(name=="sub_list") action=WebAction::view_sub_list_action(r,"名字","list","children");
        else if(name=="component") action=WebAction::simple_component_action(r,"名字","table");
        else if(name=="modify") action=WebAction::modify_web_action(r,"名字","/url");
        else if(name=="modify_warning") action=WebAction::modify_web_action_with_warning(r,"名字","/url"," warning ");
        else if(name=="modify_empty_warning") action=WebAction::modify_web_action_with_warning(r,"名字","/url","");
        else if(name=="default_modify") action=WebAction::default_modify_web_action(r);
        else if(name=="delete") action=WebAction::delete_web_action(r);
        else if(name=="delete_warning") action=WebAction::delete_web_action_with_warning(r,"/url"," warning ");
        else if(name=="audit") action=WebAction::audit_web_action(r,"/url"," warning ");
        else if(name=="discard") action=WebAction::discard_web_action(r,"/url"," warning ");
        else if(name=="goto") action=WebAction::goto_action(r,"名字","target","/url");
        else if(name=="switch") action=WebAction::switch_view_action(r,"名字","target");
        else if(name=="add_new") action=WebAction::add_new_web_action(r,"名字");
        else if(name=="upload") action=WebAction::batch_upload_web_action(r);
        auto output=detail::model_json(r);
        if(name=="common") {
            Json::Array items(r);for(const auto& a:WebAction::common_web_actions(r)) items.emplace_back(a.to_json_value(r));detail::model_put(output,"ok",items);
        } else detail::model_put(output,"ok",action.to_json_value(r));
        return output;
    }
    if(kind=="response_factory") {
        const auto& name=std::get<String>(value.data);auto response=WebResponse::success(r);
        if(name=="fail") response=WebResponse::fail(r," failed ");
        else if(name=="empty") response=WebResponse::empty_list(r," none ");
        else if(name=="push") response.with_record_count(99).push_json(*Json::parse(r,R"({"data":[1,2]})"));
        auto output=detail::model_json(r);detail::model_put(output,"ok",response.to_json_value(r));return output;
    }
    if(kind=="navigation") {
        const auto& v=std::get<Json::Object>(value.data);
        auto integer=[&](std::string_view key) { return detail::model_integer<std::int32_t>(v.find(key)->second,key); };
        auto x=integer("x"),y=integer("y");XlsBlockBuildContext context(std::allocator_arg,Allocator(r),"page",x,y);
        auto block=context.to_block(*Json::parse(r,R"({"value":"data"})"));block.span(integer("width"),integer("height"));
        auto result=detail::model_json(r),output=detail::model_json(r);
        detail::model_put(result,"context",context.to_json_value(r));detail::model_put(result,"next",context.next().to_json_value(r));
        detail::model_put(result,"newLine",context.new_line().to_json_value(r));detail::model_put(result,"nextLine",context.next_line().to_json_value(r));
        detail::model_put(result,"block",block.to_json_value(r));detail::model_put(result,"width",block.width());detail::model_put(result,"height",block.height());
        detail::model_put(result,"contains",block.contains(x,y));detail::model_put(output,"ok",result);return output;
    }
    if(kind=="style") return decode<WebStyle>(r,value);
    if(kind=="action") return decode<WebAction>(r,value);
    if(kind=="response") return decode<WebResponse>(r,value);
    if(kind=="block") return decode<XlsBlock>(r,value);
    if(kind=="context") return decode<XlsBlockBuildContext>(r,value);
    if(kind=="page") return decode<XlsPage>(r,value);
    if(kind=="workbook") return decode<XlsWorkbook>(r,value);
    throw std::runtime_error("Unknown presentation fixture");
}
int main() {
    GlobalContext global;std::string line;
    while(std::getline(std::cin,line)) {
        auto user=global.make_user_context();auto r=user->memory_resource();auto input=Json::parse(r,line);
        if(!input) throw std::runtime_error("Invalid fixture JSON");
        std::cout<<evaluate(r,*input).dump(r)<<'\n';
    }
}
