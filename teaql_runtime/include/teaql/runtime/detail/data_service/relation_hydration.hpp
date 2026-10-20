#pragma once
#include <teaql/runtime/detail/data_service/entity_service.hpp>
#include <teaql/runtime/detail/data_service/relation_execution.hpp>
#include <teaql/runtime/detail/data_service/relation_installation.hpp>
#include <teaql/runtime/detail/data_service/dynamic_loading.hpp>
#include <teaql/runtime/detail/data_service/relation_facets.hpp>

namespace teaql::detail {
enum class RelationHydrationMode { Embedded, Flat, CompactFlat };
template <class E>
using RelationHydrationResult = Result<std::monostate, DataServiceError<typename E::Error>>;
template <class E>
using EntityRelationFetch = ContextFunction<AsyncOperation<RelationRowsResult<E>>&(
    const EntityDataService<E>&, SelectQuery)>;

inline Vector<CompactRow> relation_key_rows(Memory r,
                                            std::span<const CompactRow> parents,
                                            std::span<const RelationLoadPlan> plans) {
    Vector<CompactRow> keys(r);
    if (plans.empty()) {
        return keys;
    }
    keys.reserve(parents.size());
    for (const auto& parent : parents) {
        Record values(r);
        for (const auto& plan : plans) {
            if (auto* value = parent.get(plan.local_key)) {
                values.emplace(String(plan.local_key, r), Value(r, *value));
            }
        }
        keys.emplace_back(values);
    }
    return keys;
}

// Plans and parent storage are stable Context borrows. A forest owns one plan
// snapshot; descendant nodes borrow it instead of copying each subtree again.
template <class E>
class RelationHydrationNode {
    using R = RelationHydrationResult<E>;

public:
    using allocator_type = Allocator;

    RelationHydrationNode(std::allocator_arg_t,
                          Allocator a,
                          const EntityDataService<E>& service,
                          std::span<const CompactRow> parents,
                          std::span<CompactRow> mutable_parents,
                          const RelationLoadPlan& plan,
                          RelationHydrationMode mode,
                          const EntityRuntimeState* root,
                          EntityGraphBuilder* graph,
                          const EntityRelationFetch<E>& fetch)
        : service_(std::allocator_arg, a, service), parents_(parents),
          mutable_parents_(mutable_parents), plan_(&plan), mode_(mode), root_(root), graph_(graph),
          fetch_(std::allocator_arg, a, fetch), children_(a.resource()),
          attachment_keys_(a.resource()), flat_buckets_(a.resource()),
          flat_shapes_(std::allocator_arg, a) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (mode_ != RelationHydrationMode::Embedded) {
            return poll_body(wake);
        }
        if (!scope_) {
            const auto& cs = EntityDataServiceAccess::context_service(service_);
            auto capabilities = cs.executor.get().capabilities();
            auto count = unique_relation_values(r, parents_, plan_->local_key).size();
            auto selected = relation_top_n_execution_plan(capabilities, *plan_, count);
            auto operation = relation_top_n_operation(r, *plan_, selected, count);
            scope_ = &cs.metadata.context().start_runtime_operation(operation);
        }
        auto result = scope_->poll_with_context([&] {
            return poll_body(wake);
        });
        if (result) {
            if (*result) {
                RuntimeAttributes attrs(r);
                attrs.emplace(String("teaql.result.cardinality", r),
                              std::make_obj_using_allocator<RuntimeAttributeValue>(
                                  Allocator(r), std::uint64_t(parents_.size())));
                scope_->success(attrs);
            } else {
                scope_->failure("relation_load_error");
            }
            scope_->close();
        }
        return result;
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        rows_operation_ = nullptr;
        descendant_ = nullptr;
        dynamic_ = nullptr;
        facets_ = nullptr;
        if (scope_) {
            scope_->close();
        }
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (descendant_) {
            descendant_->cancel();
            descendant_ = nullptr;
        }
        if (rows_operation_) {
            rows_operation_->cancel();
            rows_operation_ = nullptr;
        }
        if (dynamic_) {
            dynamic_->cancel();
            dynamic_ = nullptr;
        }
        if (facets_) {
            facets_->cancel();
            facets_ = nullptr;
        }
        if (scope_) {
            scope_->close();
        }
    }

private:
    enum class Phase { Fetch, Dynamic, AncestorFacets, Descendants, Install, Facets, FlatFacets };

