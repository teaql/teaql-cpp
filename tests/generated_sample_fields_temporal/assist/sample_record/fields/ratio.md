# Field Assist: sample_record.ratio

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `ratio()` / `updateRatio(const std::optional<float>&)`; changed `changedRatio()`; evaluation `evalRatio()`; expression `getRatio()`.
- Scalar predicates: `withRatioIs`, `withRatioIsNot`, `withRatioGreaterThan`, `withRatioGreaterThanOrEqualTo`, `withRatioLessThan`, `withRatioLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withRatioBetween(lower, upper)`, `withRatioIn(std::span<const float>)`, `withRatioNotIn(std::span<const float>)`, `withRatioIsNull()`, `withRatioIsNotNull()`.
- Projection/order/group: `selectRatio()`, `orderByRatioAsc()`, `orderByRatioDesc()`, `groupByRatio()`.
- Group/count: `groupByRatio()`, `groupByRatioAs(alias)`, `groupByRatioWithFunction(alias, function)`, `countRatio()` and `countRatioAs(alias)`.
- Scalar aggregate: `sumRatio()` uses alias `"sum_ratio"`; `sumRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRatio()` uses alias `"avg_ratio"`; `avgRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRatio()` uses alias `"min_ratio"`; `minRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRatio()` uses alias `"max_ratio"`; `maxRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationRatio()` and `standardDeviationRatioAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationRatio()` and `squareRootOfPopulationStandardDeviationRatioAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceRatio()` and `sampleVarianceRatioAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceRatio()` and `samplePopulationVarianceRatioAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withRatioIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withRatioIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
