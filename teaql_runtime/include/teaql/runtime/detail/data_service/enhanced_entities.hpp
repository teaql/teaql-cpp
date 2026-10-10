#pragma once
#include <teaql/runtime/detail/data_service/id_set_page.hpp>

namespace teaql::detail {
template <class E, Entity T>
class EnhancedEntitiesOperation {
    using Error = typename E::Error;
    using R = Result<SmartList<T>, DataServiceError<Error>>;
    using P = Result<IdSetPage, DataServiceError<Error>>;
    using H = RelationHydrationResult<E>;

public:
    using allocator_type = Allocator;

    EnhancedEntitiesOperation(std::allocator_arg_t,
                              Allocator a,
                              const EntityDataService<E>& service,
                              const SelectQuery& query,
                              std::span<const RelationAggregate> aggregates)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query),
          aggregates_(aggregates.begin(), aggregates.end(), a.resource()), rows_(a.resource()),
          fetch_(std::allocator_arg,
                 a,
                 [](const EntityDataService<E>& child, SelectQuery query) -> auto& {
                     return EntityDataServiceAccess::fetch_all(child, query);
                 }) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        for (;;) {
            if (phase_ == Phase::Prepare) {
                auto prepared = EntityDataServiceAccess::prepare_select(service_, query_);
                if (!prepared) {
                    return runtime(std::move(prepared).error());
                }
                query_ = std::move(prepared).value();
                ensure_identity_projection(context(), query_);
                const auto& cs = EntityDataServiceAccess::context_service(service_);
                if (!EntityDataServiceAccess::has_intent(service_) &&
                    cs.metadata.capture_execution_metadata()) {
                    service_ = EntityDataServiceAccess::query_scoped(service_);
                }
                page_ = &prepare_id_set_page(service_, query_);
                phase_ = Phase::Page;
            }
            if (phase_ == Phase::Page) {
                forwarding_.attach(page_->wake_signal(), wake);
                auto* result = page_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                auto* completed = std::exchange(page_, nullptr);
                if (!*result) {
                    return R::err(r, copy_or_take_error(*completed, *result));
                }
                query_ = clone_in(r, result->value().query);
                total_ = result->value().total_count;
                direct_ = aggregates_.empty() && !query_.continuous_page_fetch &&
                          query_.object_group_bys.empty() && query_.child_enhancements.empty() &&
                          query_.relations.empty();
                if (!direct_) {
                    auto plans = EntityDataServiceAccess::flat_relation_plans(service_, query_);
                    if (!plans) {
                        return runtime(std::move(plans).error());
                    }
                    if (plans.value()) {
                        plans_.emplace(std::allocator_arg, Allocator(r), *plans.value());
                        query_.relations.clear();
                    }
                    compact_ = aggregates_.empty() && !query_.continuous_page_fetch &&
                               query_.object_group_bys.empty() &&
                               query_.child_enhancements.empty() && bool(plans_);
                }
                if (direct_ || compact_) {
                    try {
                        query_ = query_.prepare_for_list();
                    } catch (const std::invalid_argument& error) {
                        return runtime(RuntimeError::graph(r, error.what()));
                    }
                    row_work_ =
                        &make_operation<RelationRowsResult<E>, PreparedEntityQueryOperation<E>>(
                            r, service_, query_, true, compact_);
                } else {
                    row_work_ = &make_operation<RelationRowsResult<E>, EntityRowsOperation<E>>(
                        r,
                        service_,
                        query_,
                        std::span<const RelationAggregate>(aggregates_),
                        true,
                        true);
                }
                phase_ = Phase::Fetch;
            }
            if (phase_ == Phase::Fetch) {
                forwarding_.attach(row_work_->wake_signal(), wake);
                auto* result = row_work_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                auto* completed = std::exchange(row_work_, nullptr);
                if (!*result) {
                    return R::err(r, copy_or_take_error(*completed, *result));
                }
                rows_ = Vector<CompactRow>(result->value(), r);
                if (direct_) {
                    root_ = &context().template make<EntityRuntimeState>();
                    return decode();
                }
                traced_ = &context().template make<EntityDataService<E>>(
                    EntityDataServiceAccess::scoped(service_, query_.entity));
                traced_->with_trace_context(query_.trace_chain);
                if (plans_) {
                    root_ = &context().template make<EntityRuntimeState>();
                    graph_ = &context().template make<EntityGraphBuilder>();
                    phase_ = Phase::QueryPlans;
                } else {
                    auto plans = traced_->relation_plans();
                    if (!plans) {
                        return runtime(std::move(plans).error());
                    }
                    hydration_ = &enhance_relation_plans(*traced_, rows_, plans.value(), fetch_);
                    phase_ = Phase::Embedded;
                }
            }
            if (phase_ == Phase::QueryPlans || phase_ == Phase::BehaviorPlans) {
                if (!hydration_) {
                    const auto& plans =
                        phase_ == Phase::QueryPlans ? plans_->query : plans_->behavior;
                    hydration_ =
                        compact_
                            ? &hydrate_compact_flat_plans(
                                  *traced_, rows_, plans, *root_, *graph_, fetch_)
                            : &hydrate_flat_plans(*traced_, rows_, plans, *root_, *graph_, fetch_);
                }
                forwarding_.attach(hydration_->wake_signal(), wake);
                auto* result = hydration_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                auto* completed = std::exchange(hydration_, nullptr);
                if (!*result) {
                    return R::err(r, copy_or_take_error(*completed, *result));
                }
                if (phase_ == Phase::QueryPlans) {
                    phase_ = Phase::BehaviorPlans;
                    continue;
                }
                if (!root_->freeze_graph(std::move(*graph_))) {
                    return entity(
                        EntityError(std::allocator_arg,
                                    Allocator(r),
                                    compact_ ? T::ENTITY_NAME : std::string_view(query_.entity),
                                    "identity graph was already frozen"));
                }
                return decode();
            }
            if (phase_ == Phase::Embedded) {
                forwarding_.attach(hydration_->wake_signal(), wake);
                auto* result = hydration_->poll();
                if (!result) {
                    return {};
                }
                forwarding_.reset();
                auto* completed = std::exchange(hydration_, nullptr);
                if (!*result) {
                    return R::err(r, copy_or_take_error(*completed, *result));
                }
                auto root = attach_flat_relation_graph(context(), query_.entity, rows_);
                if (!root) {
                    return entity(std::move(root).error());
                }
                root_ = &context().template make<EntityRuntimeState>(std::move(root).value());
                return decode();
            }
        }
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        page_ = nullptr;
        row_work_ = nullptr;
        hydration_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (page_) {
            page_->cancel();
        }
        if (row_work_) {
            row_work_->cancel();
        }
        if (hydration_) {
            hydration_->cancel();
        }
        finish_and_wait();
    }

