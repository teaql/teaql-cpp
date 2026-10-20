# Field Assist: sample_record.quantity

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `quantity()` / `updateQuantity(const std::int64_t&)`; changed `changedQuantity()`; evaluation `evalQuantity()`; expression `getQuantity()`.
- Scalar predicates: `withQuantityIs`, `withQuantityIsNot`, `withQuantityGreaterThan`, `withQuantityGreaterThanOrEqualTo`, `withQuantityLessThan`, `withQuantityLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withQuantityBetween(lower, upper)`, `withQuantityIn(std::span<const std::int64_t>)`, `withQuantityNotIn(std::span<const std::int64_t>)`, `withQuantityIsNull()`, `withQuantityIsNotNull()`.
- Projection/order/group: `selectQuantity()`, `orderByQuantityAsc()`, `orderByQuantityDesc()`, `groupByQuantity()`.
- Group/count: `groupByQuantity()`, `groupByQuantityAs(alias)`, `groupByQuantityWithFunction(alias, function)`, `countQuantity()` and `countQuantityAs(alias)`.
- Scalar aggregate: `sumQuantity()` uses alias `"sum_quantity"`; `sumQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgQuantity()` uses alias `"avg_quantity"`; `avgQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minQuantity()` uses alias `"min_quantity"`; `minQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxQuantity()` uses alias `"max_quantity"`; `maxQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationQuantity()` and `standardDeviationQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationQuantity()` and `squareRootOfPopulationStandardDeviationQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceQuantity()` and `sampleVarianceQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceQuantity()` and `samplePopulationVarianceQuantityAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withQuantityIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withQuantityIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
