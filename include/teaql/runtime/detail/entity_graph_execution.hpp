#pragma once
#include <teaql/runtime/detail/entity_mutation.hpp>
#include <teaql/runtime/detail/governance_scope.hpp>

namespace teaql::detail {
template <class E>
class GraphCurrentRowOperation {
    using R = Result<std::optional<CompactRow>, DataServiceError<typename E::Error>>;

public:
    using allocator_type = Allocator;

    GraphCurrentRowOperation(std::allocator_arg_t,
                             Allocator a,
                             const EntityDataService<E>& service,
                             std::string_view entity,
                             std::string_view id_property,
                             const Value& id,
                             std::span<const TraceNode> trace)
        : service_(std::allocator_arg, a, EntityDataServiceAccess::scoped(service, entity)),
          query_(std::allocator_arg, a, entity) {
        query_.filter(Expr::eq(a.resource(), id_property, id));
        for (const auto& frame : trace) {
            query_.trace_chain.emplace_back(frame);
        }
    }

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_) {
            child_ = &EntityDataServiceAccess::fetch_all(service_, query_);
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(r, copy_or_take_error(*child_, *result));
        }
        std::optional<CompactRow> row;
        if (!result->value().empty()) {
            row.emplace(std::allocator_arg, Allocator(r), result->value().back());
        }
        return R::ok(r, std::move(row));
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
    AsyncOperation<RelationRowsResult<E>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::fetch_graph_current_row_internal(const EntityDataService<E>& service,
                                                               std::string_view entity,
                                                               std::string_view property,
                                                               const Value& id,
                                                               std::span<const TraceNode> trace)
    -> AsyncOperation<Result<std::optional<CompactRow>, DataServiceError<typename E::Error>>>& {
    using R = Result<std::optional<CompactRow>, DataServiceError<typename E::Error>>;
    return make_operation<R, GraphCurrentRowOperation<E>>(
        service.memory_resource(), service, entity, property, id, trace);
}

template <class E>
class GraphExecutionOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<GraphNode, Error>;

public:
    using allocator_type = Allocator;

    GraphExecutionOperation(std::allocator_arg_t,
                            Allocator a,
                            const EntityDataService<E>& service,
                            GraphMutationPlan plan)
        : service_(std::allocator_arg, a, service), plan_(std::allocator_arg, a, std::move(plan)) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& context = EntityDataServiceAccess::context(service_);
        if (!plan_.planned_root) {
            return fail(RuntimeError::graph(r, "graph mutation plan has no planned root"));
        }
        while (batch_ < plan_.batches.size()) {
            auto& batch = plan_.batches[batch_];
            if (batch.items.empty() ||
                (batch.kind == GraphMutationKind::Update && batch.update_fields.empty()) ||
                batch.kind == GraphMutationKind::Reference) {
                ++batch_;
                continue;
            }
            if (!mutation_) {
                if (batch.kind == GraphMutationKind::Create) {
                    BatchInsertCommand command(std::allocator_arg, Allocator(r), batch.entity);
                    for (const auto& item : batch.items) {
                        command.batch_values.emplace_back(item.values);
                        command.trace_chains.emplace_back(trace(r, item.scope_token));
                    }
                    mutation_ =
                        &EntityDataServiceAccess::execute_prepared_batch_insert(service_, command);
                } else if (batch.kind == GraphMutationKind::Update) {
                    BatchUpdateCommand command(std::allocator_arg, Allocator(r), batch.entity);
                    for (const auto& field : batch.update_fields) {
                        command.update_fields.emplace_back(field);
                    }
                    for (const auto& item : batch.items) {
                        auto id = item.values.fields.find(std::string_view("id"));
                        if (id == item.values.fields.end()) {
                            return missing_id("update", batch.entity);
                        }
                        command.batch_ids.emplace_back(id->second);
                        command.batch_values.emplace_back(item.values);
                        auto version = item.values.fields.find(std::string_view("version"));
                        command.batch_expected_versions.emplace_back(
                            version != item.values.fields.end() &&
                                    std::holds_alternative<std::int64_t>(version->second.data)
                                ? std::optional<std::int64_t>(
                                      std::get<std::int64_t>(version->second.data))
                                : std::nullopt);
                        command.batch_old_values.emplace_back(clone_in(r, item.old_values));
                        command.trace_chains.emplace_back(trace(r, item.scope_token));
                    }
                    mutation_ =
                        &EntityDataServiceAccess::execute_prepared_batch_update(service_, command);
                } else {
                    auto& item = batch.items[item_];
                    auto id = item.values.fields.find(std::string_view("id"));
                    if (id == item.values.fields.end()) {
                        return missing_id("delete", batch.entity);
                    }
                    DeleteCommand command(
                        std::allocator_arg, Allocator(r), batch.entity, id->second);
                    auto version = item.values.fields.find(std::string_view("version"));
                    if (version != item.values.fields.end()) {
                        if (auto v = std::get_if<std::int64_t>(&version->second.data)) {
                            command.expected_version(*v);
                        }
                    }
                    auto lineage = trace(r, item.scope_token);
                    mutation_ = &EntityDataServiceAccess::delete_scoped_internal(
                        service_, command, lineage);
                }
            }
            forwarding_.attach(mutation_->wake_signal(), wake);
            auto* result = mutation_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            auto* completed = std::exchange(mutation_, nullptr);
            if (!*result) {
                return R::err(r, copy_or_take_error(*completed, *result));
            }
            if (batch.kind == GraphMutationKind::Delete && ++item_ < batch.items.size()) {
                continue;
            }
            ++batch_;
            item_ = 0;
        }
        auto& root = *plan_.planned_root;
        if (root.operation == GraphOperation::Remove) {
            return R::ok(r, std::move(root));
        }
        if (!read_) {
            auto descriptor = context.require_entity(root.entity);
            if (!descriptor) {
                return fail(std::move(descriptor).error());
            }
            auto* property = descriptor.value().get().id_property();
            if (!property) {
                String message("entity ", r);
                message += root.entity;
                message += " has no id property";
                return fail(RuntimeError::graph(r, message));
            }
            auto id = root.values.fields.find(property->name);
            if (id == root.values.fields.end()) {
                String message("saved ", r);
                message += root.entity;
                message += " missing identity field ";
                message += property->name;
                return fail(RuntimeError::graph(r, message));
            }
            TraceChain lineage(r);
            if (root.comment) {
                lineage.emplace_back(
                    TraceKind::AuditReason, root.entity, *root.comment, id->second.try_u64());
            }
            read_ = &EntityDataServiceAccess::fetch_graph_current_row_internal(
                service_, root.entity, property->name, id->second, lineage);
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
            String message("persisted ", r);
            message += root.entity;
            message += " record could not be read back";
            return fail(RuntimeError::graph(r, message));
        }
        root.values.fields = result->value()->into_map(r);
        return R::ok(r, std::move(root));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        mutation_ = nullptr;
        read_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (mutation_) {
            mutation_->cancel();
            mutation_ = nullptr;
        }
        if (read_) {
            read_->cancel();
            read_ = nullptr;
        }
    }

