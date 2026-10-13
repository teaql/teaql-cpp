#pragma once
#include <teaql/runtime/data_service.hpp>
#include <teaql/core/request.hpp>
#include <teaql/runtime/facet_runtime.hpp>

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

// Hidden generated-support save contract remains distinct from AuditedSaveExt.
template <class T, class C>
concept AuditedSave = std::derived_from<C, TeaqlRuntime> && requires(T audited, const C& context) {
    typename T::Error;
    {
        std::move(audited).save(context)
    } -> std::same_as<AsyncOperation<Result<GraphNode, typename T::Error>>&>;
};

}
