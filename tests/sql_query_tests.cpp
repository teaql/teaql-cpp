#include <teaql/sql/executor.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
enum class Error{Fetch};
struct Dialect final:SqlDialect {
    mutable unsigned compiles=0;bool probes=true;
    DatabaseKind kind() const override{return DatabaseKind::Sqlite;}
    String quote_ident(Memory r,std::string_view s) const override{return quote_identifier_if_needed(r,s,'"');}
    String placeholder(Memory r,std::size_t) const override{return String("?",r);}
    bool prefers_small_parent_relation_probes() const override{return probes;}
    CompiledQuery compile_select(Memory r,const EntityDescriptor& e,const SelectQuery& q) const override{++compiles;return SqlDialect::compile_select(r,e,q);}
};
struct Schema final:SchemaProvider {
    const EntityDescriptor& entity;mutable unsigned lookups=0;
    explicit Schema(const EntityDescriptor& entity):entity(entity) {}
    const EntityDescriptor* get_entity(std::string_view name) const override{++lookups;return name==entity.name?&entity:nullptr;}
};
struct Transport final:SqlTransport<Error> {
    using allocator_type=Allocator;enum Mode{Success,Failure,Panic,Pending};Mode mode=Success;
    unsigned calls=0,repeats=0,finished=0,cancelled=0;std::size_t probe_index=0;Vector<CompiledQuery> seen;
    Transport(std::allocator_arg_t,Allocator a):seen(a.resource()) {}
    struct Fetch {
        using allocator_type=Allocator;Memory resource;Transport& owner;const CompiledQuery& query;Vector<Value> values;Mode mode;
        Fetch(std::allocator_arg_t,Allocator a,Transport& owner,const CompiledQuery& query,std::span<const Value> values):resource(a.resource()),owner(owner),query(query),values(values.begin(),values.end(),a.resource()),mode(owner.mode) {}
        std::optional<Result<Rows,Error>> poll(WakeSignal&){
            if(mode==Pending)return {};
            if(mode==Panic)throw std::runtime_error("PRIVATE-PROVIDER-ERROR");
            if(mode==Failure)return Result<Rows,Error>::err(resource,Error::Fetch);
            Rows rows(resource);if(values.empty()){rows.emplace_back();if(!query.params.empty())rows.back().insert("value",query.params[0]);}
            else for(const auto& v:values){rows.emplace_back();rows.back().insert("value",v);}
            return Result<Rows,Error>::ok(resource,std::move(rows));
        }
        void finish_and_wait() noexcept{++owner.finished;}
        void cancel_and_wait() noexcept{++owner.cancelled;}
    };
    DataServiceOperationResult<Rows,Error>& fetch_all_compact_sql(Memory r,const CompiledQuery& query) override{++calls;seen.emplace_back(query);return make_operation<Result<Rows,Error>,Fetch>(r,*this,query,std::span<const Value>{});}
    DataServiceOperationResult<Rows,Error>& fetch_repeated_compact_sql(Memory r,const CompiledQuery& query,std::size_t index,std::span<const Value> values) override{++repeats;probe_index=index;seen.emplace_back(query);return make_operation<Result<Rows,Error>,Fetch>(r,*this,query,values);}
    DataServiceOperationResult<std::uint64_t,Error>& execute_sql(Memory,const CompiledQuery&) override{throw std::logic_error("unused test method");}
};
static EntityDescriptor& model(GlobalContext& global){auto& e=global.make<EntityDescriptor>("Record");e.property("id",DataType::U64).id();e.property("name",DataType::Text);e.mask_fields({"name"});return e;}
static void execution_and_diagnostics(){
    GlobalContext global;auto user=global.make_user_context();auto r=user->memory_resource();auto& e=model(global);auto& d=global.make_resource<Dialect>();auto& schema=global.make_resource<Schema>(e);auto& transport=global.make<Transport>();auto& cache=global.make<SqlExecutorState>();auto& executor=global.make<SqlDataServiceExecutor<Error>>(d,transport,schema,cache);
    auto& query=user->make<SelectQuery>("Record");query.filter(Expr::eq(r,"name","PRIVATE-VALUE")).comment("query comment");auto request=[&](bool metadata=true){QueryRequest req(std::allocator_arg,Allocator(r),query,true,metadata);req.comment.emplace("request comment",r);return req;};
    auto& observed=user->make<Vector<ExecutionMetadata>>();auto observer=ExecutionObserver(std::allocator_arg,Allocator(r),[&](ExecutionMetadata m){check(transport.finished+transport.cancelled>0,"provider released before sink");observed.emplace_back(std::move(m));});
    auto& operation=executor.query_observed(r,request(),observer);check(schema.lookups==0 && transport.calls==0,"query is lazy before first poll");
    const auto& result=operation.wait().value();check(observed.empty() && result.metadata.sql_log.execution_outcome==SqlExecutionOutcome::Success && result.metadata.result_count==1,"success metadata is returned without observer duplication");
    check(result.metadata.comment=="request comment" && transport.seen[0].comment=="query comment" && !result.metadata.debug_query && result.metadata.parameterized_query->find("PRIVATE-VALUE")==String::npos,"request and compiled comments stay separate and debug does not expand private values");
    check(result.metadata.sql_log.parameter_policies[0]==SqlParameterLogPolicy::Masked,"compiler provenance survives executor metadata");
    auto& clone=global.make<SqlDataServiceExecutor<Error>>(executor);query.filter(Expr::eq(r,"name","replacement"));const auto& cached=clone.query(r,request()).wait().value();check(schema.lookups==1 && d.compiles==1 && cached.metadata.params[0]==Value(r,"replacement"),"executor clones borrow shared descriptor/plan caches and rebind current literals");
    const auto& unrecorded=executor.query(r,request(false)).wait().value();check(unrecorded.metadata.backend.empty() && unrecorded.metadata.started_at==ExecutionMetadata::SystemTime{} && unrecorded.metadata.params.empty(),"metadata opt-out returns count only");
    transport.mode=Transport::Failure;check(executor.query_observed(r,request(),observer).wait().error().kind()==SqlExecutorError<Error>::Kind::Transport,"typed transport failures remain transport errors");
    check(observed.size()==1 && observed[0].sql_log.execution_outcome==SqlExecutionOutcome::Failure && !observed[0].result_count,"failure observer gets one statement without fabricated row count");
    transport.mode=Transport::Panic;bool threw=false;try{executor.query_observed(r,request(),observer).wait();}catch(const std::runtime_error&){threw=true;}check(threw && observed.size()==2 && observed[1].sql_log.execution_outcome==SqlExecutionOutcome::Failure,"unexpected provider unwind reports failure then preserves exception");
    transport.mode=Transport::Pending;auto& pending=executor.query_observed(r,request(),observer);check(!pending.poll(),"query waits without blocking");pending.cancel();check(observed.size()==3 && observed[2].sql_log.execution_outcome==SqlExecutionOutcome::Cancelled,"explicit abandonment reports cancellation exactly once");
    query.project("missing");auto& invalid=executor.query_observed(r,request(),observer);check(invalid.wait().error().kind()==SqlExecutorError<Error>::Kind::Compile && observed.size()==3,"compile errors happen before statement diagnostic creation");
    query.projection.clear();auto intent=executor.query_log_intent(r,query);Vector<String> secrets(r);intent.extend_secrets(false,secrets);check(secrets.size()==1 && secrets[0]=="replacement","intent compiles provenance without transport I/O");
    check(executor.capabilities().query && executor.capabilities().mutation && executor.capabilities().batch_mutation,"query and mutation capabilities are advertised");
}
static void probes_and_cache(){
    GlobalContext global;auto user=global.make_user_context();auto r=user->memory_resource();auto& e=model(global);auto& d=global.make_resource<Dialect>();auto& schema=global.make_resource<Schema>(e);auto& transport=global.make<Transport>();auto& cache=global.make<SqlExecutorState>();auto& executor=global.make<SqlDataServiceExecutor<Error>>(d,transport,schema,cache);
    auto& q=user->make<SelectQuery>("Record");Vector<Value> values(r);values.emplace_back(7);values.emplace_back(9);q.filter(Expr::in_list(r,"id",values)).partition_by("id").limit(1).order_desc("id");
    auto request=[&](bool capture){return QueryRequest(std::allocator_arg,Allocator(r),q,false,capture);};
    check(executor.query(r,request(false)).wait().value().rows.size()==2 && transport.repeats==1 && transport.calls==0,"unrecorded SQLite Top-N uses one repeated transport boundary");
    executor.query(r,request(true)).wait();check(transport.calls==1,"metadata capture keeps partition/window query observable");
    q.top_n_probe_parent_threshold_value=0;executor.query(r,request(false)).wait();check(transport.calls==2,"explicit zero threshold disables implicit repeated-probe shortcut");
    auto& state=global.make<SqlExecutorState>();auto simple=SelectQuery(std::allocator_arg,Allocator(r),"Record");simple.filter(Expr::eq(r,"name","first"));auto before=d.compiles;(void)state.compile_select_cached(r,d,e,simple);simple.filter(Expr::eq(r,"name","second"));(void)state.compile_select_cached(r,d,e,simple);check(d.compiles==before+1,"normalized shape retains cache hit");
    auto public_model=EntityDescriptor(std::allocator_arg,Allocator(r),e);public_model.mask_fields({});auto compiled=state.compile_select_cached(r,d,public_model,simple);check(compiled.log_context.parameter_policies[0]==SqlParameterLogPolicy::Plain && d.compiles==before+2,"descriptor changes invalidate cached provenance");
    auto& bounded=global.make<SqlExecutorState>();SelectQuery raw(std::allocator_arg,Allocator(r),"Record");for(unsigned i=0;i<257;++i){raw.slice=Slice{i,0};(void)bounded.compile_select_cached(r,d,e,raw);}check(bounded.cached_plans()==256,"plan cache uses source FIFO bound");before=d.compiles;raw.slice=Slice{0,0};(void)bounded.compile_select_cached(r,d,e,raw);check(d.compiles==before+1,"oldest plan is evicted at capacity");
}
static void close_and_journal(){
    GlobalContext global;auto& delivered=global.make<Vector<ExecutionMetadata>>();unsigned provider_cancelled=0;
    {auto user=global.make_user_context();auto r=user->memory_resource();auto observer=ExecutionObserver(std::allocator_arg,Allocator(r),[&](ExecutionMetadata m){delivered.emplace_back(std::move(m));});
        auto& journal=user->make<detail::FailureJournal>(observer);auto recorder=journal.recorder();
        ExecutionMetadata one(std::allocator_arg,Allocator(r),DataServiceOperation::Insert);one.result_count=1;(*recorder)(std::move(one));
        ExecutionMetadata two(std::allocator_arg,Allocator(r),DataServiceOperation::Update);two.result_count=2;(*recorder)(std::move(two));
    }
    check(delivered.size()==2 && delivered[0].result_count==1 && delivered[1].result_count==2,"journal flushes entries in execution order before context objects are destroyed");
    {auto user=global.make_user_context();auto r=user->memory_resource();auto observer=ExecutionObserver(std::allocator_arg,Allocator(r),[&](ExecutionMetadata){throw 1;});auto& journal=user->make<detail::FailureJournal>(observer);auto recorder=journal.recorder();ExecutionMetadata m(std::allocator_arg,Allocator(r),DataServiceOperation::Query);(*recorder)(std::move(m));journal.flush();journal.flush();}
    {auto user=global.make_user_context();auto r=user->memory_resource();auto observer=ExecutionObserver(std::allocator_arg,Allocator(r),[&](ExecutionMetadata m){delivered.emplace_back(std::move(m));});auto& journal=user->make<detail::FailureJournal>(observer);auto recorder=journal.recorder();ExecutionMetadata m(std::allocator_arg,Allocator(r),DataServiceOperation::Query);(*recorder)(std::move(m));journal.disarm();}
    check(delivered.size()==2,"disarmed success journal does not duplicate metadata");
    auto& e=model(global);auto& d=global.make_resource<Dialect>();auto& schema=global.make_resource<Schema>(e);auto& transport=global.make<Transport>();transport.mode=Transport::Pending;auto& cache=global.make<SqlExecutorState>();auto& executor=global.make<SqlDataServiceExecutor<Error>>(d,transport,schema,cache);
    {auto user=global.make_user_context();auto r=user->memory_resource();auto& q=user->make<SelectQuery>("Record");auto observer=ExecutionObserver(std::allocator_arg,Allocator(r),[&](ExecutionMetadata m){check(transport.cancelled==1,"close joins provider before diagnostic sink");++provider_cancelled;delivered.emplace_back(std::move(m));});auto& operation=executor.query_observed(r,QueryRequest(std::allocator_arg,Allocator(r),q,false,true),observer);check(!operation.poll(),"context-close operation pending");}
    check(provider_cancelled==1 && delivered.back().sql_log.execution_outcome==SqlExecutionOutcome::Cancelled,"context close emits one cancellation while observers are alive");
}
static void cross_context_cache_and_request(){
    GlobalContext global;
    auto& e=model(global);auto& dialect=global.make_resource<Dialect>();
    auto& schema=global.make_resource<Schema>(e);auto& transport=global.make<Transport>();
    auto& cache=global.make<SqlExecutorState>();
    auto& executor=global.make<SqlDataServiceExecutor<Error>>(dialect,transport,schema,cache);
    auto target=global.make_user_context();auto r=target->memory_resource();
    DataServiceOperationResult<QueryResult,SqlExecutorError<Error>>* pending=nullptr;
    {
        auto source=global.make_user_context();auto sr=source->memory_resource();
        auto& q=source->make<SelectQuery>("Record");
        q.filter(Expr::eq(sr,"name","a long private value owned by the source context"));
        QueryRequest request(std::allocator_arg,Allocator(sr),q,false,true);
        request.trace_chain.emplace_back(TraceKind::Request,"Record","a long trace owned by the source context");
        pending=&executor.query(r,std::move(request));
    }
    const auto& result=pending->wait().value();
    check(result.metadata.params[0]==Value(r,"a long private value owned by the source context") && result.metadata.trace_chain.size()==1,"request owns an explicit deep copy after source Context closes");
    auto& retained=global.make<QueryResult>(result);
    target.reset();
    auto next=global.make_user_context();auto nr=next->memory_resource();
    auto& q=next->make<SelectQuery>("Record");q.filter(Expr::eq(nr,"name","next context value"));
    const auto& again=executor.query(nr,QueryRequest(std::allocator_arg,Allocator(nr),q,false,true)).wait().value();
    check(dialect.compiles==1 && schema.lookups==1 && again.metadata.params[0]==Value(nr,"next context value"),"global cache keeps no query/descriptor borrow from the previous UserContext");
    check(retained.metadata.trace_chain[0].comment=="a long trace owned by the source context" && retained.rows[0].get("value"),"explicit global result copy survives both source scopes");
    bool rejected=false;try{next->make<SqlDataServiceExecutor<Error>>(executor);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"executor copies cannot implicitly retain another Context's cache");
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());execution_and_diagnostics();probes_and_cache();close_and_journal();cross_context_cache_and_request();std::pmr::set_default_resource(previous);std::cout<<"SQL query executor contracts passed\n";}catch(const SqlCompileError& e){std::cerr<<"unexpected compile error "<<unsigned(e.code)<<'\n';return 1;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