    const UserContext& context() const {
        return EntityDataServiceAccess::context_service(service_).metadata.context();
    }

    std::optional<R> poll_body(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_service_) {
            child_service_ = &context().template make<EntityDataService<E>>(
                EntityDataServiceAccess::relation_child(service_, *plan_));
            RelationFetch<E> call(
                std::allocator_arg, Allocator(r), [this](SelectQuery query) -> auto& {
                    return fetch_(*child_service_, std::move(query));
                });
            rows_operation_ =
                &fetch_relation_rows(EntityDataServiceAccess::context_service(service_),
                                     *plan_,
                                     parents_,
                                     mode_ == RelationHydrationMode::CompactFlat,
                                     call);
        }
        if (phase_ == Phase::Fetch) {
            forwarding_.attach(rows_operation_->wake_signal(), wake);
            auto* result = rows_operation_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return R::err(r, copy_or_take_error(*rows_operation_, *result));
            }
            for (const auto& row : result->value()) {
                children_.emplace_back(row);
            }
            forwarding_.reset();
            rows_operation_ = nullptr;
            phase_ = Phase::Dynamic;
        }
        if (phase_ == Phase::Dynamic) {
            if (plan_->query && plan_->query->dynamic_field_selection) {
                if (!dynamic_) {
                    auto query = base_relation_query(context(), *plan_);
                    auto intent = EntityDataServiceAccess::request_intent(*child_service_, query);
                    if (!intent) {
                        return R::err(r,
                                      DataServiceError<typename E::Error>::runtime(
                                          r, std::move(intent).error()));
                    }
                    dynamic_ = &EntityDataServiceAccess::load_dynamic_fields_into_rows(
                        service_,
                        children_,
                        plan_->target_entity,
                        *plan_->query->dynamic_field_selection,
                        intent.value());
                }
                forwarding_.attach(dynamic_->wake_signal(), wake);
                auto* result = dynamic_->poll();
                if (!result) {
                    return {};
                }
                if (!*result) {
                    return R::err(r, copy_or_take_error(*dynamic_, *result));
                }
                forwarding_.reset();
                dynamic_ = nullptr;
            }
            if (mode_ != RelationHydrationMode::CompactFlat) {
                for (auto& child : children_) {
                    child.remove(PARTITION_RANK_PROPERTY);
                }
            }
            if (mode_ == RelationHydrationMode::Embedded) {
                attachment_keys_.reserve(children_.size());
                for (const auto& child : children_) {
                    auto* key = child.get(plan_->foreign_key);
                    attachment_keys_.push_back(key ? std::optional<Value>(Value(r, *key))
                                                   : std::optional<Value>{});
                }
                descendant_keys_.emplace(relation_key_rows(r, children_, plan_->children));
            }
            installed_before_children_ = mode_ == RelationHydrationMode::CompactFlat &&
                                         !plan_->children.empty() && !plan_->many &&
                                         plan_->local_key != "id";
            if (installed_before_children_ && has_facets()) {
                phase_ = Phase::AncestorFacets;
            } else if (installed_before_children_) {
                Vector<CompactRow> ancestor(children_, r);
                auto result = install_compact_flat_relation(
                    context(), mutable_parents_, *plan_, std::move(ancestor), *root_, *graph_);
                if (!result) {
                    return R::err(
                        r,
                        DataServiceError<typename E::Error>::entity(r, std::move(result).error()));
                }
            }
            if (phase_ != Phase::AncestorFacets) {
                phase_ = Phase::Descendants;
            }
        }
        if (phase_ == Phase::AncestorFacets) {
            auto result = poll_flat_facets(wake);
            if (!result || !*result) {
                return result;
            }
            phase_ = Phase::Descendants;
        }
        while (phase_ == Phase::Descendants) {
            if (plan_index_ == plan_->children.size()) {
                phase_ = Phase::Install;
                break;
            }
            if (!descendant_) {
                descendant_ = &make_operation<R, RelationHydrationNode>(
                    r,
                    *child_service_,
                    mode_ == RelationHydrationMode::Embedded
                        ? std::span<const CompactRow>(*descendant_keys_)
                        : std::span<const CompactRow>(children_),
                    std::span<CompactRow>(children_),
                    plan_->children[plan_index_],
                    mode_,
                    root_,
                    graph_,
                    fetch_);
            }
            forwarding_.attach(descendant_->wake_signal(), wake);
            auto* result = descendant_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return R::err(r, copy_or_take_error(*descendant_, *result));
            }
            forwarding_.reset();
            descendant_ = nullptr;
            ++plan_index_;
        }
        if (mode_ == RelationHydrationMode::Embedded) {
            if (phase_ != Phase::Facets) {
                attach_relation_rows(context(),
                                     mutable_parents_,
                                     parents_,
                                     *plan_,
                                     std::move(children_),
                                     attachment_keys_);
                phase_ = Phase::Facets;
            }
            if (plan_->query && !plan_->query->facets.empty()) {
                while (facet_index_ < mutable_parents_.size()) {
                    if (!facets_) {
                        using F = typename RelationFacetOperation<E>::R;
                        facets_ = &make_operation<F, RelationFacetOperation<E>>(
                            r,
                            *child_service_,
                            *plan_,
                            parents_[facet_index_].get(plan_->local_key));
                    }
                    forwarding_.attach(facets_->wake_signal(), wake);
                    auto* result = facets_->poll();
                    if (!result) {
                        return {};
                    }
                    if (!*result) {
                        return R::err(r, copy_or_take_error(*facets_, *result));
                    }
                    auto owned = copy_or_take_completed(*facets_, *result);
                    auto loaded =
                        mutable_parents_[facet_index_].take_loaded_relation(plan_->relation_name);
                    if (loaded) {
                        loaded->facets = std::move(owned).value();
                        mutable_parents_[facet_index_].set_loaded_relation(plan_->relation_name,
                                                                           *loaded);
                    }
                    forwarding_.reset();
                    facets_ = nullptr;
                    ++facet_index_;
                }
            }
            return R::ok(r, {});
        }
        if (!installed_before_children_ && has_facets()) {
            phase_ = Phase::FlatFacets;
            return poll_flat_facets(wake);
        }
        auto result =
            installed_before_children_
                ? install_compact_forward_views(
                      context(), mutable_parents_, *plan_, children_, *root_, *graph_)
            : mode_ == RelationHydrationMode::CompactFlat
                ? install_compact_flat_relation(
                      context(), mutable_parents_, *plan_, std::move(children_), *root_, *graph_)
                : install_flat_relation(
                      context(), mutable_parents_, *plan_, std::move(children_), *root_, *graph_);
        if (!result) {
            return R::err(
                r, DataServiceError<typename E::Error>::entity(r, std::move(result).error()));
        }
        return R::ok(r, {});
    }

    bool has_facets() const {
        return plan_->query && !plan_->query->facets.empty();
    }

    R facet_owner_error() const {
        auto r = service_.memory_resource();
        return R::err(r,
                      DataServiceError<typename E::Error>::entity(
                          r,
                          EntityError(std::allocator_arg,
                                      Allocator(r),
                                      plan_->parent_entity,
                                      "Facet owner is missing its u64 id")));
    }

    std::optional<R> poll_flat_facets(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const bool forward_batch =
            mode_ == RelationHydrationMode::CompactFlat && !plan_->many && plan_->local_key != "id";
        if (!forward_batch && !flat_buckets_ready_) {
            for (const auto& child : children_) {
                if (const auto* key = child.get(plan_->foreign_key)) {
                    flat_buckets_.try_emplace(graph_identity_key(r, *key))
                        .first->second.emplace_back(child);
                }
            }
            flat_buckets_ready_ = true;
        }
        while (facet_index_ < mutable_parents_.size()) {
            auto& parent = mutable_parents_[facet_index_];
            auto* id_value = parent.get("id");
            auto id = id_value ? id_value->try_u64() : std::nullopt;
            if (forward_batch && !id) {
                return facet_owner_error();
            }
            if (!forward_batch && mode_ == RelationHydrationMode::CompactFlat) {
                parent.mark_relation_loaded(plan_->relation_name, flat_shapes_);
            }
            if (!facets_) {
                using F = typename RelationFacetOperation<E>::R;
                facets_ = &make_operation<F, RelationFacetOperation<E>>(
                    r, *child_service_, *plan_, parent.get(plan_->local_key));
            }
            forwarding_.attach(facets_->wake_signal(), wake);
            auto* result = facets_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return R::err(r, copy_or_take_error(*facets_, *result));
            }
            auto owned = copy_or_take_completed(*facets_, *result);
            auto& values = owned.value();
            if (!plan_->many && !values.empty()) {
                if (!id) {
                    return facet_owner_error();
                }
                graph_->install_relation_facets(
                    plan_->parent_entity, *id, plan_->relation_name, values);
            }
            if (!forward_batch) {
                Vector<CompactRow> related(r);
                if (const auto* key = parent.get(plan_->local_key)) {
                    auto found = flat_buckets_.find(graph_identity_key(r, *key));
                    if (found != flat_buckets_.end()) {
                        if (plan_->local_key == "id") {
                            related.swap(found->second);
                            flat_buckets_.erase(found);
                        } else {
                            for (const auto& child : found->second) {
                                related.emplace_back(child);
                            }
                        }
                    }
                }
                auto parent_span = std::span<CompactRow>(&parent, 1);
                auto installed = mode_ == RelationHydrationMode::CompactFlat
                                     ? install_compact_flat_relation(context(),
                                                                     parent_span,
                                                                     *plan_,
                                                                     std::move(related),
                                                                     *root_,
                                                                     *graph_,
                                                                     &values)
                                     : install_flat_relation(context(),
                                                             parent_span,
                                                             *plan_,
                                                             std::move(related),
                                                             *root_,
                                                             *graph_,
                                                             &values);
                if (!installed) {
                    return R::err(r,
                                  DataServiceError<typename E::Error>::entity(
                                      r, std::move(installed).error()));
                }
            }
            forwarding_.reset();
            facets_ = nullptr;
            ++facet_index_;
        }
        if (forward_batch) {
            Vector<CompactRow> rows(children_, r);
            auto installed = install_compact_flat_relation(
                context(), mutable_parents_, *plan_, std::move(rows), *root_, *graph_);
            if (!installed) {
                return R::err(
                    r,
                    DataServiceError<typename E::Error>::entity(r, std::move(installed).error()));
            }
        }
        return R::ok(r, {});
    }

    EntityDataService<E> service_;
    std::span<const CompactRow> parents_;
    std::span<CompactRow> mutable_parents_;
    const RelationLoadPlan* plan_;
    RelationHydrationMode mode_;
    const EntityRuntimeState* root_;
    EntityGraphBuilder* graph_;
    EntityRelationFetch<E> fetch_;
    EntityDataService<E>* child_service_ = nullptr;
    Vector<CompactRow> children_;
    Vector<std::optional<Value>> attachment_keys_;
    std::optional<Vector<CompactRow>> descendant_keys_;
    std::pmr::map<String, Vector<CompactRow>, std::less<>> flat_buckets_;
    RelationShapeCache flat_shapes_;
    bool flat_buckets_ready_ = false;
    AsyncOperation<RelationRowsResult<E>>* rows_operation_ = nullptr;
    AsyncOperation<R>* descendant_ = nullptr;
    AsyncOperation<R>* dynamic_ = nullptr;
    AsyncOperation<typename RelationFacetOperation<E>::R>* facets_ = nullptr;
    WakeSignal::Forward forwarding_;
    FailOpenRuntimeTelemetryScope* scope_ = nullptr;
    Phase phase_ = Phase::Fetch;
    bool installed_before_children_ = false;
    std::size_t plan_index_ = 0;
    std::size_t facet_index_ = 0;
};

