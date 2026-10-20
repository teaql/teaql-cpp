#pragma once
#include <teaql/runtime/detail/data_service/relation_hydration.hpp>

namespace teaql::detail {
Value default_aggregate_value(Memory, bool single);
void attach_empty_relation_aggregate(std::span<CompactRow>, std::string_view alias, bool single);
void attach_relation_aggregate_rows(std::span<CompactRow>,
                                    const RelationLoadPlan&,
                                    const RelationAggregate&,
                                    Vector<CompactRow>);
void attach_relation_aggregate_rows(std::span<CompactRow>,
                                    std::span<const CompactRow>,
                                    const RelationLoadPlan&,
                                    const RelationAggregate&,
                                    Vector<CompactRow>);
void attach_object_group_rows(std::span<CompactRow>,
                              const ObjectGroupBy&,
                              std::span<const CompactRow>);
Result<std::monostate, DynamicFieldError> attach_child_query_rows(std::span<CompactRow>,
                                                                  std::span<const CompactRow>);

inline Result<Vector<CompactRow>, RuntimeError>
relation_aggregate_key_rows(const UserContext& context,
                            std::string_view entity,
                            std::span<const CompactRow> parents,
                            std::span<const RelationAggregate> aggregates) {
    using R = Result<Vector<CompactRow>, RuntimeError>;
    auto r = context.memory_resource();
    Vector<CompactRow> keys(r);
    if (aggregates.empty()) {
        return R::ok(r, std::move(keys));
    }
    validate_relation_domain(context, parents);
    auto descriptor = context.require_entity(entity);
    if (!descriptor) {
        return R::err(r, std::move(descriptor).error());
    }
    Vector<String> fields(r);
    for (const auto& aggregate : aggregates) {
        auto* relation = descriptor.value().get().relation_by_name(aggregate.relation_name);
        if (!relation) {
            return R::err(r, RuntimeError::missing_relation(r, entity, aggregate.relation_name));
        }
        fields.emplace_back(relation->local_key);
    }
    for (const auto& parent : parents) {
        Record values(r);
        for (const auto& field : fields) {
            if (auto* value = parent.get(field)) {
                values.emplace(String(field, r), Value(r, *value));
            }
        }
        keys.emplace_back(values);
    }
    return R::ok(r, std::move(keys));
}

// Parent rows are stable, same-Context borrows, as in relation hydration. Query
// inputs are snapshots; the fetch callback keeps the entity-runtime boundary.
template <class E>
class RelationAggregateOperation {
    using R = RelationHydrationResult<E>;

public:
    using allocator_type = Allocator;

    RelationAggregateOperation(std::allocator_arg_t,
                               Allocator a,
                               const EntityDataService<E>& service,
                               std::span<CompactRow> parents,
                               std::span<const RelationAggregate> aggregates,
                               std::optional<AggregationCacheOptions> cache,
                               std::span<const TraceNode> trace,
                               const EntityRelationFetch<E>& fetch)
        : service_(std::allocator_arg, a, service), parents_(parents),
          aggregates_(aggregates.begin(), aggregates.end(), a.resource()), cache_(cache),
          trace_(trace.begin(), trace.end(), a.resource()), fetch_(std::allocator_arg, a, fetch) {
        validate_relation_domain(context(), parents);
    }

