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
                   const request::QueryOptions& options,
                   const SelectQuery& diagnostic_source)
        : runtime_(std::allocator_arg, a, runtime, resource_arena(a.resource())),
          outer_(std::allocator_arg, a, outer), options_(std::allocator_arg, a, options),
          diagnostic_source_(std::allocator_arg, a, diagnostic_source), results_(a.resource()),
          name_(a.resource()) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = outer_.memory_resource();
        if (!intent_) {
            auto intent = QueryIntent::from_optional(
                r,
                outer_.comment_text ? std::optional<std::string_view>(*outer_.comment_text)
                                    : std::nullopt,
                outer_.purpose_text ? std::optional<std::string_view>(*outer_.purpose_text)
                                    : std::nullopt);
            if (!intent) {
                return Output::err(r, RuntimeError::request_intent(r, intent.error()));
            }
            intent_.emplace(std::move(intent).value());
        }
        while (index_ < options_.facets.size()) {
            if (!query_) {
                const auto& facet = options_.facets[index_];
                const auto& context = runtime_.get().user_context();
                auto* descriptor = context.entity(outer_.entity);
                if (!descriptor) {
                    return Output::err(r, RuntimeError::missing_entity(r, outer_.entity));
                }
                auto* relation = descriptor->relation_by_name(facet.relation_name);
                if (!relation) {
                    return Output::err(
                        r, RuntimeError::missing_relation(r, outer_.entity, facet.relation_name));
                }
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
                nested_options_.emplace(std::allocator_arg, Allocator(r), selection.query_options);
                auto query = std::move(selection).into_query();
                query.comment_text.emplace(intent_->comment(), r);
                query.trace_chain = clone_in(r, outer_.trace_chain);
                String edge(outer_.entity, r);
                edge += ".";
                edge += relation->name;
                query.trace_chain.emplace_back(TraceKind::Relation, relation->name, edge);
                name_ = facet.facet_name;
                query_.emplace(std::allocator_arg, Allocator(r), query, intent_->purpose());
                query_->diagnostic_source_.emplace(
                    std::allocator_arg, Allocator(r), diagnostic_source_);
                auto& child = runtime_.get().fetch_facet_smart_list(
                    query_->as_query().entity, *query_, *aggregates_, {});
                if (child.memory_resource() != r) {
                    throw std::invalid_argument(
                        "Facet result operation belongs to another Context");
                }
                pending_ = &child;
                forward_.attach(child.wake_signal(), wake);
            }
            if (pending_) {
                auto result = pending_->poll();
                if (!result) {
                    return {};
                }
                forward_.reset();
                pending_ = nullptr;
                if (!*result) {
                    return Output::err(r, clone_in(r, result->error()));
                }
                rows_.emplace(std::allocator_arg, Allocator(r), result->value());
                nested_ = &make_operation<Output, FacetExecution>(
                    r, runtime_.get(), query_->as_query(), *nested_options_, diagnostic_source_);
                forward_.attach(nested_->wake_signal(), wake);
            }
            auto nested_result = nested_->poll();
            if (!nested_result) {
                return {};
            }
            forward_.reset();
            nested_ = nullptr;
            if (!*nested_result) {
                return Output::err(r, clone_in(r, nested_result->error()));
            }
            rows_->facets = clone_in(r, nested_result->value());
            results_.insert_or_assign(
                String(name_, r),
                SmartList<CompactRow>(std::allocator_arg, Allocator(r), std::move(*rows_)));
            rows_.reset();
            query_.reset();
            aggregates_.reset();
            nested_options_.reset();
            ++index_;
        }
        return Output::ok(r, std::move(results_));
    }

    void cancel_and_wait() noexcept {
        forward_.reset();
        if (nested_) {
            nested_->cancel();
            nested_ = nullptr;
        }
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
    SelectQuery diagnostic_source_;
    std::optional<QueryIntent> intent_;
    FacetResults results_;
    std::size_t index_ = 0;
    String name_;
    std::optional<Vector<RelationAggregate>> aggregates_;
    std::optional<PurposedSelectQuery> query_;
    std::optional<request::QueryOptions> nested_options_;
    std::optional<SmartList<CompactRow>> rows_;
    AsyncOperation<Result<SmartList<CompactRow>, RuntimeError>>* pending_ = nullptr;
    AsyncOperation<Output>* nested_ = nullptr;
    WakeSignal::Forward forward_;
};
}

AsyncOperation<Result<FacetResults, RuntimeError>>& execute_facets(
    const TeaqlRuntime& runtime, const SelectQuery& outer, const request::QueryOptions& options) {
    auto r = runtime.user_context().memory_resource();
    auto source = outer.clone(r);
    auto copied_options = detail::clone_in(r, options);
    for (const auto& facet : copied_options.facets) {
        source.child_enhancements.emplace_back(detail::facet_diagnostic_source(facet.query));
    }
    return make_operation<Result<FacetResults, RuntimeError>, detail::FacetExecution>(
        r, runtime, outer, copied_options, source);
}
}
