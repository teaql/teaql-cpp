# Field Assist: sample_record.signed_max

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `signedMax()` / `updateSignedMax(const std::int64_t&)`; changed `changedSignedMax()`; evaluation `evalSignedMax()`; expression `getSignedMax()`.
- Scalar predicates: `withSignedMaxIs`, `withSignedMaxIsNot`, `withSignedMaxGreaterThan`, `withSignedMaxGreaterThanOrEqualTo`, `withSignedMaxLessThan`, `withSignedMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withSignedMaxBetween(lower, upper)`, `withSignedMaxIn(std::span<const std::int64_t>)`, `withSignedMaxNotIn(std::span<const std::int64_t>)`, `withSignedMaxIsNull()`, `withSignedMaxIsNotNull()`.
- Projection/order/group: `selectSignedMax()`, `orderBySignedMaxAsc()`, `orderBySignedMaxDesc()`, `groupBySignedMax()`.
- Group/count: `groupBySignedMax()`, `groupBySignedMaxAs(alias)`, `groupBySignedMaxWithFunction(alias, function)`, `countSignedMax()` and `countSignedMaxAs(alias)`.
- Scalar aggregate: `sumSignedMax()` uses alias `"sum_signed_max"`; `sumSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgSignedMax()` uses alias `"avg_signed_max"`; `avgSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minSignedMax()` uses alias `"min_signed_max"`; `minSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxSignedMax()` uses alias `"max_signed_max"`; `maxSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationSignedMax()` and `standardDeviationSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationSignedMax()` and `squareRootOfPopulationStandardDeviationSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceSignedMax()` and `sampleVarianceSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceSignedMax()` and `samplePopulationVarianceSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withSignedMaxIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withSignedMaxIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
