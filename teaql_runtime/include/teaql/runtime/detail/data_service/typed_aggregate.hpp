#pragma once
#include <teaql/runtime/detail/data_service/entity_query.hpp>

namespace teaql::detail {
// Retained crate-internal typed aggregation, with a fresh decoding root and
// independent row enhancement. It does not enter enhanced-entity ID paging.
template <class E, Entity T>
class TypedAggregateOperation {
    using Error = typename E::Error;
    using R = Result<SmartList<T>, DataServiceError<Error>>;

public:
    using allocator_type = Allocator;

    TypedAggregateOperation(std::allocator_arg_t,
                            Allocator allocator,
                            const EntityDataService<E>& service,
                            const SelectQuery& query,
                            std::span<const RelationAggregate> aggregates)
        : service_(std::allocator_arg, allocator, service),
          query_(std::allocator_arg, allocator, query),
          aggregates_(aggregates.begin(), aggregates.end(), allocator.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto resource = service_.memory_resource();
        if (!child_) {
            const auto& context = EntityDataServiceAccess::context(service_);
            ensure_identity_projection(context, query_);
            root_ = &context.template make<EntityRuntimeState>();
            child_ = &make_operation<RelationRowsResult<E>, EntityRowsOperation<E>>(
                resource, service_, query_, std::span<const RelationAggregate>(aggregates_), true);
        }
        forwarding_.attach(child_->wake_signal(), wake);
        const auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(resource, copy_or_take_error(*child_, *result));
        }
        SmartList<T> list(std::allocator_arg, Allocator(resource));
        list.data.reserve(result->value().size());
        for (const auto& row : result->value()) {
            auto decoded =
                T::from_compact_row_with_context(resource, clone_in(resource, row), AnyRef(*root_));
            if (!decoded) {
                return R::err(
                    resource,
                    DataServiceError<Error>::entity(resource, std::move(decoded).error()));
            }
            list.data.emplace_back(std::move(decoded).value());
        }
        list.loaded();
        return R::ok(resource, std::move(list));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        child_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
    }

private:
    EntityDataService<E> service_;
    SelectQuery query_;
    Vector<RelationAggregate> aggregates_;
    EntityRuntimeState* root_ = nullptr;
    AsyncOperation<RelationRowsResult<E>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E, Entity T>
AsyncOperation<Result<SmartList<T>, DataServiceError<typename E::Error>>>&
fetch_typed_aggregate_entities(const EntityDataService<E>& service,
                               const SelectQuery& query,
                               std::span<const RelationAggregate> aggregates) {
    using R = Result<SmartList<T>, DataServiceError<typename E::Error>>;
    return make_operation<R, TypedAggregateOperation<E, T>>(
        service.memory_resource(), service, query, aggregates);
}
}

namespace teaql::detail {
template <class E, Entity T>
auto EntityDataServiceAccess::fetch_entities_with_relation_aggregates(
    const EntityDataService<E>& service,
    const SelectQuery& query,
    std::span<const RelationAggregate> aggregates)
    -> AsyncOperation<Result<SmartList<T>, DataServiceError<typename E::Error>>>& {
    return fetch_typed_aggregate_entities<E, T>(service, query, aggregates);
}
}