template <class E>
class RelationHydrationForest {
    using R = RelationHydrationResult<E>;

public:
    using allocator_type = Allocator;

    RelationHydrationForest(std::allocator_arg_t,
                            Allocator a,
                            const EntityDataService<E>& service,
                            std::span<const CompactRow> parents,
                            std::span<CompactRow> mutable_parents,
                            std::span<const RelationLoadPlan> plans,
                            RelationHydrationMode mode,
                            const EntityRuntimeState* root,
                            EntityGraphBuilder* graph,
                            const EntityRelationFetch<E>& fetch)
        : service_(std::allocator_arg, a, service), parents_(parents),
          mutable_parents_(mutable_parents), plans_(plans.begin(), plans.end(), a.resource()),
          mode_(mode), root_(root), graph_(graph), fetch_(std::allocator_arg, a, fetch) {
        validate_relation_domain(
            EntityDataServiceAccess::context_service(service_).metadata.context(),
            parents,
            root,
            graph);
        if (mode != RelationHydrationMode::Embedded && (!root || !graph)) {
            throw std::invalid_argument("Identity graph hydration requires a root and graph");
        }
        if (mode != RelationHydrationMode::CompactFlat &&
            parents.size() != mutable_parents.size()) {
            throw std::invalid_argument("This relation path requires mutable rows");
        }
    }

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (mode_ == RelationHydrationMode::Embedded && !keys_) {
            keys_.emplace(relation_key_rows(r, parents_, plans_));
        }
        while (index_ < plans_.size()) {
            if (!child_) {
                child_ = &make_operation<R, RelationHydrationNode<E>>(
                    r,
                    service_,
                    mode_ == RelationHydrationMode::Embedded ? std::span<const CompactRow>(*keys_)
                                                             : parents_,
                    mutable_parents_,
                    plans_[index_],
                    mode_,
                    root_,
                    graph_,
                    fetch_);
            }
            forwarding_.attach(child_->wake_signal(), wake);
            auto* result = child_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return R::err(r, copy_or_take_error(*child_, *result));
            }
            forwarding_.reset();
            child_ = nullptr;
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
    EntityDataService<E> service_;
    std::span<const CompactRow> parents_;
    std::span<CompactRow> mutable_parents_;
    Vector<RelationLoadPlan> plans_;
    RelationHydrationMode mode_;
    const EntityRuntimeState* root_;
    EntityGraphBuilder* graph_;
    EntityRelationFetch<E> fetch_;
    std::optional<Vector<CompactRow>> keys_;
    std::size_t index_ = 0;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto& enhance_relation_plans(const EntityDataService<E>& service,
                             std::span<CompactRow> parents,
                             std::span<const RelationLoadPlan> plans,
                             const EntityRelationFetch<E>& fetch) {
    return make_operation<RelationHydrationResult<E>, RelationHydrationForest<E>>(
        service.memory_resource(),
        service,
        std::span<const CompactRow>(parents),
        parents,
        plans,
        RelationHydrationMode::Embedded,
        nullptr,
        nullptr,
        fetch);
}

template <class E>
auto& hydrate_flat_plans(const EntityDataService<E>& service,
                         std::span<CompactRow> parents,
                         std::span<const RelationLoadPlan> plans,
                         const EntityRuntimeState& root,
                         EntityGraphBuilder& graph,
                         const EntityRelationFetch<E>& fetch) {
    return make_operation<RelationHydrationResult<E>, RelationHydrationForest<E>>(
        service.memory_resource(),
        service,
        std::span<const CompactRow>(parents),
        parents,
        plans,
        RelationHydrationMode::Flat,
        &root,
        &graph,
        fetch);
}

template <class E>
auto& hydrate_compact_flat_plans(const EntityDataService<E>& service,
                                 std::span<CompactRow> parents,
                                 std::span<const RelationLoadPlan> plans,
                                 const EntityRuntimeState& root,
                                 EntityGraphBuilder& graph,
                                 const EntityRelationFetch<E>& fetch) {
    return make_operation<RelationHydrationResult<E>, RelationHydrationForest<E>>(
        service.memory_resource(),
        service,
        parents,
        parents,
        plans,
        RelationHydrationMode::CompactFlat,
        &root,
        &graph,
        fetch);
}
}
