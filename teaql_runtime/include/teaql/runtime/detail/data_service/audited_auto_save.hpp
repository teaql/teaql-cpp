#pragma once
#include <teaql/runtime/detail/data_service/audited_save.hpp>
#include <teaql/runtime/detail/runtime_state_bridge.hpp>

namespace teaql::detail {
template <Entity T, bool Ledger>
class AuditedAutomaticSave {
    using R = Result<T, RuntimeError>;

public:
    using allocator_type = Allocator;

    AuditedAutomaticSave(std::allocator_arg_t,
                         Allocator a,
                         const UserContext& context,
                         Audited<T> audited)
        : context_(context), audited_(std::in_place, std::allocator_arg, a, std::move(audited)) {
        if constexpr (Ledger) {
            static_assert(LedgerEntity<T>);
        }
    }

    // The generic entry has already applied audit and resolved its carrier.
    // Resume the same body under the existing Graph Fix operation wrapper.
    AuditedAutomaticSave(std::allocator_arg_t,
                         Allocator,
                         const UserContext& context,
                         T entity,
                         std::optional<EntityRuntimeState> root)
        : context_(context), prepared_entity_(std::move(entity)), root_(std::move(root)),
          use_ledger_(true) {}

    std::optional<R> poll(WakeSignal& wake) {
        if (scoped_) {
            return poll_scoped(wake);
        }
        auto r = context_.memory_resource();
        if (!save_) {
            auto entity = [&]() -> T {
                if (prepared_entity_) {
                    return std::move(*prepared_entity_);
                }
                // Only the generic source entry samples metadata before applying audit.
                if constexpr (!Ledger) {
                    (void)T::entity_descriptor(r);
                }
                return std::move(*audited_).into_entity();
            }();
            if (!prepared_entity_) {
                use_ledger_ = Ledger || entity.has_pending_dynamic_mutations() ||
                              entity.dynamic_field_values();
                if (use_ledger_) {
                    if constexpr (Ledger) {
                        root_ = entity.entity_runtime_state();
                    } else {
                        root_ = borrow_entity_runtime_state(entity);
                    }
                    if (!root_ && !Ledger) {
                        return fail(RuntimeError::graph(
                            r, "dynamic save requires the runtime mutation ledger carrier"));
                    }
                    if constexpr (!Ledger) {
                        auto& child = make_operation<R, AuditedAutomaticSave>(
                            r, context_, std::move(entity), std::move(root_));
                        scoped_ = &with_graph_fix_scope(context_, child);
                        return poll_scoped(wake);
                    }
                }
            }
            prepared_entity_.reset();
            if (use_ledger_) {
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
            }
            auto node = extract_graph_node_from_entity(context_, std::move(entity));
            if (!node) {
                return fail(std::move(node).error());
            }
            if (!use_ledger_) {
                auto preflight =
                    preflight_graph(context_, node.value(), ObjectLocation::root(r), nullptr);
                if (!preflight) {
                    return fail(std::move(preflight).error());
                }
            }
            // Automatic ledger saving checks this before its ledger preflight;
            // plain AuditedSaveExt checks it after its ordinary preflight.
            auto saver = context_.template require_resource<DynGraphSaver>();
            if (!saver) {
                String message("no DynGraphSaver registered — did you call register_executor()? (",
                               r);
                message += saver.error().to_string(r);
                message += ')';
                return fail(RuntimeError::graph(r, message));
            }
            saver_.emplace(std::allocator_arg, Allocator(r), saver.value());
            if (use_ledger_) {
                if (root_) {
                    auto current = node.value().values.fields.find(std::string_view("id"));
                    Value missing(r, std::int64_t(0));
                    EntityKey root_key(
                        std::allocator_arg,
                        Allocator(r),
                        node.value().entity,
                        current == node.value().values.fields.end() ? missing : current->second);
                    auto cancelled = reject_cancelled_new_root(*root_, root_key);
                    if (!cancelled) {
                        return fail(std::move(cancelled).error());
                    }
                    auto changes = root_->current_change_set();
                    if (auto record = changes.changes().find(root_key);
                        record != changes.changes().end()) {
                        for (const auto& [field, value] : record->second.fields) {
                            node.value().values.fields.insert_or_assign(String(field, r),
                                                                        Value(r, value));
                        }
                    }
                    EntityRuntimeState::Keys visited(r);
                    visited.emplace(root_key);
                    auto hydrate =
                        hydrate_ledger_relations(context_, *root_, node.value(), visited);
                    if (!hydrate) {
                        return fail(std::move(hydrate).error());
                    }
                    auto preflight =
                        preflight_graph(context_, node.value(), ObjectLocation::root(r), &*root_);
                    if (!preflight) {
                        return fail(std::move(preflight).error());
                    }
                    auto merge = merge_relation_mutations_into_root(*root_, node.value());
                    if (!merge) {
                        return fail(std::move(merge).error());
                    }
                    auto pending =
                        preflight_pending_dynamic_entities(context_, *root_, node.value());
                    if (!pending) {
                        return fail(std::move(pending).error());
                    }
                    auto captured = capture_ledger_trace_scopes(*root_, node.value());
                    if (!captured) {
                        return fail(std::move(captured).error());
                    }
                    bool dirty = root_->has_pending_dynamic_mutations() ||
                                 !root_->current_change_set().changes().empty() ||
                                 !root_->deleted_keys().empty() || !root_->new_keys().empty();
                    if (dirty) {
                        auto descriptor = context_.require_entity(node.value().entity);
                        if (!descriptor) {
                            return fail(std::move(descriptor).error());
                        }
                        preserve_ = can_preserve_loaded_relations(
                            *root_, root_key, descriptor.value().get());
                        save_ = &saver_->get().save_ledger_dyn(
                            context_, std::move(node).value(), *root_);
                        ledger_path_ = true;
                    }
                }
                if (!save_) {
                    auto preflight =
                        preflight_graph(context_, node.value(), ObjectLocation::root(r), nullptr);
                    if (!preflight) {
                        return fail(std::move(preflight).error());
                    }
                }
            }
            if (!save_) {
                save_ = &saver_->get().save_graph_dyn(context_, std::move(node).value());
            }
        }
        forwarding_.attach(save_->wake_signal(), wake);
        auto* result = save_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return fail(clone_in(r, result->error()));
        }
        auto values = result->value().values.into_map(r);
        CompactRow row(std::allocator_arg, Allocator(r), values);
        auto entity = ledger_path_ && preserve_
                          ? T::from_compact_row_with_context(r, std::move(row), AnyRef(*root_))
                          : T::from_compact_row(r, std::move(row));
        if (!entity) {
            return fail(RuntimeError::graph(r, entity.error().to_string(r)));
        }
        if (use_ledger_) {
            auto installed = install_saved_dynamic_fields(
                r, entity.value(), result->value().dynamic_fields.get());
            if (!installed) {
                return fail(std::move(installed).error());
            }
        }
        return R::ok(r, std::move(entity).value());
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        save_ = nullptr;
        scoped_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (scoped_) {
            scoped_->cancel();
            scoped_ = nullptr;
        }
        if (save_) {
            save_->cancel();
            save_ = nullptr;
        }
    }

private:
    std::optional<R> poll_scoped(WakeSignal& wake) {
        forwarding_.attach(scoped_->wake_signal(), wake);
        const auto* result = scoped_->poll();
        if (!result) {
            return {};
        }
        return copy_or_take_completed(*scoped_, *result);
    }

