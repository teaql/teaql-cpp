#pragma once
#include <teaql/runtime/detail/data_service/ledger_execution.hpp>
#include <teaql/runtime/detail/commit_audit.hpp>

namespace teaql::detail {
using GraphSaveResult = Result<GraphNode, RuntimeError>;

class DynGraphSaver {
public:
    virtual ~DynGraphSaver() = default;
    virtual AsyncOperation<GraphSaveResult>& save_graph_dyn(const UserContext&,
                                                            GraphNode) const = 0;
    virtual AsyncOperation<GraphSaveResult>&
    save_ledger_dyn(const UserContext&, GraphNode, const EntityRuntimeState&) const = 0;
};

template <class E>
RuntimeError save_runtime_error(Memory r, const DataServiceError<E>& error) {
    if (error.kind() == DataServiceError<E>::Kind::Runtime) {
        return clone_in(r, error.runtime_error());
    }
    return RuntimeError::graph(r, error.to_string(r));
}

template <class E>
RuntimeError save_provider_error(Memory r, E error) {
    return RuntimeError::graph(r, DataServiceError<E>::executor(r, std::move(error)).to_string(r));
}

// The ledger write and its authoritative root read share one governance scope.
template <class E>
class LedgerSaveExecution {
    using Error = DataServiceError<E>;
    using R = Result<GraphNode, Error>;
    using Tx = DataServiceTransaction<E>;

public:
    using allocator_type = Allocator;

    LedgerSaveExecution(std::allocator_arg_t,
                        Allocator a,
                        const UserContext& context,
                        Tx& transaction,
                        const GraphNode& node,
                        const EntityRuntimeState& root,
                        const EntityKey& root_key,
                        std::string_view id_property)
        : service_(
              std::allocator_arg,
              a,
              context,
              node.entity,
              ResourceBorrow<Tx>(std::allocator_arg, a, transaction, resource_arena(a.resource()))),
          node_(std::allocator_arg, a, node), root_(std::allocator_arg, a, root),
          root_key_(std::allocator_arg, a, root_key), id_property_(id_property, a.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!read_) {
            if (!ledger_) {
                auto bound = EntityDataServiceAccess::bind_mutation_intent(
                    service_,
                    node_.comment ? std::optional<std::string_view>(*node_.comment) : std::nullopt);
                if (!bound) {
                    return R::err(r, Error::runtime(r, std::move(bound).error()));
                }
                auto privacy = EntityDataServiceAccess::bind_ledger_privacy(service_, root_);
                if (!privacy) {
                    return R::err(r, Error::runtime(r, std::move(privacy).error()));
                }
                auto locations = ledger_object_locations(r, node_);
                ledger_ = &EntityDataServiceAccess::execute_ledger_plan_internal(
                    service_, root_, locations);
            }
            forwarding_.attach(ledger_->wake_signal(), wake);
            auto* result = ledger_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return R::err(r, copy_or_take_error(*ledger_, *result));
            }
            if (auto* readbacks = result->value().dynamic_readbacks.get()) {
                if (auto fields = readbacks->rows.find(root_key_);
                    fields != readbacks->rows.end()) {
                    node_.dynamic_fields = make_box<DynamicFieldValues>(r, fields->second);
                }
            }
            if (auto generated = result->value().find(root_key_);
                generated != result->value().end()) {
                node_.values.fields.insert_or_assign(String(id_property_, r),
                                                     Value(r, generated->second));
            }
            auto id = node_.values.fields.find(id_property_);
            if (id == node_.values.fields.end()) {
                String message("saved ", r);
                message += node_.entity;
                message += " missing identity field ";
                message += id_property_;
                return fail(message);
            }
            read_ = &EntityDataServiceAccess::fetch_graph_current_row_internal(
                service_, node_.entity, id_property_, id->second, root_.get_trace_chain(root_key_));
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
            message += node_.entity;
            message += " record could not be read back";
            return fail(message);
        }
        node_.values.fields = result->value()->into_map(r);
        return R::ok(r, std::move(node_));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        ledger_ = nullptr;
        read_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (ledger_) {
            ledger_->cancel();
            ledger_ = nullptr;
        }
        if (read_) {
            read_->cancel();
            read_ = nullptr;
        }
    }

private:
    R fail(std::string_view message) {
        auto r = service_.memory_resource();
        return R::err(r, Error::runtime(r, RuntimeError::graph(r, message)));
    }

    EntityDataService<Tx> service_;
    GraphNode node_;
    EntityRuntimeState root_;
    EntityKey root_key_;
    String id_property_;
    AsyncOperation<Result<LedgerExecutionOutcome, Error>>* ledger_ = nullptr;
    AsyncOperation<Result<std::optional<CompactRow>, Error>>* read_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
