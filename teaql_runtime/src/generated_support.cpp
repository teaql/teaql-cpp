#include <teaql/runtime/generated_support.hpp>
#include <teaql/async/pending.hpp>

namespace teaql {
Result<request::QuerySelection, RuntimeError>
restrict_facet_to_outer_query(const TeaqlRuntime& runtime,
                              request::QuerySelection selection,
                              const SelectQuery& outer,
                              std::string_view relation_name) {
    const auto& context = runtime.user_context();
    auto r = context.memory_resource();
    using R = Result<request::QuerySelection, RuntimeError>;
    auto* entity = context.entity(outer.entity);
    if (!entity) {
        String message("missing entity: ", r);
        message += outer.entity;
        return R::err(r, RuntimeError::graph(r, message));
    }
    auto descriptor = detail::clone_in(r, *entity);
    auto* found = descriptor.relation_by_name(relation_name);
    if (!found) {
        return R::err(r, RuntimeError::missing_relation(r, outer.entity, relation_name));
    }
    auto relation = detail::clone_in(r, *found);
    auto subquery = outer.clone(r);
    subquery.projection.clear();
    subquery.expr_projection.clear();
    subquery.order_by.clear();
    subquery.slice.reset();
    subquery.aggregates.clear();
    subquery.group_by_fields.clear();
    subquery.relations.clear();
    selection.query.and_filter(
        Expr::in_subquery(r, relation.foreign_key, descriptor, subquery, relation.local_key));
    return R::ok(r, std::move(selection));
}

namespace detail {
class FacetExecution {
public:
    using allocator_type = Allocator;
    using Output = Result<FacetResults, RuntimeError>;

    FacetExecution(std::allocator_arg_t,
                   Allocator a,
                   const TeaqlRuntime& runtime,
                   const SelectQuery& outer,
                   const request::QueryOptions& options)
        : runtime_(std::allocator_arg, a, runtime, resource_arena(a.resource())),
          outer_(std::allocator_arg, a, outer), options_(std::allocator_arg, a, options),
          results_(a.resource()), name_(a.resource()) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = outer_.memory_resource();
        while (index_ < options_.facets.size()) {
            if (!pending_) {
                const auto& facet = options_.facets[index_];
                auto selection = clone_in(r, facet.query);
                request::merge_outer_filter_into_facet_aggregates(selection, outer_);
                if (!facet.include_all_facets) {
                    auto restricted = restrict_facet_to_outer_query(
                        runtime_.get(), std::move(selection), outer_, facet.relation_name);
                    if (!restricted) {
                        return Output::err(r, std::move(restricted).error());
                    }
                    selection = std::move(restricted).value();
                }
                aggregates_ = request::runtime_relation_aggregates(r, selection.query_options);
                auto query = request::apply_runtime_metadata(std::move(selection.query),
                                                             selection.query_options,
                                                             selection.child_enhancements);
                trace_.emplace(outer_.trace_chain, r);
                trace_->emplace_back(TraceKind::Relation, query.entity, facet.facet_name);
                name_ = facet.facet_name;
                String purpose("Calculate facet ", r);
                purpose += name_;
                query_.emplace(std::allocator_arg, Allocator(r), query, purpose);
                auto& child = runtime_.get().fetch_facet_smart_list(
                    query_->as_query().entity, *query_, *aggregates_, *trace_);
                if (child.memory_resource() != r) {
                    throw std::invalid_argument(
                        "Facet result operation belongs to another Context");
                }
                pending_ = &child;
                forward_.attach(child.wake_signal(), wake);
            }
            auto result = pending_->poll();
            if (!result) {
                return {};
            }
            forward_.reset();
            pending_ = nullptr;
            if (!*result) {
                return Output::err(r, clone_in(r, result->error()));
            }
            results_.insert_or_assign(String(name_, r),
                                      SmartList<CompactRow>(std::allocator_arg,
                                                            Allocator(r),
                                                            std::move(*result).value()));
            query_.reset();
            trace_.reset();
            aggregates_.reset();
            ++index_;
        }
        return Output::ok(r, std::move(results_));
    }

    void cancel_and_wait() noexcept {
        forward_.reset();
        if (pending_) {
            pending_->cancel();
            pending_ = nullptr;
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

private:
    ResourceBorrow<const TeaqlRuntime> runtime_;
    SelectQuery outer_;
    request::QueryOptions options_;
    FacetResults results_;
    std::size_t index_ = 0;
    String name_;
    std::optional<Vector<RelationAggregate>> aggregates_;
    std::optional<TraceChain> trace_;
    std::optional<PurposedSelectQuery> query_;
    AsyncOperation<Result<SmartList<CompactRow>, RuntimeError>>* pending_ = nullptr;
    WakeSignal::Forward forward_;
};
}

AsyncOperation<Result<FacetResults, RuntimeError>>& execute_facets(
    const TeaqlRuntime& runtime, const SelectQuery& outer, const request::QueryOptions& options) {
    return make_operation<Result<FacetResults, RuntimeError>, detail::FacetExecution>(
        runtime.user_context().memory_resource(), runtime, outer, options);
}
}
