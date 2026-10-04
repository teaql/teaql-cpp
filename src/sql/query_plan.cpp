#include <teaql/sql/detail/query_plan.hpp>

namespace teaql::detail {
namespace {
bool expr_matches(const Expr& a,const Expr& b) {
    if(a.kind!=b.kind)return false;
    if(a.kind==Expr::Kind::Column)return a.name==b.name;
    if(a.kind==Expr::Kind::Value){auto* x=a.literal.as_list();auto* y=b.literal.as_list();return x&&y?x->size()==y->size():!x&&!y;}
    if(a.kind==Expr::Kind::Function && a.function_kind!=b.function_kind)return false;
    if((a.kind==Expr::Kind::Binary || a.kind==Expr::Kind::SubQuery) && a.op!=b.op)return false;
    if(a.args.size()!=b.args.size())return false;
    for(std::size_t i=0;i<a.args.size();++i)if(!expr_matches(a.args[i],b.args[i]))return false;
    return a.kind!=Expr::Kind::SubQuery || (a.subquery_data->entity==b.subquery_data->entity && select_plan_matches(a.subquery_data->query,b.subquery_data->query));
}
bool optional_matches(const std::optional<Expr>& a,const std::optional<Expr>& b){return a&&b?expr_matches(*a,*b):!a&&!b;}
void normalize(Expr& expr) {
    auto r=expr.memory_resource();
    if(expr.kind==Expr::Kind::Value){
        if(auto* values=std::get_if<Value::List>(&expr.literal.data)){for(auto& v:*values)v=Value(r);}
        else expr.literal=Value(r);
        return;
    }
    for(auto& arg:expr.args)normalize(arg);
    if(expr.kind==Expr::Kind::SubQuery)expr.subquery_data->query=select_plan_key(r,expr.subquery_data->query);
}
void collect_expr(Memory r,const Expr& expr,Vector<Value>& params,bool array) {
    using K=Expr::Kind;
    if(expr.kind==K::Column)return;
    if(expr.kind==K::Value){params.emplace_back(expr.literal);return;}
    if(expr.kind==K::SubQuery){collect_expr(r,expr.args.at(0),params,array);auto nested=collect_select_params(r,expr.subquery_data->entity,expr.subquery_data->query,array);for(auto& v:nested)params.emplace_back(std::move(v));return;}
    if(expr.kind==K::Binary){
        collect_expr(r,expr.args.at(0),params,array);const auto& right=expr.args.at(1);auto* list=right.kind==K::Value?right.literal.as_list():nullptr;
        bool in=expr.op==BinaryOp::In || expr.op==BinaryOp::NotIn || expr.op==BinaryOp::InLarge || expr.op==BinaryOp::NotInLarge;
        if(list && in){if(array && (expr.op==BinaryOp::InLarge || expr.op==BinaryOp::NotInLarge))params.emplace_back(right.literal);else for(const auto& v:*list)params.emplace_back(v);}
        else collect_expr(r,right,params,array);
        return;
    }
    for(const auto& arg:expr.args)collect_expr(r,arg,params,array);
}
const Expr* partition_predicate(const Expr& expr,std::string_view field){
    if(expr.kind==Expr::Kind::Binary && (expr.op==BinaryOp::In || expr.op==BinaryOp::InLarge) && expr.args.at(0).kind==Expr::Kind::Column && expr.args[0].name==field)return &expr;
    if(expr.kind==Expr::Kind::And)for(const auto& arg:expr.args)if(auto* found=partition_predicate(arg,field))return found;
    return nullptr;
}
}
bool select_plan_matches(const SelectQuery& a,const SelectQuery& b) {
    if(a.hard_limit_value!=b.hard_limit_value || a.entity!=b.entity || a.projection!=b.projection || a.expr_projection.size()!=b.expr_projection.size() || a.search_text.has_value()!=b.search_text.has_value() || !optional_matches(a.predicate,b.predicate) || !optional_matches(a.having_predicate,b.having_predicate) || a.order_by.size()!=b.order_by.size())return false;
    for(std::size_t i=0;i<a.expr_projection.size();++i)if(a.expr_projection[i].alias!=b.expr_projection[i].alias || !expr_matches(a.expr_projection[i].expr,b.expr_projection[i].expr))return false;
    for(std::size_t i=0;i<a.order_by.size();++i)if(a.order_by[i].field!=b.order_by[i].field || a.order_by[i].direction!=b.order_by[i].direction || !optional_matches(a.order_by[i].expr,b.order_by[i].expr))return false;
    // Deliberately omit comment/trace, probe threshold and id-set pagination,
    // exactly as the fixed source's plan matcher does.
    return a.slice==b.slice && a.partition_field==b.partition_field && a.aggregates==b.aggregates && a.group_by_fields==b.group_by_fields && a.relations==b.relations && a.aggregation_cache==b.aggregation_cache && a.raw_sql_text==b.raw_sql_text && a.raw_sql_search_criteria==b.raw_sql_search_criteria && a.dynamic_properties==b.dynamic_properties && a.raw_projections==b.raw_projections && a.object_group_bys==b.object_group_bys && a.child_enhancements==b.child_enhancements && a.stream_config==b.stream_config && a.continuous_page_fetch==b.continuous_page_fetch;
}
SelectQuery select_plan_key(Memory r,const SelectQuery& query) {
    auto key=query.clone(r);key.comment_text.reset();key.trace_chain.clear();if(key.search_text)key.search_text->clear();
    for(auto& p:key.expr_projection)normalize(p.expr);
    if(key.predicate)normalize(*key.predicate);
    if(key.having_predicate)normalize(*key.having_predicate);
    for(auto& order:key.order_by)if(order.expr)normalize(*order.expr);
    return key;
}
Vector<Value> collect_select_params(Memory r,const EntityDescriptor& entity,const SelectQuery& query,bool array) {
    Vector<Value> params(r);if(query.raw_sql_text)return params;
    for(const auto& p:query.expr_projection)collect_expr(r,p.expr,params,array);
    bool partitioned=query.partition_field && query.slice;
    if(partitioned)for(const auto& order:query.order_by)if(order.expr)collect_expr(r,*order.expr,params,array);
    if(query.predicate)collect_expr(r,*query.predicate,params,array);
    if(query.search_text){String pattern("%",r);pattern+=*query.search_text;pattern+='%';for(const auto& p:entity.properties)if(p.data_type==DataType::Text || p.data_type==DataType::LargeText)params.emplace_back(pattern);}
    if(partitioned)return params;
    if(query.having_predicate)collect_expr(r,*query.having_predicate,params,array);
    for(const auto& order:query.order_by)if(order.expr)collect_expr(r,*order.expr,params,array);
    return params;
}
const Value::List* partition_probe_values(const SelectQuery& query) {
    if(!query.partition_field || !query.predicate)return nullptr;
    auto find=[&](auto&& self,const Expr& expr)->const Value::List* {
        if(expr.kind==Expr::Kind::Binary && (expr.op==BinaryOp::In || expr.op==BinaryOp::InLarge) && expr.args.at(0).kind==Expr::Kind::Column && expr.args[0].name==*query.partition_field)
            return expr.args.at(1).kind==Expr::Kind::Value?expr.args[1].literal.as_list():nullptr;
        if(expr.kind==Expr::Kind::And)for(const auto& arg:expr.args)if(auto* values=self(self,arg))return values;
        return nullptr;
    };
    return find(find,*query.predicate);
}
std::optional<SelectQuery> scalar_partition_probe_query(Memory r,const SelectQuery& query,const Value& value) {
    if(!query.partition_field || !query.predicate)return {};
    auto scalar=query.clone(r);auto* found=const_cast<Expr*>(partition_predicate(*scalar.predicate,*scalar.partition_field));if(!found)return {};
    found->op=BinaryOp::Eq;found->args.at(1)=Expr::value(r,value);scalar.partition_field.reset();return scalar;
}
const EntityDescriptor* SqlExecutorState::entity_descriptor(const SchemaProvider& provider,std::string_view name) const {
    auto found=descriptors_.find(name);if(found!=descriptors_.end())return &found->second;
    auto* descriptor=provider.get_entity(name);if(!descriptor)return nullptr;
    return &descriptors_.emplace(String(name,memory_resource()),*descriptor).first->second;
}
CompiledQuery SqlExecutorState::compile_select_cached(Memory r,const SqlDialect& dialect,const EntityDescriptor& entity,const SelectQuery& query) const {
    for(const auto& plan:plans_)if(plan->descriptor==entity && select_plan_matches(plan->key,query)){
        CompiledQuery out(std::allocator_arg,Allocator(r));out.log_context=clone_in(r,plan->log);out.sql=plan->sql;out.params=collect_select_params(r,entity,query,dialect.large_in_uses_array_param());out.comment=clone_in(r,query.comment_text);return out;
    }
    auto key=select_plan_key(memory_resource(),query);auto result=dialect.compile_select(r,entity,query);
    if(plans_.size()>=256)plans_.erase(plans_.begin());
    bool exists=false;for(const auto& plan:plans_)if(plan->descriptor==entity && select_plan_matches(plan->key,query)){exists=true;break;}
    if(!exists)plans_.emplace_back(make_box<CachedSelectPlan>(memory_resource(),entity,std::move(key),result));
    return result;
}
}
