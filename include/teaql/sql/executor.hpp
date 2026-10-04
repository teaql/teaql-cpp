#pragma once
#include <teaql/sql/error.hpp>
#include <teaql/sql/transport.hpp>
#include <teaql/sql/detail/query_plan.hpp>
#include <teaql/sql/detail/diagnostic.hpp>
#include <teaql/sql/detail/mutation_execution.hpp>
#include <teaql/sql/detail/query_execution.hpp>
#include <teaql/sql/transaction.hpp>

namespace teaql {
// Explicitly allocate this shared state in GlobalContext or UserContext. Executor
// copies in that scope borrow it; no shared owner can prolong the scope.
using SqlExecutorState=detail::SqlExecutorState;
// Provider-independent execution. Transaction and stream adapters remain separate.
template<class E,class T=SqlTransport<E>> class SqlDataServiceExecutor : public QueryExecutor<SqlExecutorError<E>>,public GuardedMutationExecutor<SqlExecutorError<E>>,public detail::SqlTransactionExecutorSupport<E,T>,public detail::SqlStreamExecutorSupport<E,T> {
public:
    using allocator_type=Allocator;
    const SqlDialect& dialect;
    T& transport;
    const SchemaProvider& schema_provider;
    SqlExecutorState& state;
    SqlDataServiceExecutor(std::allocator_arg_t,Allocator a,const SqlDialect& dialect,T& transport,const SchemaProvider& schema,SqlExecutorState& state)
        :detail::SqlTransactionExecutorSupport<E,T>(dialect,transport,schema,state),detail::SqlStreamExecutorSupport<E,T>(dialect,transport,schema,state),dialect(dialect),transport(transport),schema_provider(schema),state(state) {
        if(a.resource()!=state.memory_resource())throw std::invalid_argument("Executor and cache state must share an owning Context");
    }
    SqlDataServiceExecutor(std::allocator_arg_t,Allocator a,const SqlDataServiceExecutor& other)
        :SqlDataServiceExecutor(std::allocator_arg,a,other.dialect,other.transport,other.schema_provider,other.state) {}
    Memory memory_resource() const{return state.memory_resource();}
    DataServiceCapabilities capabilities() const override{DataServiceCapabilities c;c.query=true;c.mutation=true;c.batch_mutation=true;c.small_parent_relation_probes=dialect.prefers_small_parent_relation_probes();return c;}
    SqlIntentRedactions query_log_intent(Memory r,const SelectQuery& query) const override {
        if(auto* entity=state.entity_descriptor(schema_provider,query.entity))try{auto compiled=state.compile_select_cached(r,dialect,*entity,query);return SqlIntentRedactions::from_bindings(r,compiled.log_context,compiled.params,compiled.sql);}catch(const SqlCompileError&){}
        return SqlIntentRedactions::from_unclassified_query(r,query);
    }
    DataServiceOperationResult<MutationResult,SqlExecutorError<E>>& mutate(Memory r,MutationRequest request) override{return mutate_observed(r,std::move(request),{});}
    DataServiceOperationResult<MutationResult,SqlExecutorError<E>>& mutate_observed(Memory r,MutationRequest request,std::optional<ExecutionObserver> observer) override {
        return detail::execute_mutation(r,dialect,transport,schema_provider,state,std::move(request),false,std::move(observer));
    }
    DataServiceOperationResult<MutationResult,SqlExecutorError<E>>& mutate_guarded(Memory r,GuardedMutationRequest request) override{return mutate_guarded_observed(r,std::move(request),{});}
    DataServiceOperationResult<MutationResult,SqlExecutorError<E>>& mutate_guarded_observed(Memory r,GuardedMutationRequest request,std::optional<ExecutionObserver> observer) override {
        return detail::execute_mutation(r,dialect,transport,schema_provider,state,std::move(request.mutation),true,std::move(observer),std::optional<Expr>(std::move(request.guard)));
    }
    DataServiceOperationResult<QueryResult,SqlExecutorError<E>>& query(Memory r,QueryRequest request) override{return query_observed(r,std::move(request),{});}
    DataServiceOperationResult<QueryResult,SqlExecutorError<E>>& query_observed(Memory r,QueryRequest request,std::optional<ExecutionObserver> observer) override {
        using State=detail::SqlQueryOperation<E>;
        return make_operation<typename State::Output,State>(r,dialect,transport,schema_provider,state,std::move(request),std::move(observer),true);
    }
};
}