class TransactionalGraphSave {
    using ProviderError = typename E::Error;
    using Error = DataServiceError<ProviderError>;
    using Tx = DataServiceTransaction<ProviderError>;
    using Execution = Result<GraphNode, Error>;
    enum class Phase { Prepare, Begin, Execute, Terminal };

public:
    using allocator_type = Allocator;

    TransactionalGraphSave(std::allocator_arg_t,
                           Allocator a,
                           const UserContext& context,
                           GraphNode node,
                           const EntityRuntimeState* root)
        : context_(context), node_(std::allocator_arg, a, std::move(node)),
          id_property_(a.resource()) {
        if (root) {
            root_.emplace(std::allocator_arg, a, *root);
        }
    }

    std::optional<GraphSaveResult> poll(WakeSignal& wake) {
        auto r = context_.memory_resource();
        if (phase_ == Phase::Prepare) {
            if (root_ && root_->has_pending_dynamic_mutations()) {
                auto provider = context_.dynamic_graph_provider();
                if (!provider) {
                    return fail(std::move(provider).error());
                }
            }
            if (root_) {
                auto captured = capture_ledger_trace_scopes(*root_, node_);
                if (!captured) {
                    return fail(std::move(captured).error());
                }
            }
            if (!executor_) {
                auto executor = ContextEntityAccess::require_executor<E>(context_);
                if (!executor) {
                    return fail(RuntimeError::graph(r, executor.error().to_string(r)));
                }
                executor_.emplace(std::allocator_arg, Allocator(r), executor.value());
            }
            if (root_) {
                auto descriptor = context_.require_entity(node_.entity);
                if (!descriptor) {
                    return fail(std::move(descriptor).error());
                }
                auto* id = descriptor.value().get().id_property();
                if (!id) {
                    String message("entity ", r);
                    message += node_.entity;
                    message += " has no id property";
                    return fail(RuntimeError::graph(r, message));
                }
                id_property_ = id->name;
                auto current = node_.values.fields.find(id_property_);
                Value missing(r, std::int64_t(0));
                root_key_.emplace(std::allocator_arg,
                                  Allocator(r),
                                  node_.entity,
                                  current == node_.values.fields.end() ? missing : current->second);
                auto cancelled = reject_cancelled_new_root(*root_, *root_key_);
                if (!cancelled) {
                    return fail(std::move(cancelled).error());
                }
                auto policy = ledger_policy_plan(context_, node_, *root_);
                auto governance = context_.review_mutation_plan(policy);
                if (!governance) {
                    return fail(std::move(governance).error());
                }
                governance_.emplace(
                    std::allocator_arg, Allocator(r), std::move(governance).value());
            } else {
                if (!planning_) {
                    auto& service = context_.template make<EntityDataService<E>>(
                        context_, node_.entity, *executor_);
                    planning_ = &service.plan_graph(clone_in(r, node_));
                }
                forwarding_.attach(planning_->wake_signal(), wake);
                auto* result = planning_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return fail(save_runtime_error(r, result->error()));
                }
                plan_.emplace(std::allocator_arg, Allocator(r), result->value());
                auto policy = graph_policy_plan(context_, *plan_);
                if (!policy) {
                    return fail(std::move(policy).error());
                }
                auto governance = context_.review_mutation_plan(policy.value());
                if (!governance) {
                    return fail(std::move(governance).error());
                }
                governance_.emplace(
                    std::allocator_arg, Allocator(r), std::move(governance).value());
            }
            phase_ = Phase::Begin;
        }
        if (phase_ == Phase::Begin) {
            if (!begin_) {
                begin_ = &executor_->get().begin(r);
            }
            forwarding_.attach(begin_->wake_signal(), wake);
            auto* result = begin_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            auto* completed = std::exchange(begin_, nullptr);
            if (!*result) {
                return fail(save_provider_error(r, copy_or_take_error(*completed, *result)));
            }
            transaction_ = &result->value().get();
            if (root_) {
                execution_ = &make_operation<Execution, LedgerSaveExecution<ProviderError>>(
                    r, context_, *transaction_, node_, *root_, *root_key_, id_property_);
            } else {
                ResourceBorrow<Tx> borrow(
                    std::allocator_arg, Allocator(r), *transaction_, resource_arena(r));
                auto& service =
                    context_.template make<EntityDataService<Tx>>(context_, node_.entity, borrow);
                execution_ = &EntityDataServiceAccess::execute_graph_plan_internal(
                    service, std::move(*plan_));
            }
            // Retain the unwrapped child if wrapper allocation throws; cancel
            // and join it before releasing the acquired native transaction.
            execution_ = &with_mutation_governance(context_, *governance_, *execution_);
            audits_ = make_box<PendingAuditBatch>(r, context_);
            phase_ = Phase::Execute;
        }
        if (phase_ == Phase::Execute) {
            forwarding_.attach(execution_->wake_signal(), wake);
            const Execution* result;
            {
                AuditActivation active(context_, *audits_);
                result = execution_->poll();
            }
            if (!result) {
                return {};
            }
            forwarding_.reset();
            if (*result) {
                saved_.emplace(std::allocator_arg, Allocator(r), result->value());
                terminal_ = &transaction_->commit(r);
            } else {
                error_ = &result->error();
                terminal_ = &transaction_->rollback(r);
            }
            phase_ = Phase::Terminal;
        }
        forwarding_.attach(terminal_->wake_signal(), wake);
        auto* completed = terminal_->poll();
        if (!completed) {
            return {};
        }
        if (!*completed) {
            return fail(save_provider_error(r, copy_or_take_error(*terminal_, *completed)));
        }
        if (error_) {
            return fail(save_runtime_error(r, *error_));
        }
        if (root_) {
            root_->clear_committed();
        }
        auto delivered = deliver_committed_audits(context_, std::move(audits_->events));
        if (!delivered) {
            return fail(std::move(delivered).error());
        }
        return GraphSaveResult::ok(r, std::move(*saved_));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        release_transaction();
        audits_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (planning_) {
            planning_->cancel();
        }
        if (begin_) {
            begin_->cancel();
        }
        if (execution_) {
            execution_->cancel();
        }
        if (terminal_) {
            terminal_->cancel();
        }
        release_transaction();
        audits_.reset();
    }

