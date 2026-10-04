#include <teaql/runtime/check_result.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/uppercase_tables.hpp>
#include <algorithm>
#include <charconv>

namespace teaql {
namespace {
char32_t next_codepoint(std::string_view text,std::size_t& i) {
    auto byte=static_cast<unsigned char>(text[i++]);char32_t code=byte;
    if(byte<0x80)return code;
    unsigned count=byte>=0xF0 && byte<=0xF4?3:byte>=0xE0 && byte<=0xEF?2:byte>=0xC2 && byte<=0xDF?1:0;
    if(!count || i+count>text.size())throw std::invalid_argument("Invalid UTF-8 in object location");
    code&=(1u<<(6-count))-1;
    for(unsigned j=0;j<count;++j){auto next=static_cast<unsigned char>(text[i++]);if((next&0xC0)!=0x80)throw std::invalid_argument("Invalid UTF-8 in object location");code=(code<<6)|(next&63);}
    if((count==1 && code<0x80)||(count==2 && code<0x800)||(count==3 && code<0x10000)||code>0x10FFFF||(code>=0xD800 && code<=0xDFFF))throw std::invalid_argument("Invalid UTF-8 in object location");
    return code;
}
void append_codepoint(String& out,char32_t c) {
    if(c<0x80)out+=char(c);
    else if(c<0x800){out+=char(0xC0|(c>>6));out+=char(0x80|(c&63));}
    else if(c<0x10000){out+=char(0xE0|(c>>12));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
    else {out+=char(0xF0|(c>>18));out+=char(0x80|((c>>12)&63));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
}
String uppercase_initial(Memory r,std::string_view text) {
    String out(r);out.reserve(text.size());
    std::size_t i=0;
    if(i<text.size()) {
        auto code=next_codepoint(text,i);
        auto expanded=std::lower_bound(detail::uppercase_expansions.begin(),detail::uppercase_expansions.end(),code,[](const auto& e,char32_t c){return e.from<c;});
        if(expanded!=detail::uppercase_expansions.end() && expanded->from==code){for(auto c:expanded->to)if(c)append_codepoint(out,c);out+=text.substr(i);return out;}
        auto range=std::lower_bound(detail::uppercase_ranges.begin(),detail::uppercase_ranges.end(),code,[](const auto& e,char32_t c){return e.last<c;});
        if(range!=detail::uppercase_ranges.end() && code>=range->first && (code-range->first)%range->step==0)
            code=char32_t(std::int64_t(code)+range->delta);
        append_codepoint(out,code);
    }
    out+=text.substr(i);return out;
}
void append_index(String& out,std::size_t value) {
    char buffer[3*sizeof(std::size_t)];auto result=std::to_chars(std::begin(buffer),std::end(buffer),value);out.append(buffer,result.ptr);
}
String render_name(Memory r,std::string_view name,JsonFieldNamingProfile profile) {
    if(profile==JsonFieldNamingProfile::SnakeCase)return String(name,r);
    String result(r);std::size_t start=0;bool first=true;
    for(;;){auto end=name.find('_',start);auto part=name.substr(start,end==name.npos?name.size()-start:end-start);if(first)result+=part;else result+=uppercase_initial(r,part);first=false;if(end==name.npos)break;start=end+1;}
    return profile==JsonFieldNamingProfile::PascalCase?uppercase_initial(r,result):std::move(result);
}
template<class T> void nullable(Json& out,std::string_view key,const std::optional<T>& value) {
    if(value)detail::model_put(out,key,*value);else detail::model_put(out,key,Json(out.memory_resource()));
}
}
std::string_view wire_id(CheckRule rule) {
    switch(rule){case CheckRule::Required:return "required";case CheckRule::InvalidType:return "invalid_type";case CheckRule::Min:return "min";case CheckRule::Max:return "max";case CheckRule::MinStringLength:return "min_string_length";case CheckRule::MaxStringLength:return "max_string_length";case CheckRule::ContextRootMissing:return "context_root_missing";case CheckRule::ContextRootMismatch:return "context_root_mismatch";}
    throw std::logic_error("Invalid check rule");
}
std::string_view debug_name(CheckRule rule) {
    switch(rule){case CheckRule::Required:return "Required";case CheckRule::InvalidType:return "InvalidType";case CheckRule::Min:return "Min";case CheckRule::Max:return "Max";case CheckRule::MinStringLength:return "MinStringLength";case CheckRule::MaxStringLength:return "MaxStringLength";case CheckRule::ContextRootMissing:return "ContextRootMissing";case CheckRule::ContextRootMismatch:return "ContextRootMismatch";}
    throw std::logic_error("Invalid check rule");
}
Result<JsonFieldNamingProfile,String> json_field_naming_from_model_value(Memory r,std::string_view value) {
    using R=Result<JsonFieldNamingProfile,String>;
    if(value.empty() || value=="camelCase")return R::ok(r,JsonFieldNamingProfile::CamelCase);
    if(value=="snake_case")return R::ok(r,JsonFieldNamingProfile::SnakeCase);
    if(value=="PascalCase")return R::ok(r,JsonFieldNamingProfile::PascalCase);
    String message("unsupported json_field_naming: ",r);message+=value;return R::err(r,std::move(message));
}
String ObjectLocation::model_path(Memory r) const {
    String out(r);for(const auto& segment:segments_){if(auto* name=std::get_if<String>(&segment.data)){if(!out.empty())out+='.';out+=*name;}else{out+='[';append_index(out,std::get<std::size_t>(segment.data));out+=']';}}return out;
}
String ObjectLocation::to_string(Memory r) const {
    if(is_root())return String("$",r);
    String out(r);bool first=true;for(const auto& segment:segments_){if(auto* name=std::get_if<String>(&segment.data)){if(!first)out+='.';out+=*name;}else{out+='[';append_index(out,std::get<std::size_t>(segment.data));out+=']';}first=false;}return out;
}
String ObjectLocation::instance_path_with(Memory r,JsonFieldNamingProfile profile) const {
    String out(r);for(const auto& segment:segments_){out+='/';if(auto* name=std::get_if<String>(&segment.data)){for(auto c:render_name(r,*name,profile)){if(c=='~')out+="~0";else if(c=='/')out+="~1";else out+=c;}}else append_index(out,std::get<std::size_t>(segment.data));}return out;
}
CheckResult::CheckResult(std::allocator_arg_t,Allocator a,const CheckResult& other)
    :rule(other.rule),location(std::allocator_arg,a,other.location),input_value(detail::clone_in(a.resource(),other.input_value)),system_value(detail::clone_in(a.resource(),other.system_value)),message(detail::clone_in(a.resource(),other.message)),entity_type(detail::clone_in(a.resource(),other.entity_type)),source_instance_path(detail::clone_in(a.resource(),other.source_instance_path)) {}
String CheckResult::to_string(Memory r) const {
    if(message)return String(*message,r);
    auto out=location.to_string(r);out+=": ";out+=debug_name(rule);return out;
}
WireCheckResult CheckResult::to_wire(Memory r,JsonFieldNamingProfile profile) const {
    WireCheckResult out(std::allocator_arg,Allocator(r));out.rule_id=wire_id(rule);out.entity_type=detail::clone_in(r,entity_type);out.source_instance_path=detail::clone_in(r,source_instance_path);out.message=detail::clone_in(r,message);
    for(const auto& segment:location.segments()){if(auto* name=std::get_if<String>(&segment.data))out.location.emplace_back(std::string_view(*name));else out.location.emplace_back(std::get<std::size_t>(segment.data));}
    out.instance_path=location.instance_path_with(r,profile);if(input_value)out.input_value=input_value->to_json_value(r);if(system_value)out.system_value=system_value->to_json_value(r);return out;
}
WireCheckResult::WireCheckResult(std::allocator_arg_t,Allocator a,const WireCheckResult& other)
    :rule_id(other.rule_id,a.resource()),entity_type(detail::clone_in(a.resource(),other.entity_type)),location(other.location,a.resource()),instance_path(other.instance_path,a.resource()),source_instance_path(detail::clone_in(a.resource(),other.source_instance_path)),input_value(detail::clone_in(a.resource(),other.input_value)),system_value(detail::clone_in(a.resource(),other.system_value)),message(detail::clone_in(a.resource(),other.message)) {}
Json WireLocationSegment::to_json(Memory r) const {
    auto out=detail::model_json(r);
    if(auto* name=std::get_if<String>(&data)){detail::model_put(out,"kind","property");detail::model_put(out,"name",*name);}
    else{detail::model_put(out,"kind","index");detail::model_put(out,"index",std::get<std::size_t>(data));}
    return out;
}
Json WireCheckResult::to_json(Memory r) const {
    auto out=detail::model_json(r);detail::model_put(out,"ruleId",rule_id);nullable(out,"entityType",entity_type);Json::Array segments(r);
    for(const auto& segment:location)segments.emplace_back(segment.to_json(r));
    detail::model_put(out,"location",segments);detail::model_put(out,"instancePath",instance_path);nullable(out,"sourceInstancePath",source_instance_path);nullable(out,"inputValue",input_value);nullable(out,"systemValue",system_value);nullable(out,"message",message);return out;
}
}
