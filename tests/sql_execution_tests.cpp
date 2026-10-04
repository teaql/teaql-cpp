#include "sqlite_execution_fixture.hpp"
#include <iostream>
static void sqlite_execution(){
    GlobalContext global;auto& db=global.make_resource<Database>();auto& d=global.make_resource<SqliteDialect>();auto& e=model(global);auto& schema=global.make_resource<Schema>(e);auto& t=global.make<Transport>(db);auto& cache=global.make<SqlExecutorState>();auto& executor=global.make<SqlDataServiceExecutor<String>>(d,t,schema,cache);
    auto user=global.make_user_context();auto r=user->memory_resource();auto& events=user->make<Vector<ExecutionMetadata>>();auto observer=ExecutionObserver(std::allocator_arg,Allocator(r),[&](ExecutionMetadata m){events.emplace_back(std::move(m));});
    auto req=update(r);std::get<UpdateCommand>(req.data).expected_version(1);auto& lazy=executor.mutate_observed(r,std::move(req),observer);check(t.writes==0,"mutation must be lazy");const auto& ordinary=lazy.wait().value();
    check(ordinary.affected_rows==1 && !ordinary.persisted_snapshot && ordinary.generated_values.fields.empty() && t.reads==0 && events.empty(),"ordinary mutation does not invent generated values or readback");
    auto guarded=[&](std::uint64_t tenant,std::int64_t version=2){auto req=update(r);std::get<UpdateCommand>(req.data).expected_version(version);return GuardedMutationRequest(std::allocator_arg,Allocator(r),req,Expr::eq(r,"tenant",tenant));};
    const auto& denied=executor.mutate_guarded_observed(r,guarded(99),observer).wait().value();check(denied.affected_rows==0 && !denied.persisted_snapshot && t.reads==0,"guard is part of UPDATE and prevents readback when no row changed");
    const auto& success=executor.mutate_guarded_observed(r,guarded(2),observer).wait().value();check(success.persisted_snapshot && success.persisted_snapshot->fields.at(String("version",r))==Value(r,std::int64_t(3)) && events.empty(),"guarded readback returns authoritative SQLite version and success journal is discarded");
    t.fail_read=true;auto& failed=executor.mutate_guarded_observed(r,guarded(2,3),observer);check(failed.wait().error().kind()==SqlExecutorError<String>::Kind::Transport,"readback transport error stays typed");
    check(events.size()==2 && events[0].operation==DataServiceOperation::Update && events[0].sql_log.execution_outcome==SqlExecutionOutcome::Success && events[1].operation==DataServiceOperation::Query && events[1].sql_log.execution_outcome==SqlExecutionOutcome::Failure,"readback failure flushes successful write then failed query");
    Vector<String> secrets(r);events[1].sql_log.intent_redactions.extend_secrets(false,secrets);check(std::ranges::find(secrets,String("new",r))!=secrets.end(),"readback inherits mutation redaction intent");t.fail_read=false;
    events.clear();MutationRequest::Batch children(r);children.emplace_back(update(r,"batch first"));InsertCommand bad(std::allocator_arg,Allocator(r),"Unknown");children.emplace_back(std::move(bad));children.emplace_back(update(r,"never executed"));auto before=t.writes;
    auto batch=MutationRequest(std::allocator_arg,Allocator(r),std::move(children));check(executor.mutate_observed(r,std::move(batch),observer).wait().error().kind()==SqlExecutorError<String>::Kind::Compile && t.writes==before+1 && events.size()==1,"later compile failure flushes earlier leaf without starting following leaf");
    // The same kernel supports transaction-owned ordinary readback. The public
    // transaction wrapper is tested separately when that adapter is connected.
    InsertCommand insert(std::allocator_arg,Allocator(r),"Record");insert.value("id",std::uint64_t(8)).value("version",std::int64_t(1)).value("name","inserted");
    auto request=MutationRequest(std::allocator_arg,Allocator(r),std::move(insert));const auto& readback=detail::execute_mutation(r,d,t,schema,cache,std::move(request),true,std::optional<ExecutionObserver>{}).wait().value();
    check(readback.persisted_snapshot && readback.persisted_snapshot->fields.at(String("tenant",r))==Value(r,std::int64_t(2)),"readback includes database defaults, distinct from mutation input");
}
static void close_and_resume(){
    GlobalContext global;auto& db=global.make_resource<Database>();auto& d=global.make_resource<SqliteDialect>();auto& e=model(global);auto& schema=global.make_resource<Schema>(e);auto& t=global.make<Transport>(db);auto& cache=global.make<SqlExecutorState>();auto& executor=global.make<SqlDataServiceExecutor<String>>(d,t,schema,cache);auto& events=global.make<Vector<ExecutionMetadata>>();t.pause_read=true;
    {
        auto user=global.make_user_context();auto r=user->memory_resource();auto observer=ExecutionObserver(std::allocator_arg,Allocator(r),[&](ExecutionMetadata m){check(t.cancelled==1,"close stops pending transport before delivering journal");events.emplace_back(std::move(m));});
        auto req=update(r);auto& pending=executor.mutate_guarded_observed(r,GuardedMutationRequest(std::allocator_arg,Allocator(r),req,Expr::eq(r,"tenant",std::uint64_t(2))),observer);
        check(!pending.poll() && t.writes==1 && t.reads==1 && events.empty(),"success journal waits while readback is pending");
    }
    check(events.size()==2 && events[0].sql_log.execution_outcome==SqlExecutionOutcome::Success && events[1].sql_log.execution_outcome==SqlExecutionOutcome::Cancelled,"Context close emits each statement once before reclamation");
    auto user=global.make_user_context();auto r=user->memory_resource();DataServiceOperationResult<MutationResult,SqlExecutorError<String>>* pending=nullptr;
    {auto source=global.make_user_context();auto sr=source->memory_resource();auto req=update(sr,"copied across source Context");pending=&executor.mutate_guarded(r,GuardedMutationRequest(std::allocator_arg,Allocator(sr),req,Expr::eq(sr,"tenant",std::uint64_t(2))));}
    check(!pending->poll(),"request deep copy survives source close and waits on readback");auto writes=t.writes;t.pause_read=false;
    const auto& result=pending->wait().value();check(t.writes==writes && result.persisted_snapshot,"resuming readback never re-executes the write");
    auto& retained=global.make<MutationResult>(result);user.reset();check(retained.persisted_snapshot->fields.at(String("name",global.memory_resource()))==Value(global.memory_resource(),"copied across source Context"),"explicit result copy preserves snapshot across destination close");
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());sqlite_execution();close_and_resume();std::pmr::set_default_resource(previous);std::cout<<"SQL mutation execution contracts passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
