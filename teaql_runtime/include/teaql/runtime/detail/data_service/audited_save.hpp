#pragma once
#include <teaql/runtime/detail/data_service/graph_saver.hpp>

namespace teaql {
// Allocator-aware equivalent of the source (entity, optional pending ledger).
template <LedgerEntity T>
struct AuditedLedgerResult {
    using allocator_type = Allocator;
    T entity;
    std::optional<EntityRuntimeState> ledger;

    AuditedLedgerResult(std::allocator_arg_t,
                        Allocator a,
                        T entity,
                        const std::optional<EntityRuntimeState>& ledger)
        : AuditedLedgerResult(a, make_parts(a, std::move(entity), ledger)) {}

    AuditedLedgerResult(std::allocator_arg_t, Allocator a, const AuditedLedgerResult& other)
        requires detail::domain_copyable<T>
        : AuditedLedgerResult(a, copy_parts(a, other)) {}

    AuditedLedgerResult(std::allocator_arg_t, Allocator a, AuditedLedgerResult&& other)
        : AuditedLedgerResult(a, move_parts(a, std::move(other))) {}

    AuditedLedgerResult(const AuditedLedgerResult&) = delete;
    AuditedLedgerResult(AuditedLedgerResult&&) noexcept = default;

private:
    struct Parts {
        T entity;
        std::optional<EntityRuntimeState> ledger;
    };

    AuditedLedgerResult(Allocator a, Parts parts)
        : entity(std::make_obj_using_allocator<T>(a, std::move(parts.entity))),
          ledger(detail::move_in(a.resource(), std::move(parts.ledger))) {}

    static Parts copy_parts(Allocator a, const AuditedLedgerResult& other) {
        std::optional<Parts> parts;
        auto copy = [&] {
            parts.emplace(Parts{std::make_obj_using_allocator<T>(a, other.entity),
                                detail::clone_in(a.resource(), other.ledger)});
        };
        detail::with_entity_runtime_copy(a.resource(), copy);
        return std::move(*parts);
    }

    static Parts move_parts(Allocator a, AuditedLedgerResult&& other) {
        std::optional<Parts> parts;
        auto copy = [&] {
            parts.emplace(Parts{std::make_obj_using_allocator<T>(a, std::move(other.entity)),
                                detail::move_in(a.resource(), std::move(other.ledger))});
        };
        detail::with_entity_runtime_copy(a.resource(), copy);
        return std::move(*parts);
    }

    static Parts
    make_parts(Allocator a, T entity, const std::optional<EntityRuntimeState>& ledger) {
        std::optional<Parts> parts;
        auto copy = [&] {
            parts.emplace(Parts{std::make_obj_using_allocator<T>(a, std::move(entity)),
                                detail::clone_in(a.resource(), ledger)});
        };
        detail::with_entity_runtime_copy(a.resource(), copy);
        return std::move(*parts);
    }
};

namespace detail {
template <LedgerEntity T, class E>
class AuditedWithExecutor {
    using Error = DataServiceError<typename E::Error>;
    using R = Result<AuditedLedgerResult<T>, RuntimeError>;
    enum class Phase { Start, Ledger, Read, Graph };

public:
    using allocator_type = Allocator;

