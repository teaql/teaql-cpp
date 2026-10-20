# Field Assist: person.ratio

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `ratio()` / `updateRatio(const float&)`; changed `changedRatio()`; evaluation `evalRatio()`; expression `getRatio()`.
- Candidate filters: `whoseRatioIsFloat()` and `whoseRatioIsNotFloat()`; exact value `float()`.
- Scalar predicates: `whoseRatioIs`, `whoseRatioIsNot`, `whoseRatioGreaterThan`, `whoseRatioGreaterThanOrEqualTo`, `whoseRatioLessThan`, `whoseRatioLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `whoseRatioBetween(lower, upper)`, `whoseRatioIn(std::span<const float>)`, `whoseRatioNotIn(std::span<const float>)`, `whoseRatioIsNull()`, `whoseRatioIsNotNull()`.
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

Null alias: `whoseRatioIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseRatioIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
