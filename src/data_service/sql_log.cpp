#include <teaql/data_service/sql_log.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <algorithm>
#include <charconv>

namespace teaql {
std::string_view as_str(SqlExecutionOutcome outcome) {
    switch(outcome) {
        case SqlExecutionOutcome::Success:return "success";
        case SqlExecutionOutcome::Failure:return "failure";
        case SqlExecutionOutcome::Cancelled:return "cancelled";
    }
    throw std::invalid_argument("Invalid SQL execution outcome");
}
bool is_credential_log_name(std::string_view name) {
    // Streaming normalization needs no allocation and ignores all non-ASCII
    // bytes, just as chars().filter(is_ascii_alphanumeric) does in Rust.
    constexpr std::string_view words[]={"password","passwd","passphrase","privatekey","secret","accesstoken","refreshtoken","idtoken","apikey","authorization","credential","sessiontoken","magiclinktoken"};
    char tail[16]{};std::size_t size=0;
    for(unsigned char c:name) {
        if(c>='A' && c<='Z') c+=32;
        if(!((c>='a' && c<='z') || (c>='0' && c<='9'))) continue;
        if(size==sizeof(tail)) {std::move(tail+1,tail+size,tail);--size;}
        tail[size++]=char(c);
        for(auto word:words) if(size>=word.size() && std::string_view(tail+size-word.size(),word.size())==word) return true;
    }
    return false;
}
SqlLogContext::SqlLogContext(std::allocator_arg_t,Allocator a,const SqlLogContext& other)
    : projection_state(std::allocator_arg,a,other.projection_state),intent_redactions(std::allocator_arg,a,other.intent_redactions),
      execution_outcome(other.execution_outcome),database_kind(detail::clone_in(a.resource(),other.database_kind)),
      parameter_policies(other.parameter_policies,a.resource()),generated_sql(other.generated_sql),masked_parameters(other.masked_parameters,a.resource()),
      mode(detail::clone_in(a.resource(),other.mode)),omission_reason(detail::clone_in(a.resource(),other.omission_reason)) {}
namespace {
template<class T> String integer_text(Memory r,T value) {
    String result(r);char buffer[32];auto [end,error]=std::to_chars(buffer,buffer+sizeof(buffer),value);(void)error;result.append(buffer,end);return result;
}
String scalar_text(Memory r,const Value& value) {
    return std::visit([&](const auto& item)->String {
        using T=std::decay_t<decltype(item)>;
        if constexpr(std::same_as<T,String>) return String(item,r);
        else if constexpr(std::same_as<T,bool>) return String(item ? "true":"false",r);
        else if constexpr(std::same_as<T,std::int64_t> || std::same_as<T,std::uint64_t>) return integer_text(r,item);
        else if constexpr(std::same_as<T,double>) {String out(r);detail::append_float(out,item,false);return out;}
        else if constexpr(std::same_as<T,Decimal> || std::same_as<T,Date>) return item.to_string(r);
        else if constexpr(std::same_as<T,Timestamp>) return integer_text(r,item.as_millis());
        else return String(r);
    },value.data);
}
}
SqlIntentRedactions SqlIntentRedactions::from_bindings(Memory r,const SqlLogContext& source,std::span<const Value> params,std::string_view sql) {
    const bool invalid=source.parameter_policies.size()!=params.size();
    const bool statement_credential=is_credential_log_name(sql) && (!source.generated_sql || invalid);
    SqlIntentRedactions output(std::allocator_arg,Allocator(r),source.intent_redactions);
    for(std::size_t index=0;index<params.size();++index) {
        auto policy=invalid ? SqlParameterLogPolicy::Unknown:source.parameter_policies[index];const auto& value=params[index];
        bool container=value.as_object() || value.as_list() || std::holds_alternative<Json>(value.data);
        if(!statement_credential && policy==SqlParameterLogPolicy::Plain && !container) continue;
        bool forced=statement_credential || policy==SqlParameterLogPolicy::Unknown || policy==SqlParameterLogPolicy::Credential;
        if(auto text=value.try_text()) {if(!text->empty()) output.values_.emplace_back(*text,forced);continue;}
        Vector<const Value*> pending(r);pending.push_back(&value);Vector<String> strings(r);
        bool credential=statement_credential || policy==SqlParameterLogPolicy::Credential;
        while(!pending.empty()) {
            auto* item=pending.back();pending.pop_back();
            if(item->is_null()) continue;
            if(auto* object=item->as_object()) {
                for(const auto& [key,child]:*object) {credential|=is_credential_log_name(key);pending.push_back(&child);}continue;
            }
            if(auto* list=item->as_list()) {for(const auto& child:*list) pending.push_back(&child);continue;}
            if(auto* json=std::get_if<Json>(&item->data)) {
                Vector<const Json*> json_pending(r);json_pending.push_back(json);
                while(!json_pending.empty()) {
                    auto* child=json_pending.back();json_pending.pop_back();
                    if(auto* object=std::get_if<Json::Object>(&child->data)) {
                        for(const auto& [key,nested]:*object) {credential|=is_credential_log_name(key);json_pending.push_back(&nested);}
                    } else if(auto* array=std::get_if<Json::Array>(&child->data)) {
                        for(const auto& nested:*array) json_pending.push_back(&nested);
                    } else if(!std::holds_alternative<std::monostate>(child->data)) {
                        auto* text=std::get_if<String>(&child->data);auto rendered=text ? String(*text,r):child->dump(r);
                        if(!rendered.empty()) strings.emplace_back(std::move(rendered));
                    }
                }
                continue;
            }
            auto text=scalar_text(r,*item);if(!text.empty()) strings.emplace_back(std::move(text));
        }
        forced=credential || policy==SqlParameterLogPolicy::Unknown;
        if(forced || policy==SqlParameterLogPolicy::Masked) for(const auto& text:strings) output.values_.emplace_back(text,forced);
    }
    return output;
}
SqlIntentRedactions SqlIntentRedactions::from_unclassified_query(Memory r,const SelectQuery& query) {
    using Node=std::variant<const SelectQuery*,const Expr*>;
    Vector<Node> pending(r);pending.emplace_back(&query);Vector<Value> values(r);
    while(!pending.empty()) {
        auto node=pending.back();pending.pop_back();
        if(auto p=std::get_if<const SelectQuery*>(&node)) {
            const auto& q=**p;
            if(q.predicate) pending.emplace_back(&*q.predicate);
            if(q.having_predicate) pending.emplace_back(&*q.having_predicate);
            for(const auto& field:q.expr_projection) pending.emplace_back(&field.expr);
            for(const auto& order:q.order_by) if(order.expr) pending.emplace_back(&*order.expr);
            if(q.search_text) values.emplace_back(*q.search_text);
        } else {
            const auto& expr=**std::get_if<const Expr*>(&node);
            if(expr.kind==Expr::Kind::Value) values.emplace_back(expr.literal);
            else if(expr.kind!=Expr::Kind::Column) {
                for(const auto& arg:expr.args) pending.emplace_back(&arg);
                if(expr.kind==Expr::Kind::SubQuery && expr.subquery_data) pending.emplace_back(&expr.subquery_data->query);
            }
        }
    }
    return from_bindings(r,SqlLogContext(r),values,{});
}
void SqlIntentRedactions::extend(const SqlIntentRedactions& other) {
    if(this==&other) return;
    for(const auto& value:other.values_) if(std::find(values_.begin(),values_.end(),value)==values_.end()) values_.emplace_back(value);
}
void SqlIntentRedactions::capture_target_id(const Value& id) {
    auto r=memory_resource();SqlLogContext source(r);source.generated_sql=true;source.parameter_policies.push_back(SqlParameterLogPolicy::Masked);
    auto captured=from_bindings(r,source,std::span(&id,1),{});
    for(const auto& value:captured.values_) {
        bool exists=std::any_of(values_.begin(),values_.end(),[&](const auto& entry){return entry.text==value.text && entry.forced;});
        if(!exists) values_.emplace_back(value.text,true);
    }
}
void SqlIntentRedactions::extend_secrets(bool allow_plaintext,Vector<String>& secrets) const {
    for(const auto& entry:values_) if(entry.forced || !allow_plaintext) secrets.emplace_back(entry.text);
}
String SqlIntentRedactions::debug(Memory r) const {
    String text("SqlIntentRedactions { count: ",r);text+=integer_text(r,values_.size());text+=" }";return text;
}
}