    R fail(RuntimeError error) {
        return R::err(context_.memory_resource(), std::move(error));
    }

    const UserContext& context_;
    std::optional<Audited<T>> audited_;
    std::optional<T> prepared_entity_;
    std::optional<EntityRuntimeState> root_;
    AsyncOperation<R>* scoped_ = nullptr;
    bool use_ledger_ = Ledger;
    std::optional<ResourceBorrow<const DynGraphSaver>> saver_;
    AsyncOperation<GraphSaveResult>* save_ = nullptr;
    bool preserve_ = false, ledger_path_ = false;
    WakeSignal::Forward forwarding_;
};
}

namespace teaql {
// Runtime extension trait adapter: core Audited remains independent of runtime.
template <teaql::Entity T>
struct AuditedSaveExt {
    using Entity = T;

    static AsyncOperation<Result<T, RuntimeError>>& save(Audited<T> audited,
                                                         const UserContext& context) {
        if constexpr (requires { T::generated_ledger_carrier; }) {
            static_assert(T::generated_ledger_carrier && LedgerEntity<T>);
            auto& child =
                make_operation<Result<T, RuntimeError>, detail::AuditedAutomaticSave<T, true>>(
                    context.memory_resource(), context, std::move(audited));
            return detail::with_graph_fix_scope(context, child);
        } else {
            return make_operation<Result<T, RuntimeError>, detail::AuditedAutomaticSave<T, false>>(
                context.memory_resource(), context, std::move(audited));
        }
    }
};

template <LedgerEntity T>
AsyncOperation<Result<T, RuntimeError>>& save_audited_ledger_entity(Audited<T> audited,
                                                                    const UserContext& context) {
    using R = Result<T, RuntimeError>;
    auto& child = make_operation<R, detail::AuditedAutomaticSave<T, true>>(
        context.memory_resource(), context, std::move(audited));
    return detail::with_graph_fix_scope(context, child);
}
}
