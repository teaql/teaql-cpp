#pragma once
#include <teaql/runtime/detail/data_service/graph_node_commands.hpp>

namespace teaql::detail {
struct RecursiveGraphRelation {
    using allocator_type = Allocator;
    String name;
    RelationDescriptor descriptor;
    Vector<GraphNode> children, saved;
    std::pmr::set<String, std::less<>> seen;
    Value local;
    String child_id;
    std::size_t index = 0;
    bool initialized = false;

    RecursiveGraphRelation(std::allocator_arg_t,
                           Allocator a,
                           std::string_view name,
                           const RelationDescriptor& descriptor,
                           Vector<GraphNode> children)
        : name(name, a.resource()), descriptor(std::allocator_arg, a, descriptor),
          children(move_in(a.resource(), std::move(children))), saved(a.resource()),
          seen(a.resource()), local(a.resource()), child_id(a.resource()) {}
};

template <class E>
class RecursiveGraphWriteOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<GraphNode, Error>;
    enum class Phase { Start, Exists, Delegate, Remove, Delete, One, Write, Read, Many };

public:
    using allocator_type = Allocator;

    RecursiveGraphWriteOperation(std::allocator_arg_t,
                                 Allocator a,
                                 const EntityDataService<E>& service,
                                 GraphNode node,
                                 bool insert,
                                 std::span<const TraceNode> parent)
        : service_(std::allocator_arg, a, service), node_(std::allocator_arg, a, std::move(node)),
          insert_(insert), parent_(parent.begin(), parent.end(), a.resource()),
          active_(a.resource()), identity_(a.resource()), one_(a.resource()), many_(a.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& context = EntityDataServiceAccess::context(service_);
        for (;;) {
            if (phase_ == Phase::Start) {
                if (insert_ && node_.operation == GraphOperation::Remove) {
                    String message("create graph cannot remove node ", r);
                    message += node_.entity;
                    return fail(message);
                }
                active_ = clone_in(r, parent_);
                if ((!insert_ || node_.operation != GraphOperation::Reference) &&
                    graph_has_comment(node_)) {
                    active_.emplace_back(TraceKind::AuditReason,
                                         node_.entity,
                                         *node_.comment,
                                         graph_legacy_trace_id(node_));
                }
                if (node_.operation == GraphOperation::Reference) {
                    graph_ = &EntityDataServiceAccess::validate_reference_node(
                        service_, std::move(node_), active_);
                    phase_ = Phase::Delegate;
                    continue;
                }
                if (node_.operation == GraphOperation::Remove) {
                    removal_ =
                        &EntityDataServiceAccess::validate_remove_node(service_, node_, active_);
                    phase_ = Phase::Remove;
                    continue;
                }
                auto descriptor = context.require_entity(node_.entity);
                if (!descriptor) {
                    return runtime_failure(std::move(descriptor).error());
                }
                descriptor_.emplace(std::allocator_arg, Allocator(r), descriptor.value());
                if (!insert_) {
                    auto* property = descriptor_->get().id_property();
                    if (!property) {
                        String message("entity ", r);
                        message += node_.entity;
                        message += " has no id property for graph upsert";
                        return fail(message);
                    }
                    auto* id = graph_field(node_, property->name);
                    if (graph_unassigned(id) || node_.operation == GraphOperation::Create) {
                        delegate_insert();
                        continue;
                    }
                    identity_ = Value(r, *id);
                    read_ = &EntityDataServiceAccess::fetch_graph_current_row_internal(
                        service_, node_.entity, property->name, identity_, active_);
                    phase_ = Phase::Exists;
                    continue;
                }
                if (auto error = split_relations()) {
                    return error;
                }
                phase_ = Phase::One;
            }
            if (phase_ == Phase::Delegate) {
                forwarding_.attach(graph_->wake_signal(), wake);
                auto* result = graph_->poll();
                if (!result) {
                    return {};
                }
                return copy_or_take_completed(*graph_, *result);
            }
            if (phase_ == Phase::Remove) {
                forwarding_.attach(removal_->wake_signal(), wake);
                auto* result = removal_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return R::err(r, copy_or_take_error(*removal_, *result));
                }
                forwarding_.reset();
                removal_ = nullptr;
                const ScopedCommentNode* parent_scope = nullptr;
                for (const auto& track : parent_) {
                    parent_scope = &context.template make<ScopedCommentNode>(parent_scope, track);
                }
                mutation_ =
                    &EntityDataServiceAccess::delete_graph_node(service_, node_, parent_scope);
                phase_ = Phase::Delete;
            }
            if (phase_ == Phase::Delete) {
                forwarding_.attach(mutation_->wake_signal(), wake);
                auto* result = mutation_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return R::err(r, copy_or_take_error(*mutation_, *result));
                }
                return R::ok(r, std::move(node_));
            }
            if (phase_ == Phase::Exists || phase_ == Phase::Read) {
                forwarding_.attach(read_->wake_signal(), wake);
                auto* result = read_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return R::err(r, copy_or_take_error(*read_, *result));
                }
                forwarding_.reset();
                if (phase_ == Phase::Exists) {
                    bool missing = !result->value();
                    read_ = nullptr;
                    if (missing) {
                        delegate_insert();
                        continue;
                    }
                    if (auto error = split_relations()) {
                        return error;
                    }
                    phase_ = Phase::One;
                } else {
                    if (!result->value()) {
                        String message("persisted ", r);
                        message += node_.entity;
                        message += " record could not be read back";
                        return fail(message);
                    }
                    node_.values.fields = result->value()->into_map(r);
                    read_ = nullptr;
                    relation_ = 0;
                    phase_ = Phase::Many;
                }
            }
            if (phase_ == Phase::One || phase_ == Phase::Many) {
                bool many = phase_ == Phase::Many;
                auto& relations = many ? many_ : one_;
                if (relation_ == relations.size()) {
                    if (many) {
                        return R::ok(r, std::move(node_));
                    }
                    relation_ = 0;
                    phase_ = Phase::Write;
                    continue;
                }
                auto& frame = *relations[relation_];
                if (!frame.initialized) {
                    if (!many && frame.children.size() > 1) {
                        String message("relation ", r);
                        message += node_.entity;
                        message += '.';
                        message += frame.name;
                        message += " expects one child, got ";
                        message += std::to_string(frame.children.size());
                        return fail(message);
                    }
                    if (many) {
                        auto* local = graph_field(node_, frame.descriptor.local_key);
                        if (!local) {
                            String message("parent ", r);
                            message += node_.entity;
                            message += " missing local key ";
                            message += frame.descriptor.local_key;
                            message += " for relation ";
                            message += frame.name;
                            return fail(message);
                        }
                        frame.local = Value(r, *local);
                        if (!insert_) {
                            auto target = context.require_entity(frame.descriptor.target_entity);
                            if (!target) {
                                return runtime_failure(std::move(target).error());
                            }
                            auto* id = target.value().get().id_property();
                            if (!id) {
                                String message("entity ", r);
                                message += frame.descriptor.target_entity;
                                message += " has no id property";
                                return fail(message);
                            }
                            frame.child_id = String(id->name, r);
                        }
                    }
                    frame.initialized = true;
                }
                if (graph_) {
                    forwarding_.attach(graph_->wake_signal(), wake);
                    auto* result = graph_->poll();
                    if (!result) {
                        return {};
                    }
                    auto completed = copy_or_take_completed(*graph_, *result);
                    forwarding_.reset();
                    graph_ = nullptr;
                    if (!completed) {
                        return std::move(completed);
                    }
                    auto child = std::move(completed).value();
                    if (!many && frame.descriptor.attach) {
                        auto* foreign = graph_field(child, frame.descriptor.foreign_key);
                        if (!foreign) {
                            String message("saved child ", r);
                            message += frame.descriptor.target_entity;
                            message += " missing foreign key ";
                            message += frame.descriptor.foreign_key;
                            message += " for relation ";
                            message += node_.entity;
                            message += '.';
                            message += frame.name;
                            return fail(message);
                        }
                        node_.values.fields.insert_or_assign(String(frame.descriptor.local_key, r),
                                                             Value(r, *foreign));
                    }
                    frame.saved.emplace_back(std::move(child));
                    ++frame.index;
                }
                if (frame.index == frame.children.size()) {
                    node_.relations.try_emplace(String(frame.name, r), std::move(frame.saved));
                    ++relation_;
                    continue;
                }
                auto& child = frame.children[frame.index];
                if (child.entity != frame.descriptor.target_entity) {
                    String message("relation ", r);
                    message += node_.entity;
                    message += '.';
                    message += frame.name;
                    message += " expects ";
                    message += frame.descriptor.target_entity;
                    message += ", got ";
                    message += child.entity;
                    return fail(message);
                }
                if (many && frame.descriptor.attach &&
                    (insert_ || child.operation != GraphOperation::Reference)) {
                    child.values.fields.insert_or_assign(String(frame.descriptor.foreign_key, r),
                                                         Value(r, frame.local));
                }
                if (many && !insert_) {
                    auto* id = graph_field(child, frame.child_id);
                    if (!graph_unassigned(id)) {
                        auto key = graph_identity_key(r, *id);
                        if (!frame.seen.emplace(key).second) {
                            String message("duplicate child id ", r);
                            message += key;
                            message += " in relation ";
                            message += node_.entity;
                            message += '.';
                            message += frame.name;
                            return fail(message);
                        }
                    }
                }
                auto scoped = EntityDataServiceAccess::scoped(service_, child.entity);
                graph_ = &make_operation<R, RecursiveGraphWriteOperation<E>>(
                    r, scoped, std::move(child), insert_, active_);
                continue;
            }
            if (phase_ == Phase::Write) {
                if (!mutation_) {
                    if (insert_) {
                        InsertCommand command(std::allocator_arg, Allocator(r), node_.entity);
                        command.values.fields = clone_record(r, node_.values.fields);
                        auto prepared = service_.prepare_insert_command(command);
                        if (!prepared) {
                            return runtime_failure(std::move(prepared).error());
                        }
                        insert_command_.emplace(
                            std::allocator_arg, Allocator(r), std::move(prepared).value());
                        mutation_ = &EntityDataServiceAccess::execute_prepared_insert(
                            service_, *insert_command_, active_);
                    } else {
                        auto* property = descriptor_->get().id_property();
                        auto update = EntityDataServiceAccess::graph_update_command(
                            service_, node_, descriptor_->get(), *property, identity_);
                        if (!update) {
                            return R::err(r, std::move(update).error());
                        }
                        if (update.value().values.fields.empty()) {
                            phase_ = Phase::Many;
                            continue;
                        }
                        auto prepared = service_.prepare_update_command(update.value());
                        if (!prepared) {
                            return runtime_failure(std::move(prepared).error());
                        }
                        update_command_.emplace(
                            std::allocator_arg, Allocator(r), std::move(prepared).value());
                        mutation_ = &EntityDataServiceAccess::execute_prepared_update(
                            service_, *update_command_, active_);
                    }
                }
                forwarding_.attach(mutation_->wake_signal(), wake);
                auto* result = mutation_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return R::err(r, copy_or_take_error(*mutation_, *result));
                }
                forwarding_.reset();
                mutation_ = nullptr;
                if (insert_) {
                    node_.values.fields = clone_record(r, insert_command_->values.fields);
                } else {
                    for (const auto& [name, value] : update_command_->values.fields) {
                        node_.values.fields.insert_or_assign(String(name, r), Value(r, value));
                    }
                    if (auto* version = descriptor_->get().version_property();
                        version && update_command_->expected_version_value) {
                        node_.value(
                            version->name,
                            mutation_next_version(*update_command_->expected_version_value));
                    }
                }
                auto* property = descriptor_->get().id_property();
                auto* id = property ? graph_field(node_, property->name) : nullptr;
                if (insert_ && (!property || !id)) {
                    phase_ = Phase::Many;
                    continue;
                }
                read_ = &EntityDataServiceAccess::fetch_graph_current_row_internal(
                    service_, node_.entity, property->name, insert_ ? *id : identity_, active_);
                phase_ = Phase::Read;
            }
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        graph_ = nullptr;
        mutation_ = nullptr;
        read_ = nullptr;
        removal_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (graph_) {
            graph_->cancel();
            graph_ = nullptr;
        }
        if (mutation_) {
            mutation_->cancel();
            mutation_ = nullptr;
        }
        if (read_) {
            read_->cancel();
            read_ = nullptr;
        }
        if (removal_) {
            removal_->cancel();
            removal_ = nullptr;
        }
    }

