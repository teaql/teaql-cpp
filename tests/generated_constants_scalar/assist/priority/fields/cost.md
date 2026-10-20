# Field Assist: priority.cost

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `cost()` / `updateCost(const Decimal&)`; changed `changedCost()`; evaluation `evalCost()`; expression `getCost()`.
- Candidate filters: `withCostIsValue1250()` and `withCostIsNotValue1250()`; exact value `12.50`.
- Candidate filters: `withCostIsValue3125()` and `withCostIsNotValue3125()`; exact value `-3.125`.
- Scalar predicates: `withCostIs`, `withCostIsNot`, `withCostGreaterThan`, `withCostGreaterThanOrEqualTo`, `withCostLessThan`, `withCostLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withCostBetween(lower, upper)`, `withCostIn(std::span<const Decimal>)`, `withCostNotIn(std::span<const Decimal>)`, `withCostIsNull()`, `withCostIsNotNull()`.
- Projection/order/group: `selectCost()`, `orderByCostAsc()`, `orderByCostDesc()`, `groupByCost()`.
- Group/count: `groupByCost()`, `groupByCostAs(alias)`, `groupByCostWithFunction(alias, function)`, `countCost()` and `countCostAs(alias)`.
- Scalar aggregate: `sumCost()` uses alias `"sum_cost"`; `sumCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCost()` uses alias `"avg_cost"`; `avgCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCost()` uses alias `"min_cost"`; `minCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCost()` uses alias `"max_cost"`; `maxCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationCost()` and `standardDeviationCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationCost()` and `squareRootOfPopulationStandardDeviationCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceCost()` and `sampleVarianceCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceCost()` and `samplePopulationVarianceCostAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withCostIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withCostIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
