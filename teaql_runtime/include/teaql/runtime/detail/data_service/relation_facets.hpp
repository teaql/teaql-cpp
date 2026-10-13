#pragma once
#include <teaql/runtime/detail/data_service/entity_service.hpp>
#include <teaql/runtime/facet_runtime.hpp>

namespace teaql::detail {
template <class E>
class EntityFacetResultOperation {
    using Child = Result<SmartList<CompactRow>, DataServiceError<typename E::Error>>;
    using R = Result<SmartList<CompactRow>, RuntimeError>;

public:
    using allocator_type = Allocator;

    EntityFacetResultOperation(std::allocator_arg_t, Allocator a, AsyncOperation<Child>& child)
        : resource_(a.resource()), child_(&child) {}

    std::optional<R> poll(WakeSignal& wake) {
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(resource_,
                          RuntimeError::graph(resource_, result->error().to_string(resource_)));
        }
        return R::ok(
            resource_,
            SmartList<CompactRow>(std::allocator_arg, Allocator(resource_), result->value()));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        child_->cancel();
    }

private:
    Memory resource_;
    AsyncOperation<Child>* child_;
    WakeSignal::Forward forwarding_;
};

template <class E>
class EntityFacetRuntime final : public TeaqlRuntime {
public:
    using allocator_type = Allocator;

    EntityFacetRuntime(std::allocator_arg_t, Allocator a, const EntityDataService<E>& service)
        : service_(std::allocator_arg, a, service) {}

    const UserContext& user_context() const override {
        return EntityDataServiceAccess::context(service_);
    }

    AsyncOperation<Result<SmartList<CompactRow>, RuntimeError>>&
    fetch_facet_smart_list(std::string_view entity,
                           const PurposedSelectQuery& query,
                           std::span<const RelationAggregate> aggregates,
                           std::span<const TraceNode> trace) const override {
        auto& context = user_context();
        auto& service = context.template make<EntityDataService<E>>(
            EntityDataServiceAccess::scoped(service_, entity));
        service.with_trace_context(trace);
        auto& child = service.fetch_smart_list_with_relation_aggregates(query, aggregates);
        using R = Result<SmartList<CompactRow>, RuntimeError>;
        return make_operation<R, EntityFacetResultOperation<E>>(context.memory_resource(), child);
    }

private:
    EntityDataService<E> service_;
};

template <class E>
class RelationFacetOperation {
public:
    using allocator_type = Allocator;
    using R = Result<FacetResults, DataServiceError<typename E::Error>>;

    RelationFacetOperation(std::allocator_arg_t,
                           Allocator a,
                           const EntityDataService<E>& child_service,
                           const RelationLoadPlan& plan,
                           const Value* local_value)
        : service_(std::allocator_arg, a, child_service), plan_(&plan) {
        if (local_value) {
            value_.emplace(a.resource(), *local_value);
        }
    }

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_) {
            auto outer = clone_in(r, *plan_->query);
            request::QueryOptions options(std::allocator_arg, Allocator(r));
            options.facets.swap(outer.facets);
            outer.slice.reset();
            outer.partition_field.reset();
            outer.order_by.clear();
            outer.and_filter(value_ && !value_->is_null() ? Expr::eq(r, plan_->foreign_key, *value_)
                                                          : Expr::value(r, false));
            auto intent = EntityDataServiceAccess::request_intent(service_, outer);
            if (!intent) {
                return R::err(
                    r, DataServiceError<typename E::Error>::runtime(r, std::move(intent).error()));
            }
            outer.comment_text.emplace(intent.value().comment(), r);
            outer.purpose_text.emplace(intent.value().purpose(), r);
            auto prefix = EntityDataServiceAccess::trace(service_);
            TraceChain trace(prefix.begin(), prefix.end(), r);
            for (const auto& node : outer.trace_chain) {
                trace.emplace_back(node);
            }
            outer.trace_chain.swap(trace);
            auto& context = EntityDataServiceAccess::context(service_);
            auto& runtime = context.template make<EntityFacetRuntime<E>>(service_);
            child_ = &execute_facets(runtime, outer, options);
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(
                service_.memory_resource(),
                DataServiceError<typename E::Error>::runtime(r, clone_in(r, result->error())));
        }
        auto owned = copy_or_take_completed(*child_, *result);
        return R::ok(r, std::move(owned).value());
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
        }
    }

private:
    EntityDataService<E> service_;
    const RelationLoadPlan* plan_;
    std::optional<Value> value_;
    AsyncOperation<Result<FacetResults, RuntimeError>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};
}
