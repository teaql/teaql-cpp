# Field Assist: record.ordinary

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `ordinary()` / `updateOrdinary(const Decimal&)`; changed `changedOrdinary()`; evaluation `evalOrdinary()`; expression `getOrdinary()`.
- Scalar predicates: `withOrdinaryIs`, `withOrdinaryIsNot`, `withOrdinaryGreaterThan`, `withOrdinaryGreaterThanOrEqualTo`, `withOrdinaryLessThan`, `withOrdinaryLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withOrdinaryBetween(lower, upper)`, `withOrdinaryIn(std::span<const Decimal>)`, `withOrdinaryNotIn(std::span<const Decimal>)`, `withOrdinaryIsNull()`, `withOrdinaryIsNotNull()`.
- Projection/order/group: `selectOrdinary()`, `orderByOrdinaryAsc()`, `orderByOrdinaryDesc()`, `groupByOrdinary()`.
- Group/count: `groupByOrdinary()`, `groupByOrdinaryAs(alias)`, `groupByOrdinaryWithFunction(alias, function)`, `countOrdinary()` and `countOrdinaryAs(alias)`.
- Scalar aggregate: `sumOrdinary()` uses alias `"sum_ordinary"`; `sumOrdinaryAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOrdinary()` uses alias `"avg_ordinary"`; `avgOrdinaryAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOrdinary()` uses alias `"min_ordinary"`; `minOrdinaryAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOrdinary()` uses alias `"max_ordinary"`; `maxOrdinaryAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOrdinary()` and `standardDeviationOrdinaryAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOrdinary()` and `squareRootOfPopulationStandardDeviationOrdinaryAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOrdinary()` and `sampleVarianceOrdinaryAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOrdinary()` and `samplePopulationVarianceOrdinaryAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOrdinaryIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOrdinaryIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
