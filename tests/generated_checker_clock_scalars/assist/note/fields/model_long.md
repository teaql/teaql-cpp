# Field Assist: note.long

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `modelLong()` / `updateModelLong(const std::int64_t&)`; changed `changedModelLong()`; evaluation `evalModelLong()`; expression `getModelLong()`.
- Candidate filters: `withLongIsLongCreateFunctionNow()` and `withLongIsNotLongCreateFunctionNow()`; exact value `long(createFunction=now`.
- Candidate filters: `withLongIsUpdateFunctionNow()` and `withLongIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withLongIs`, `withLongIsNot`, `withLongGreaterThan`, `withLongGreaterThanOrEqualTo`, `withLongLessThan`, `withLongLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withLongBetween(lower, upper)`, `withLongIn(std::span<const std::int64_t>)`, `withLongNotIn(std::span<const std::int64_t>)`, `withLongIsNull()`, `withLongIsNotNull()`.
- Projection/order/group: `selectModelLong()`, `orderByModelLongAsc()`, `orderByModelLongDesc()`, `groupByModelLong()`.
- Group/count: `groupByModelLong()`, `groupByModelLongAs(alias)`, `groupByModelLongWithFunction(alias, function)`, `countModelLong()` and `countModelLongAs(alias)`.
- Scalar aggregate: `sumModelLong()` uses alias `"sum_long"`; `sumModelLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelLong()` uses alias `"avg_long"`; `avgModelLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelLong()` uses alias `"min_long"`; `minModelLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelLong()` uses alias `"max_long"`; `maxModelLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelLong()` and `standardDeviationModelLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelLong()` and `squareRootOfPopulationStandardDeviationModelLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelLong()` and `sampleVarianceModelLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelLong()` and `samplePopulationVarianceModelLongAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withLongIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withLongIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
