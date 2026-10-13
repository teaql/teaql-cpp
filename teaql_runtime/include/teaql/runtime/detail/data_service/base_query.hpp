#pragma once
#include <teaql/runtime/detail/data_service/declarations.hpp>

namespace teaql::detail {
template <class M, class E, class T, bool List>
class BaseQueryOperation {
    using Error = typename E::Error;
    using Output = std::conditional_t<List, SmartList<T>, Vector<CompactRow>>;
    using R = Result<Output, DataServiceError<Error>>;

public:
    using allocator_type = Allocator;

    BaseQueryOperation(std::allocator_arg_t,
                       Allocator a,
                       const ResourceBorrow<const M>& metadata,
                       const ResourceBorrow<E>& executor,
                       const SelectQuery& query)
        : resource_(a.resource()), metadata_(std::allocator_arg, a, metadata),
          executor_(std::allocator_arg, a, executor), query_(std::allocator_arg, a, query) {}

    std::optional<R> poll(WakeSignal& wake) {
        if (!child_) {
            const auto& store = metadata_.get();
            auto checked = QueryRequest::from_query(
                resource_, query_, store.capture_query_debug(), store.capture_execution_metadata());
            if (!checked) {
                return R::err(
                    resource_,
                    DataServiceError<Error>::runtime(
                        resource_, RuntimeError::request_intent(resource_, checked.error())));
            }
            auto request = std::move(checked).value();
            auto& child = executor_.get().query_observed(
                resource_, std::move(request), store.query_diagnostic_observer(resource_));
            if (child.memory_resource() != resource_) {
                child.cancel();
                throw std::invalid_argument(
                    "Runtime query operation must belong to its result Context");
            }
            child_ = &child;
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(
                resource_,
                DataServiceError<Error>::executor(resource_, copy_or_take_error(*child_, *result)));
        }
        if constexpr (!List) {
            return R::ok(resource_, Vector<CompactRow>(result->value().rows, resource_));
        } else {
            metadata_.get().record_metadata_log(result->value().metadata);
            SmartList<T> values(std::allocator_arg, Allocator(resource_));
            values.data.reserve(result->value().rows.size());
            for (const auto& row : result->value().rows) {
                if constexpr (std::same_as<T, CompactRow>) {
                    values.data.emplace_back(row);
                } else {
                    auto decoded = T::from_compact_row(resource_, clone_in(resource_, row));
                    if (!decoded) {
                        return R::err(
                            resource_,
                            DataServiceError<Error>::entity(resource_, std::move(decoded).error()));
                    }
                    values.data.emplace_back(std::move(decoded).value());
                }
            }
            values.loaded();
            return R::ok(resource_, std::move(values));
        }
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
    Memory resource_;
    ResourceBorrow<const M> metadata_;
    ResourceBorrow<E> executor_;
    SelectQuery query_;
    DataServiceOperationResult<QueryResult, Error>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class M, class E>
auto RuntimeDataService<M, E>::fetch_all(const SelectQuery& query) const
    -> DataServiceOperationResult<Vector<CompactRow>, DataServiceError<Error>>& {
    using R = Result<Vector<CompactRow>, DataServiceError<Error>>;
    return make_operation<R, BaseQueryOperation<M, E, CompactRow, false>>(
        memory_resource(), metadata_, executor_, query);
}

template <class M, class E>
auto RuntimeDataService<M, E>::fetch_smart_list(const SelectQuery& query) const
    -> DataServiceOperationResult<SmartList<CompactRow>, DataServiceError<Error>>& {
    using R = Result<SmartList<CompactRow>, DataServiceError<Error>>;
    return make_operation<R, BaseQueryOperation<M, E, CompactRow, true>>(
        memory_resource(), metadata_, executor_, query);
}

template <class M, class E>
template <Entity T>
auto RuntimeDataService<M, E>::fetch_entities(const SelectQuery& query) const
    -> DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>& {
    using R = Result<SmartList<T>, DataServiceError<Error>>;
    return make_operation<R, BaseQueryOperation<M, E, T, true>>(
        memory_resource(), metadata_, executor_, query);
}
}
