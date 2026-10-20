# Field Assist: entry.height

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `height()` / `updateHeight(const double&)`; changed `changedHeight()`; evaluation `evalHeight()`; expression `getHeight()`.
- Scalar predicates: `withHeightIs`, `withHeightIsNot`, `withHeightGreaterThan`, `withHeightGreaterThanOrEqualTo`, `withHeightLessThan`, `withHeightLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withHeightBetween(lower, upper)`, `withHeightIn(std::span<const double>)`, `withHeightNotIn(std::span<const double>)`, `withHeightIsNull()`, `withHeightIsNotNull()`.
- Projection/order/group: `selectHeight()`, `orderByHeightAsc()`, `orderByHeightDesc()`, `groupByHeight()`.
- Group/count: `groupByHeight()`, `groupByHeightAs(alias)`, `groupByHeightWithFunction(alias, function)`, `countHeight()` and `countHeightAs(alias)`.
- Scalar aggregate: `sumHeight()` uses alias `"sum_height"`; `sumHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgHeight()` uses alias `"avg_height"`; `avgHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minHeight()` uses alias `"min_height"`; `minHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxHeight()` uses alias `"max_height"`; `maxHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationHeight()` and `standardDeviationHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationHeight()` and `squareRootOfPopulationStandardDeviationHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceHeight()` and `sampleVarianceHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceHeight()` and `samplePopulationVarianceHeightAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withHeightIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withHeightIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