    RelationAggregateOperation(std::allocator_arg_t,
                               Allocator a,
                               const EntityDataService<E>& service,
                               std::span<CompactRow> parents,
                               std::span<const CompactRow> keys,
                               std::span<const RelationAggregate> aggregates,
                               std::optional<AggregationCacheOptions> cache,
                               std::span<const TraceNode> trace,
                               const EntityRelationFetch<E>& fetch)
        : RelationAggregateOperation(
              std::allocator_arg, a, service, parents, aggregates, cache, trace, fetch) {
        validate_relation_domain(context(), keys);
        provided_keys_ = keys;
    }

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!provided_keys_ && !keys_) {
            auto keys =
                relation_aggregate_key_rows(context(), service_.entity(), parents_, aggregates_);
            if (!keys) {
                return R::err(
                    r, DataServiceError<typename E::Error>::runtime(r, std::move(keys).error()));
            }
            keys_.emplace(std::move(keys).value());
        }
        auto keys = provided_keys_ ? *provided_keys_ : std::span<const CompactRow>(*keys_);
        while (index_ < aggregates_.size()) {
            const auto& aggregate = aggregates_[index_];
            if (!child_) {
                RelationLoad load(
                    std::allocator_arg, Allocator(r), aggregate.relation_name, aggregate.query);
                auto plans = build_relation_plans_from_loads(
                    context(), service_.entity(), std::span<const RelationLoad>(&load, 1));
                if (!plans) {
                    return R::err(
                        r,
                        DataServiceError<typename E::Error>::runtime(r, std::move(plans).error()));
                }
                if (plans.value().empty()) {
                    return R::err(r,
                                  DataServiceError<typename E::Error>::runtime(
                                      r,
                                      RuntimeError::missing_relation(
                                          r, service_.entity(), aggregate.relation_name)));
                }
                plan_.emplace(std::move(plans.value().front()));
                Vector<Value> ids(r);
                for (const auto& parent : keys) {
                    if (auto* id = parent.get(plan_->local_key)) {
                        ids.emplace_back(*id);
                    }
                }
                if (ids.empty()) {
                    attach_empty_relation_aggregate(
                        parents_, aggregate.alias, aggregate.single_result);
                    plan_.reset();
                    ++index_;
                    continue;
                }
                auto query = clone_in(r, aggregate.query);
                query.entity = plan_->target_entity;
                if (!query.aggregation_cache && cache_ && cache_->propagate) {
                    query.aggregation_cache = AggregationCacheOptions{
                        .cache_expired_millis = cache_->propagate_cache_expired_millis};
                }
                query.projection.clear();
                query.expr_projection.clear();
                query.order_by.clear();
                query.slice.reset();
                query.relations.clear();
                if (query.aggregates.empty()) {
                    query.aggregates.emplace_back(Aggregate::count(
                        r, aggregate.single_result ? std::string_view(aggregate.alias) : "count"));
                }
                if (std::find(query.group_by_fields.begin(),
                              query.group_by_fields.end(),
                              plan_->foreign_key) == query.group_by_fields.end()) {
                    query.group_by(plan_->foreign_key);
                }
                query.and_filter(Expr::in_list(r, plan_->foreign_key, ids));
                auto parent = EntityDataServiceAccess::scoped(service_, service_.entity());
                parent.with_trace_context(trace_);
                auto& repo = context().template make<EntityDataService<E>>(
                    EntityDataServiceAccess::relation_child(parent, *plan_));
                auto& child = fetch_(repo, std::move(query));
                if (child.memory_resource() != r) {
                    child.cancel();
                    throw std::invalid_argument(
                        "Relation aggregate result must belong to its query Context");
                }
                child_ = &child;
            }
            forwarding_.attach(child_->wake_signal(), wake);
            auto* result = child_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return R::err(r, copy_or_take_error(*child_, *result));
            }
            Vector<CompactRow> rows(result->value(), r);
            // Metadata is resolved after the asynchronous fetch, like the source.
            if (auto* entity = context().entity(plan_->target_entity)) {
                if (auto* property = entity->property_by_name(plan_->foreign_key);
                    property && property->column_name != plan_->foreign_key) {
                    for (auto& row : rows) {
                        if (!row.contains_key(plan_->foreign_key)) {
                            if (auto value = row.remove(property->column_name)) {
                                row.insert(plan_->foreign_key, *value);
                            }
                        }
                    }
                }
            }
            attach_relation_aggregate_rows(parents_, keys, *plan_, aggregate, std::move(rows));
            forwarding_.reset();
            child_ = nullptr;
            plan_.reset();
            ++index_;
        }
        return R::ok(r, {});
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
    const UserContext& context() const {
        return EntityDataServiceAccess::context_service(service_).metadata.context();
    }

    EntityDataService<E> service_;
    std::span<CompactRow> parents_;
    std::optional<Vector<CompactRow>> keys_;
    std::optional<std::span<const CompactRow>> provided_keys_;
    Vector<RelationAggregate> aggregates_;
    std::optional<AggregationCacheOptions> cache_;
    TraceChain trace_;
    EntityRelationFetch<E> fetch_;
    std::optional<RelationLoadPlan> plan_;
    std::size_t index_ = 0;
    AsyncOperation<RelationRowsResult<E>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E, class Enhancement>