    AuditedWithExecutor(std::allocator_arg_t,
                        Allocator a,
                        const UserContext& context,
                        Audited<T> audited,
                        const ResourceBorrow<E>& executor)
        : context_(context), audited_(std::allocator_arg, a, std::move(audited)),
          executor_(std::allocator_arg, a, executor), id_property_(a.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = context_.memory_resource();
        if (phase_ == Phase::Start) {
            auto entity = std::move(audited_).into_entity();
            root_ = entity.entity_runtime_state();
            if (root_) {
                if (auto error = EntityRuntimeAccess::first_composition_error(*root_)) {
                    String message("generated entity graph attachment failed before save: ", r);
                    message += error->to_string(r);
                    return fail(RuntimeError::graph(r, message));
                }
            }
            if (root_ && root_->has_pending_dynamic_mutations()) {
                auto provider = context_.dynamic_graph_provider();
                if (!provider) {
                    return fail(std::move(provider).error());
                }
            }
            auto node = extract_graph_node_from_entity(context_, std::move(entity));
            if (!node) {
                return fail(std::move(node).error());
            }
            node_.emplace(std::allocator_arg, Allocator(r), std::move(node).value());
            if (root_) {
                auto current = node_->values.fields.find(std::string_view("id"));
                Value missing(r, std::int64_t(0));
                root_key_.emplace(std::allocator_arg,
                                  Allocator(r),
                                  node_->entity,
                                  current == node_->values.fields.end() ? missing
                                                                        : current->second);
                auto cancelled = reject_cancelled_new_root(*root_, *root_key_);
                if (!cancelled) {
                    return fail(std::move(cancelled).error());
                }
                auto changes = root_->current_change_set();
                if (auto record = changes.changes().find(*root_key_);
                    record != changes.changes().end()) {
                    for (const auto& [field, value] : record->second.fields) {
                        node_->values.fields.insert_or_assign(String(field, r), Value(r, value));
                    }
                }
                EntityRuntimeState::Keys visited(r);
                visited.emplace(*root_key_);
                auto hydrate = hydrate_ledger_relations(context_, *root_, *node_, visited);
                if (!hydrate) {
                    return fail(std::move(hydrate).error());
                }
                auto preflight =
                    preflight_graph(context_, *node_, ObjectLocation::root(r), &*root_);
                if (!preflight) {
                    return fail(std::move(preflight).error());
                }
                auto merge = merge_relation_mutations_into_root(*root_, *node_);
                if (!merge) {
                    return fail(std::move(merge).error());
                }
                auto pending = preflight_pending_dynamic_entities(context_, *root_, *node_);
                if (!pending) {
                    return fail(std::move(pending).error());
                }
                auto captured = capture_ledger_trace_scopes(*root_, *node_);
                if (!captured) {
                    return fail(std::move(captured).error());
                }
                bool dirty = root_->has_pending_dynamic_mutations() ||
                             !root_->current_change_set().changes().empty() ||
                             !root_->deleted_keys().empty() || !root_->new_keys().empty();
                if (dirty) {
                    auto descriptor = context_.require_entity(node_->entity);
                    if (!descriptor) {
                        return fail(std::move(descriptor).error());
                    }
                    preserve_ =
                        can_preserve_loaded_relations(*root_, *root_key_, descriptor.value().get());
                    auto* id = descriptor.value().get().id_property();
                    if (!id) {
                        String message("entity ", r);
                        message += node_->entity;
                        message += " has no id property";
                        return fail(RuntimeError::graph(r, message));
                    }
                    id_property_ = id->name;
                    service_ = &context_.template make<EntityDataService<E>>(
                        context_, node_->entity, executor_);
                    auto bound = EntityDataServiceAccess::bind_mutation_intent(
                        *service_,
                        node_->comment ? std::optional<std::string_view>(*node_->comment)
                                       : std::nullopt);
                    if (!bound) {
                        return fail(std::move(bound).error());
                    }
                    auto privacy = EntityDataServiceAccess::bind_ledger_privacy(*service_, *root_);
                    if (!privacy) {
                        return fail(std::move(privacy).error());
                    }
                    auto locations = ledger_object_locations(r, *node_);
                    auto policy = ledger_policy_plan(context_, *node_, *root_);
                    auto governance = context_.review_mutation_plan(policy);
                    if (!governance) {
                        return fail(std::move(governance).error());
                    }
                    ledger_ = &EntityDataServiceAccess::execute_ledger_plan_internal(
                        *service_, *root_, locations);
                    ledger_ = &with_mutation_governance(context_, governance.value(), *ledger_);
                    phase_ = Phase::Ledger;
                }
            }
            if (phase_ == Phase::Start) {
                // Source deliberately performs this second pass when a runtime
                // state exists but its post-preflight ledger is still empty.
                auto preflight =
                    preflight_graph(context_, *node_, ObjectLocation::root(r), nullptr);
                if (!preflight) {
                    return fail(std::move(preflight).error());
                }
                service_ = &context_.template make<EntityDataService<E>>(
                    context_, node_->entity, executor_);
                graph_ =
                    &EntityDataServiceAccess::save_graph_internal(*service_, std::move(*node_));
                phase_ = Phase::Graph;
            }
        }
        if (phase_ == Phase::Ledger) {
            forwarding_.attach(ledger_->wake_signal(), wake);
            auto* result = ledger_->poll();
            if (!result) {
                return {};
            }
            forwarding_.reset();
            if (!*result) {
                return fail(save_runtime_error(r, result->error()));
            }
            if (auto* readbacks = result->value().dynamic_readbacks.get()) {
                if (auto fields = readbacks->rows.find(*root_key_);
                    fields != readbacks->rows.end()) {
                    node_->dynamic_fields = make_box<DynamicFieldValues>(r, fields->second);
                }
            }
            if (auto generated = result->value().find(*root_key_);
                generated != result->value().end()) {
                node_->values.fields.insert_or_assign(String(id_property_, r),
                                                      Value(r, generated->second));
            }
            auto id = node_->values.fields.find(id_property_);
            if (id == node_->values.fields.end()) {
                String message("saved ", r);
                message += node_->entity;
                message += " missing identity field ";
                message += id_property_;
                return fail(RuntimeError::graph(r, message));
            }
            // This reread is outside the source ledger governance scope.
            read_ = &EntityDataServiceAccess::fetch_graph_current_row_internal(
                *service_,
                node_->entity,
                id_property_,
                id->second,
                root_->get_trace_chain(*root_key_));
            phase_ = Phase::Read;
        }
        if (phase_ == Phase::Read) {
            forwarding_.attach(read_->wake_signal(), wake);
            auto* result = read_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return fail(save_runtime_error(r, result->error()));
            }
            if (!result->value()) {
                String message("persisted ", r);
                message += node_->entity;
                message += " record could not be read back";
                return fail(RuntimeError::graph(r, message));
            }
            return materialize(clone_in(r, *result->value()), true);
        }
        forwarding_.attach(graph_->wake_signal(), wake);
        auto* result = graph_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return fail(save_runtime_error(r, result->error()));
        }
        auto values = result->value().values.into_map(r);
        return materialize(CompactRow(std::allocator_arg, Allocator(r), values), false);
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (ledger_) {
            ledger_->cancel();
        }
        if (read_) {
            read_->cancel();
        }
        if (graph_) {
            graph_->cancel();
        }
    }

private:
    R fail(RuntimeError error) {
        return R::err(context_.memory_resource(), std::move(error));
    }

