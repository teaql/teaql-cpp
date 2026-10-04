#include <teaql/core/web.hpp>
#include <teaql/core/detail/model_json.hpp>

namespace teaql {
WebStyle::WebStyle(std::allocator_arg_t,Allocator a,const WebStyle& other)
    : background_color(detail::clone_in(a.resource(),other.background_color)),color(detail::clone_in(a.resource(),other.color)),
      class_names(detail::clone_in(a.resource(),other.class_names)),resource_(a.resource()) {}
WebStyle WebStyle::with_background_color(Memory r,std::string_view value) { WebStyle result(std::allocator_arg,Allocator(r));result.set_background_color(value);return result; }
WebStyle WebStyle::with_font_color(Memory r,std::string_view value) { WebStyle result(std::allocator_arg,Allocator(r));result.font_color(value);return result; }
WebStyle WebStyle::with_class_names(Memory r,std::string_view value) { WebStyle result(std::allocator_arg,Allocator(r));result.set_class_names(value);return result; }
Json WebStyle::to_json_value(Memory r) const {
    auto result=detail::model_json(r);
    detail::model_optional(result,"backgroundColor",background_color);
    detail::model_optional(result,"color",color);
    detail::model_optional(result,"classNames",class_names);
    return result;
}
ModelResult<WebStyle> WebStyle::from_json_value(Memory r,const Json& input) {
    return detail::decode_model<WebStyle>(r,[&] {
        detail::ModelReader reader(input,3);WebStyle result(std::allocator_arg,Allocator(r));
        result.background_color=detail::model_optional_string(r,reader.field("backgroundColor",0,true),"backgroundColor");
        result.color=detail::model_optional_string(r,reader.field("color",1,true),"color");
        result.class_names=detail::model_optional_string(r,reader.field("classNames",2,true),"classNames");
        return result;
    });
}
WebAction::WebAction(std::allocator_arg_t,Allocator a,const WebAction& other)
    : key(detail::clone_in(a.resource(),other.key)),
      name(detail::clone_in(a.resource(),other.name)),
      level(detail::clone_in(a.resource(),other.level)),
      execute(detail::clone_in(a.resource(),other.execute)),
      target(detail::clone_in(a.resource(),other.target)),
      component(detail::clone_in(a.resource(),other.component)),
      warning_message(detail::clone_in(a.resource(),other.warning_message)),
      role_for_list(detail::clone_in(a.resource(),other.role_for_list)),
      request_url(detail::clone_in(a.resource(),other.request_url)),resource_(a.resource()) {}
bool WebAction::operator==(const WebAction& other) const { return key==other.key && name==other.name && level==other.level && execute==other.execute && target==other.target && component==other.component && warning_message==other.warning_message && role_for_list==other.role_for_list && request_url==other.request_url; }
Json WebAction::to_json_value(Memory r) const {
    auto result=detail::model_json(r);
    detail::model_optional(result,"key",key);
    detail::model_optional(result,"name",name);
    detail::model_optional(result,"level",level);
    detail::model_optional(result,"execute",execute);
    detail::model_optional(result,"target",target);
    detail::model_optional(result,"component",component);
    detail::model_optional(result,"warningMessage",warning_message);
    detail::model_optional(result,"roleForList",role_for_list);
    detail::model_optional(result,"requestURL",request_url);
    return result;
}
ModelResult<WebAction> WebAction::from_json_value(Memory r,const Json& input) {
    return detail::decode_model<WebAction>(r,[&] {
        detail::ModelReader reader(input,9);WebAction result(std::allocator_arg,Allocator(r));
        result.key=detail::model_optional_string(r,reader.field("key",0,true),"key");
        result.name=detail::model_optional_string(r,reader.field("name",1,true),"name");
        result.level=detail::model_optional_string(r,reader.field("level",2,true),"level");
        result.execute=detail::model_optional_string(r,reader.field("execute",3,true),"execute");
        result.target=detail::model_optional_string(r,reader.field("target",4,true),"target");
        result.component=detail::model_optional_string(r,reader.field("component",5,true),"component");
        result.warning_message=detail::model_optional_string(r,reader.field("warningMessage",6,true),"warningMessage");
        result.role_for_list=detail::model_optional_string(r,reader.field("roleForList",7,true),"roleForList");
        result.request_url=detail::model_optional_string(r,reader.field("requestURL",8,true),"requestURL");
        return result;
    });
}
void WebStyle::bind_record(Record& record) const {
    auto r=record.get_allocator().resource();record.insert_or_assign(String(STYLE_KEY,r),Value(r,to_json_value(r)));
}
void WebAction::bind_record(Record& record) const {
    auto r=record.get_allocator().resource();auto action=to_json_value(r);
    auto it=record.find(ACTION_LIST_KEY);
    if(it!=record.end()) {
        if(auto* json=std::get_if<Json>(&it->second.data)) if(auto* items=std::get_if<Json::Array>(&json->data)) {
            items->emplace_back(std::move(action));return;
        }
    }
    Json replacement(r);replacement.data.emplace<Json::Array>(r);auto& items=std::get<Json::Array>(replacement.data);
    if(it!=record.end()) items.emplace_back(it->second.to_json_value(r));
    items.emplace_back(std::move(action));
    record.insert_or_assign(String(ACTION_LIST_KEY,r),Value(r,std::move(replacement)));
}
WebAction WebAction::view_web_action(Memory r) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name("VIEW DETAIL").set_level("view").set_execute("switchview").set_target("detail");return result;
}
WebAction WebAction::view_sub_list_action(Memory r,std::string_view name,std::string_view list_view_name,std::string_view role) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name(name).set_level("view").set_execute("gotoList").set_role_for_list(role).set_target(list_view_name);return result;
}
WebAction WebAction::simple_component_action(Memory r,std::string_view name,std::string_view component) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name(name).set_component(component);return result;
}
WebAction WebAction::default_modify_web_action(Memory r) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name("UPDATE").set_level("modify").set_execute("switchview").set_target("modify");return result;
}
WebAction WebAction::delete_web_action(Memory r) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name("DELETE").set_level("delete").set_execute("switchview").set_target("deleteview");return result;
}
WebAction WebAction::goto_action(Memory r,std::string_view name,std::string_view target,std::string_view url) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name(name).set_level("modify").set_execute("gotoview").set_target(target).set_request_url(url);return result;
}
WebAction WebAction::switch_view_action(Memory r,std::string_view name,std::string_view target) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name(name).set_level("modify").set_execute("switchview").set_target(target);return result;
}
WebAction WebAction::batch_upload_web_action(Memory r) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name("BATCH UPLOAD").set_level("modify").set_execute("switchview").set_target("batchupload");return result;
}
WebAction WebAction::modify_web_action(Memory r,std::string_view name,std::string_view url) { return modify_web_action_with_warning(r,name,url,{}); }
WebAction WebAction::modify_web_action_with_warning(Memory r,std::string_view name,std::string_view url,std::optional<std::string_view> warning) {
    WebAction result(std::allocator_arg,Allocator(r));
    result.set_name(name).set_key(name).set_level("modify").set_execute("switchview").set_target("modify").set_request_url(url).optional_warning_message(warning);return result;
}
WebAction WebAction::delete_web_action_with_warning(Memory r,std::string_view url,std::string_view warning) { return modify_web_action_with_warning(r,"web.action.delete",url,warning); }
WebAction WebAction::audit_web_action(Memory r,std::string_view url,std::string_view warning) { return modify_web_action_with_warning(r,"AUDIT",url,warning); }
WebAction WebAction::discard_web_action(Memory r,std::string_view url,std::string_view warning) { return modify_web_action_with_warning(r,"DISCARD",url,warning); }
WebAction WebAction::add_new_web_action(Memory r,std::string_view display_name) {
    String name("NEW ",r);name+=display_name;auto result=switch_view_action(r,name,"addnew");return result;
}
Vector<WebAction> WebAction::common_web_actions(Memory r) { Vector<WebAction> actions(r);actions.emplace_back(view_web_action(r));actions.emplace_back(default_modify_web_action(r));return actions; }
WebResponse::WebResponse(std::allocator_arg_t,Allocator a)
    : data(a.resource()),status(String("YES",a.resource())),version(WEB_RESPONSE_VERSION,a.resource()),facets(a.resource()) {}