class RowQueryEnhancementOperation {
    using R = RelationHydrationResult<E>;
    static constexpr bool ObjectGroup = std::same_as<Enhancement, ObjectGroupBy>;

public:
    using allocator_type = Allocator;

    RowQueryEnhancementOperation(std::allocator_arg_t,
                                 Allocator a,
                                 const EntityDataService<E>& service,
                                 std::span<CompactRow> parents,
                                 std::span<const Enhancement> enhancements,
                                 std::span<const TraceNode> trace,
                                 const EntityRelationFetch<E>& fetch)
        : service_(std::allocator_arg, a, service), parents_(parents),
          enhancements_(enhancements.begin(), enhancements.end(), a.resource()),
          trace_(trace.begin(), trace.end(), a.resource()), fetch_(std::allocator_arg, a, fetch) {
        validate_relation_domain(
            EntityDataServiceAccess::context_service(service_).metadata.context(), parents);
    }

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        while (index_ < enhancements_.size()) {
            const auto& enhancement = enhancements_[index_];
            if (!child_ && !dynamic_) {
                std::string_view field = "id";
                if constexpr (ObjectGroup) {
                    field = enhancement.storage_field;
                }
                Vector<Value> ids(r);
                for (const auto& row : parents_) {
                    if (auto* value = row.get(field)) {
                        ids.emplace_back(*value);
                    }
                }
                if (ids.empty()) {
                    ++index_;
                    continue;
                }
                auto query = [&] {
                    if constexpr (ObjectGroup) {
                        return clone_in(r, enhancement.query);
                    } else {
                        return clone_in(r, enhancement);
                    }
                }();
                if (!query.projection.empty() &&
                    std::find(query.projection.begin(), query.projection.end(), "id") ==
                        query.projection.end()) {
                    query.project("id");
                }
                query.and_filter(Expr::in_list(r, "id", ids));
                auto& context =
                    EntityDataServiceAccess::context_service(service_).metadata.context();
                auto& repo = context.template make<EntityDataService<E>>(
                    EntityDataServiceAccess::scoped(service_, query.entity));
                repo.with_trace_context(trace_);
                if constexpr (!ObjectGroup) {
                    auto intent = EntityDataServiceAccess::request_intent(repo, query);
                    if (!intent) {
                        return R::err(r,
                                      DataServiceError<typename E::Error>::runtime(
                                          r, std::move(intent).error()));
                    }
                    if (query.dynamic_field_selection) {
                        dynamic_ = make_box<DynamicPayload>(r, query, repo, intent.value());
                    }
                }
                auto& child = fetch_(repo, std::move(query));
                if (child.memory_resource() != r) {
                    child.cancel();
                    throw std::invalid_argument(
                        "Row enhancement result must belong to its query Context");
                }
                child_ = &child;
            }
            const Vector<CompactRow>* fetched = nullptr;
            if (child_) {
                forwarding_.attach(child_->wake_signal(), wake);
                auto* result = child_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return R::err(r, copy_or_take_error(*child_, *result));
                }
                forwarding_.reset();
                if (dynamic_) {
                    dynamic_->rows = Vector<CompactRow>(result->value(), r);
                } else {
                    fetched = &result->value();
                }
                child_ = nullptr;
            }
            if constexpr (ObjectGroup) {
                attach_object_group_rows(parents_, enhancement, *fetched);
            } else {
                if (dynamic_) {
                    if (!dynamic_->work) {
                        dynamic_
                            ->work = &make_operation<R, DynamicRowsOperation<typename E::Error>>(
                            r,
                            EntityDataServiceAccess::context_service(service_).metadata.context(),
                            std::span<CompactRow>(dynamic_->rows),
                            dynamic_->query.entity,
                            *dynamic_->query.dynamic_field_selection,
                            dynamic_->intent);
                    }
                    forwarding_.attach(dynamic_->work->wake_signal(), wake);
                    auto* result = dynamic_->work->poll();
                    if (!result) {
                        return {};
                    }
                    if (!*result) {
                        return R::err(r, copy_or_take_error(*dynamic_->work, *result));
                    }
                    forwarding_.reset();
                    fetched = &dynamic_->rows;
                }
                auto merged = attach_child_query_rows(parents_, *fetched);
                if (!merged) {
                    return R::err(r,
                                  DataServiceError<typename E::Error>::runtime(
                                      r, RuntimeError::dynamic_field(r, merged.error())));
                }
            }
            dynamic_.reset();
            ++index_;
        }
        return R::ok(r, {});
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
        if (dynamic_ && dynamic_->work) {
            dynamic_->work->cancel();
        }
    }

