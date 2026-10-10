#pragma once
#include <teaql/runtime/detail/entity_graph_execution.hpp>
#include <teaql/core/entity_graph.hpp>

namespace teaql::detail {
// These adapters retain the source's independent entry boundaries. In
// particular only the status-only route has the no-persistence shortcut.
enum class EntitySaveAdapter { Graph, Status, StatusComment, Comment, CreateComment };

template <class E, Entity T>
class EntitySaveAdapterOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<GraphNode, Error>;

public:
    using allocator_type = Allocator;

    EntitySaveAdapterOperation(std::allocator_arg_t,
                               Allocator a,
                               const EntityDataService<E>& service,
                               T entity,
                               EntitySaveAdapter adapter,
                               EntityStatus status,
                               std::string_view comment)
        : service_(std::allocator_arg, a, service),
          entity_(move_to(a.resource(), std::move(entity))), adapter_(adapter), status_(status),
          comment_(comment, a.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_) {
            if (adapter_ == EntitySaveAdapter::Status && !need_persist(status_)) {
                return R::ok(r, GraphNode(std::allocator_arg, Allocator(r), service_.entity()));
            }
            auto extracted = service_.graph_node_from_entity(std::move(*entity_));
            entity_.reset();
            if (!extracted) {
                return R::err(r, Error::runtime(r, std::move(extracted).error()));
            }
            auto node = std::move(extracted).value();
            if ((adapter_ == EntitySaveAdapter::Status ||
                 adapter_ == EntitySaveAdapter::StatusComment) &&
                is_deleted(status_)) {
                node.operation = GraphOperation::Remove;
                node.relations.clear();
            }
            if (adapter_ == EntitySaveAdapter::CreateComment) {
                node.operation = GraphOperation::Create;
            }
            if (adapter_ == EntitySaveAdapter::StatusComment ||
                adapter_ == EntitySaveAdapter::Comment ||
                adapter_ == EntitySaveAdapter::CreateComment) {
                node.set_comment(comment_);
            }
            child_ = &EntityDataServiceAccess::save_graph_internal(service_, std::move(node));
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        return copy_or_take_completed(*child_, *result);
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        child_ = nullptr;
        entity_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
        entity_.reset();
    }

private:
    EntityDataService<E> service_;
    std::optional<T> entity_;
    EntitySaveAdapter adapter_;
    EntityStatus status_;
    String comment_;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E, Entity T>
auto make_entity_save_adapter(const EntityDataService<E>& service,
                              T entity,
                              EntitySaveAdapter adapter,
                              EntityStatus status,
                              std::string_view comment = {})
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    using R = Result<GraphNode, DataServiceError<typename E::Error>>;
    return make_operation<R, EntitySaveAdapterOperation<E, T>>(
        service.memory_resource(), service, std::move(entity), adapter, status, comment);
}

template <class E, Entity T>
auto EntityDataServiceAccess::save_entity_graph_internal(const EntityDataService<E>& service,
                                                         T entity)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    return make_entity_save_adapter(
        service, std::move(entity), EntitySaveAdapter::Graph, EntityStatus::New);
}

template <class E, Entity T>
auto EntityDataServiceAccess::save_entity_internal(const EntityDataService<E>& service,
                                                   T entity,
                                                   EntityStatus status)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    return make_entity_save_adapter(service, std::move(entity), EntitySaveAdapter::Status, status);
}

template <class E, Entity T>
auto EntityDataServiceAccess::save_entity_with_comment_internal(const EntityDataService<E>& service,
                                                                T entity,
                                                                EntityStatus status,
                                                                std::string_view comment)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    return make_entity_save_adapter(
        service, std::move(entity), EntitySaveAdapter::StatusComment, status, comment);
}

template <class E, Entity T>
auto EntityDataServiceAccess::save_entity_graph_with_comment_internal(
    const EntityDataService<E>& service, T entity, std::string_view comment)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    return make_entity_save_adapter(
        service, std::move(entity), EntitySaveAdapter::Comment, EntityStatus::New, comment);
}

template <class E, Entity T>
auto EntityDataServiceAccess::create_entity_graph_with_comment_internal(
    const EntityDataService<E>& service, T entity, std::string_view comment)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    return make_entity_save_adapter(
        service, std::move(entity), EntitySaveAdapter::CreateComment, EntityStatus::New, comment);
}

inline GraphNode convert_save_graph(Memory r, const core::EntityGraphNode& source) {
    GraphNode node(std::allocator_arg, Allocator(r), source.entity_type);
    node.values.fields = clone_record(r, source.values.fields);
    node.operation = source.operation == core::EntityGraphOperation::Save ? GraphOperation::Upsert
                                                                          : GraphOperation::Remove;
    node.comment = clone_in(r, source.comment);
    for (const auto& [name, child] : source.children) {
        node.relations.try_emplace(String(name, r))
            .first->second.emplace_back(convert_save_graph(r, child));
    }
    return node;
}

template <class E>
class CoreEntityGraphSaveOperation {
    using R = Result<GraphNode, DataServiceError<typename E::Error>>;

public:
    using allocator_type = Allocator;

    CoreEntityGraphSaveOperation(std::allocator_arg_t,
                                 Allocator a,
                                 const EntityDataService<E>& service,
                                 core::EntityGraph graph)
        : service_(std::allocator_arg, a, service),
          graph_(std::allocator_arg, a, std::move(graph)) {}

    std::optional<R> poll(WakeSignal& wake) {
        if (!child_) {
            child_ = &EntityDataServiceAccess::save_graph_internal(
                service_, convert_save_graph(service_.memory_resource(), graph_.root));
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        return copy_or_take_completed(*child_, *result);
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
    core::EntityGraph graph_;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::save_entity_graph_from_internal(const EntityDataService<E>& service,
                                                              core::EntityGraph graph)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    using R = Result<GraphNode, DataServiceError<typename E::Error>>;
    return make_operation<R, CoreEntityGraphSaveOperation<E>>(
        service.memory_resource(), service, std::move(graph));
}
}
