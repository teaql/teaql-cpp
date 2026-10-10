#pragma once
#include <teaql/runtime/detail/entity_graph_execution.hpp>

namespace teaql::detail {
template <class E, bool Remove>
class GraphNodeValidationOperation {
    using Error = DataServiceError<typename E::Error>;
    using Output = std::conditional_t<Remove, std::monostate, GraphNode>;
    using R = Result<Output, Error>;

public:
    using allocator_type = Allocator;

    GraphNodeValidationOperation(std::allocator_arg_t,
                                 Allocator a,
                                 const EntityDataService<E>& service,
                                 const GraphNode& node,
                                 std::span<const TraceNode> trace)
        : service_(std::allocator_arg, a, service), node_(std::allocator_arg, a, node),
          trace_(trace.begin(), trace.end(), a.resource()), id_(a.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!read_) {
            if (!node_.relations.empty()) {
                String message(role(), r);
                message += " node ";
                message += node_.entity;
                message += " cannot contain child relations";
                return fail(message);
            }
            auto descriptor =
                EntityDataServiceAccess::context(service_).require_entity(node_.entity);
            if (!descriptor) {
                return R::err(r, Error::runtime(r, std::move(descriptor).error()));
            }
            descriptor_.emplace(std::allocator_arg, Allocator(r), descriptor.value());
            auto* property = descriptor_->get().id_property();
            if (!property) {
                String message("entity ", r);
                message += node_.entity;
                message += " has no id property for graph ";
                message += role();
                return fail(message);
            }
            auto* id = graph_field(node_, property->name);
            if (graph_unassigned(id)) {
                String message(role(), r);
                message += " node ";
                message += node_.entity;
                message += " missing id property ";
                message += property->name;
                return fail(message);
            }
            id_ = Value(r, *id);
            if constexpr (!Remove) {
                auto* version = descriptor_->get().version_property();
                for (const auto& [name, value] : node_.values.fields) {
                    if (name == property->name || (version && name == version->name)) {
                        continue;
                    }
                    String message("reference node ", r);
                    message += node_.entity;
                    message += " cannot carry mutable field ";
                    message += name;
                    return fail(message);
                }
            }
            read_ = &EntityDataServiceAccess::fetch_graph_current_row_internal(
                service_, node_.entity, property->name, id_, trace_);
        }
        forwarding_.attach(read_->wake_signal(), wake);
        auto* result = read_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(r, copy_or_take_error(*read_, *result));
        }
        if (!result->value()) {
            return identity_error(" does not exist");
        }
        const auto& current = *result->value();
        if (auto* version = descriptor_->get().version_property()) {
            auto* value = current.get(version->name);
            auto* existing = value ? std::get_if<std::int64_t>(&value->data) : nullptr;
            if (existing) {
                if (*existing < 0) {
                    return identity_error(Remove ? " is already deleted" : " is deleted");
                }
                if constexpr (!Remove) {
                    auto* expected_value = graph_field(node_, version->name);
                    auto* expected =
                        expected_value ? std::get_if<std::int64_t>(&expected_value->data) : nullptr;
                    if (expected && *expected != *existing) {
                        return R::err(
                            r,
                            Error::runtime(r,
                                           RuntimeError::optimistic_lock_conflict(
                                               r, node_.entity, graph_identity_key(r, id_))));
                    }
                }
            }
        }
        if constexpr (Remove) {
            return R::ok(r, std::monostate{});
        } else {
            GraphNode out(std::allocator_arg, Allocator(r), node_.entity);
            out.values.fields = current.into_map(r);
            out.operation = GraphOperation::Reference;
            return R::ok(r, std::move(out));
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        read_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (read_) {
            read_->cancel();
            read_ = nullptr;
        }
    }

private:
    static constexpr std::string_view role() {
        return Remove ? "remove" : "reference";
    }

    R fail(std::string_view message) {
        auto r = service_.memory_resource();
        return R::err(r, Error::runtime(r, RuntimeError::graph(r, message)));
    }

    R identity_error(std::string_view suffix) {
        auto r = service_.memory_resource();
        String message(role(), r);
        message += " node ";
        message += node_.entity;
        message += '(';
        message += graph_identity_key(r, id_);
        message += ')';
        message += suffix;
        return fail(message);
    }

    EntityDataService<E> service_;
    GraphNode node_;
    TraceChain trace_;
    Value id_;
    std::optional<ResourceBorrow<const EntityDescriptor>> descriptor_;
    AsyncOperation<Result<std::optional<CompactRow>, Error>>* read_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::validate_reference_node(const EntityDataService<E>& service,
                                                      GraphNode node,
                                                      std::span<const TraceNode> trace)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    using R = Result<GraphNode, DataServiceError<typename E::Error>>;
    return make_operation<R, GraphNodeValidationOperation<E, false>>(
        service.memory_resource(), service, node, trace);
}

