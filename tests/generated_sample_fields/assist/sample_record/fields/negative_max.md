# Field Assist: sample_record.negative_max

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `negativeMax()` / `updateNegativeMax(const std::int64_t&)`; changed `changedNegativeMax()`; evaluation `evalNegativeMax()`; expression `getNegativeMax()`.
- Scalar predicates: `withNegativeMaxIs`, `withNegativeMaxIsNot`, `withNegativeMaxGreaterThan`, `withNegativeMaxGreaterThanOrEqualTo`, `withNegativeMaxLessThan`, `withNegativeMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withNegativeMaxBetween(lower, upper)`, `withNegativeMaxIn(std::span<const std::int64_t>)`, `withNegativeMaxNotIn(std::span<const std::int64_t>)`, `withNegativeMaxIsNull()`, `withNegativeMaxIsNotNull()`.
- Projection/order/group: `selectNegativeMax()`, `orderByNegativeMaxAsc()`, `orderByNegativeMaxDesc()`, `groupByNegativeMax()`.
- Group/count: `groupByNegativeMax()`, `groupByNegativeMaxAs(alias)`, `groupByNegativeMaxWithFunction(alias, function)`, `countNegativeMax()` and `countNegativeMaxAs(alias)`.
- Scalar aggregate: `sumNegativeMax()` uses alias `"sum_negative_max"`; `sumNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNegativeMax()` uses alias `"avg_negative_max"`; `avgNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNegativeMax()` uses alias `"min_negative_max"`; `minNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNegativeMax()` uses alias `"max_negative_max"`; `maxNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationNegativeMax()` and `standardDeviationNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationNegativeMax()` and `squareRootOfPopulationStandardDeviationNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceNegativeMax()` and `sampleVarianceNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceNegativeMax()` and `samplePopulationVarianceNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withNegativeMaxIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withNegativeMaxIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
