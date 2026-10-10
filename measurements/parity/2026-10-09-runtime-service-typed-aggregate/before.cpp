#define main teaql_existing_query_probe_main
#include "../../../tests/parity/entity_query_probe.cpp"
#undef main
#include "../../../tests/relation_hydration_fixture.hpp"

auto& missing(const EntityDataService<Mock>& service, const SelectQuery& query) {
    return detail::EntityDataServiceAccess::
        fetch_entities_with_relation_aggregates<Mock, hydration_fixture::Model<0>>(
            service, query, std::span<const RelationAggregate>{});
}
