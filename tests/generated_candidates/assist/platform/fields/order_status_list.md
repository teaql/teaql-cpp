# Field Assist: platform.order_status_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalOrderStatusList()`; query view handle: `orderStatusListHandle()`.
- Reverse relation: `orderStatusList()` returns SmartList<OrderStatus>&; query `selectOrderStatusList()`, `selectOrderStatusListWith(OrderStatusRequest<P>)`, `withOrderStatusList(OrderStatusRequest<P>)`; expression `getOrderStatusList()`.

Presence: `haveOrderStatuses()`, `haveNoOrderStatuses()`. Matching: `withOrderStatusListMatching(child)`, `withoutOrderStatusListMatching(child)`.

Count: `countOrderStatuses()`, `countOrderStatusesAs(alias)`, `countOrderStatusesWith(alias, child)`. Statistics/details: `statsFromOrderStatuses(child)`, `statsFromOrderStatusesAs(alias, child)`, `groupByOrderStatusesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
