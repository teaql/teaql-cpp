#pragma once
#include <teaql/runtime/detail/data_service/entity_service.hpp>
#include <teaql/runtime/detail/data_service/diagnostic_query.hpp>

namespace teaql::detail {
template <class Error>
class EntityStreamItems {
    using Item = Result<StreamChunk, DataServiceError<Error>>;

public:
    using allocator_type = Allocator;

    EntityStreamItems(std::allocator_arg_t,
                      Allocator a,
                      QueryStream<Error>& source,
                      const DynamicPropertyDefinitions* definitions)
        : resource_(a.resource()),
          source_(std::allocator_arg, a, source, resource_arena(source.memory_resource())) {
        if (source.memory_resource() != resource_) {
            source.cancel();
            throw std::invalid_argument("Entity stream must belong to its UserContext");
        }
        if (definitions) {
            definitions_.emplace(std::allocator_arg, a, *definitions);
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
        auto chunk = clone_in(resource_, next.item->value());
        if (definitions_) {
            CompactRow::attach_dynamic_property_definitions(chunk.rows, definitions_->get());
        }
        return std::optional<Item>(Item::ok(resource_, std::move(chunk)));
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
    std::optional<ArenaBorrow<DynamicPropertyDefinitions>> definitions_;
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
        const bool capture = cs.metadata.capture_execution_metadata();
        auto intent = EntityDataServiceAccess::request_intent(service_, query_);
        if (!intent) {
            return R::err(r, DataServiceError<Error>::runtime(r, std::move(intent).error()));
        }
        QueryRequest request(std::allocator_arg,
                             Allocator(r),
                             query_,
                             intent.value(),
                             cs.metadata.capture_query_debug(),
                             capture);
        if (!capture) {
            request.trace_chain.clear();
        }
        const auto size = query_.stream_config ? query_.stream_config->chunk_size : 1000;
        auto& executor = cs.executor.get();
        QueryStream<Error>* selected_source = nullptr;
        if (query_.dynamic_field_selection) {
            auto* store = executor.dynamic_field_store();
            if (!store) {
                return R::err(r,
                              DataServiceError<Error>::runtime(
                                  r,
                                  RuntimeError::dynamic_field(
                                      r,
                                      DynamicFieldError(
                                          r, "DYNAMIC_FIELD_STORAGE_UNSUPPORTED", query_.entity))));
            }
            auto plan = cs.metadata.context().prepare_dynamic_field_stream(
                *store, query_.entity, *query_.dynamic_field_selection, intent.value());
            if (!plan) {
                return R::err(r,
                              DataServiceError<Error>::runtime(
                                  r, RuntimeError::dynamic_field(r, plan.error())));
            }
            auto source = executor.query_stream_with_dynamic_fields(
                r,
                std::move(request),
                size,
                std::move(plan).value(),
                capture ? EntityDataServiceAccess::observer(service_)
                        : std::optional<ExecutionObserver>{});
            if (!source) {
                return R::err(r,
                              DataServiceError<Error>::runtime(
                                  r, RuntimeError::dynamic_field(r, source.error())));
            }
            selected_source = source.value();
            if (!selected_source) {
                throw std::invalid_argument("Dynamic entity stream returned a null handle");
            }
        } else {
            selected_source = capture ? &executor.query_stream_observed(
                                            r,
                                            std::move(request),
                                            size,
                                            std::move(*EntityDataServiceAccess::observer(service_)))
                                      : &executor.query_stream(r, std::move(request), size);
        }
        auto& source = *selected_source;
        source_ = &source;
        if (source.memory_resource() != r) {
            source.cancel();
            source_ = nullptr;
            throw std::invalid_argument("Entity stream must belong to its UserContext");
        }
        // Transfer cancellation only after publishing the returned handle.
        auto& stream =
            make_stream<Result<StreamChunk, DataServiceError<Error>>, EntityStreamItems<Error>>(
                r, source, query_.dynamic_property_definitions.get());
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
    return detail::execute_scoped_purposed_query<EntityQueryStream<Error>>(
        *this,
        query,
        {},
        +[](const EntityDataService<E>& service,
            const SelectQuery& actual,
            std::span<const RelationAggregate>) -> auto& {
            return make_operation<R, detail::EntityStreamOperation<E>>(
                service.memory_resource(), service, actual);
        });
}
}
