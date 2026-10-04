#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/runtime/context.hpp>
#include <sqlite3.h>
#include <iostream>
using namespace teaql;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
class TestDialect:public SqlDialect {
public:
    DatabaseKind kind() const override{return DatabaseKind::Sqlite;}
    String quote_ident(Memory r,std::string_view name) const override{return quote_identifier_if_needed(r,name,'"');}
    String placeholder(Memory r,std::size_t) const override{return String("?",r);}
};
// Test-only native resource. This does not replace the planned SqlTransport.
struct Database {
    sqlite3* handle=nullptr;
    Database(){check(sqlite3_open(":memory:",&handle)==SQLITE_OK,"open test SQLite");}
    ~Database(){sqlite3_close(handle);}
    void sql(const char* sql){check(sqlite3_exec(handle,sql,nullptr,nullptr,nullptr)==SQLITE_OK,"execute test SQL");}
    int execute(const CompiledQuery& query){
        sqlite3_stmt* stmt=nullptr;check(sqlite3_prepare_v2(handle,query.sql.c_str(),-1,&stmt,nullptr)==SQLITE_OK,"prepare compiled mutation");
        struct Finalize{sqlite3_stmt* stmt;~Finalize(){sqlite3_finalize(stmt);}} cleanup{stmt};
        check(sqlite3_bind_parameter_count(stmt)==int(query.params.size()),"parameter count matches compiled bindings");
        for(std::size_t i=0;i<query.params.size();++i){
            const auto& v=query.params[i];int result=SQLITE_ERROR;
            if(v.is_null())result=sqlite3_bind_null(stmt,int(i+1));
            else if(auto p=std::get_if<std::int64_t>(&v.data))result=sqlite3_bind_int64(stmt,int(i+1),*p);
            else if(auto p=std::get_if<std::uint64_t>(&v.data))result=sqlite3_bind_int64(stmt,int(i+1),static_cast<sqlite3_int64>(*p));
            else if(auto p=std::get_if<String>(&v.data))result=sqlite3_bind_text(stmt,int(i+1),p->data(),int(p->size()),SQLITE_TRANSIENT);
            check(result==SQLITE_OK,"bind supported test fixture value");
        }
        check(sqlite3_step(stmt)==SQLITE_DONE,"execute compiled mutation");return sqlite3_changes(handle);
    }
    std::int64_t scalar(const char* sql){
        sqlite3_stmt* stmt=nullptr;check(sqlite3_prepare_v2(handle,sql,-1,&stmt,nullptr)==SQLITE_OK,"prepare observation");
        struct Finalize{sqlite3_stmt* stmt;~Finalize(){sqlite3_finalize(stmt);}} cleanup{stmt};
        check(sqlite3_step(stmt)==SQLITE_ROW,"observe row");return sqlite3_column_int64(stmt,0);
    }
};
static void sqlite_execution(){
    GlobalContext global;auto r=global.memory_resource();auto& dialect=global.make_resource<SqliteDialect>();auto& db=global.make_resource<Database>();
    auto& model=global.make<EntityDescriptor>("Record");model.table("records");model.property("id",DataType::U64).id();model.property("version",DataType::I64).version();model.property("name",DataType::Text);model.property("tenant",DataType::U64);model.mask_fields({"name"});
    auto ddl=dialect.compile_create_table(r,model);db.sql(ddl.c_str());
    for(const auto& sql:dialect.schema_indexes_sqls(r,model))db.sql(sql.c_str());
    check(db.scalar("SELECT count(*) FROM sqlite_master WHERE type='index' AND name='PK_RECORDS_ID_VERSION'")==1,"generated version index executes in SQLite");
    auto& insert=global.make<BatchInsertCommand>("Record");
    for(int i=1;i<=3;++i){insert.batch_values.emplace_back();auto& row=insert.batch_values.back().fields;row.emplace(String("id",r),Value(r,i));row.emplace(String("version",r),Value(r,1));row.emplace(String("tenant",r),Value(r,i%2));row.emplace(String("name",r),Value(r,"old"));}
    check(db.execute(dialect.compile_batch_insert(r,model,insert))==3,"batch insert applies all rows");
    auto& update=global.make<UpdateCommand>("Record",1);update.value("name","new").expected_version(1);
    check(db.execute(dialect.compile_guarded_update(r,model,update,Expr::eq(r,"tenant",0)))==0,"guard excludes a different tenant");
    check(db.execute(dialect.compile_guarded_update(r,model,update,Expr::eq(r,"tenant",1)))==1,"matching guard updates row");
    check(db.execute(dialect.compile_update(r,model,update))==0,"stale expected version matches no row");
    check(db.scalar("SELECT version FROM records WHERE id=1")==2,"successful update advances version once");
    auto& batch=global.make<BatchUpdateCommand>("Record");batch.update_fields.emplace_back("name");
    for(int i=1;i<=3;++i){batch.batch_ids.emplace_back(i);batch.batch_values.emplace_back();batch.batch_values.back().fields.emplace(String("name",r),Value(r,"batch"));}
    batch.batch_expected_versions={2,99,std::nullopt};
    check(db.execute(dialect.compile_batch_update(r,model,batch))==2,"batch expected versions filter only their corresponding records");
    check(db.scalar("SELECT version FROM records WHERE id=1")==3 && db.scalar("SELECT version FROM records WHERE id=3")==1,"unversioned batch row keeps version");
    auto& remove=global.make<DeleteCommand>("Record",1);remove.expected_version(3);
    check(db.execute(dialect.compile_guarded_delete(r,model,remove,Expr::eq(r,"tenant",1)))==1 && db.scalar("SELECT version FROM records WHERE id=1")==-4,"soft delete binds negative next version");
    auto& recover=global.make<RecoverCommand>("Record",1,-4);
    check(db.execute(dialect.compile_guarded_recover(r,model,recover,Expr::eq(r,"tenant",0)))==0,"recover guard excludes row");
    check(db.execute(dialect.compile_recover(r,model,recover))==1 && db.scalar("SELECT version FROM records WHERE id=1")==5,"recover advances deleted version magnitude");
    remove.expected_version(5).hard_delete();check(db.execute(dialect.compile_delete(r,model,remove))==1,"hard delete binds identity and expected version");
}
static void domains_and_dispatch(){
    GlobalContext global;auto r=global.memory_resource();auto user=global.make_user_context();auto u=user->memory_resource();
    struct HookDialect final:TestDialect {
        mutable unsigned updates=0,deletes=0,recovers=0;mutable const Expr* seen=nullptr;
        CompiledQuery compile_update_with_guard(Memory r,const EntityDescriptor& e,const UpdateCommand& c,const Expr* guard) const override{++updates;seen=guard;return SqlDialect::compile_update_with_guard(r,e,c,guard);}
        CompiledQuery compile_delete_with_guard(Memory r,const EntityDescriptor& e,const DeleteCommand& c,const Expr* guard) const override{++deletes;seen=guard;return SqlDialect::compile_delete_with_guard(r,e,c,guard);}
        CompiledQuery compile_recover_with_guard(Memory r,const EntityDescriptor& e,const RecoverCommand& c,const Expr* guard) const override{++recovers;seen=guard;return SqlDialect::compile_recover_with_guard(r,e,c,guard);}
    };
    auto& d=global.make_resource<HookDialect>();const SqlDialect& api=d;auto& e=user->make<EntityDescriptor>("Record");e.property("id",DataType::U64).id();e.property("version",DataType::I64).version();e.property("payload",DataType::Json);e.mask_fields({});
    auto& c=user->make<UpdateCommand>("Record",1);c.expected_version(1);
    {Value::Object payload(u);String large(4096,'p',u);payload.emplace(String("nested",u),Value(u,large));c.value("payload",Value(u,payload));}
    auto& guard=user->make<Expr>(Expr::eq(u,"id",1));
    auto& retained=global.make<CompiledQuery>(api.compile_guarded_update(r,e,c,guard));check(d.updates==1 && d.seen==&guard,"guarded update dispatches shared override with original guard");
    auto& temporary=user->make<CompiledQuery>(api.compile_update(u,e,c));auto& copy=global.make<CompiledQuery>(temporary);check(d.updates==2 && !d.seen,"unguarded update dispatches with absent guard");
    auto& del=user->make<DeleteCommand>("Record",1);(void)api.compile_delete(r,e,del);(void)api.compile_guarded_delete(r,e,del,guard);check(d.deletes==2 && d.seen==&guard,"delete retains guarded extension point");
    auto& recover=user->make<RecoverCommand>("Record",1,-1);(void)api.compile_recover(r,e,recover);(void)api.compile_guarded_recover(r,e,recover,guard);check(d.recovers==2 && d.seen==&guard,"recover retains guarded extension point");
    user.reset();
    check(retained.params[0]==copy.params[0] && retained.params[0].memory_resource()==r,"compiled mutation parameters use explicit destination lifetime");
    const auto& nested=std::get<Value::Object>(copy.params[0].data).begin()->second;check(nested.memory_resource()==r && std::get<String>(nested.data).size()==4096,"deep copied nested payload survives source close");
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());sqlite_execution();domains_and_dispatch();std::pmr::set_default_resource(previous);std::cout<<"SQL mutation contracts passed\n";}catch(const SqlCompileError& e){std::cerr<<"unexpected compile error "<<unsigned(e.code)<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