template <class E>
auto EntityDataServiceAccess::validate_remove_node(const EntityDataService<E>& service,
                                                   const GraphNode& node,
                                                   std::span<const TraceNode> trace)
    -> AsyncOperation<Result<std::monostate, DataServiceError<typename E::Error>>>& {
    using R = Result<std::monostate, DataServiceError<typename E::Error>>;
    return make_operation<R, GraphNodeValidationOperation<E, true>>(
        service.memory_resource(), service, node, trace);
}

template <class E>
auto EntityDataServiceAccess::graph_update_command(const EntityDataService<E>& service,
                                                   GraphNode& node,
                                                   const EntityDescriptor& descriptor,
                                                   const PropertyDescriptor& id_property,
                                                   const Value& id)
    -> Result<UpdateCommand, DataServiceError<typename E::Error>> {
    auto r = service.memory_resource();
    using Error = DataServiceError<typename E::Error>;
    using R = Result<UpdateCommand, Error>;
    Arena::Quiescence context_guard;
    context_guard.attach(resource_arena(r), nullptr, [](void*) noexcept {
        std::terminate();
    });
    Arena::Quiescence node_guard;
    if (node.memory_resource() != r) {
        node_guard.attach(resource_arena(node.memory_resource()), nullptr, [](void*) noexcept {
            std::terminate();
        });
    }
    Value identity(r, id);
    mark_entity_status(node.values, CheckObjectStatus::Update);
    auto checked = context(service).check_and_fix_values(node.entity, node.values);
    clear_entity_status(node.values);
    if (!checked) {
        return R::err(r, Error::runtime(r, std::move(checked).error()));
    }
    UpdateCommand command(std::allocator_arg, Allocator(r), node.entity, std::move(identity));
    command.old_values = clone_in(r, node.original_values);
    if (auto* version = descriptor.version_property()) {
        auto* value = graph_field(node, version->name);
        if (auto* number = value ? std::get_if<std::int64_t>(&value->data) : nullptr) {
            command.expected_version(*number);
        }
    }
    for (const auto& property : descriptor.properties) {
        if (property.is_id || property.is_version || property.name == id_property.name ||
            (node.dirty_fields && !node.dirty_fields->contains(property.name))) {
            continue;
        }
        if (auto* value = graph_field(node, property.name)) {
            command.value(property.name, *value);
        }
    }
    return R::ok(r, std::move(command));
}

template <class E>
class GraphNodeDeleteOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<std::uint64_t, Error>;

public:
    using allocator_type = Allocator;

    GraphNodeDeleteOperation(std::allocator_arg_t,
                             Allocator a,
                             const EntityDataService<E>& service,
                             const GraphNode& node,
                             const ScopedCommentNode* parent)
        : service_(std::allocator_arg, a, service), node_(std::allocator_arg, a, node),
          parent_(parent ? parent->to_trace_chain(a.resource()) : TraceChain(a.resource())) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_) {
            auto descriptor =
                EntityDataServiceAccess::context(service_).require_entity(node_.entity);
            if (!descriptor) {
                return R::err(r, Error::runtime(r, std::move(descriptor).error()));
            }
            auto* property = descriptor.value().get().id_property();
            if (!property) {
                String message("entity ", r);
                message += node_.entity;
                message += " has no id property for graph remove";
                return fail(message);
            }
            auto* id = graph_field(node_, property->name);
            if (graph_unassigned(id)) {
                String message("remove node ", r);
                message += node_.entity;
                message += " missing id property ";
                message += property->name;
                return fail(message);
            }
            DeleteCommand command(std::allocator_arg, Allocator(r), node_.entity, *id);
            if (auto* version = descriptor.value().get().version_property()) {
                auto* value = graph_field(node_, version->name);
                if (auto* number = value ? std::get_if<std::int64_t>(&value->data) : nullptr) {
                    command.expected_version(*number);
                }
            }
            auto lineage = clone_in(r, parent_);
            if (node_.comment && !node_.comment->empty()) {
                lineage.emplace_back(
                    TraceKind::AuditReason, node_.entity, *node_.comment, graph_trace_id(node_));
            }
            child_ = &EntityDataServiceAccess::delete_scoped_internal(service_, command, lineage);
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
    R fail(std::string_view message) {
        auto r = service_.memory_resource();
        return R::err(r, Error::runtime(r, RuntimeError::graph(r, message)));
    }

    EntityDataService<E> service_;
    GraphNode node_;
    TraceChain parent_;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::delete_graph_node(const EntityDataService<E>& service,
                                                const GraphNode& node,
                                                const ScopedCommentNode* parent)
    -> AsyncOperation<Result<std::uint64_t, DataServiceError<typename E::Error>>>& {
    using R = Result<std::uint64_t, DataServiceError<typename E::Error>>;
    return make_operation<R, GraphNodeDeleteOperation<E>>(
        service.memory_resource(), service, node, parent);
}
}
