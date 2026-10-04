#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_formatter.hpp>
#include <iostream>
#include <ctime>
#include <cstdlib>
using namespace teaql;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> static void rejected(F&& f,const char* message){bool failed=false;try{f();}catch(const std::logic_error&){failed=true;}check(failed,message);}
struct ConservativeFormatter final:LogFormatter {
    String format_sql_log(Memory r,std::span<const TraceNode>,const SqlLogEntry&) const override{return String("sql",r);}
    String format_audit_log(Memory r,const RawAuditEvent&) const override{return String("field names only",r);}
};
static void factory_and_domains(){
    GlobalContext global;auto user=global.make_user_context();auto r=user->memory_resource(),g=global.memory_resource();auto& factory=global.log_formatter_factory();
    setenv("TEAQL_LOG_FORMAT","debug",1);check(&factory==&global.log_formatter_factory(),"factory is owned once per explicit application");setenv("TEAQL_LOG_FORMAT","human",1);auto& formatter=factory.get_formatter();check(dynamic_cast<const HumanReaderFormatter*>(&formatter),"selection reads environment lazily on first get");
    setenv("TEAQL_LOG_FORMAT","json",1);check(&formatter==&factory.get_formatter(),"later environment changes do not replace selected formatter");
    GlobalContext other;check(dynamic_cast<const DebugReaderFormatter*>(&other.log_formatter_factory().get_formatter()),"independent Global selects independently; json means DebugReaderFormatter");
    rejected([&]{user->make<LogFormatterFactory>(global);},"factory cannot belong to UserContext");rejected([&]{global.make<LogFormatterFactory>(other);},"factory cannot retain a different GlobalContext");
    setenv("TEAQL_AUDIT_LOG_ENTITIES"," LongEntityForAllocation , SecondEntityForAllocation ",1);setenv("TEAQL_SQL_LOG_TABLES"," LongTableForAllocation ",1);setenv("TEAQL_TOOL_LOG_FOCUS"," LongModuleForAllocation ",1);auto& source=user->make<LogConfig>(LogConfig::load(r));auto& config=global.make<LogConfig>(source);auto& assigned=global.make<LogConfig>();assigned=detail::clone_in(r,source);source.audit_entities->clear();
    auto& event=user->make<RawAuditEvent>(RawAuditEvent::schema_created(r,"LongEntityForAllocation","long_table_for_allocation",3));auto& output=global.make<String>(formatter.format_audit_log(r,event));user.reset();
    check(config.should_log_audit("longentityforallocation") && assigned.should_log_audit("SECONDENTITYFORALLOCATION"),"configuration copies remain independent after User closes");
    check(config.audit_entities->get_allocator().resource()==g && config.audit_entities->at(0).get_allocator().resource()==g && assigned.sql_tables->at(0).get_allocator().resource()==g,"filter lists and nested strings propagate allocator");
    check(output.find("LongEntityForAllocation")!=String::npos && output.get_allocator().resource()==g,"formatted result explicitly retained in Global survives source close");
    unsetenv("TEAQL_AUDIT_LOG_ENTITIES");unsetenv("TEAQL_SQL_LOG_TABLES");unsetenv("TEAQL_TOOL_LOG_FOCUS");unsetenv("TEAQL_LOG_FORMAT");
}
static void formats(){
    GlobalContext global;auto r=global.memory_resource();auto& human=global.make<HumanReaderFormatter>();auto& debug=global.make<DebugReaderFormatter>();Record values(r);values.emplace("id",Value(r,std::uint64_t(7)));values.emplace("name",Value(r,"private school name"));values.emplace("_internal",Value(r,"internal private value"));auto event=RawAuditEvent::created(r,"School",values);
    for(const LogFormatter* f:{static_cast<const LogFormatter*>(&human),static_cast<const LogFormatter*>(&debug)}){
        auto ordinary=f->format_audit_log(r,event);check(ordinary.find("name")!=String::npos && ordinary.find("private school name")==String::npos && ordinary.find("_internal")==String::npos,"ordinary audit discloses field names, hides business values and internal fields");
        check(f->format_sensitive_audit_log(r,event).find("private school name")!=String::npos,"sensitive audit override retains values for upstream projection to protect");
    }
    ConservativeFormatter custom;check(custom.format_sensitive_audit_log(r,event)==custom.format_audit_log(r,event),"third-party formatter default sensitive method remains conservative");
    SqlLogEntry sql(std::allocator_arg,Allocator(r));sql.sql="raw SQL must not substitute for missing debug projection";sql.elapsed={0,500};auto before=std::time(nullptr);auto text=human.format_sql_log(r,{},sql);auto after=std::time(nullptr);
    check(text.find("[    1µs]")!=String::npos && text.find("[SQL omitted: missing safe projection; NOT REPLAYABLE]")!=String::npos && text.find(sql.sql)==String::npos,"SQL format rounds microseconds and visibly omits missing projection");
    bool utc=false;for(auto stamp:{before,after}){std::tm tm{};gmtime_r(&stamp,&tm);char buf[32];std::strftime(buf,sizeof(buf),"[%Y-%m-%d %H:%M:%S",&tm);if(text.starts_with(buf))utc=true;}check(utc && text.size()>24 && text[20]=='.' && text[24]==']',"human wall-clock prefix is UTC with exactly three fractional digits");
    event.new_values->insert_or_assign(String("id",r),Value(r));check(debug.format_audit_log(r,event).find("id=Null")!=String::npos,"explicit Null in new snapshot wins over old ID");
    auto bounded=detail::truncate_sensitive_sql_log(r,"SELECT '🔐private-value'",11);check(bounded.find("TRUNCATED; NOT EXECUTABLE")!=String::npos && bounded.find("private-value")==String::npos,"byte bound marks partial SQL non-executable without splitting Unicode");
}
static void config_edges(){
    GlobalContext global;auto r=global.memory_resource();setenv("TEAQL_AUDIT_LOG","_full_with_payload",1);setenv("TEAQL_SQL_LOG","_silent",1);setenv("TEAQL_TOOL_LOG"," FULL ",1);setenv("TEAQL_SQL_LOG_TABLES","",1);auto config=LogConfig::load(r);check(config.should_log_sensitive_audit("Order") && !config.should_log_sql("SELECT") && config.tool_level==LogLevel::Full,"level tokens are exact and defaults remain source-specific");
    config.sql_level=LogLevel::Summary;check(config.should_log_sql("anything"),"empty table token matches all SQL");config.sql_tables->clear();check(!config.should_log_sql("anything"),"present empty table vector matches no SQL");
    setenv("TEAQL_AUDIT_LOG_ENTITIES","\x80",1);auto invalid=LogConfig::load(r);check(!invalid.audit_entities,"invalid UTF-8 environment is treated as unavailable like Rust env::var");unsetenv("TEAQL_AUDIT_LOG_ENTITIES");
    setenv("TEAQL_LOG_FORMAT","\x80",1);check(dynamic_cast<const HumanReaderFormatter*>(&global.log_formatter_factory().get_formatter()),"invalid UTF-8 formatter selection uses human default");
    unsetenv("TEAQL_AUDIT_LOG");unsetenv("TEAQL_SQL_LOG");unsetenv("TEAQL_TOOL_LOG");unsetenv("TEAQL_SQL_LOG_TABLES");unsetenv("TEAQL_LOG_FORMAT");
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());factory_and_domains();formats();config_edges();std::pmr::set_default_resource(previous);std::cout<<"Log formatter contracts passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
