#include <teaql/sql/bindings.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static void rendering(){
    GlobalContext global;auto r=global.memory_resource();auto& query=global.make<CompiledQuery>();
    query.sql="SELECT '?', [?], \"?\", `?`, ? /* outer /* ? */ ? */ -- ?\n, ?";query.params.emplace_back("O'Reilly");query.params.emplace_back(23);
    query.comment.emplace("why */ nested /* comment */",r);
    check(query.sql_with_comment(r).starts_with("/* why * / nested /* comment * / */ "),"comment terminators are escaped exactly");
    // A nested opening comment remains significant to the strict renderer.
    check(query.debug_sql(r,DatabaseKind::Sqlite)=="[SQL omitted: unterminated SQL comment; NOT REPLAYABLE]","malformed diagnostic comment cannot look replayable");
    query.comment.reset();auto rendered=query.debug_sql(r,DatabaseKind::Sqlite);
    check(rendered.find("'O''Reilly'")!=String::npos && rendered.ends_with("\n, 23"),"quoted strings and comments do not consume placeholders");
    auto callback=[&](std::size_t){return SqlRenderResult::err(r,"privacy projection omitted");};
    check(render_sql_with(r,"SELECT ?",DatabaseKind::Sqlite,1,callback).error()=="privacy projection omitted","literal projection failure propagates without partial replay text");
    check(std::get<String>(query.params[0].data)=="O'Reilly","diagnostic rendering does not alter binding data");
}
static void bindings_and_scope(){
    GlobalContext global;auto r=global.memory_resource();auto& entity=global.make<EntityDescriptor>("User");entity.mask_fields({"name"});entity.property("name",DataType::Text);entity.property("visible",DataType::Text);entity.property("token",DataType::Text,"access_token");
    auto& bindings=global.make<SqlBindings>();Value text(r,"payload");using P=SqlParameterLogPolicy;
    {detail::SqlBindingScope outer(bindings,P::Masked);bindings.push(text);
        {detail::SqlBindingScope inner(bindings,P::Credential);bindings.push(text);
            {detail::SqlBindingScope subquery(bindings,{},false);bindings.push(text);}bindings.push(text);}
        bindings.push(text);bindings.push_field(entity,"visible",text);
        try{detail::SqlBindingScope exception(bindings,P::Credential);throw 1;}catch(int){}bindings.push(text);
    }bindings.push(text);bindings.push_field(entity,"missing",text);
    auto log=bindings.log_context(r,DatabaseKind::Sqlite);
    const P expected[]={P::Masked,P::Credential,P::Unknown,P::Credential,P::Masked,P::Plain,P::Masked,P::Unknown,P::Unknown};
    check(std::equal(log.parameter_policies.begin(),log.parameter_policies.end(),std::begin(expected),std::end(expected)),"scalar scopes combine, subqueries isolate and exceptions restore provenance");
    auto scalar=Expr::binary(Expr::column(r,"token"),BinaryOp::Eq,Expr::value(r,"secret"));
    check(detail::sql_expression_policy(entity,scalar)==P::Credential && !detail::sql_expression_policy(entity,scalar.or_expr(Expr::eq(r,"name","n"))),"logical siblings do not inherit scalar credential provenance");
    check(detail::sql_field_policy(entity,"token")==P::Credential,"physical column names classify credentials");
    auto& query=global.make<SelectQuery>("User");query.filter(Expr::binary(Expr::eq(r,"visible","looks public"),BinaryOp::Eq,Expr::column(r,"token")));
    auto compiled=global.make_resource<SqliteDialect>().compile_select(r,entity,query);
    check(compiled.log_context.parameter_policies[0]==P::Credential,"a nested plain scalar cannot weaken enclosing credential provenance");
    bindings.mark_untrusted_sql();check(!bindings.log_context(r,DatabaseKind::Sqlite).generated_sql,"untrusted text cannot claim compiler-generated provenance");
}
class FailingResource : public std::pmr::memory_resource {
public:int remaining=-1;
private:
    void* do_allocate(std::size_t n,std::size_t a) override{if(remaining==0)throw std::bad_alloc{};if(remaining>0)--remaining;return std::pmr::new_delete_resource()->allocate(n,a);}
    void do_deallocate(void* p,std::size_t n,std::size_t a) override{std::pmr::new_delete_resource()->deallocate(p,n,a);}
    bool do_is_equal(const std::pmr::memory_resource& r) const noexcept override{return this==&r;}
};
static void failure_and_domains(){
    GlobalContext global;auto r=global.memory_resource();String long_text(4096,'x',r);Value v(r,long_text);
    for(int fail=0;fail<3;++fail){FailingResource resource;SqlBindings bindings(&resource);resource.remaining=fail;bool threw=false;try{bindings.push(v);}catch(const std::bad_alloc&){threw=true;}resource.remaining=-1;check(threw && bindings.size()==0 && bindings.log_context(r,DatabaseKind::Sqlite).parameter_policies.empty(),"allocation failure cannot split value and policy positions");}
    auto user=global.make_user_context();auto u=user->memory_resource();auto& query=user->make<CompiledQuery>();query.sql="SELECT ?";query.params.emplace_back(long_text);query.comment.emplace("a long comment owned by the temporary user context",u);query.log_context.parameter_policies.push_back(SqlParameterLogPolicy::Masked);
    auto& retained=global.make<CompiledQuery>(query);auto& error=user->make<SqlCompileError>(SqlCompileError::Code::InvalidSchemaShape,"a long property name needing a heap allocation");error.reason="invalid precision and scale combination";auto& retained_error=global.make<SqlCompileError>(error);
    user.reset();check(retained.comment->get_allocator().resource()==r && retained.params[0].memory_resource()==r && retained.debug_sql(r,DatabaseKind::Sqlite).find(long_text)!=String::npos,"compiled SQL and nested provenance survive explicit destination copy");
    check(retained_error.reason.get_allocator().resource()==r && retained_error.to_string(r).find("invalid precision")!=String::npos,"structured compile errors keep their payload and destination domain");
    auto& model=global.make<EntityDescriptor>("Device");model.property("id",DataType::U64).id();auto& select=global.make<SelectQuery>("Device");select.comment("");
    auto compiled=global.make_resource<SqliteDialect>().compile_select(r,model,select);check(compiled.comment && compiled.comment->empty() && compiled.log_context.generated_sql,"compilation is independent of UserContext and preserves Some empty comment");
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());rendering();bindings_and_scope();failure_and_domains();std::pmr::set_default_resource(previous);std::cout<<"SQL model contracts passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
