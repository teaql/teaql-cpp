# Field Assist: priority.weight

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `weight()` / `updateWeight(const double&)`; changed `changedWeight()`; evaluation `evalWeight()`; expression `getWeight()`.
- Candidate filters: `withWeightIsValue25()` and `withWeightIsNotValue25()`; exact value `2.5`.
- Candidate filters: `withWeightIsValue475()` and `withWeightIsNotValue475()`; exact value `-4.75`.
- Scalar predicates: `withWeightIs`, `withWeightIsNot`, `withWeightGreaterThan`, `withWeightGreaterThanOrEqualTo`, `withWeightLessThan`, `withWeightLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withWeightBetween(lower, upper)`, `withWeightIn(std::span<const double>)`, `withWeightNotIn(std::span<const double>)`, `withWeightIsNull()`, `withWeightIsNotNull()`.
- Projection/order/group: `selectWeight()`, `orderByWeightAsc()`, `orderByWeightDesc()`, `groupByWeight()`.
- Group/count: `groupByWeight()`, `groupByWeightAs(alias)`, `groupByWeightWithFunction(alias, function)`, `countWeight()` and `countWeightAs(alias)`.
- Scalar aggregate: `sumWeight()` uses alias `"sum_weight"`; `sumWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgWeight()` uses alias `"avg_weight"`; `avgWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minWeight()` uses alias `"min_weight"`; `minWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxWeight()` uses alias `"max_weight"`; `maxWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationWeight()` and `standardDeviationWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationWeight()` and `squareRootOfPopulationStandardDeviationWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceWeight()` and `sampleVarianceWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceWeight()` and `samplePopulationVarianceWeightAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withWeightIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withWeightIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
