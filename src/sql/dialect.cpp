#include <teaql/sql/dialect.hpp>
#include <algorithm>
#include <charconv>
#include <limits>

namespace teaql {
namespace {
template<class T> void number(String& out,T n){char buf[32];auto [end,error]=std::to_chars(buf,buf+sizeof(buf),n);(void)error;out.append(buf,end);}
String join(Memory r,const Vector<String>& parts,std::string_view separator){String out(r);bool first=true;for(const auto& part:parts){if(!first)out+=separator;first=false;out+=part;}return out;}
[[noreturn]] void fail(Memory r,SqlCompileError::Code code,std::string_view name={}){throw SqlCompileError(std::allocator_arg,Allocator(r),code,name);}
void add_unique(Vector<String>& parts,String value){if(std::find(parts.begin(),parts.end(),value)==parts.end())parts.emplace_back(std::move(value));}
std::string_view type_name(DataType t){constexpr std::string_view names[]={"Bool","I64","U64","F64","Decimal","Text","LargeText","Json","Date","Timestamp"};return names[unsigned(t)];}
void validate_schema_shape(Memory r,const PropertyDescriptor& property){
    auto invalid=[&](std::string_view reason){SqlCompileError error(std::allocator_arg,Allocator(r),SqlCompileError::Code::InvalidSchemaShape,property.name);error.reason=reason;throw error;};
    if(property.data_type==DataType::Text){if(property.max_length==0)invalid("max_length must be greater than zero");}
    else if(property.max_length){String reason("max_length is only valid for Text, not ",r);reason+=type_name(property.data_type);invalid(reason);}
    if(property.data_type==DataType::Decimal){
        auto p=property.numeric_precision,s=property.numeric_scale;
        if(!p && !s)return;
        if(p==0)invalid("numeric_precision must be greater than zero");
        if(p && s){if(*s<=*p)return;String reason("numeric_scale (",r);number(reason,*s);reason+=") must not exceed numeric_precision (";number(reason,*p);reason+=')';invalid(reason);}
        invalid("numeric_precision and numeric_scale must be specified together");
    }else if(property.numeric_precision || property.numeric_scale){String reason("numeric_precision/numeric_scale are only valid for Decimal, not ",r);reason+=type_name(property.data_type);invalid(reason);}
}
}
String quote_identifier_if_needed(Memory r,std::string_view ident,char quote){
    if(!ident.empty() && ((ident.front()=='"' && ident.back()=='"')||(ident.front()=='`' && ident.back()=='`')||(ident.front()=='[' && ident.back()==']')))return String(ident,r);
    constexpr std::string_view keywords[]={"all","alter","and","as","asc","between","by","case","create","delete","desc","distinct","drop","exists","false","from","group","having","in","insert","into","is","join","like","limit","not","null","offset","on","or","order","select","set","table","true","type","union","update","values","where"};
    auto alpha=[](unsigned char c){return (c>='A' && c<='Z')||(c>='a' && c<='z');};
    bool needs=ident.empty() || !(ident.front()=='_' || alpha(static_cast<unsigned char>(ident.front())));String lower(r);
    for(unsigned char c:ident){needs|=!(alpha(c)||(c>='0' && c<='9')||c=='_');lower+=char(c>='A' && c<='Z' ? c+32:c);}
    needs|=std::binary_search(std::begin(keywords),std::end(keywords),std::string_view(lower));
    if(!needs)return String(ident,r);
    String out(r);out+=quote;for(char c:ident){out+=c;if(c==quote)out+=quote;}out+=quote;return out;
}
String bounded_sql_identifier(Memory r,std::string_view full,std::size_t max){
    if(full.size()<=max)return String(full,r);
    if(max<17)throw std::invalid_argument("SQL identifier byte limit is too small");
    std::uint64_t hash=0xcbf29ce484222325ULL;for(unsigned char c:full)hash=(hash^c)*0x100000001b3ULL;
    auto end=max-17;while(end && (static_cast<unsigned char>(full[end])&0xC0)==0x80)--end;
    String out(full.substr(0,end),r);out+='_';constexpr char hex[]="0123456789ABCDEF";for(int shift=60;shift>=0;shift-=4)out+=hex[(hash>>shift)&15];return out;
}
bool storage_length_covers(std::uint32_t expected,std::optional<std::uint32_t> actual){return !actual || *actual>=expected;}
bool storage_numeric_covers(std::uint32_t ep,std::uint32_t es,std::optional<std::uint32_t> ap,std::optional<std::uint32_t> as){if(!ap && !as)return true;if(!ap || !as)return false;return *as>=es && (*ap>*as ? *ap-*as:0)>=(ep>es ? ep-es:0);}
String SqlDialect::schema_type_sql(Memory r,DataType type,const PropertyDescriptor& property) const{
    switch(type){
        case DataType::Bool:return String("BOOLEAN",r);
        case DataType::I64:case DataType::U64:return String("INTEGER",r);
        case DataType::F64:return String("REAL",r);
        case DataType::Decimal:{String out("NUMERIC",r);if(property.numeric_precision && property.numeric_scale){out+='(';number(out,*property.numeric_precision);out+=',';number(out,*property.numeric_scale);out+=')';}return out;}
        case DataType::Text:{String out("VARCHAR(",r);number(out,property.max_length.value_or(255));out+=')';return out;}
        default:return String("TEXT",r);
    }
}
String SqlDialect::column_definition_sql(Memory r,const PropertyDescriptor& property) const{
    validate_schema_shape(r,property);auto out=quote_ident(r,property.column_name);out+=' ';out+=schema_type_sql(r,property.data_type,property);
    if(property.is_id)out+=" PRIMARY KEY";
    if(property.is_id || !property.nullable)out+=" NOT NULL";
    return out;
}
String SqlDialect::compile_create_table(Memory r,const EntityDescriptor& entity) const{
    Vector<String> columns(r);for(const auto& property:entity.properties)columns.emplace_back(column_definition_sql(r,property));
    String out("CREATE TABLE IF NOT EXISTS ",r);out+=quote_ident(r,entity.table_name);out+=" (";out+=join(r,columns,", ");out+=')';return out;
}
std::string_view SqlDialect::fallback_default_value_sql(DataType type) const{
    switch(type){case DataType::Bool:return "FALSE";case DataType::I64:case DataType::U64:case DataType::F64:case DataType::Decimal:return "0";case DataType::Text:case DataType::LargeText:return "''";case DataType::Json:return "'{}'";case DataType::Date:return "'1970-01-01'";case DataType::Timestamp:return "'1970-01-01 00:00:00Z'";}throw std::invalid_argument("Invalid data type");
}
String SqlDialect::compile_add_column(Memory r,const EntityDescriptor& entity,const PropertyDescriptor& property) const{
    auto def=column_definition_sql(r,property);if(!property.nullable && !property.is_id){def+=" DEFAULT ";def+=fallback_default_value_sql(property.data_type);}
    String out("ALTER TABLE ",r);out+=quote_ident(r,entity.table_name);out+=" ADD COLUMN ";out+=def;return out;
}
CompiledQuery SqlDialect::compile_select(Memory r,const EntityDescriptor& entity,const SelectQuery& query) const{
    SqlBindings bindings(r);CompiledQuery out(std::allocator_arg,Allocator(r));out.sql=compile_select_sql(r,entity,query,bindings);out.log_context=bindings.log_context(r,kind());out.params=std::move(bindings).into_values();out.comment=detail::clone_in(r,query.comment_text);return out;
}
String SqlDialect::compile_select_sql(Memory r,const EntityDescriptor& entity,const SelectQuery& query,SqlBindings& params) const{
    if(query.raw_sql_text || !query.raw_sql_search_criteria.empty() || !query.raw_projections.empty() || !query.dynamic_properties.empty())params.mark_untrusted_sql();
    if(query.raw_sql_text)return String(*query.raw_sql_text,r);
    auto projection=compile_projection(r,entity,query,params);bool partitioned=query.partition_field.has_value() && query.slice.has_value();
    if(partitioned){auto column=column_sql(r,entity,*query.partition_field);String order(r);if(!query.order_by.empty()){Vector<String> parts(r);for(const auto& item:query.order_by)parts.emplace_back(order_by_sql(r,entity,item,params));order=" ORDER BY ";order+=join(r,parts,", ");}projection+=", ROW_NUMBER() OVER (PARTITION BY ";projection+=column;projection+=order;projection+=") AS ";projection+=quote_ident(r,PARTITION_RANK_PROPERTY);}
    String sql("SELECT ",r);sql+=projection;sql+=" FROM ";sql+=quote_ident(r,entity.table_name);
    Vector<String> where(r);if(query.predicate)where.emplace_back(compile_expr(r,entity,*query.predicate,params));
    if(query.search_text){Vector<String> parts(r);String pattern("%",r);pattern+=*query.search_text;pattern+='%';for(const auto& property:entity.properties)if(property.data_type==DataType::Text || property.data_type==DataType::LargeText){params.push_field(entity,property.name,Value(r,pattern));auto part=quote_ident(r,property.column_name);part+=" LIKE ";part+=placeholder(r,params.size());parts.emplace_back(std::move(part));}if(!parts.empty()){String part("(",r);part+=join(r,parts," OR ");part+=')';where.emplace_back(std::move(part));}}
    for(const auto& raw:query.raw_sql_search_criteria)where.emplace_back(raw);
    if(!where.empty()){sql+=" WHERE ";sql+=join(r,where," AND ");}
    if(partitioned){auto rank=quote_ident(r,PARTITION_RANK_PROPERTY);String out("SELECT * FROM (",r);out+=sql;out+=") AS ";out+=quote_ident(r,"__teaql_partitioned");out+=" WHERE ";out+=rank;out+=" > ";number(out,query.slice->offset);if(query.slice->limit){out+=" AND ";out+=rank;out+=" <= ";auto offset=query.slice->offset,limit=*query.slice->limit;number(out,UINT64_MAX-offset<limit ? UINT64_MAX:offset+limit);}out+=" ORDER BY ";out+=rank;return out;}
    if(!query.group_by_fields.empty()){Vector<String> parts(r);for(const auto& field:query.group_by_fields)parts.emplace_back(column_sql(r,entity,field));sql+=" GROUP BY ";sql+=join(r,parts,", ");}
    if(query.having_predicate){auto having=compile_expr(r,entity,*query.having_predicate,params);sql+=" HAVING ";sql+=having;}
    if(!query.order_by.empty()){Vector<String> parts(r);for(const auto& order:query.order_by)parts.emplace_back(order_by_sql(r,entity,order,params));sql+=" ORDER BY ";sql+=join(r,parts,", ");}
    if(query.slice){if(query.slice->limit){sql+=" LIMIT ";number(sql,*query.slice->limit);}if(query.slice->offset){sql+=" OFFSET ";number(sql,query.slice->offset);}}
    return sql;
}
String SqlDialect::column_sql(Memory r,const EntityDescriptor& entity,std::string_view field) const{auto* p=entity.property_by_name(field);if(!p)fail(r,SqlCompileError::Code::UnknownField,field);return quote_ident(r,p->column_name);}
String SqlDialect::column_with_alias(Memory r,const PropertyDescriptor& property) const{auto out=quote_ident(r,property.column_name);if(property.column_name!=property.name){out+=" AS ";out+=quote_ident(r,property.name);}return out;}
String SqlDialect::resolve_order_field(Memory r,const EntityDescriptor& entity,const OrderBy& order,SqlBindings& params) const{return order.expr ? compile_expr(r,entity,*order.expr,params):column_sql(r,entity,order.field);}
String SqlDialect::order_by_sql(Memory r,const EntityDescriptor& entity,const OrderBy& order,SqlBindings& params) const{auto out=resolve_order_field(r,entity,order,params);out+=order.direction==SortDirection::Asc ? " ASC":" DESC";return out;}
String SqlDialect::select_projection(Memory r,const EntityDescriptor& entity,const SelectQuery& query,SqlBindings& params) const{
    Vector<String> parts(r);
    if(query.projection.empty() && query.expr_projection.empty() && query.raw_projections.empty() && query.dynamic_properties.empty()){for(const auto& p:entity.properties)parts.emplace_back(column_with_alias(r,p));return join(r,parts,", ");}
    for(std::size_t i=0;i<query.projection.size();++i){const auto& field=query.projection[i];if(std::find(query.projection.begin(),query.projection.begin()+i,field)!=query.projection.begin()+i)continue;auto* p=entity.property_by_name(field);if(!p)fail(r,SqlCompileError::Code::UnknownField,field);parts.emplace_back(column_with_alias(r,*p));}
    for(const auto& p:query.expr_projection){auto part=compile_expr(r,entity,p.expr,params);part+=" AS ";part+=quote_ident(r,p.alias);parts.emplace_back(std::move(part));}
    for(const auto* projections:{&query.raw_projections,&query.dynamic_properties})for(const auto& p:*projections){String part(p.raw_sql_segment,r);part+=" AS ";part+=quote_ident(r,p.property_name);parts.emplace_back(std::move(part));}
    return join(r,parts,", ");
}
String SqlDialect::aggregate_projection(Memory r,const EntityDescriptor& entity,const SelectQuery& query,SqlBindings& params) const{
    Vector<String> parts(r);for(const auto& field:query.group_by_fields)add_unique(parts,column_sql(r,entity,field));
    for(const auto& p:query.expr_projection){auto part=compile_expr(r,entity,p.expr,params);part+=" AS ";part+=quote_ident(r,p.alias);add_unique(parts,std::move(part));}
    for(const auto* projections:{&query.raw_projections,&query.dynamic_properties})for(const auto& p:*projections){String part(p.raw_sql_segment,r);part+=" AS ";part+=quote_ident(r,p.property_name);add_unique(parts,std::move(part));}
    for(const auto& aggregate:query.aggregates){auto field=resolve_aggregate_field(r,entity,aggregate);auto part=aggregate_call_sql(r,aggregate.function,field);part+=" AS ";part+=quote_ident(r,aggregate.alias);parts.emplace_back(std::move(part));}
    return join(r,parts,", ");
}
String SqlDialect::compile_projection(Memory r,const EntityDescriptor& entity,const SelectQuery& query,SqlBindings& params) const{return query.aggregates.empty()?select_projection(r,entity,query,params):aggregate_projection(r,entity,query,params);}
std::string_view SqlDialect::aggregate_function_sql(AggregateFunction function) const{constexpr std::string_view names[]={"COUNT","SUM","AVG","MIN","MAX","STDDEV","STDDEV_POP","VAR_SAMP","VAR_POP","BIT_AND","BIT_OR","BIT_XOR"};return names[unsigned(function)];}
String SqlDialect::aggregate_call_sql(Memory r,AggregateFunction function,std::string_view field) const{String out(aggregate_function_sql(function),r);out+='(';out+=field;out+=')';return out;}
String SqlDialect::resolve_aggregate_field(Memory r,const EntityDescriptor& entity,const Aggregate& aggregate) const{return aggregate.function==AggregateFunction::Count && aggregate.field=="*" ? String("*",r):column_sql(r,entity,aggregate.field);}
String SqlDialect::compile_expr(Memory r,const EntityDescriptor& entity,const Expr& expr,SqlBindings& params) const{
    detail::SqlBindingScope scope(params,detail::sql_expression_policy(entity,expr));using K=Expr::Kind;
    auto arg=[&](std::size_t i){return compile_expr(r,entity,expr.args.at(i),params);};
    switch(expr.kind){
        case K::Column:return column_sql(r,entity,expr.name);
        case K::Value:params.push(expr.literal);return placeholder(r,params.size());
        case K::Function:return compile_function(r,entity,expr.function_kind,expr.args,params);
        case K::SubQuery:return compile_subquery(r,entity,expr.args.at(0),expr.op,expr.subquery_data->entity,expr.subquery_data->query,params);
        case K::Binary:{if(expr.op==BinaryOp::In || expr.op==BinaryOp::NotIn || expr.op==BinaryOp::InLarge || expr.op==BinaryOp::NotInLarge)return compile_in(r,entity,expr.args.at(0),expr.op,expr.args.at(1),params);auto left=arg(0),right=arg(1);constexpr std::string_view ops[]={"=","!=",">",">=","<","<=","LIKE","NOT LIKE"};String out("(",r);out+=left;out+=' ';out+=ops[unsigned(expr.op)];out+=' ';out+=right;out+=')';return out;}
        case K::Between:{auto value=arg(0),lower=arg(1),upper=arg(2);String out("(",r);out+=value;out+=" BETWEEN ";out+=lower;out+=" AND ";out+=upper;out+=')';return out;}
        case K::IsNull:case K::IsNotNull:{auto value=arg(0);String out("(",r);out+=value;out+=expr.kind==K::IsNull ? " IS NULL)":" IS NOT NULL)";return out;}
        case K::Not:{auto value=arg(0);String out("(NOT ",r);out+=value;out+=')';return out;}
        case K::And:return compile_joined(r,entity,expr.args,"AND",params);
        case K::Or:return compile_joined(r,entity,expr.args,"OR",params);
    }
    throw std::invalid_argument("Invalid expression kind");
}
String SqlDialect::compile_function(Memory r,const EntityDescriptor& entity,ExprFunction function,std::span<const Expr> args,SqlBindings& params) const{
    // SOUNDEX has its own branch in the source trait; overriding the generic
    // single-argument helper must not intercept this extension point.
    if(function==ExprFunction::Soundex){
        if(args.size()!=1)fail(r,SqlCompileError::Code::InvalidFunctionArguments,"SOUNDEX expects exactly one argument");
        auto arg=compile_expr(r,entity,args[0],params);
        String out("SOUNDEX(",r);out+=arg;out+=')';return out;
    }
    if(function==ExprFunction::Gbk)return compile_gbk_function(r,entity,args,params);
    if(function==ExprFunction::Count && args.empty())return String("COUNT(*)",r);
    constexpr std::string_view names[]={"SOUNDEX","GBK","COUNT","SUM","AVG","MIN","MAX","STDDEV","STDDEV_POP","VAR_SAMP","VAR_POP","BIT_AND","BIT_OR","BIT_XOR"};
    return compile_single_arg_function(r,entity,names[unsigned(function)],args,params);
}
String SqlDialect::compile_single_arg_function(Memory r,const EntityDescriptor& entity,std::string_view function,std::span<const Expr> args,SqlBindings& params) const{
    if(args.size()!=1){String message(function,r);message+=" expects exactly one argument";fail(r,SqlCompileError::Code::InvalidFunctionArguments,message);}auto arg=compile_expr(r,entity,args[0],params);String out(function,r);out+='(';out+=arg;out+=')';return out;
}
String SqlDialect::compile_gbk_function(Memory r,const EntityDescriptor& entity,std::span<const Expr> args,SqlBindings& params) const{
    if(args.size()!=1)fail(r,SqlCompileError::Code::InvalidFunctionArguments,"GBK expects exactly one argument");
    return compile_expr(r,entity,args[0],params);
}
String SqlDialect::compile_subquery(Memory r,const EntityDescriptor& entity,const Expr& left,BinaryOp op,const EntityDescriptor& sub_entity,const SelectQuery& query,SqlBindings& params) const{
    auto lhs=compile_expr(r,entity,left,params);std::string_view sql_op;
    if(op==BinaryOp::In || op==BinaryOp::InLarge)sql_op="IN";else if(op==BinaryOp::NotIn || op==BinaryOp::NotInLarge)sql_op="NOT IN";
    else {constexpr std::string_view names[]={"Eq","Ne","Gt","Gte","Lt","Lte","Like","NotLike"};fail(r,SqlCompileError::Code::InvalidSubQueryOperator,names[unsigned(op)]);}
    detail::SqlBindingScope subquery_scope(params,{},false);auto subquery=compile_select_sql(r,sub_entity,query,params);String out("(",r);out+=lhs;out+=' ';out+=sql_op;out+=" (";out+=subquery;out+="))";return out;
}
String SqlDialect::compile_joined(Memory r,const EntityDescriptor& entity,std::span<const Expr> parts,std::string_view joiner,SqlBindings& params) const{
    Vector<String> compiled(r);for(const auto& part:parts)compiled.emplace_back(compile_expr(r,entity,part,params));String sep(" ",r);sep+=joiner;sep+=' ';String out("(",r);out+=join(r,compiled,sep);out+=')';return out;
}
String SqlDialect::compile_in(Memory r,const EntityDescriptor& entity,const Expr& left,BinaryOp op,const Expr& right,SqlBindings& params) const{
    auto lhs=compile_expr(r,entity,left,params);String out("(",r);out+=lhs;out+=op==BinaryOp::In || op==BinaryOp::InLarge ? " IN (":" NOT IN (";
    auto* list=right.kind==Expr::Kind::Value ? right.literal.as_list():nullptr;
    if(list){if(list->empty())fail(r,SqlCompileError::Code::EmptyInList);bool first=true;for(const auto& value:*list){if(!first)out+=", ";first=false;params.push(value);out+=placeholder(r,params.size());}}
    else out+=compile_expr(r,entity,right,params);
    out+="))";return out;
}
}
