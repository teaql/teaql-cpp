#include <teaql/runtime/context.hpp>
#include <teaql/runtime/log_manager.hpp>
#include <teaql/runtime/detail/audit_privacy.hpp>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
using namespace teaql;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> static void rejected(F&& f,const char* message){bool failed=false;try{f();}catch(const std::logic_error&){failed=true;}check(failed,message);}
struct Redirect {
    std::FILE* stream;int saved;std::FILE* target;
    explicit Redirect(std::FILE* stream,const char* path=nullptr):stream(stream),saved(::dup(::fileno(stream))),target(path?std::fopen(path,"wb+"):std::tmpfile()){
        if(saved<0 || !target)throw std::runtime_error("capture setup failed");std::fflush(stream);if(::dup2(::fileno(target),::fileno(stream))<0)throw std::runtime_error("redirect failed");std::clearerr(stream);
    }
    ~Redirect(){std::fflush(stream);::dup2(saved,::fileno(stream));::close(saved);std::clearerr(stream);std::fclose(target);}
    std::string read(){std::fflush(stream);std::rewind(target);std::string out;char buf[4096];for(;;){auto n=std::fread(buf,1,sizeof(buf),target);out.append(buf,n);if(n<sizeof(buf))break;}return out;}
};
static std::size_t occurrences(std::string_view text,std::string_view needle){std::size_t result=0,pos=0;while((pos=text.find(needle,pos))!=text.npos){++result;pos+=needle.size();}return result;}
static void reset_env(){for(const char* key:{"TEAQL_LOG_FORMAT","TEAQL_LOG_ENDPOINT","TEAQL_DOMAIN","TEAQL_TRACE_MODE","TEAQL_TRACE_OFF_ACK","TEAQL_AUDIT_LOG","TEAQL_SQL_LOG","TEAQL_TOOL_LOG","TEAQL_AUDIT_LOG_ENTITIES","TEAQL_SQL_LOG_TABLES","TEAQL_TOOL_LOG_FOCUS","TEAQL_SQL_DEBUG_ENDPOINT","TEAQL_AUDIT_DEBUG_ENDPOINT","TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS"})unsetenv(key);setenv("TEAQL_LOG_ENDPOINT","/dev/null",1);setenv("TEAQL_LOG_FORMAT","debug",1);}
static RawAuditEvent event(Memory r){Record values(r);values.emplace("id",Value(r,std::uint64_t(7)));values.emplace("name",Value(r,"private school name for arena allocation"));return RawAuditEvent::created(r,"Order",values);}
static void warning_domains(){
    reset_env();setenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS","I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK",1);Redirect errors(stderr);
    {
        GlobalContext global;auto one=global.make_user_context();auto two=global.make_user_context();auto r=one->memory_resource();Vector<String> masks(r);masks.emplace_back("name");
        (void)build_safe_audit_field(r,"password","secret-token",{},{});(void)build_safe_audit_field(r,"name",{},masks,{});check(errors.read().empty(),"credential short-circuit and absent values do not trigger the source warning");
        (void)build_safe_audit_field(r,"name","school",masks,{});check(detail::audit_plaintext_enabled(two->memory_resource()),"second User sees enabled application policy");
        check(occurrences(errors.read(),"TeaQL WARNING:")==1,"direct fields and Manager share one warning across UserContexts");
        unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");check(!global.log_manager().plaintext_enabled(),"acknowledgement is not cached");setenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS","I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK",1);(void)event(two->memory_resource()).build_safe_event(two->memory_resource(),{},{});check(occurrences(errors.read(),"TeaQL WARNING:")==1,"re-enabled logging does not repeat the application warning");
    }
    {GlobalContext independent;auto user=independent.make_user_context();(void)event(user->memory_resource()).build_safe_event(user->memory_resource(),{},{});}
    check(occurrences(errors.read(),"TeaQL WARNING:")==2,"independent applications do not inherit hidden process warning state");
}
static void domains_and_reclamation(){
    reset_env();GlobalContext global;GlobalContext other;auto user=global.make_user_context();auto r=user->memory_resource();auto& manager=global.log_manager();check(&manager==&global.log_manager(),"one manager per application");
    rejected([&]{user->make<LogManager>(global);},"manager cannot be User-owned");rejected([&]{global.make<LogManager>(other);},"manager cannot retain another application");
    {auto e=event(r);manager.write_audit_log(r,e);rejected([&]{other.log_manager().write_audit_log(r,e);},"output scratch arena must belong to the manager application");}
    user.reset();auto before=global.allocation_stats();
    for(unsigned i=0;i<32;++i){auto request=global.make_user_context();auto memory=request->memory_resource();{auto e=event(memory);e.values.insert_or_assign(String("large",memory),Value(memory,String(65536,'x',memory)));manager.write_audit_log(memory,e);}request->close();check(request->allocation_stats().live_blocks==0,"all request blocks reclaimed after synchronous log output");}
    auto after=global.allocation_stats();check(after.live_bytes==before.live_bytes && after.total_blocks==before.total_blocks,"Global manager retains configuration only, not per-request payloads or temporary projections");
    auto closed=global.make_user_context();auto* arena=closed->memory_resource();closed->close();auto e=event(global.memory_resource());rejected([&]{manager.write_audit_log(arena,e);},"closed output arena is rejected before policy checks");
}
static void quiescence(){
    reset_env();bool user_done=false,global_done=false;Arena::Quiescence global_stop;
    struct Evidence {GlobalContext* global;Memory memory;bool* done;};Evidence root{};
    {
        GlobalContext global;auto user=global.make_user_context();Evidence child{&global,user->memory_resource(),&user_done};Arena::Quiescence user_stop;
        auto callback=[](void* raw) noexcept {auto& e=*static_cast<Evidence*>(raw);try{auto input=event(e.memory);e.global->log_manager().write_audit_log(e.memory,input);*e.done=&detail::application_context(e.memory)==e.global;}catch(...){*e.done=false;}};
        user_stop.attach(*dynamic_cast<Arena*>(child.memory),&child,callback);user.reset();check(user_done,"User quiescence can resolve its application and finish diagnostics before reclamation");
        root={&global,global.memory_resource(),&global_done};global_stop.attach(*dynamic_cast<Arena*>(root.memory),&root,callback);
    }
    check(global_done,"Global closes its runtime arena while application state is still alive");
}
static void poison_and_fail_open(){
    reset_env();setenv("TEAQL_LOG_ENDPOINT","stdout",1);GlobalContext global;auto user=global.make_user_context();auto r=user->memory_resource();auto e=event(r);auto& manager=global.log_manager();
    {Redirect full(stdout,"/dev/full");bool failed=false;try{manager.write_audit_log(r,e);}catch(const std::runtime_error&){failed=true;}check(failed,"stdout write failure is surfaced");}
    bool poisoned=false;try{manager.write_audit_log(r,e);}catch(const std::runtime_error&){poisoned=true;}check(poisoned,"failed stdout header poisons initialization as Rust Once does");
    reset_env();setenv("TEAQL_LOG_ENDPOINT","/dev/full",1);GlobalContext file;auto request=file.make_user_context();file.log_manager().write_audit_log(request->memory_resource(),e);file.log_manager().write_audit_log(request->memory_resource(),e);
    setenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS","I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK",1);GlobalContext warning;{Redirect full(stderr,"/dev/full");bool failed=false;try{warning.log_manager().plaintext_enabled();}catch(const std::runtime_error&){failed=true;}check(failed,"stderr warning failure is surfaced");}
    poisoned=false;try{warning.log_manager().plaintext_enabled();}catch(const std::runtime_error&){poisoned=true;}check(poisoned,"warning Once retains poisoned state");
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());warning_domains();domains_and_reclamation();quiescence();poison_and_fail_open();reset_env();std::pmr::set_default_resource(previous);std::cout<<"Log manager contracts passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