    R materialize(CompactRow row, bool ledger) {
        auto r = context_.memory_resource();
        auto entity = ledger && preserve_
                          ? T::from_compact_row_with_context(r, std::move(row), AnyRef(*root_))
                          : T::from_compact_row(r, std::move(row));
        if (!entity) {
            return fail(RuntimeError::graph(r, entity.error().to_string(r)));
        }
        auto installed =
            install_saved_dynamic_fields(r, entity.value(), node_->dynamic_fields.get());
        if (!installed) {
            return fail(std::move(installed).error());
        }
        std::optional<EntityRuntimeState> none;
        const auto& pending = ledger ? root_ : none;
        AuditedLedgerResult<T> result(
            std::allocator_arg, Allocator(r), std::move(entity).value(), pending);
        return R::ok(r, std::move(result));
    }

    const UserContext& context_;
    Audited<T> audited_;
    ResourceBorrow<E> executor_;
    std::optional<EntityRuntimeState> root_;
    std::optional<GraphNode> node_;
    std::optional<EntityKey> root_key_;
    String id_property_;
    bool preserve_ = false;
    EntityDataService<E>* service_ = nullptr;
    AsyncOperation<Result<LedgerExecutionOutcome, Error>>* ledger_ = nullptr;
    AsyncOperation<Result<std::optional<CompactRow>, Error>>* read_ = nullptr;
    AsyncOperation<Result<GraphNode, Error>>* graph_ = nullptr;
    Phase phase_ = Phase::Start;
    WakeSignal::Forward forwarding_;
};
}

template <LedgerEntity T, class E>
AsyncOperation<Result<AuditedLedgerResult<T>, RuntimeError>>&
save_audited_ledger_entity_with_executor(Audited<T> audited,
                                         const UserContext& context,
                                         const ResourceBorrow<E>& executor) {
    using R = Result<AuditedLedgerResult<T>, RuntimeError>;
    auto& work = make_operation<R, detail::AuditedWithExecutor<T, E>>(
        context.memory_resource(), context, std::move(audited), executor);
    return detail::with_graph_fix_scope(context, work);
}

template <LedgerEntity T, class E>
AsyncOperation<Result<AuditedLedgerResult<T>, RuntimeError>>&
save_audited_ledger_entity_with_executor(Audited<T> audited,
                                         const UserContext& context,
                                         Ref<E> executor) {
    auto borrow = detail::ContextEntityAccess::executor_borrow(context, executor);
    return save_audited_ledger_entity_with_executor(std::move(audited), context, borrow);
}
}
