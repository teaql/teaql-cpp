#include <teaql/sql/dialect.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
class TestDialect : public SqlDialect {
public:
    DatabaseKind kind() const override{return DatabaseKind::Sqlite;}
    String quote_ident(Memory r,std::string_view name) const override{return quote_identifier_if_needed(r,name,'"');}
    String placeholder(Memory r,std::size_t) const override{return String("?",r);}
};
// The source trait deliberately exposes individual compiler stages for provider
// overrides. These tests ensure dispatch still occurs through those stages.
class HookDialect final : public TestDialect {
public:
    mutable unsigned single_calls=0,gbk_calls=0,projection_calls=0,aggregate_calls=0,order_calls=0;
    String compile_single_arg_function(Memory r,const EntityDescriptor& e,std::string_view fn,std::span<const Expr> args,SqlBindings& bindings) const override{
        ++single_calls;return SqlDialect::compile_single_arg_function(r,e,fn,args,bindings);
    }
    String compile_gbk_function(Memory r,const EntityDescriptor& e,std::span<const Expr> args,SqlBindings& bindings) const override{
        ++gbk_calls;auto out=SqlDialect::compile_gbk_function(r,e,args,bindings);out+=" COLLATE fixture";return out;
    }
    String select_projection(Memory r,const EntityDescriptor& e,const SelectQuery& q,SqlBindings& bindings) const override{
        ++projection_calls;return SqlDialect::select_projection(r,e,q,bindings);
    }
    String aggregate_projection(Memory r,const EntityDescriptor& e,const SelectQuery& q,SqlBindings& bindings) const override{
        ++aggregate_calls;return SqlDialect::aggregate_projection(r,e,q,bindings);
    }
    String resolve_order_field(Memory r,const EntityDescriptor& e,const OrderBy& order,SqlBindings& bindings) const override{
        ++order_calls;return SqlDialect::resolve_order_field(r,e,order,bindings);
    }
    std::string_view aggregate_function_sql(AggregateFunction fn) const override{
        return fn==AggregateFunction::BitXor?"fixture_bit_xor":SqlDialect::aggregate_function_sql(fn);
    }
    String schema_type_sql(Memory r,DataType type,const PropertyDescriptor& p) const override{
        return type==DataType::Json?String("JSON",r):SqlDialect::schema_type_sql(r,type,p);
    }
    std::string_view fallback_default_value_sql(DataType type) const override{
        return type==DataType::Json?"'[]'":SqlDialect::fallback_default_value_sql(type);
    }
};
static void extension_points(){
    GlobalContext global;auto r=global.memory_resource();auto& d=global.make_resource<HookDialect>();const SqlDialect& api=d;
    auto& e=global.make<EntityDescriptor>("Record");e.property("name",DataType::Text);e.property("payload",DataType::Json).not_null();
    auto& q=global.make<SelectQuery>("Record");q.project_expr("sound",Expr::soundex(Expr::column(r,"name"))).project_expr("total",Expr::count_all(r)).order_gbk_asc("name");
    auto plain=api.compile_select(r,e,q);
    check(d.single_calls==0 && d.gbk_calls==1 && d.projection_calls==1 && d.order_calls==1,"SOUNDEX, zero-argument COUNT, GBK and order retain distinct dispatch");
    check(plain.sql.find("ORDER BY name COLLATE fixture ASC")!=String::npos,"provider GBK override is used");
    q.project_expr("n",Expr::sum_expr(Expr::column(r,"name"))).aggregate(Aggregate::bit_xor(r,"name","xor"));
    auto aggregate=api.compile_select(r,e,q);
    check(d.single_calls==1 && d.aggregate_calls==1 && d.projection_calls==1,"aggregate projection is independent of ordinary projection");
    check(aggregate.sql.find("fixture_bit_xor(name) AS xor")!=String::npos,"aggregate naming override is used");
    auto& payload=e.properties[1];check(api.compile_add_column(r,e,payload)=="ALTER TABLE record_data ADD COLUMN payload JSON NOT NULL DEFAULT '[]'","schema type and fallback value are independent hooks");
    check(!api.large_in_uses_array_param() && !api.prefers_small_parent_relation_probes() && api.schema_setup_sqls().empty(),"default capability flags do not claim provider support");
}
static void scope_after_failures(){
    GlobalContext global;auto r=global.memory_resource();auto& d=global.make_resource<TestDialect>();
    auto& outer=global.make<EntityDescriptor>("Outer");outer.property("token",DataType::Text,"access_token");outer.mask_fields({});
    auto& inner=global.make<EntityDescriptor>("Inner");inner.property("public",DataType::Text);inner.mask_fields({});
    auto& q=global.make<SelectQuery>("Inner");q.project_expr("first",Expr::value(r,"inner plain literal"));q.filter(Expr::eq(r,"missing","value"));
    Expr subquery(r,Expr::Kind::SubQuery);subquery.op=BinaryOp::In;subquery.args.emplace_back(Expr::column(r,"token"));subquery.subquery_data=detail::make_box<SubQueryExpr>(r,inner,q);
    auto expr=Expr::binary(std::move(subquery),BinaryOp::Eq,Expr::column(r,"token"));auto& bindings=global.make<SqlBindings>();
    bool failed=false;try{(void)d.compile_expr(r,outer,expr,bindings);}catch(const SqlCompileError& e){failed=e.code==SqlCompileError::Code::UnknownField;}
    check(failed,"subquery compilation reports structured inner error");
    (void)d.compile_expr(r,outer,Expr::value(r,"after failure"),bindings);
    auto policies=bindings.log_context(r,DatabaseKind::Sqlite).parameter_policies;
    check(bindings.size()==2 && policies[0]==SqlParameterLogPolicy::Unknown && policies[1]==SqlParameterLogPolicy::Unknown,"inner scope isolation and outer scope restoration survive exceptions");
    // Failure after binding must restore scope even when it comes from a
    // provider hook instead of a SqlCompileError.
    struct ThrowingDialect final:TestDialect {
        String placeholder(Memory,std::size_t) const override{throw std::bad_alloc{};}
    } failing;
    try{(void)failing.compile_expr(r,outer,Expr::eq(r,"token","credential"),bindings);}catch(const std::bad_alloc&){}
    (void)d.compile_expr(r,outer,Expr::value(r,"outside"),bindings);
    policies=bindings.log_context(r,DatabaseKind::Sqlite).parameter_policies;
    check(policies[2]==SqlParameterLogPolicy::Credential && policies[3]==SqlParameterLogPolicy::Unknown,"allocation failure does not leave credential scope active");
}
static void destination_lifetime(){
    GlobalContext global;auto r=global.memory_resource();auto user=global.make_user_context();auto u=user->memory_resource();
    auto& d=global.make_resource<TestDialect>();auto& model=user->make<EntityDescriptor>("Temporary");model.property("payload",DataType::Json);model.mask_fields({});
    auto& q=user->make<SelectQuery>("Temporary");
    {Value::Object payload(u);String large(4096,'x',u);payload.emplace(String("nested",u),Value(u,large));q.filter(Expr::eq(u,"payload",Value(u,payload))).comment("temporary comment with an independently allocated buffer");}
    auto& retained=global.make<CompiledQuery>(d.compile_select(r,model,q));
    auto& transient=user->make<CompiledQuery>(d.compile_select(u,model,q));auto& cloned=global.make<CompiledQuery>(transient);
    user.reset();
    check(retained==cloned && retained.sql=="SELECT payload FROM temporary_data WHERE (payload = ?)","compiler destination and explicit deep copy outlive inputs");
    const auto& object=std::get<Value::Object>(retained.params[0].data);check(object.get_allocator().resource()==r && object.begin()->second.memory_resource()==r,"nested binding objects use destination allocation domain");
    check(std::get<String>(object.begin()->second.data).size()==4096 && retained.comment->get_allocator().resource()==r,"nested text and comments do not retain user memory");
}
static void schema_domains(){
    GlobalContext global;auto r=global.memory_resource();auto user=global.make_user_context();auto u=user->memory_resource();
    auto& model=user->make<EntityDescriptor>("LongEntity");model.table("straße_with_a_long_physical_table_name");
    model.property("id",DataType::U64,"the_long_physical_identity_column").id();
    model.property("version",DataType::I64,"the_long_physical_version_column").version();
    model.property("ownerId",DataType::U64,"the_long_physical_owner_column");
    SchemaIndexSpec* retained=nullptr;SchemaIndexSpec* assigned=nullptr;SqlCompileError* retained_error=nullptr;
    {
        auto specs=schema_index_specs(u,model,31);check(specs.size()==2 && specs[0].unique && !specs[1].unique,"version and property-derived indexes remain independent");
        retained=&global.make<SchemaIndexSpec>(specs[0]);assigned=&global.make<SchemaIndexSpec>();*assigned=std::move(specs[1]);
        check(assigned->memory_resource()==r && assigned->columns[0].get_allocator().resource()==r,"cross-domain assignment deep copies nested index columns");
        const EntityDescriptor* entities[]={&model};
        try{validate_schema_identifier_lengths(u,entities,16);}catch(const SqlCompileError& e){retained_error=&global.make<SqlCompileError>(e);}
    }
    user.reset();
    check(retained->name.size()<=31 && retained->table=="straße_with_a_long_physical_table_name" && retained->columns[0]=="the_long_physical_identity_column","generated names are bounded without changing physical model names");
    check(retained_error && retained_error->identifier==retained->table && retained_error->actual_bytes==retained->table.size(),"physical-name error carries original UTF-8 bytes in explicit destination lifetime");
    check(assigned->columns[0]=="the_long_physical_owner_column","assigned nested columns survive source close");
    const std::string_view invalid[]={std::string_view("\xC0\x80",2),std::string_view("\xED\xA0\x80",3),std::string_view("\xF4\x90\x80\x80",4),std::string_view("\xF0\x9F",2)};
    for(auto bytes:invalid){bool rejected=false;try{(void)schema_foreign_key_name(r,bytes,"a","b","c",63);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"C++ byte strings outside Rust UTF-8 domain are rejected");}
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());extension_points();scope_after_failures();destination_lifetime();schema_domains();std::pmr::set_default_resource(previous);std::cout<<"SQL dialect contracts passed\n";}catch(const SqlCompileError& e){std::cerr<<"unexpected SQL compile error: "<<unsigned(e.code)<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