private:
    enum class Phase { Prepare, Page, Fetch, QueryPlans, BehaviorPlans, Embedded };

    const UserContext& context() const {
        return EntityDataServiceAccess::context(service_);
    }

    R runtime(RuntimeError error) const {
        auto r = service_.memory_resource();
        return R::err(r, DataServiceError<Error>::runtime(r, std::move(error)));
    }

    R entity(EntityError error) const {
        auto r = service_.memory_resource();
        return R::err(r, DataServiceError<Error>::entity(r, std::move(error)));
    }

    R decode() {
        auto r = service_.memory_resource();
        SmartList<T> list(std::allocator_arg, Allocator(r));
        list.data.reserve(rows_.size());
        for (const auto& row : rows_) {
            auto value = T::from_compact_row_with_context(r, clone_in(r, row), AnyRef(*root_));
            if (!value) {
                return entity(std::move(value).error());
            }
            list.data.emplace_back(std::move(value).value());
        }
        list.loaded();
        list.total_count = total_;
        return R::ok(r, std::move(list));
    }

    EntityDataService<E> service_;
    SelectQuery query_;
    Vector<RelationAggregate> aggregates_;
    Vector<CompactRow> rows_;
    std::optional<FlatRelationPlans> plans_;
    EntityRelationFetch<E> fetch_;
    std::optional<std::uint64_t> total_;
    EntityRuntimeState* root_ = nullptr;
    EntityGraphBuilder* graph_ = nullptr;
    EntityDataService<E>* traced_ = nullptr;
    AsyncOperation<P>* page_ = nullptr;
    AsyncOperation<RelationRowsResult<E>>* row_work_ = nullptr;
    AsyncOperation<H>* hydration_ = nullptr;
    WakeSignal::Forward forwarding_;
    Phase phase_ = Phase::Prepare;
    bool direct_ = false, compact_ = false;
};
}

namespace teaql {
template <class E>
template <Entity T>
auto EntityDataService<E>::fetch_enhanced_entities(const PurposedSelectQuery& query) const
    -> DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>& {
    return fetch_enhanced_entities_with_relation_aggregates<T>(query, {});
}

template <class E>
template <Entity T>
auto EntityDataService<E>::fetch_enhanced_entities_with_relation_aggregates(
    const PurposedSelectQuery& query, std::span<const RelationAggregate> aggregates) const
    -> DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>& {
    using R = Result<SmartList<T>, DataServiceError<Error>>;
    return make_operation<R, detail::EnhancedEntitiesOperation<E, T>>(
        memory_resource(), *this, query.as_query(), aggregates);
}

template <class E>
template <Entity T>
auto EntityDataService<E>::fetch_enhanced_entities_with_relation_aggregates_owned(
    PurposedSelectQuery query, std::span<const RelationAggregate> aggregates) const
    -> DataServiceOperationResult<SmartList<T>, DataServiceError<Error>>& {
    return fetch_enhanced_entities_with_relation_aggregates<T>(query, aggregates);
}
}
