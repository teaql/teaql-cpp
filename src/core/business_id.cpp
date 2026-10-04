#include <teaql/core/business_id.hpp>
#include <array>
#include <stdexcept>

namespace teaql {
std::string_view business_id_error_code_name(BusinessIdErrorCode code) {
    switch(code) {
        case BusinessIdErrorCode::InvalidDefinition:return "InvalidDefinition";
        case BusinessIdErrorCode::InvalidFormat:return "InvalidFormat";
        case BusinessIdErrorCode::Immutable:return "Immutable";
        case BusinessIdErrorCode::Exhausted:return "Exhausted";
        case BusinessIdErrorCode::Allocation:return "Allocation";
        case BusinessIdErrorCode::KeyNotFound:return "KeyNotFound";
        case BusinessIdErrorCode::Encoding:return "Encoding";
    }
    throw std::invalid_argument("Unknown BusinessIdErrorCode");
}
BusinessIdError::BusinessIdError(std::allocator_arg_t,Allocator a,BusinessIdErrorCode c,std::string_view text)
    : code(c),message(text,a.resource()) {}
BusinessIdError::BusinessIdError(std::allocator_arg_t,Allocator a,const BusinessIdError& other)
    : BusinessIdError(std::allocator_arg,a,other.code,other.message) {}
String BusinessIdError::to_string(Memory r) const {
    String result(business_id_error_code_name(code),r);result+=": ";result+=message;return result;
}
BusinessIdDefinition::BusinessIdDefinition(std::allocator_arg_t,Allocator a,std::string_view field,std::string_view p,std::string_view ns)
    : field_name(field,a.resource()),profile(DEFAULT_BUSINESS_ID_PROFILE,a.resource()),prefix(p,a.resource()),
      namespace_name(ns,a.resource()),separator("-",a.resource()) {}
BusinessIdDefinition::BusinessIdDefinition(std::allocator_arg_t,Allocator a,const BusinessIdDefinition& other)
    : field_name(other.field_name,a.resource()),profile(other.profile,a.resource()),prefix(other.prefix,a.resource()),
      namespace_name(other.namespace_name,a.resource()),split_by_date(other.split_by_date),preserve_digits(other.preserve_digits),separator(other.separator,a.resource()) {}
BusinessIdDefinition BusinessIdDefinition::daily_sequence(Memory r,std::string_view field,std::string_view prefix,std::string_view ns) {
    return {std::allocator_arg,Allocator(r),field,prefix,ns};
}
BusinessIdDefinition BusinessIdDefinition::legacy_daily_sequence(Memory r,std::string_view field,std::string_view prefix,std::string_view ns) {
    auto result=daily_sequence(r,field,prefix,ns);result.profile=LEGACY_DAILY_SEQUENCE_PROFILE;result.preserve_digits=8;return result;
}
Result<std::uint64_t,BusinessIdError> BusinessIdDefinition::max_sequence() const {
    using Outcome=Result<std::uint64_t,BusinessIdError>;auto r=memory_resource();
    if(profile==DEFAULT_BUSINESS_ID_PROFILE) {
        if(preserve_digits!=DEFAULT_BUSINESS_ID_DIGITS)
            return Outcome::err(r,BusinessIdError(std::allocator_arg,Allocator(r),BusinessIdErrorCode::InvalidDefinition,"daily-permuted-v1 requires exactly 6 preserve_digits"));
        return Outcome::ok(r,BUSINESS_ID_V1_DOMAIN_SIZE-1);
    }
    if(preserve_digits==0 || preserve_digits>19)
        return Outcome::err(r,BusinessIdError(std::allocator_arg,Allocator(r),BusinessIdErrorCode::InvalidDefinition,"preserve_digits must be between 1 and 19"));
    std::uint64_t power=1;
    for(std::uint8_t i=0;i<preserve_digits;++i) power*=10;
    return Outcome::ok(r,power-1);
}
bool BusinessIdDefinition::operator==(const BusinessIdDefinition& other) const {
    return field_name==other.field_name && profile==other.profile && prefix==other.prefix && namespace_name==other.namespace_name &&
        split_by_date==other.split_by_date && preserve_digits==other.preserve_digits && separator==other.separator;
}
BusinessIdScope::BusinessIdScope(std::allocator_arg_t,Allocator a,std::string_view root,std::string_view aggregate,std::string_view ns,std::string_view period)
    : domain_root_key(root,a.resource()),aggregate_type(aggregate,a.resource()),namespace_name(ns,a.resource()),period_key(period,a.resource()) {}
BusinessIdScope::BusinessIdScope(std::allocator_arg_t,Allocator a,const BusinessIdScope& other)
    : BusinessIdScope(std::allocator_arg,a,other.domain_root_key,other.aggregate_type,other.namespace_name,other.period_key) {}
String BusinessIdScope::canonical_key(Memory r) const {
    String result(r);bool first=true;
    for(std::string_view field:std::array<std::string_view,4>{domain_root_key,aggregate_type,namespace_name,period_key}) {
        if(!first) result+='|';
        first=false;
        for(char c:field) {
            if(c=='%') result+="%25";
            else if(c=='|') result+="%7C";
            else result+=c;
        }
    }
    return result;
}
bool BusinessIdScope::operator==(const BusinessIdScope& other) const {
    return domain_root_key==other.domain_root_key && aggregate_type==other.aggregate_type && namespace_name==other.namespace_name && period_key==other.period_key;
}
BusinessIdGenerationRequest::BusinessIdGenerationRequest(std::allocator_arg_t,Allocator a,const BusinessIdDefinition& d,std::string_view root,std::string_view aggregate,Date date)
    : definition(std::allocator_arg,a,d),domain_root_key(root,a.resource()),aggregate_type(aggregate,a.resource()),business_date(date) {}
BusinessIdGenerationRequest::BusinessIdGenerationRequest(std::allocator_arg_t,Allocator a,const BusinessIdGenerationRequest& other)
    : BusinessIdGenerationRequest(std::allocator_arg,a,other.definition,other.domain_root_key,other.aggregate_type,other.business_date) {}
BusinessIdPlan::BusinessIdPlan(std::allocator_arg_t,Allocator a,const BusinessIdScope& s,std::string_view p,std::optional<std::string_view> date,std::uint8_t digits,std::string_view sep,std::uint64_t initial,std::uint64_t maximum)
    : scope(std::allocator_arg,a,s),prefix(p,a.resource()),preserve_digits(digits),separator(sep,a.resource()),initial_sequence(initial),max_sequence(maximum) {
    if(date) date_text.emplace(*date,a.resource());
}
BusinessIdPlan::BusinessIdPlan(std::allocator_arg_t,Allocator a,const BusinessIdPlan& other)
    : scope(std::allocator_arg,a,other.scope),prefix(other.prefix,a.resource()),date_text(detail::clone_in(a.resource(),other.date_text)),
      preserve_digits(other.preserve_digits),separator(other.separator,a.resource()),initial_sequence(other.initial_sequence),max_sequence(other.max_sequence) {}
Result<std::monostate,BusinessIdError> BusinessIdPlan::validate_allocation_range() const {
    using Outcome=Result<std::monostate,BusinessIdError>;auto r=memory_resource();
    if(initial_sequence>max_sequence)
        return Outcome::err(r,BusinessIdError(std::allocator_arg,Allocator(r),BusinessIdErrorCode::InvalidDefinition,"business ID allocation range requires initial_sequence <= max_sequence"));
    return Outcome::ok(r,{});
}
bool BusinessIdPlan::operator==(const BusinessIdPlan& other) const {
    return scope==other.scope && prefix==other.prefix && date_text==other.date_text && preserve_digits==other.preserve_digits &&
        separator==other.separator && initial_sequence==other.initial_sequence && max_sequence==other.max_sequence;
}
BusinessIdAllocation::BusinessIdAllocation(std::allocator_arg_t,Allocator a,const BusinessIdScope& s,std::uint64_t n)
    : scope(std::allocator_arg,a,s),sequence(n) {}
BusinessIdAllocation::BusinessIdAllocation(std::allocator_arg_t,Allocator a,const BusinessIdAllocation& other)
    : BusinessIdAllocation(std::allocator_arg,a,other.scope,other.sequence) {}
BusinessIdValue::BusinessIdValue(std::allocator_arg_t,Allocator a,std::string_view text) : value(text,a.resource()) {}
BusinessIdValue::BusinessIdValue(std::allocator_arg_t,Allocator a,const BusinessIdValue& other) : BusinessIdValue(std::allocator_arg,a,other.value) {}
}
