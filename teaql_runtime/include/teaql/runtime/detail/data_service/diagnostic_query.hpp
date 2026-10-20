#pragma once
#include <teaql/runtime/detail/data_service/entity_service.hpp>

namespace teaql::detail {
template <class Value, class E>
class DiagnosticQueryOperation {
public:
    using allocator_type = Allocator;
    using Output = Result<Value, DataServiceError<typename E::Error>>;
    using Operation = AsyncOperation<Output>;
    using Execute = Operation& (*)(const EntityDataService<E>&,
                                   const SelectQuery&,
                                   std::span<const RelationAggregate>);

    DiagnosticQueryOperation(std::allocator_arg_t,
                             Allocator a,
                             const EntityDataService<E>& service,
                             const SelectQuery& query,
                             const SelectQuery& source,
                             std::span<const RelationAggregate> aggregates,
                             Execute execute)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query),
          source_(std::allocator_arg, a, source),
          aggregates_(aggregates.begin(), aggregates.end(), a.resource()), execute_(execute) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_) {
            auto scoped = EntityDataServiceAccess::query_scoped(service_, source_);
            if (!scoped) {
                return Output::err(
                    r, DataServiceError<typename E::Error>::runtime(r, std::move(scoped).error()));
            }
            service_ = std::move(scoped).value();
            child_ = &execute_(service_, query_, aggregates_);
        }
        forwarding_.attach(child_->wake_signal(), wake);
        if (!child_->poll()) {
            return {};
        }
        forwarding_.reset();
        return child_->take_result();
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
        }
    }

private:
    EntityDataService<E> service_;
    SelectQuery query_, source_;
    Vector<RelationAggregate> aggregates_;
    Execute execute_;
    Operation* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class Value, class E>
auto& execute_scoped_purposed_query(const EntityDataService<E>& service,
                                    const PurposedSelectQuery& query,
                                    std::span<const RelationAggregate> aggregates,
                                    typename DiagnosticQueryOperation<Value, E>::Execute execute) {
    using State = DiagnosticQueryOperation<Value, E>;
    auto r = service.memory_resource();
    auto* diagnostic = EntityDataServiceAccess::diagnostic_source(query);
    auto source = clone_in(r, diagnostic ? *diagnostic : query.as_query());
    const auto& context_service = EntityDataServiceAccess::context_service(service);
    if (context_service.metadata.capture_execution_metadata()) {
        // Separate aggregate arguments also contribute ancestry before root SQL.
        // They are diagnostic children, not extra executable relations.
        for (const auto& aggregate : aggregates) {
            source.child_enhancements.emplace_back(aggregate.query);
        }
    }
    return make_operation<typename State::Output, State>(
        r, service, query.as_query(), source, aggregates, execute);
}

template <class Value, class E>
auto& execute_purposed_query(const EntityDataService<E>& service,
                             const PurposedSelectQuery& query,
                             std::span<const RelationAggregate> aggregates,
                             typename DiagnosticQueryOperation<Value, E>::Execute execute) {
    using State = DiagnosticQueryOperation<Value, E>;
    if (auto* source = EntityDataServiceAccess::diagnostic_source(query)) {
        return make_operation<typename State::Output, State>(
            service.memory_resource(), service, query.as_query(), *source, aggregates, execute);
    }
    return execute(service, query.as_query(), aggregates);
}
}
