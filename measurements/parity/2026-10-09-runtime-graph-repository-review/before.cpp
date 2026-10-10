#include <teaql/runtime/data_service.hpp>
#include "../../../tests/relation_hydration_fixture.hpp"

void missing(teaql::EntityDataService<hydration_fixture::UnusedExecutor>& service,
             hydration_fixture::Model<0> entity) {
    (void)service.graph_node_from_entity(std::move(entity));
}
