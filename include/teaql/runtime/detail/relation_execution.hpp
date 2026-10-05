#pragma once
#include <teaql/runtime/detail/data_service.hpp>
#include <teaql/runtime/relation.hpp>

namespace teaql::detail {
enum class RelationTopNExecutionPlan { Window, BoundedProbes };

inline RelationTopNExecutionPlan
relation_top_n_execution_plan(const DataServiceCapabilities& capabilities,
                              const RelationLoadPlan& plan,
                              std::size_t parent_count) {
    if (!plan.query || !plan.many || !plan.query->slice) {
        return RelationTopNExecutionPlan::Window;
    }
    const auto threshold = plan.query->top_n_probe_parent_threshold_value;
    const bool probes = threshold ? *threshold != 0 && parent_count <= *threshold
                                  : capabilities.small_parent_relation_probes;
    return probes ? RelationTopNExecutionPlan::BoundedProbes : RelationTopNExecutionPlan::Window;
}

inline RuntimeOperation relation_top_n_operation(Memory r,
                                                 const RelationLoadPlan& plan,
                                                 RelationTopNExecutionPlan selected,
                                                 std::size_t parent_count) {
    String name(plan.parent_entity, r);
    name += '.';
    name += plan.path;
    RuntimeOperation operation(std::allocator_arg, Allocator(r), "relation_load", name);
    operation.attribute("teaql.entity.type", std::string_view(plan.parent_entity))
        .attribute("teaql.relation.name", std::string_view(plan.path))
        .attribute("teaql.relation.parent_count", std::uint64_t(parent_count))
        .attribute("teaql.relation.per_parent_limit",
                   std::uint64_t(
                       plan.query && plan.query->slice ? plan.query->slice->limit.value_or(0) : 0))
        .attribute("teaql.relation.configured_probe_threshold",
                   std::uint64_t(
                       plan.query ? plan.query->top_n_probe_parent_threshold_value.value_or(0) : 0))
        .attribute("teaql.relation.selected_plan",
                   selected == RelationTopNExecutionPlan::Window ? "WINDOW" : "BOUNDED_PROBES")
        .attribute(
            "teaql.relation.probe_count",
            std::uint64_t(selected == RelationTopNExecutionPlan::BoundedProbes ? parent_count : 0));
    return operation;
}

inline SelectQuery base_relation_query(const UserContext& context, const RelationLoadPlan& plan) {
    auto query = query_for_relation_plan(context, plan, {}, false);
    // The shared constructor sets a semantic partition for a parent set. The
    // base query preserves the caller's original partition until probe dispatch.
    query.partition_field = plan.query
                                ? clone_in(context.memory_resource(), plan.query->partition_field)
                                : std::nullopt;
    return query;
}
template <class E>
using RelationRowsResult = Result<Vector<CompactRow>, DataServiceError<typename E::Error>>;
template <class E>
using RelationFetch = ContextFunction<AsyncOperation<RelationRowsResult<E>>&(SelectQuery)>;

// The recursive entity query entry point supplies the fetch callback. This
// planner never bypasses that entry point by calling a SQL provider directly.
template <class E>
class RelationRowsOperation {
    using R = RelationRowsResult<E>;

public:
    using allocator_type = Allocator;

    RelationRowsOperation(std::allocator_arg_t,
                          Allocator a,
                          const ContextDataService<E>& service,
                          const RelationLoadPlan& plan,
                          std::span<const CompactRow> parents,
                          bool compact,
                          const RelationFetch<E>& fetch)
        : service_(std::allocator_arg, a, service), plan_(std::allocator_arg, a, plan),
          parents_(parents.begin(), parents.end(), a.resource()), compact_(compact),
          fetch_(std::allocator_arg, a, fetch), ids_(a.resource()), rows_(a.resource()) {}

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& context = service_.metadata.context();
        if (!started_) {
            started_ = true;
            ids_ = unique_relation_values(r, parents_, plan_.local_key);
            if (ids_.empty()) {
                return R::ok(r, std::move(rows_));
            }
            auto capabilities = service_.executor.get().capabilities();
            probes_ = relation_top_n_execution_plan(capabilities, plan_, ids_.size()) ==
                      RelationTopNExecutionPlan::BoundedProbes;
            const bool provider_batch =
                capabilities.small_parent_relation_probes &&
                (!plan_.query || !plan_.query->top_n_probe_parent_threshold_value);
            if (provider_batch && !service_.metadata.capture_execution_metadata()) {
                probes_ = false;
            }
        }
        for (;;) {
            if (!child_) {
                auto query = probes_ ? base_relation_query(context, plan_)
                                     : query_for_relation_plan(context, plan_, parents_, compact_);
                if (probes_) {
                    query.and_filter(Expr::eq(r, plan_.foreign_key, ids_[index_]));
                    query.partition_field.reset();
                }
                auto& child = fetch_(std::move(query));
                if (child.memory_resource() != r) {
                    throw std::invalid_argument(
                        "Relation fetch result must belong to its query Context");
                }
                child_ = &child;
            }
            forwarding_.attach(child_->wake_signal(), wake);
            auto* result = child_->poll();
            if (!result) {
                return {};
            }
            if (!*result) {
                return R::err(r, clone_in(r, result->error()));
            }
            for (const auto& row : result->value()) {
                rows_.emplace_back(row);
            }
            forwarding_.reset();
            child_ = nullptr;
            if (!probes_ || ++index_ == ids_.size()) {
                return R::ok(r, std::move(rows_));
            }
        }
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
    ContextDataService<E> service_;
    RelationLoadPlan plan_;
    Vector<CompactRow> parents_;
    bool compact_;
    RelationFetch<E> fetch_;
    Vector<Value> ids_;
    Vector<CompactRow> rows_;
    AsyncOperation<R>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
    bool started_ = false, probes_ = false;
    std::size_t index_ = 0;
};

template <class E>
AsyncOperation<RelationRowsResult<E>>& fetch_relation_rows(const ContextDataService<E>& service,
                                                           const RelationLoadPlan& plan,
                                                           std::span<const CompactRow> parents,
                                                           bool compact,
                                                           const RelationFetch<E>& fetch) {
    return make_operation<RelationRowsResult<E>, RelationRowsOperation<E>>(
        service.memory_resource(), service, plan, parents, compact, fetch);
}
}
