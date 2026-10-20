# Field Assist: sample_record.zero_bound

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `zeroBound()` / `updateZeroBound(const std::int64_t&)`; changed `changedZeroBound()`; evaluation `evalZeroBound()`; expression `getZeroBound()`.
- Scalar predicates: `withZeroBoundIs`, `withZeroBoundIsNot`, `withZeroBoundGreaterThan`, `withZeroBoundGreaterThanOrEqualTo`, `withZeroBoundLessThan`, `withZeroBoundLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withZeroBoundBetween(lower, upper)`, `withZeroBoundIn(std::span<const std::int64_t>)`, `withZeroBoundNotIn(std::span<const std::int64_t>)`, `withZeroBoundIsNull()`, `withZeroBoundIsNotNull()`.
- Projection/order/group: `selectZeroBound()`, `orderByZeroBoundAsc()`, `orderByZeroBoundDesc()`, `groupByZeroBound()`.
- Group/count: `groupByZeroBound()`, `groupByZeroBoundAs(alias)`, `groupByZeroBoundWithFunction(alias, function)`, `countZeroBound()` and `countZeroBoundAs(alias)`.
- Scalar aggregate: `sumZeroBound()` uses alias `"sum_zero_bound"`; `sumZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgZeroBound()` uses alias `"avg_zero_bound"`; `avgZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minZeroBound()` uses alias `"min_zero_bound"`; `minZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxZeroBound()` uses alias `"max_zero_bound"`; `maxZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationZeroBound()` and `standardDeviationZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationZeroBound()` and `squareRootOfPopulationStandardDeviationZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceZeroBound()` and `sampleVarianceZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceZeroBound()` and `samplePopulationVarianceZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withZeroBoundIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withZeroBoundIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
