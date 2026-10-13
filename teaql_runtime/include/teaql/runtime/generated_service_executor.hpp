#pragma once
#include <teaql/data_service/executor.hpp>

namespace teaql {
// The frozen producer's application executor keeps default query/mutation
// observation and intent behavior while forwarding SQL work and stream/Tx SPIs.
// Inner is the existing SQL executor; generated domains provide a distinct type.
template <class Inner>
class GeneratedServiceRuntimeExecutor : public Inner {
public:
    using Error = typename Inner::Error;
    using allocator_type = Allocator;
    using Inner::Inner;

    GeneratedServiceRuntimeExecutor(std::allocator_arg_t,
                                    Allocator a,
                                    const GeneratedServiceRuntimeExecutor& other)
        : Inner(std::allocator_arg, a, static_cast<const Inner&>(other)) {}

    SqlIntentRedactions query_log_intent(Memory r, const SelectQuery& query) const override {
        return QueryExecutor<Error>::query_log_intent(r, query);
    }

    DataServiceOperationResult<QueryResult, Error>& query(Memory r, QueryRequest request) override {
        // Qualify the observed implementation: Inner::query normally dispatches
        // virtually to query_observed and would re-enter this default adapter.
        return Inner::query_observed(r, std::move(request), std::nullopt);
    }

    DataServiceOperationResult<QueryResult, Error>&
    query_observed(Memory r, QueryRequest request, std::optional<ExecutionObserver>) override {
        return query(r, std::move(request));
    }

    DataServiceOperationResult<MutationResult, Error>& mutate(Memory r,
                                                              MutationRequest request) override {
        return Inner::mutate_observed(r, std::move(request), std::nullopt);
    }

    DataServiceOperationResult<MutationResult, Error>&
    mutate_observed(Memory r, MutationRequest request, std::optional<ExecutionObserver>) override {
        return mutate(r, std::move(request));
    }
};
}
