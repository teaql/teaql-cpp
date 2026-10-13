#pragma once
#include <teaql/runtime/detail/query_results.hpp>
#include <teaql/core/request.hpp>

namespace teaql::detail {
// Rebuild builder state at a snapshot boundary, preserving the retained core API.
inline request::QuerySelection
capture_generated_selection(const SelectQuery& core,
                            const request::QueryOptions& origins,
                            std::span<const request::RelationFilter> filters,
                            std::span<const request::QuerySelection> children) {
    auto r = core.memory_resource();
    request::QuerySelection selection(std::allocator_arg, Allocator(r), core);
    auto& query = selection.query;
    auto& options = selection.query_options;
    if (query.comment_text) {
        options.comment.emplace(*query.comment_text, r);
        query.comment_text.reset();
        std::erase_if(query.trace_chain, [](const auto& node) {
            return node.kind == TraceKind::Comment;
        });
    }
    if (query.raw_sql_text) {
        options.raw_sql.emplace(*query.raw_sql_text, r);
    }
    query.raw_sql_text.reset();
    options.raw_sql_search_criteria = std::move(query.raw_sql_search_criteria);
    for (const auto& property : query.dynamic_properties) {
        options.dynamic_properties.emplace_back(
            property.property_name,
            request::UnsafeRawSqlSegment::trusted(r, property.raw_sql_segment));
    }
    query.dynamic_properties.clear();
    for (const auto& projection : query.raw_projections) {
        options.raw_projections.emplace_back(
            projection.property_name,
            request::UnsafeRawSqlSegment::trusted(r, projection.raw_sql_segment));
    }
    query.raw_projections.clear();
    for (const auto& aggregate : origins.relation_aggregates) {
        options.relation_aggregates.emplace_back(aggregate);
    }
    for (const auto& group : origins.object_group_bys) {
        options.object_group_bys.emplace_back(group);
    }
    for (const auto& facet : query.facets) {
        options.facets.emplace_back(facet);
    }
    query.relation_aggregates.clear();
    query.object_group_bys.clear();
    query.facets.clear();
    for (const auto& filter : filters) {
        selection.relation_filters.emplace_back(filter);
    }
    for (const auto& child : children) {
        selection.child_enhancements.emplace_back(child);
    }
    query.child_enhancements.clear();
    return selection;
}

template <Entity T, class E>
class GeneratedFacetListOperation {
public:
    using allocator_type = Allocator;
    using Error = DataServiceError<typename E::Error>;
    using Output = Result<SmartList<T>, Error>;
    using FacetOutput = Result<SmartList<CompactRow>, Error>;

    GeneratedFacetListOperation(std::allocator_arg_t,
                                Allocator a,
                                const EntityDataService<E>& service,
                                const PurposedSelectQuery& query)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query),
          facets_(a.resource()) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!rows_) {
            if (!main_) {
                main_ = &service_.template fetch_enhanced_entities_with_relation_aggregates<T>(
                    query_, query_.as_query().relation_aggregates);
            }
            forwarding_.attach(main_->wake_signal(), wake);
            if (!main_->poll()) {
                return {};
            }
            forwarding_.reset();
            auto result = main_->take_result();
            if (!result) {
                return Output::err(r, generated_query_error(r, result.error()));
            }
            rows_.emplace(std::move(result).value());
        }
        const auto& outer = query_.as_query();
        while (index_ < outer.facets.size()) {
            const auto& facet = outer.facets[index_];
            if (!facet_work_) {
                auto selection =
                    request::QuerySelection(std::allocator_arg, Allocator(r), facet.query);
                request::merge_outer_filter_into_facet_aggregates(selection, outer);
                const auto& context = EntityDataServiceAccess::context(service_);
                if (!facet.include_all_facets) {
                    auto* descriptor = context.entity(outer.entity);
                    if (!descriptor) {
                        String message("missing entity: ", r);
                        message += outer.entity;
                        return Output::err(r, Error::runtime(r, RuntimeError::graph(r, message)));
                    }
                    auto* relation = descriptor->relation_by_name(facet.relation_name);
                    if (!relation) {
                        return Output::err(
                            r,
                            Error::runtime(r,
                                           RuntimeError::missing_relation(
                                               r, outer.entity, facet.relation_name)));
                    }
                    auto subquery = outer.clone(r);
                    subquery.projection.clear();
                    subquery.expr_projection.clear();
                    subquery.order_by.clear();
                    subquery.slice.reset();
                    subquery.aggregates.clear();
                    subquery.group_by_fields.clear();
                    subquery.relations.clear();
                    selection.query.and_filter(Expr::in_subquery(
                        r, relation->foreign_key, *descriptor, subquery, relation->local_key));
                }
                auto aggregates = request::runtime_relation_aggregates(r, selection.query_options);
                auto query = request::apply_runtime_metadata(std::move(selection.query),
                                                             selection.query_options,
                                                             selection.child_enhancements);
                auto repository = context.template entity_data_service<E>(query.entity);
                if (!repository) {
                    return Output::err(
                        r,
                        Error::runtime(r, RuntimeError::graph(r, repository.error().to_string(r))));
                }
                facet_service_.emplace(std::allocator_arg, Allocator(r), repository.value().get());
                TraceChain chain(outer.trace_chain, r);
                chain.emplace_back(query.entity, facet.facet_name);
                facet_service_->with_trace_context(chain);
                String purpose("Calculate facet ", r);
                purpose += facet.facet_name;
                PurposedSelectQuery authorized(std::allocator_arg, Allocator(r), query, purpose);
                facet_work_ = &facet_service_->fetch_smart_list_with_relation_aggregates(
                    authorized, aggregates);
            }
            forwarding_.attach(facet_work_->wake_signal(), wake);
            if (!facet_work_->poll()) {
                return {};
            }
            forwarding_.reset();
            auto result = facet_work_->take_result();
            if (!result) {
                return Output::err(r, generated_query_error(r, result.error()));
            }
            facets_.insert_or_assign(String(facet.facet_name, r), std::move(result).value());
            facet_work_ = nullptr;
            facet_service_.reset();
            ++index_;
        }
        request::attach_facets(*rows_, facets_);
        return Output::ok(r, std::move(*rows_));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        main_ = nullptr;
        facet_work_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (main_) {
            main_->cancel();
        }
        if (facet_work_) {
            facet_work_->cancel();
        }
        main_ = nullptr;
        facet_work_ = nullptr;
    }

private:
    EntityDataService<E> service_;
    PurposedSelectQuery query_;
    std::optional<SmartList<T>> rows_;
    std::pmr::map<String, SmartList<CompactRow>, std::less<>> facets_;
    std::optional<EntityDataService<E>> facet_service_;
    AsyncOperation<Output>* main_ = nullptr;
    AsyncOperation<FacetOutput>* facet_work_ = nullptr;
    WakeSignal::Forward forwarding_;
    std::size_t index_ = 0;
};

template <Entity T, class E>
auto& execute_generated_list_query(const EntityDataService<E>& service,
                                   const PurposedSelectQuery& query) {
    if (query.as_query().facets.empty()) {
        return generated_query_result(
            service.template fetch_enhanced_entities_with_relation_aggregates<T>(
                query, query.as_query().relation_aggregates));
    }
    using State = GeneratedFacetListOperation<T, E>;
    return make_operation<typename State::Output, State>(service.memory_resource(), service, query);
}
}