private:
    static TraceChain trace(Memory r, const TraceScopeToken* token) {
        return token ? token->recover_trace_chain(r) : TraceChain(r);
    }

    R fail(RuntimeError error) {
        auto r = service_.memory_resource();
        return R::err(r, Error::runtime(r, std::move(error)));
    }

    R missing_id(std::string_view kind, std::string_view entity) {
        auto r = service_.memory_resource();
        String message(kind, r);
        message += " item in batch missing id for ";
        message += entity;
        return fail(RuntimeError::graph(r, message));
    }

    EntityDataService<E> service_;
    GraphMutationPlan plan_;
    std::size_t batch_ = 0, item_ = 0;
    AsyncOperation<Result<std::uint64_t, Error>>* mutation_ = nullptr;
    AsyncOperation<Result<std::optional<CompactRow>, Error>>* read_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::execute_graph_plan_internal(const EntityDataService<E>& service,
                                                          GraphMutationPlan plan)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    using R = Result<GraphNode, DataServiceError<typename E::Error>>;
    return make_operation<R, GraphExecutionOperation<E>>(
        service.memory_resource(), service, std::move(plan));
}

template <class E>
class GraphSaveOperation {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<GraphNode, Error>;

public:
    using allocator_type = Allocator;

    GraphSaveOperation(std::allocator_arg_t,
                       Allocator a,
                       const EntityDataService<E>& service,
                       GraphNode node)
        : service_(std::allocator_arg, a, service), node_(std::allocator_arg, a, std::move(node)) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& context = EntityDataServiceAccess::context(service_);
        if (!save_) {
            if (!plan_) {
                if (node_.entity != service_.entity()) {
                    String message("entity data service ", r);
                    message += service_.entity();
                    message += " cannot save graph root ";
                    message += node_.entity;
                    return R::err(r, Error::runtime(r, RuntimeError::graph(r, message)));
                }
                plan_ = &service_.plan_graph(std::move(node_));
            }
            forwarding_.attach(plan_->wake_signal(), wake);
            auto* result = plan_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return R::err(r, copy_or_take_error(*plan_, *result));
            }
            auto policy = graph_policy_plan(context, result->value());
            if (!policy) {
                return R::err(r, Error::runtime(r, std::move(policy).error()));
            }
            auto governance = context.review_mutation_plan(policy.value());
            if (!governance) {
                return R::err(r, Error::runtime(r, std::move(governance).error()));
            }
            auto& work = EntityDataServiceAccess::execute_graph_plan_internal(
                service_, clone_in(r, result->value()));
            try {
                save_ = &with_mutation_governance(context, std::move(governance).value(), work);
            } catch (...) {
                work.cancel();
                throw;
            }
        }
        forwarding_.attach(save_->wake_signal(), wake);
        auto* result = save_->poll();
        if (!result) {
            return {};
        }
        return copy_or_take_completed(*save_, *result);
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        plan_ = nullptr;
        save_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (save_) {
            save_->cancel();
            save_ = nullptr;
        }
        if (plan_) {
            plan_->cancel();
            plan_ = nullptr;
        }
    }

private:
    EntityDataService<E> service_;
    GraphNode node_;
    AsyncOperation<Result<GraphMutationPlan, Error>>* plan_ = nullptr;
    AsyncOperation<R>* save_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::save_graph_internal(const EntityDataService<E>& service,
                                                  GraphNode node)
    -> AsyncOperation<Result<GraphNode, DataServiceError<typename E::Error>>>& {
    using R = Result<GraphNode, DataServiceError<typename E::Error>>;
    return make_operation<R, GraphSaveOperation<E>>(
        service.memory_resource(), service, std::move(node));
}
}