WebResponse::WebResponse(std::allocator_arg_t,Allocator a,const WebResponse& other)
    : data(a.resource()),result_code(other.result_code),status(detail::clone_in(a.resource(),other.status)),
      message(detail::clone_in(a.resource(),other.message)),record_count(other.record_count),version(other.version,a.resource()),facets(a.resource()) {
    for(const auto& item:other.data) data.emplace_back(item);
    for(const auto& [key,value]:other.facets) facets.emplace(String(key,a.resource()),Json(a.resource(),value));
}
WebResponse WebResponse::fail(Memory r,std::string_view message) { auto result=success(r);result.result_code=1;result.status=String("NO",r);result.message=String(message,r);return result; }
WebResponse WebResponse::empty_list(Memory r,std::string_view message) { auto result=success(r);result.status.reset();result.message=String(message,r);return result; }
WebResponse WebResponse::from_records(Memory r,std::span<const Record> records) {
    auto result=success(r);for(const auto& record:records) result.push_json(record_to_json_value(r,record));return result;
}
WebResponse& WebResponse::with_data(const Json::Array& items) {
    Json::Array copy(memory_resource());for(const auto& item:items) copy.emplace_back(item);
    data=std::move(copy);record_count=data.size();return *this;
}
WebResponse& WebResponse::with_facets(const Json::Object& items) {
    auto r=memory_resource();Json::Object copy(r);for(const auto& [key,value]:items) copy.emplace(String(key,r),Json(r,value));
    facets=std::move(copy);return *this;
}
WebResponse& WebResponse::with_facets_option(const std::optional<Json::Object>& items) { if(items) return with_facets(*items);facets.clear();return *this; }
WebResponse& WebResponse::push_json(const Json& item) { data.emplace_back(item);record_count=data.size();return *this; }
bool WebResponse::operator==(const WebResponse& other) const {
    return data==other.data && result_code==other.result_code && status==other.status && message==other.message &&
        record_count==other.record_count && version==other.version && facets==other.facets;
}
Json WebResponse::to_json_value(Memory r) const {
    auto result=detail::model_json(r);detail::model_put(result,"data",data);detail::model_put(result,"resultCode",result_code);
    detail::model_optional(result,"status",status);detail::model_optional(result,"message",message);
    detail::model_put(result,"recordCount",record_count);detail::model_put(result,"version",version);detail::model_put(result,"facets",facets);return result;
}
ModelResult<WebResponse> WebResponse::from_json_value(Memory r,const Json& input) {
    return detail::decode_model<WebResponse>(r,[&] {
        detail::ModelReader reader(input,7);auto result=success(r);
        result.with_data(detail::model_array(*reader.field("data",0),"data"));
        result.result_code=detail::model_integer<std::int32_t>(*reader.field("resultCode",1),"resultCode");
        result.status=detail::model_optional_string(r,reader.field("status",2,true),"status");
        result.message=detail::model_optional_string(r,reader.field("message",3,true),"message");
        result.record_count=detail::model_integer<std::uint64_t>(*reader.field("recordCount",4),"recordCount");
        result.version=detail::model_string(*reader.field("version",5),"version");
        result.with_facets(detail::model_object(*reader.field("facets",6),"facets"));return result;
    });
}
}
