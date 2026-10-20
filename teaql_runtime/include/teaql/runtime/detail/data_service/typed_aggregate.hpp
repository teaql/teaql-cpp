#pragma once
#include <teaql/runtime/detail/data_service/entity_query.hpp>
#include <teaql/runtime/detail/data_service/dynamic_loading.hpp>
#include <teaql/runtime/detail/read_hydration.hpp>

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
          aggregates_(aggregates.begin(), aggregates.end(), allocator.resource()),
          rows_(allocator.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto resource = service_.memory_resource();
        if (!fetched_) {
            const auto& context = EntityDataServiceAccess::context(service_);
            if (!child_) {
                if (query_.dynamic_field_selection &&
                    !EntityDataServiceAccess::has_request_intent(service_)) {
                    auto source = clone_in(resource, query_);
                    if (EntityDataServiceAccess::context_service(service_)
                            .metadata.capture_execution_metadata()) {
                        for (const auto& aggregate : aggregates_) {
                            source.child_enhancements.emplace_back(aggregate.query);
                        }
                    }
                    auto scoped = EntityDataServiceAccess::query_scoped(service_, source);
                    if (!scoped) {
                        return R::err(
                            resource,
                            DataServiceError<Error>::runtime(resource, std::move(scoped).error()));
                    }
                    service_ = std::move(scoped).value();
                }
                ensure_identity_projection(context, query_);
                root_ = &context.template make<EntityRuntimeState>();
                child_ = &make_operation<RelationRowsResult<E>, EntityRowsOperation<E>>(
                    resource,
                    service_,
                    query_,
                    std::span<const RelationAggregate>(aggregates_),
                    true);
            }
            forwarding_.attach(child_->wake_signal(), wake);
            const auto* result = child_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            if (!*result) {
                return R::err(resource, copy_or_take_error(*child_, *result));
            }
            rows_ = Vector<CompactRow>(result->value(), resource);
            child_ = nullptr;
            fetched_ = true;
        }
        if (query_.dynamic_field_selection && !rows_.empty()) {
            if (!T::supports_dynamic_field_load()) {
                return R::err(
                    resource,
                    DataServiceError<Error>::entity(
                        resource,
                        EntityError(
                            std::allocator_arg,
                            Allocator(resource),
                            T::ENTITY_NAME,
                            "dynamic fields require a runtime-owned indexed entity carrier")));
            }
            if (!dynamic_) {
                auto intent = EntityDataServiceAccess::request_intent(service_, query_);
                if (!intent) {
                    return R::err(
                        resource,
                        DataServiceError<Error>::runtime(resource, std::move(intent).error()));
                }
                dynamic_ = &make_operation<typename DynamicRowsOperation<Error>::Output,
                                           DynamicRowsOperation<Error>>(
                    resource,
                    EntityDataServiceAccess::context(service_),
                    std::span<CompactRow>(rows_),
                    T::ENTITY_NAME,
                    *query_.dynamic_field_selection,
                    intent.value());
            }
            forwarding_.attach(dynamic_->wake_signal(), wake);
            const auto* result = dynamic_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            if (!*result) {
                return R::err(resource, copy_or_take_error(*dynamic_, *result));
            }
        }
        auto decoded =
            decode_compact_rows_with_read_metadata<T>(resource, std::move(rows_), *root_);
        if (!decoded) {
            return R::err(resource,
                          DataServiceError<Error>::entity(resource, std::move(decoded).error()));
        }
        SmartList<T> list(std::allocator_arg, Allocator(resource), std::move(decoded).value());
        list.loaded();
        return R::ok(resource, std::move(list));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        child_ = nullptr;
        dynamic_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
        if (dynamic_) {
            dynamic_->cancel();
            dynamic_ = nullptr;
        }
    }

private:
    EntityDataService<E> service_;
    SelectQuery query_;
    Vector<RelationAggregate> aggregates_;
    Vector<CompactRow> rows_;
    EntityRuntimeState* root_ = nullptr;
    AsyncOperation<RelationRowsResult<E>>* child_ = nullptr;
    AsyncOperation<typename DynamicRowsOperation<Error>::Output>* dynamic_ = nullptr;
    WakeSignal::Forward forwarding_;
    bool fetched_ = false;
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
