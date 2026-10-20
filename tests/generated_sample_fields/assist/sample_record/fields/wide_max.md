# Field Assist: sample_record.wide_max

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `wideMax()` / `updateWideMax(const std::int64_t&)`; changed `changedWideMax()`; evaluation `evalWideMax()`; expression `getWideMax()`.
- Scalar predicates: `withWideMaxIs`, `withWideMaxIsNot`, `withWideMaxGreaterThan`, `withWideMaxGreaterThanOrEqualTo`, `withWideMaxLessThan`, `withWideMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withWideMaxBetween(lower, upper)`, `withWideMaxIn(std::span<const std::int64_t>)`, `withWideMaxNotIn(std::span<const std::int64_t>)`, `withWideMaxIsNull()`, `withWideMaxIsNotNull()`.
- Projection/order/group: `selectWideMax()`, `orderByWideMaxAsc()`, `orderByWideMaxDesc()`, `groupByWideMax()`.
- Group/count: `groupByWideMax()`, `groupByWideMaxAs(alias)`, `groupByWideMaxWithFunction(alias, function)`, `countWideMax()` and `countWideMaxAs(alias)`.
- Scalar aggregate: `sumWideMax()` uses alias `"sum_wide_max"`; `sumWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgWideMax()` uses alias `"avg_wide_max"`; `avgWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minWideMax()` uses alias `"min_wide_max"`; `minWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxWideMax()` uses alias `"max_wide_max"`; `maxWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationWideMax()` and `standardDeviationWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationWideMax()` and `squareRootOfPopulationStandardDeviationWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceWideMax()` and `sampleVarianceWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceWideMax()` and `samplePopulationVarianceWideMaxAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withWideMaxIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withWideMaxIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