private:
    R runtime_failure(RuntimeError error) {
        auto r = service_.memory_resource();
        return R::err(r, Error::runtime(r, std::move(error)));
    }

    R fail(std::string_view message) {
        return runtime_failure(RuntimeError::graph(service_.memory_resource(), message));
    }

    void delegate_insert() {
        node_.comment.reset();
        graph_ = &make_operation<R, RecursiveGraphWriteOperation<E>>(
            service_.memory_resource(), service_, std::move(node_), true, active_);
        phase_ = Phase::Delegate;
    }

    std::optional<R> split_relations() {
        auto r = service_.memory_resource();
        const auto& context = EntityDataServiceAccess::context(service_);
        auto relations = std::move(node_.relations);
        node_.relations = decltype(node_.relations)(r);
        for (auto& [name, children] : relations) {
            auto* relation = descriptor_->get().relation_by_name(name);
            if (!relation) {
                return runtime_failure(RuntimeError::missing_relation(r, node_.entity, name));
            }
            auto& frame =
                context.template make<RecursiveGraphRelation>(name, *relation, std::move(children));
            (relation->many ? many_ : one_).push_back(&frame);
        }
        return {};
    }

    EntityDataService<E> service_;
    GraphNode node_;
    bool insert_;
    TraceChain parent_, active_;
    Value identity_;
    std::optional<ResourceBorrow<const EntityDescriptor>> descriptor_;
    Vector<RecursiveGraphRelation*> one_, many_;
    std::size_t relation_ = 0;
    Phase phase_ = Phase::Start;
    std::optional<InsertCommand> insert_command_;
    std::optional<UpdateCommand> update_command_;
    AsyncOperation<R>* graph_ = nullptr;
    AsyncOperation<Result<std::uint64_t, Error>>* mutation_ = nullptr;
    AsyncOperation<Result<std::optional<CompactRow>, Error>>* read_ = nullptr;
    AsyncOperation<Result<std::monostate, Error>>* removal_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::insert_graph_node_scoped(const EntityDataService<E>& service,
                                                       GraphNode node,
                                                       const ScopedCommentNode* parent)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    using R = Result<GraphNode, DataServiceError<typename E::Error>>;
    auto r = service.memory_resource();
    auto trace = parent ? parent->to_trace_chain(r) : TraceChain(r);
    return make_operation<R, RecursiveGraphWriteOperation<E>>(
        r, service, std::move(node), true, trace);
}

template <class E>
auto EntityDataServiceAccess::upsert_graph_node_scoped(const EntityDataService<E>& service,
                                                       GraphNode node,
                                                       const ScopedCommentNode* parent)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    using R = Result<GraphNode, DataServiceError<typename E::Error>>;
    auto r = service.memory_resource();
    auto trace = parent ? parent->to_trace_chain(r) : TraceChain(r);
    return make_operation<R, RecursiveGraphWriteOperation<E>>(
        r, service, std::move(node), false, trace);
}
}
