#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static void dialect_contract(){
    GlobalContext global;auto r=global.memory_resource();auto& dialect=global.make_resource<SqliteDialect>();const SqlDialect& base=dialect;
    auto& e=global.make<EntityDescriptor>("Entity");e.table("order");
    constexpr std::string_view types[]={"BOOLEAN","INTEGER","INTEGER","REAL","NUMERIC","VARCHAR(255)","TEXT","JSON","DATE","TIMESTAMP"};
    for(unsigned i=0;i<10;++i){PropertyDescriptor p(r,"field",DataType(i));check(base.schema_type_sql(r,p.data_type,p)==types[i],"SQLite storage type differs from generic fallback");}
    auto& p=e.property("payload",DataType::Json).not_null();
    check(base.compile_create_table(r,e)=="CREATE TABLE IF NOT EXISTS \"order\" (payload JSON NOT NULL)","create table dispatches SQLite type mapping");
    check(base.compile_add_column(r,e,p)=="ALTER TABLE \"order\" ADD COLUMN payload JSON NOT NULL","SQLite add-column preserves absence of generic DEFAULT");
    PropertyDescriptor decimal(r,"amount",DataType::Decimal);decimal.with_numeric_precision(12).with_numeric_scale(3);
    check(base.column_definition_sql(r,decimal)=="amount NUMERIC(12,3)","decimal precision delegates to generic shape handling");
    decimal.with_numeric_scale(13);bool rejected=false;try{(void)base.compile_add_column(r,e,decimal);}catch(const SqlCompileError& error){rejected=error.code==SqlCompileError::Code::InvalidSchemaShape;}
    check(rejected,"provider add-column retains generic shape validation");
    check(base.kind()==DatabaseKind::Sqlite && base.prefers_small_parent_relation_probes() && !base.large_in_uses_array_param() && base.schema_setup_sqls().empty(),"SQLite source capability flags are preserved");
    check(base.placeholder(r,1)=="?" && base.placeholder(r,1000)=="?" && base.quote_ident(r,"select")=="\"select\"","SQLite parameter and identifier overrides are dispatched");
    auto& q=global.make<SelectQuery>("Entity");q.project_expr("n",Expr::count_all(r));
    check(base.compile_select(r,e,q).sql=="SELECT COUNT(*) AS n FROM \"order\"","provider inherits full generic query compiler");
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());dialect_contract();std::pmr::set_default_resource(previous);std::cout<<"SQLite provider dialect contracts passed\n";}catch(const SqlCompileError& e){std::cerr<<"unexpected SQL compile error "<<unsigned(e.code)<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
