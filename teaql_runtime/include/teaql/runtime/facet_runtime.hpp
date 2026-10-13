#pragma once
#include <teaql/core/request.hpp>
#include <teaql/core/list.hpp>
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/purposed_query.hpp>
#include <teaql/async/operation.hpp>

namespace teaql {
class UserContext;

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

Result<request::QuerySelection, RuntimeError>
restrict_facet_to_outer_query(const TeaqlRuntime&,
                              request::QuerySelection,
                              const SelectQuery&,
                              std::string_view relation_name);
using FacetResults = std::pmr::map<String, SmartList<CompactRow>, std::less<>>;
AsyncOperation<Result<FacetResults, RuntimeError>>&
execute_facets(const TeaqlRuntime&, const SelectQuery&, const request::QueryOptions&);
}