private:
    GraphSaveResult fail(RuntimeError error) {
        return GraphSaveResult::err(context_.memory_resource(), std::move(error));
    }

    void release_transaction() noexcept {
        if (transaction_) {
            transaction_->cancel();
            transaction_ = nullptr;
        }
    }

    const UserContext& context_;
    GraphNode node_;
    std::optional<EntityRuntimeState> root_;
    std::optional<EntityKey> root_key_;
    String id_property_;
    std::optional<ResourceBorrow<E>> executor_;
    std::optional<GraphMutationPlan> plan_;
    std::optional<MutationGovernanceSnapshot> governance_;
    std::optional<GraphNode> saved_;
    ScopedBox<PendingAuditBatch> audits_;
    const Error* error_ = nullptr;
    AsyncOperation<Result<GraphMutationPlan, Error>>* planning_ = nullptr;
    AsyncOperation<Result<TransactionHandle<ProviderError>, ProviderError>>* begin_ = nullptr;
    AsyncOperation<Execution>* execution_ = nullptr;
    AsyncOperation<Result<std::monostate, ProviderError>>* terminal_ = nullptr;
    Tx* transaction_ = nullptr;
    Phase phase_ = Phase::Prepare;
    WakeSignal::Forward forwarding_;
};

template <class E>
class GraphSaverFor final : public DynGraphSaver {
public:
    using allocator_type = Allocator;
    static_assert(std::derived_from<E, QueryExecutor<typename E::Error>> &&
                  std::derived_from<E, MutationExecutor<typename E::Error>> &&
                  std::derived_from<E, TransactionExecutor<typename E::Error>>);

    GraphSaverFor(std::allocator_arg_t, Allocator) {}

    AsyncOperation<GraphSaveResult>& save_graph_dyn(const UserContext& context,
                                                    GraphNode node) const override {
        return make_operation<GraphSaveResult, TransactionalGraphSave<E>>(
            context.memory_resource(), context, std::move(node), nullptr);
    }

    AsyncOperation<GraphSaveResult>& save_ledger_dyn(
        const UserContext& context, GraphNode node, const EntityRuntimeState& root) const override {
        return make_operation<GraphSaveResult, TransactionalGraphSave<E>>(
            context.memory_resource(), context, std::move(node), &root);
    }
};
}

namespace teaql {
template <class Owner, class E>
void UserContext::register_executor(Owner& owner, E& executor) {
    auto& saver = make<detail::GraphSaverFor<E>>();
    insert_resource(*this, static_cast<detail::DynGraphSaver&>(saver));
    insert_resource(owner, executor);
}

template <class E>
void UserContext::register_executor(Ref<E> executor) {
    (void)get(executor);
    auto& saver = make<detail::GraphSaverFor<E>>();
    insert_resource(*this, static_cast<detail::DynGraphSaver&>(saver));
    insert_resource(executor);
}
}
