#pragma once
#include <teaql/runtime/data_service.hpp>
#include <teaql/core/request.hpp>

namespace teaql {
// Structural C++20 counterparts of the source query/entity service traits.
template <class R>
concept TeaqlQueryDataService = requires(const R& service,
                                         const PurposedSelectQuery& query,
                                         std::span<const RelationAggregate> aggregates) {
    typename R::Error;
    {
        service.fetch_all(query)
    } -> std::same_as<
          AsyncOperation<Result<Vector<CompactRow>, DataServiceError<typename R::Error>>>&>;
    {
        service.fetch_smart_list(query)
    } -> std::same_as<
          AsyncOperation<Result<SmartList<CompactRow>, DataServiceError<typename R::Error>>>&>;
    {
        service.fetch_smart_list_with_relation_aggregates(query, aggregates)
    } -> std::same_as<
          AsyncOperation<Result<SmartList<CompactRow>, DataServiceError<typename R::Error>>>&>;
    {
        service.fetch_stream(query)
    } -> std::same_as<AsyncOperation<
          Result<EntityQueryStream<typename R::Error>, DataServiceError<typename R::Error>>>&>;
};
template <class R, class T>
concept TeaqlEntityDataService =
    TeaqlQueryDataService<R> && Entity<T> &&
    requires(const R& service,
             const PurposedSelectQuery& query,
             std::span<const RelationAggregate> aggregates) {
        {
            service.template fetch_enhanced_entities<T>(query)
        }
        -> std::same_as<AsyncOperation<Result<SmartList<T>, DataServiceError<typename R::Error>>>&>;
        {
            service.template fetch_enhanced_entities_with_relation_aggregates<T>(query, aggregates)
        }
        -> std::same_as<AsyncOperation<Result<SmartList<T>, DataServiceError<typename R::Error>>>&>;
    };
template <TeaqlQueryDataService R>
using TeaqlDataServiceError = DataServiceError<typename R::Error>;

class TeaqlRuntime {
public:
    virtual ~TeaqlRuntime() = default;
    virtual const UserContext& user_context() const = 0;
    virtual AsyncOperation<Result<SmartList<CompactRow>, RuntimeError>>&
    fetch_facet_smart_list(std::string_view entity,
                           const PurposedSelectQuery&,
                           std::span<const RelationAggregate>,
                           std::span<const TraceNode> trace_context) const = 0;
};

// Hidden generated-support save contract remains distinct from AuditedSaveExt.
template <class T, class C>
concept AuditedSave = std::derived_from<C, TeaqlRuntime> && requires(T audited, const C& context) {
    typename T::Error;
    {
        std::move(audited).save(context)
    } -> std::same_as<AsyncOperation<Result<GraphNode, typename T::Error>>&>;
};

Result<request::QuerySelection, RuntimeError>
restrict_facet_to_outer_query(const TeaqlRuntime&,
                              request::QuerySelection,
                              const SelectQuery&,
                              std::string_view relation_name);
using FacetResults = std::pmr::map<String, SmartList<CompactRow>, std::less<>>;
AsyncOperation<Result<FacetResults, RuntimeError>>&
execute_facets(const TeaqlRuntime&, const SelectQuery&, const request::QueryOptions&);
}