private:
    struct DynamicPayload {
        using allocator_type = Allocator;
        SelectQuery query;
        QueryIntent intent;
        Vector<CompactRow> rows;
        AsyncOperation<R>* work = nullptr;

        DynamicPayload(std::allocator_arg_t,
                       Allocator a,
                       const SelectQuery& query,
                       const EntityDataService<E>&,
                       const QueryIntent& intent)
            : query(std::allocator_arg, a, query), intent(std::allocator_arg, a, intent),
              rows(a.resource()) {}
    };

    ScopedBox<DynamicPayload> dynamic_;
    EntityDataService<E> service_;
    std::span<CompactRow> parents_;
    Vector<Enhancement> enhancements_;
    TraceChain trace_;
    EntityRelationFetch<E> fetch_;
    std::size_t index_ = 0;
    AsyncOperation<RelationRowsResult<E>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto& enhance_relation_aggregates(const EntityDataService<E>& service,
                                  std::span<CompactRow> rows,
                                  std::span<const RelationAggregate> aggregates,
                                  std::optional<AggregationCacheOptions> cache,
                                  std::span<const TraceNode> trace,
                                  const EntityRelationFetch<E>& fetch) {
    return make_operation<RelationHydrationResult<E>, RelationAggregateOperation<E>>(
        service.memory_resource(), service, rows, aggregates, cache, trace, fetch);
}

template <class E>
auto& enhance_relation_aggregates(const EntityDataService<E>& service,
                                  std::span<CompactRow> rows,
                                  std::span<const CompactRow> keys,
                                  std::span<const RelationAggregate> aggregates,
                                  std::optional<AggregationCacheOptions> cache,
                                  std::span<const TraceNode> trace,
                                  const EntityRelationFetch<E>& fetch) {
    return make_operation<RelationHydrationResult<E>, RelationAggregateOperation<E>>(
        service.memory_resource(), service, rows, keys, aggregates, cache, trace, fetch);
}

template <class E>
auto& enhance_object_group_bys(const EntityDataService<E>& service,
                               std::span<CompactRow> rows,
                               std::span<const ObjectGroupBy> groups,
                               std::span<const TraceNode> trace,
                               const EntityRelationFetch<E>& fetch) {
    return make_operation<RelationHydrationResult<E>,
                          RowQueryEnhancementOperation<E, ObjectGroupBy>>(
        service.memory_resource(), service, rows, groups, trace, fetch);
}

template <class E>
auto& enhance_child_queries(const EntityDataService<E>& service,
                            std::span<CompactRow> rows,
                            std::span<const SelectQuery> queries,
                            std::span<const TraceNode> trace,
                            const EntityRelationFetch<E>& fetch) {
    return make_operation<RelationHydrationResult<E>, RowQueryEnhancementOperation<E, SelectQuery>>(
        service.memory_resource(), service, rows, queries, trace, fetch);
}
}
