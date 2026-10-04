#include <teaql/provider_sqlite/detail/schema.hpp>
#include <teaql/provider_sqlite/detail/decoding.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/event.hpp>
#include <teaql/core/detail/debug.hpp>
#include <sqlite3.h>
#include <charconv>
namespace teaql::detail {
namespace {
void number(String& out,std::int64_t value){char b[32];auto end=std::to_chars(b,b+sizeof(b),value).ptr;out.append(b,end);}
bool ascii_equal(std::string_view a,std::string_view b){if(a.size()!=b.size())return false;for(std::size_t i=0;i<a.size();++i){auto lower=[](char c){return c>='A' && c<='Z'?char(c+32):c;};if(lower(a[i])!=lower(b[i]))return false;}return true;}
MutationExecutorError error(Memory r,sqlite3& db,int code){return MutationExecutorError::sqlite(r,code,sqlite3_errcode(&db)==code?sqlite3_errmsg(&db):sqlite3_errstr(code));}
MutationExecutorError prepare_error(Memory r,sqlite3& db,int code,std::string_view sql){
#if SQLITE_VERSION_NUMBER >= 3038000
    if((code&255)==SQLITE_ERROR && sqlite3_error_offset(&db)>=0){String message(sqlite3_errmsg(&db),r);message+=" in ";message+=sql;message+=" at offset ";number(message,sqlite3_error_offset(&db));return MutationExecutorError::sqlite(r,0,message);}
#else
    (void)sql;
#endif
    return error(r,db,code);
}
struct Statement {
    Memory r;sqlite3& db;sqlite3_stmt* stmt=nullptr;
    Statement(Memory r,sqlite3& db,std::string_view sql):r(r),db(db){
        if(sql.size()>INT32_MAX)throw MutationExecutorError::sqlite(r,SQLITE_TOOBIG,"string or blob too big");
        const char* tail=nullptr;auto code=sqlite3_prepare_v2(&db,sql.data(),int(sql.size()),&stmt,&tail);
        if(code!=SQLITE_OK)throw prepare_error(r,db,code,sql);
        try{bool extra=false;auto rest=sql;while(tail && tail>rest.data() && tail<rest.data()+rest.size()){rest.remove_prefix(std::size_t(tail-rest.data()));sqlite3_stmt* next=nullptr;code=sqlite3_prepare_v2(&db,rest.data(),int(rest.size()),&next,&tail);if(code!=SQLITE_OK)throw prepare_error(r,db,code,rest);extra|=next!=nullptr;sqlite3_finalize(next);}if(extra)throw MutationExecutorError::sqlite(r,0,"Multiple statements provided");}catch(...){sqlite3_finalize(stmt);stmt=nullptr;throw;}
    }
    ~Statement(){sqlite3_finalize(stmt);}
    Statement(const Statement&)=delete;
    void bind(std::string_view text){auto code=sqlite3_bind_text64(stmt,1,text.data(),text.size(),SQLITE_TRANSIENT,SQLITE_UTF8);if(code!=SQLITE_OK)throw error(r,db,code);}
    bool next(){auto code=sqlite3_step(stmt);if(code==SQLITE_ROW)return true;if(code==SQLITE_DONE)return false;throw error(r,db,code);}
    void exec(){if(next())throw MutationExecutorError::sqlite(r,0,"Execute returned results - did you mean to call query?");}
    void require_type(int index,int expected){auto type=sqlite3_column_type(stmt,index);if(type==expected)return;String message("Invalid column type ",r);message+=type==SQLITE_INTEGER?"Integer":type==SQLITE_FLOAT?"Real":type==SQLITE_TEXT?"Text":type==SQLITE_BLOB?"Blob":"Null";message+=" at index: ";number(message,index);message+=", name: ";message+=sqlite3_column_name(stmt,index);throw MutationExecutorError::sqlite(r,0,message);}
    std::int64_t integer(int index){require_type(index,SQLITE_INTEGER);return sqlite3_column_int64(stmt,index);}
    String text(int index){require_type(index,SQLITE_TEXT);auto* data=reinterpret_cast<const char*>(sqlite3_column_text(stmt,index));if(!data)throw MutationExecutorError::sqlite(r,SQLITE_NOMEM,"out of memory");std::string_view value(data,std::size_t(sqlite3_column_bytes(stmt,index)));if(auto invalid=sqlite_utf8_error(r,value)){*invalid+=" at index ";number(*invalid,index);throw MutationExecutorError::sqlite(r,0,*invalid);}return String(value,r);}
    std::optional<String> optional_text(int index){if(sqlite3_column_type(stmt,index)==SQLITE_NULL)return {};return text(index);}
};
void execute(Memory r,sqlite3& db,std::string_view sql){Statement statement(r,db,sql);statement.exec();}
String pragma(Memory r,std::string_view name,std::string_view table){String result("PRAGMA ",r);result+=name;result+='(';result+=quote_identifier_if_needed(r,table,'"');result+=')';return result;}
bool table_exists(Memory r,sqlite3& db,std::string_view name){Statement s(r,db,"SELECT COUNT(1) FROM sqlite_master WHERE type = 'table' AND name = ?");s.bind(name);s.next();return s.integer(0)>0;}
using Columns=std::pmr::map<String,std::pair<String,bool>,std::less<>>;
Columns table_columns(Memory r,sqlite3& db,std::string_view table){Columns columns(r);Statement s(r,db,pragma(r,"table_info",table));while(s.next()){auto notnull=s.integer(3),pk=s.integer(5);bool nullable=!(notnull!=0 || pk!=0);auto name=sqlite_identifier_lower(r,s.text(1));auto type=s.text(2);columns.insert_or_assign(std::move(name),std::pair(std::move(type),nullable));}return columns;}
void required_column(Memory r,sqlite3& db,const EntityDescriptor& e,const PropertyDescriptor& p){if(p.nullable)return;String sql("SELECT EXISTS(SELECT 1 FROM ",r);sql+=quote_identifier_if_needed(r,e.table_name,'"');sql+=" LIMIT 1)";Statement s(r,db,sql);s.next();if(!s.integer(0))return;String message("ensure schema cannot add required column without a deterministic backfill: entity=",r);message+=e.name;message+=", table=";message+=e.table_name;message+=", column=";message+=p.column_name;message+="; the table contains rows, so migrate/backfill explicitly before retrying";throw MutationExecutorError::bind(r,message);}
struct ForeignKeyRow {
    using allocator_type=Allocator;
    std::int64_t id,seq;String column,table,target,update,remove;
    ForeignKeyRow(std::allocator_arg_t,Allocator a,Statement& s):id(s.integer(0)),seq(s.integer(1)),column(s.text(3),a.resource()),table(s.text(2),a.resource()),target(s.text(4),a.resource()),update(s.text(5),a.resource()),remove(s.text(6),a.resource()){}
    ForeignKeyRow(std::allocator_arg_t,Allocator a,ForeignKeyRow&& other):id(other.id),seq(other.seq),column(std::move(other.column),a.resource()),table(std::move(other.table),a.resource()),target(std::move(other.target),a.resource()),update(std::move(other.update),a.resource()),remove(std::move(other.remove),a.resource()){}
    ForeignKeyRow(const ForeignKeyRow&)=delete;ForeignKeyRow(ForeignKeyRow&&) noexcept=default;
};
void foreign_keys(Memory r,sqlite3& db,std::span<const EntityDescriptor* const> entities){
    for(auto* e:entities){auto expected=sqlite_foreign_keys_for(r,*e,entities);if(expected.empty())continue;Vector<ForeignKeyRow> actual(r);{Statement s(r,db,pragma(r,"foreign_key_list",e->table_name));while(s.next())actual.emplace_back(s);}
        for(const auto& key:expected){bool found=false;std::optional<String> incompatible;
            for(const auto& candidate:actual){if(!ascii_equal(candidate.column,key[0]) || !ascii_equal(candidate.table,key[1]) || !ascii_equal(candidate.target,key[2]))continue;found=true;std::size_t count=0;for(const auto& row:actual)if(row.id==candidate.id)++count;auto restricts=[](std::string_view action){return ascii_equal(action,"NO ACTION") || ascii_equal(action,"RESTRICT");};
                if(candidate.seq==0 && count==1 && restricts(candidate.update) && restricts(candidate.remove)){incompatible.reset();break;}
                if(!incompatible){String message("ensure schema incompatible SQLite foreign-key shape: table=",r);message+=e->table_name;message+=", column=";message+=key[0];message+=", referenced_table=";message+=key[1];message+=", referenced_column=";message+=key[2];message+=", expected=one full-column key with RESTRICT actions, installed_fk_id=";number(message,candidate.id);message+=", installed_seq=";number(message,candidate.seq);message+=", installed_key_count=";number(message,std::int64_t(count));message+=", installed_update=";message+=candidate.update;message+=", installed_delete=";message+=candidate.remove;message+="; rebuild/migrate the table explicitly before retrying";incompatible=std::move(message);}
            }
            if(!found){String message("ensure schema cannot add missing SQLite foreign key in place: table=",r);message+=e->table_name;message+=", column=";message+=key[0];message+=", referenced_table=";message+=key[1];message+=", referenced_column=";message+=key[2];message+="; rebuild/migrate the table explicitly before retrying";throw MutationExecutorError::bind(r,message);}
            if(incompatible)throw MutationExecutorError::bind(r,*incompatible);
        }
    }
}
void index_shapes(Memory r,sqlite3& db,const EntityDescriptor& e){
    auto table=strip_sqlite_identifier_quotes(e.table_name);
    for(const auto& spec:schema_index_specs(r,e,std::nullopt)){
        std::optional<String> installed;{Statement s(r,db,"SELECT tbl_name FROM sqlite_master WHERE type='index' AND name=?1");s.bind(spec.name);if(s.next())installed=s.text(0);}
        if(!installed){String message("SQLite declared index missing: table=",r);message+=table;message+=" index=";message+=spec.name;throw MutationExecutorError::bind(r,message);}
        String flags("None",r);bool flags_match=false;{Statement s(r,db,pragma(r,"index_list",table));while(s.next()){auto name=s.text(1);auto unique=s.integer(2),partial=s.integer(4);if(name!=spec.name || flags!="None")continue;flags="Some((";flags+=debug_text(r,name);flags+=", ";number(flags,unique);flags+=", ";number(flags,partial);flags+="))";flags_match=unique==std::int64_t(spec.unique) && partial==0;}}
        bool keys_match=true;std::size_t count=0;String keys("[",r);{Statement s(r,db,pragma(r,"index_xinfo",spec.name));while(s.next()){auto seq=s.integer(0),cid=s.integer(1);auto name=s.optional_text(2);auto desc=s.integer(3);auto collation=s.text(4);auto key=s.integer(5);if(key!=1)continue;
                if(count)keys+=", ";
                keys+='(';number(keys,seq);keys+=", ";number(keys,cid);keys+=", ";if(name){keys+="Some(";keys+=debug_text(r,*name);keys+=')';}else keys+="None";keys+=", ";number(keys,desc);keys+=", ";keys+=debug_text(r,collation);keys+=", ";number(keys,key);keys+=')';
                keys_match=keys_match && count<spec.columns.size() && cid>=0 && name && ascii_equal(*name,strip_sqlite_identifier_quotes(spec.columns[count])) && desc==0 && ascii_equal(collation,"BINARY");++count;
            }}keys+=']';keys_match=keys_match && count==spec.columns.size();
        if(!ascii_equal(*installed,table) || !flags_match || !keys_match){String message("SQLite declared index shape mismatch: table=",r);message+=table;message+=" index=";message+=spec.name;message+="; expected=";message+=spec.unique?"unique":"non-unique";message+=" on (";bool first=true;for(const auto& column:spec.columns){if(!first)message+=", ";first=false;message+=column;}message+=") with ascending BINARY full-column keys; installed table=";message+=*installed;message+=", flags=";message+=flags;message+=", keys=";message+=keys;message+="; drop/rename the colliding index or migrate the table explicitly";throw MutationExecutorError::bind(r,message);}
    }
}
void soundex_callback(sqlite3_context* context,int,sqlite3_value** values) noexcept{std::optional<std::string_view> input;if(sqlite3_value_type(values[0])==SQLITE_TEXT){auto* text=reinterpret_cast<const char*>(sqlite3_value_text(values[0]));if(text){std::string_view value(text,std::size_t(sqlite3_value_bytes(values[0])));if(sqlite_schema_utf8_valid(value))input=value;}}auto result=sqlite_compatible_soundex(input);sqlite3_result_text(context,result.data(),int(result.size()),SQLITE_TRANSIENT);}
void rollback(Memory r,sqlite3& db) noexcept{try{execute(r,db,"ROLLBACK");}catch(...) {}}
}
bool sqlite_table_exists(Memory r,sqlite3& db,std::string_view table){return table_exists(r,db,table);}
SqliteTableColumns sqlite_table_columns(Memory r,sqlite3& db,std::string_view table){return table_columns(r,db,table);}
void ensure_sqlite_physical_schema(Memory r,sqlite3& db,const SqliteDialect& dialect,std::span<const EntityDescriptor* const> entities,const UserContext& context){
    auto code=sqlite3_create_function_v2(&db,"soundex",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,nullptr,soundex_callback,nullptr,nullptr,nullptr);if(code!=SQLITE_OK)throw error(r,db,code);
    execute(r,db,"PRAGMA foreign_keys = true");execute(r,db,"BEGIN IMMEDIATE");
    try{
        execute(r,db,"CREATE TABLE IF NOT EXISTS teaql_id_space (type_name VARCHAR(100) PRIMARY KEY, current_level BIGINT NOT NULL)");
        for(auto* entity:entities){
            if(!table_exists(r,db,entity->table_name)){execute(r,db,compile_sqlite_create_table(r,dialect,*entity,entities));(void)context.send_event(RawAuditEvent::schema_created(r,entity->name,entity->table_name,entity->properties.size()));}
            else{auto columns=table_columns(r,db,entity->table_name);for(const auto& property:entity->properties){auto name=sqlite_identifier_lower(r,strip_sqlite_identifier_quotes(property.column_name));auto found=columns.find(name);if(found!=columns.end()){ensure_sqlite_column_compatibility(r,*entity,property,found->second.first,found->second.second);continue;}required_column(r,db,*entity,property);execute(r,db,dialect.compile_add_column(r,*entity,property));(void)context.send_event(RawAuditEvent::field_added(r,entity->name,entity->table_name,property.column_name));}}
            for(const auto& sql:dialect.schema_indexes_sqls(r,*entity))execute(r,db,sql);
            index_shapes(r,db,*entity);
            (void)context.send_event(RawAuditEvent::schema_verified(r,entity->name,entity->table_name,entity->properties.size()));
        }
        foreign_keys(r,db,entities);
    }catch(SqlCompileError& e){rollback(r,db);throw MutationExecutorError::sql_compile(r,std::move(e));}
    catch(MutationExecutorError&){rollback(r,db);throw;}
    try{execute(r,db,"COMMIT");}catch(MutationExecutorError&){rollback(r,db);throw;}
}
}
