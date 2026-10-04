#include <teaql/runtime/detail/relation_execution.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>
using namespace teaql;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static void sqlite_relation_queries(bool capture){
 GlobalContext g;Vector<CompactRow>* retained=nullptr;
 {auto c=g.make_user_context();auto r=c->memory_resource();c->disable_sql_log();if(capture)c->enable_select_sql_log();auto& connection=c->make_resource<SqliteConnection>();auto& provider=c->make<SqliteMutationExecutor>(connection);auto& sql=c->make<CompiledQuery>();
 sql.sql="CREATE TABLE children(child_id INTEGER PRIMARY KEY, owner_id INTEGER, score INTEGER)";check(bool(provider.execute(r,sql)),"create relation fixture");sql.sql="INSERT INTO children VALUES(11,1,9),(12,1,8),(13,1,8),(21,2,9),(22,2,7),(23,2,6)";check(bool(provider.execute(r,sql)),"insert relation fixture");
 auto& metadata=c->make<InMemoryMetadataStore>();EntityDescriptor child(std::allocator_arg,Allocator(r),"Child");child.table("children");child.property("child_id",DataType::U64).is_id=true;child.property("owner_id",DataType::U64);child.property("score",DataType::I64);metadata.register_entity(std::move(child));c->set_metadata(*c,metadata);
 auto& dialect=c->make_resource<SqliteDialect>();auto& state=c->make<SqlExecutorState>();auto& executor=c->make<SqlDataServiceExecutor<MutationExecutorError>>(dialect,provider,metadata,state);using E=std::remove_reference_t<decltype(executor)>;ResourceBorrow<E> borrow(std::allocator_arg,Allocator(r),executor,detail::resource_arena(r));auto& service=c->make<detail::ContextDataService<E>>(*c,borrow);
 RelationDescriptor relation(std::allocator_arg,Allocator(r),"children","Child");relation.local_key="id";relation.foreign_key="owner_id";relation.many=true;RelationLoadPlan plan(std::allocator_arg,Allocator(r),"Root",relation);SelectQuery q(std::allocator_arg,Allocator(r),"Child");q.project("child_id").project("score").order_desc("score").offset(1).limit(1);plan.query.emplace(std::move(q));
 Vector<CompactRow> parents(r);for(auto id:{2,1,1}){Record record(r);record.emplace(String("id",r),Value(r,std::uint64_t(id)));parents.emplace_back(record);}parents.emplace_back();
 auto& seen=c->make<Vector<SelectQuery>>();detail::RelationFetch<E> fetch(std::allocator_arg,Allocator(r),[&](SelectQuery query)->auto&{seen.emplace_back(query);return service.fetch_all(query);});
 auto& result=detail::fetch_relation_rows(service,plan,parents,true,fetch);parents.clear();plan.query->entity="modified caller";
 check(result.wait() && result.wait().value().size()==2,"relation execution retains its own parent and plan snapshots");
 std::pmr::map<std::uint64_t,std::uint64_t> selected(r);for(const auto& row:result.wait().value())selected.emplace(*row.get("owner_id")->try_u64(),*row.get("child_id")->try_u64());check(selected.at(1)==12 && selected.at(2)==22,"real SQLite per-parent offset, limit and ID tie-breaker");
 check(seen.size()==(capture?2:1),"provider batches semantic partition query only with metadata disabled");for(const auto& query:seen)check(query.partition_field.has_value()!=capture,"partition survives batch route and is cleared for one-parent probes");
 retained=&g.make<Vector<CompactRow>>(result.wait().value());
 }check(retained->size()==2 && retained->front().memory_resource()==g.memory_resource(),"relation result can be explicitly retained after User destruction");
}
struct EmptyExecutor:QueryExecutor<String>,MutationExecutor<String>{
 using allocator_type=Allocator;Memory r;mutable unsigned calls=0;EmptyExecutor(std::allocator_arg_t,Allocator a):r(a.resource()){}Memory memory_resource() const{return r;}
 DataServiceCapabilities capabilities() const override{++calls;return {};}
 DataServiceOperationResult<QueryResult,String>& query(Memory,QueryRequest) override{throw std::logic_error("unused");}DataServiceOperationResult<MutationResult,String>& mutate(Memory,MutationRequest) override{throw std::logic_error("unused");}
};
struct Pending {
 using allocator_type=Allocator;bool& cancelled;WakeSignal*& wake;
 Pending(std::allocator_arg_t,Allocator,bool& cancelled,WakeSignal*& wake):cancelled(cancelled),wake(wake){}
 std::optional<detail::RelationRowsResult<EmptyExecutor>> poll(WakeSignal& signal){wake=&signal;return {};}
 void finish_and_wait() noexcept{}void cancel_and_wait() noexcept{cancelled=true;wake=nullptr;}
};
static void empty_cancel_and_domains(){
 GlobalContext g;auto c=g.make_user_context();auto other=g.make_user_context();auto r=c->memory_resource();auto& e=c->make<EmptyExecutor>();ResourceBorrow<EmptyExecutor> borrow(std::allocator_arg,Allocator(r),e,detail::resource_arena(r));auto& service=c->make<detail::ContextDataService<EmptyExecutor>>(*c,borrow);auto& relation=c->make<RelationDescriptor>("children","Child");relation.local_key="id";auto& plan=c->make<RelationLoadPlan>("Root",relation);unsigned calls=0;bool cancelled=false;WakeSignal* wake=nullptr;
 detail::RelationFetch<EmptyExecutor> fetch(std::allocator_arg,Allocator(r),[&](SelectQuery)->auto&{++calls;return make_operation<detail::RelationRowsResult<EmptyExecutor>,Pending>(r,cancelled,wake);});
 auto& empty=detail::fetch_relation_rows(service,plan,{},false,fetch);check(empty.wait() && empty.wait().value().empty() && calls==0 && e.calls==0,"empty identity set performs no capability lookup or fetch");
 auto& parents=c->make<Vector<CompactRow>>();Record record(r);record.emplace(String("id",r),Value(r,std::uint64_t(1)));parents.emplace_back(record);record.clear();auto& pending=detail::fetch_relation_rows(service,plan,parents,false,fetch);check(!pending.poll() && wake,"child relation query suspends");auto ticket=pending.wake_signal().ticket();wake->notify();check(pending.wake_signal().ticket()!=ticket,"relation child wake propagates");pending.cancel();check(cancelled,"explicit cancel stops relation child");
 cancelled=false;auto& foreign=make_operation<detail::RelationRowsResult<EmptyExecutor>,Pending>(*other,cancelled,wake);detail::RelationFetch<EmptyExecutor> bad(std::allocator_arg,Allocator(r),[&](SelectQuery)->auto&{return foreign;});auto& invalid=detail::fetch_relation_rows(service,plan,parents,false,bad);bool refused=false;try{invalid.poll();}catch(const std::invalid_argument&){refused=true;}check(refused && !cancelled,"foreign result is rejected without cancelling unrelated work");foreign.cancel();
 cancelled=false;auto& closing=detail::fetch_relation_rows(service,plan,parents,false,fetch);check(!closing.poll(),"second relation fetch pending");c->close();check(cancelled && wake==nullptr && c->allocation_stats().live_blocks==0,"Context close stops relation child before returning blocks");
}
int main(){try{setenv("TEAQL_LOG_ENDPOINT","off",1);auto* old=std::pmr::set_default_resource(std::pmr::null_memory_resource());sqlite_relation_queries(false);sqlite_relation_queries(true);empty_cancel_and_domains();std::pmr::set_default_resource(old);std::cout<<"Relation execution and SQLite contracts passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
