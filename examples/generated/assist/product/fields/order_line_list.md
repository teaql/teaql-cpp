# Field Assist: product.order_line_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalOrderLineList()`; query view handle: `orderLineListHandle()`.
- Reverse relation: `orderLineList()` returns SmartList<OrderLine>&; query `selectOrderLineList()`, `selectOrderLineListWith(OrderLineRequest<P>)`, `withOrderLineList(OrderLineRequest<P>)`; expression `getOrderLineList()`.

Presence: `haveOrderLines()`, `haveNoOrderLines()`. Matching: `withOrderLineListMatching(child)`, `withoutOrderLineListMatching(child)`.

Count: `countOrderLines()`, `countOrderLinesAs(alias)`, `countOrderLinesWith(alias, child)`. Statistics/details: `statsFromOrderLines(child)`, `statsFromOrderLinesAs(alias, child)`, `groupByOrderLinesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
