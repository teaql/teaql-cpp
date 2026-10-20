# Field Assist: platform.order_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalOrderList()`; query view handle: `orderListHandle()`.
- Reverse relation: `orderList()` returns SmartList<Order>&; query `selectOrderList()`, `selectOrderListWith(OrderRequest<P>)`, `withOrderList(OrderRequest<P>)`; expression `getOrderList()`.

Presence: `haveOrders()`, `haveNoOrders()`. Matching: `withOrderListMatching(child)`, `withoutOrderListMatching(child)`.

Count: `countOrders()`, `countOrdersAs(alias)`, `countOrdersWith(alias, child)`. Statistics/details: `statsFromOrders(child)`, `statsFromOrdersAs(alias, child)`, `groupByOrdersWithDetails(child)`.

Field statistic: `sumAmountOfOrders()`, `sumAmountOfOrdersAs(alias, child)`.

Field statistic: `minAmountOfOrders()`, `minAmountOfOrdersAs(alias, child)`.

Field statistic: `maxAmountOfOrders()`, `maxAmountOfOrdersAs(alias, child)`.

Field statistic: `avgAmountOfOrders()`, `avgAmountOfOrdersAs(alias, child)`.

Field statistic: `standardDeviationAmountOfOrders()`, `standardDeviationAmountOfOrdersAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationAmountOfOrders()`, `squareRootOfPopulationStandardDeviationAmountOfOrdersAs(alias, child)`.

Field statistic: `sampleVarianceAmountOfOrders()`, `sampleVarianceAmountOfOrdersAs(alias, child)`.

Field statistic: `samplePopulationVarianceAmountOfOrders()`, `samplePopulationVarianceAmountOfOrdersAs(alias, child)`.

Field statistic: `sumPriorityOfOrders()`, `sumPriorityOfOrdersAs(alias, child)`.

Field statistic: `minPriorityOfOrders()`, `minPriorityOfOrdersAs(alias, child)`.

Field statistic: `maxPriorityOfOrders()`, `maxPriorityOfOrdersAs(alias, child)`.

Field statistic: `avgPriorityOfOrders()`, `avgPriorityOfOrdersAs(alias, child)`.

Field statistic: `standardDeviationPriorityOfOrders()`, `standardDeviationPriorityOfOrdersAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationPriorityOfOrders()`, `squareRootOfPopulationStandardDeviationPriorityOfOrdersAs(alias, child)`.

Field statistic: `sampleVariancePriorityOfOrders()`, `sampleVariancePriorityOfOrdersAs(alias, child)`.

Field statistic: `samplePopulationVariancePriorityOfOrders()`, `samplePopulationVariancePriorityOfOrdersAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
