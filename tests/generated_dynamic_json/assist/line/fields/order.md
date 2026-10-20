# Field Assist: line.order

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `orderId()` / `updateOrderId(const std::uint64_t&)`; changed `changedOrderId()`; evaluation `evalOrderId()`; expression `getOrderId()`.
- Scalar predicates: `withOrderIs`, `withOrderIsNot`, `withOrderGreaterThan`, `withOrderGreaterThanOrEqualTo`, `withOrderLessThan`, `withOrderLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withOrderBetween(lower, upper)`, `withOrderIn(std::span<const std::uint64_t>)`, `withOrderNotIn(std::span<const std::uint64_t>)`, `withOrderIsNull()`, `withOrderIsNotNull()`.
- Projection/order/group: `selectOrderId()`, `orderByOrderIdAsc()`, `orderByOrderIdDesc()`, `groupByOrder()`.
- Group/count: `groupByOrder()`, `groupByOrderAs(alias)`, `groupByOrderWithFunction(alias, function)`, `countOrder()` and `countOrderAs(alias)`.
- Forward relation: `order()` returns const Order*, `updateOrder(const Order&)`; query `selectOrder()`, `selectOrderWith(OrderRequest<P>)`, `withOrder(OrderRequest<P>)`; expression `getOrder()`.

Null alias: `withOrderIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOrderIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalOrder()`. Matching: `withOrderMatching(child)`, `withoutOrderMatching(child)`. Presence: `haveOrder()`, `haveNoOrder()`. Facets: `facetByOrderAs(name, child)`, `facetByOrderAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByOrderWith(child)`, `groupByOrderWithDetails()`, `groupByOrderWithDetailsFrom(child)`. Roll-up: `rollUpToOrder()`, `rollUpToOrderWith(child)`.

[Model/action index](../../../ASSIST.md)
