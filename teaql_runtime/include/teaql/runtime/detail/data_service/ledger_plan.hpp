#pragma once
#include <teaql/runtime/entity_save.hpp>
#include <teaql/runtime/detail/data_service/entity_service.hpp>

namespace teaql::detail {
template <class E>
Result<Vector<EntityKey>, RuntimeError>
EntityDataServiceAccess::order_new_ledger_keys(const EntityDataService<E>& service,
                                               std::span<const EntityKey> keys,
                                               const LedgerChanges& changes) {
    return detail::order_new_ledger_keys(context(service), keys, changes);
}
}
