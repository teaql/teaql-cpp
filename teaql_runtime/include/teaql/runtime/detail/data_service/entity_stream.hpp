#pragma once
#include <teaql/runtime/detail/data_service/entity_service.hpp>

namespace teaql::detail {
template <class Error>
class EntityStreamItems {
    using Item = Result<StreamChunk, DataServiceError<Error>>;

public:
    using allocator_type = Allocator;

    EntityStreamItems(std::allocator_arg_t, Allocator a, QueryStream<Error>& source)
        : resource_(a.resource()),
          source_(std::allocator_arg, a, source, resource_arena(source.memory_resource())) {
        if (source.memory_resource() != resource_) {
            source.cancel();
            throw std::invalid_argument("Entity stream must belong to its UserContext");
        }
    }

    std::optional<std::optional<Item>> poll_next(WakeSignal& wake) {
        auto& source = source_.get();
        forwarding_.attach(source.wake_signal(), wake);
        auto next = source.poll_next();
        using Kind = typename StreamPoll<Result<StreamChunk, Error>>::Kind;
        if (next.kind == Kind::Pending) {
            return {};
        }
        forwarding_.reset();
        if (next.kind == Kind::End) {
            return std::optional<Item>{};
        }
        if (!*next.item) {
            return std::optional<Item>(
                Item::err(resource_,
                          DataServiceError<Error>::executor(
                              resource_, copy_or_take_error(source, *next.item))));
        }
        return std::optional<Item>(Item::ok(resource_, clone_in(resource_, next.item->value())));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        source_.get().cancel();
    }

private:
    Memory resource_;
    ResourceBorrow<QueryStream<Error>> source_;
    WakeSignal::Forward forwarding_;
};

template <class E>
class EntityStreamOperation {
    using Error = typename E::Error;
    using R = Result<EntityQueryStream<Error>, DataServiceError<Error>>;

public:
    using allocator_type = Allocator;

    EntityStreamOperation(std::allocator_arg_t,
                          Allocator a,
                          const EntityDataService<E>& service,
                          const SelectQuery& query)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query) {}

    std::optional<R> poll(WakeSignal&) {
        auto r = service_.memory_resource();
        auto prepared = EntityDataServiceAccess::prepare_select(service_, query_);
        if (!prepared) {
            return R::err(r, DataServiceError<Error>::runtime(r, std::move(prepared).error()));
        }
        try {
            query_ = prepared.value().prepare_for_list();
        } catch (const std::invalid_argument& error) {
            return R::err(
                r, DataServiceError<Error>::runtime(r, RuntimeError::graph(r, error.what())));
        }
        if (!query_.relations.empty() || !query_.child_enhancements.empty() ||
            !query_.object_group_bys.empty()) {
            return R::err(
                r,
                DataServiceError<Error>::runtime(
                    r,
                    RuntimeError::graph(
                        r,
                        "streaming relation or aggregate enhancement is not supported; stream a root query or use execute_for_list")));
        }
        const auto& cs = EntityDataServiceAccess::context_service(service_);
        query_.comment_text = cs.resolve_final_comment(
            query_.trace_chain,
            query_.comment_text ? std::optional<std::string_view>(*query_.comment_text)
                                : std::nullopt);
        const bool capture = cs.metadata.capture_execution_metadata();
        QueryRequest request(
            std::allocator_arg, Allocator(r), query_, cs.metadata.capture_query_debug(), capture);
        if (capture) {
            for (const auto& node : query_.trace_chain) {
                request.trace_chain.emplace_back(node);
            }
            request.comment = clone_in(r, query_.comment_text);
        }
        const auto size = query_.stream_config ? query_.stream_config->chunk_size : 1000;
        auto& executor = cs.executor.get();
        auto& source = capture ? executor.query_stream_observed(
                                     r,
                                     std::move(request),
                                     size,
                                     std::move(*cs.metadata.query_diagnostic_observer(r)))
                               : executor.query_stream(r, std::move(request), size);
        source_ = &source;
        if (source.memory_resource() != r) {
            source.cancel();
            source_ = nullptr;
            throw std::invalid_argument("Entity stream must belong to its UserContext");
        }
        // Transfer cancellation only after publishing the returned handle.
        auto& stream =
            make_stream<Result<StreamChunk, DataServiceError<Error>>, EntityStreamItems<Error>>(
                r, source);
        mapped_ = &stream;
        return R::ok(
            r,
            EntityQueryStream<Error>(std::allocator_arg, Allocator(r), stream, resource_arena(r)));
    }

    void finish_and_wait() noexcept {
        source_ = nullptr;
        mapped_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        if (mapped_) {
            mapped_->cancel();
        } else if (source_) {
            source_->cancel();
        }
        finish_and_wait();
    }

private:
    EntityDataService<E> service_;
    SelectQuery query_;
    QueryStream<Error>* source_ = nullptr;
    QueryStream<DataServiceError<Error>>* mapped_ = nullptr;
};
}

namespace teaql {
template <class E>
auto EntityDataService<E>::fetch_stream(const PurposedSelectQuery& query) const
    -> DataServiceOperationResult<EntityQueryStream<Error>, DataServiceError<Error>>&
    requires std::derived_from<E, StreamQueryExecutor<Error>>
{
    using R = Result<EntityQueryStream<Error>, DataServiceError<Error>>;
    return make_operation<R, detail::EntityStreamOperation<E>>(
        memory_resource(), *this, query.as_query());
}
}
