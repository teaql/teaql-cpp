# Field Assist: note.double

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `modelDouble()` / `updateModelDouble(const double&)`; changed `changedModelDouble()`; evaluation `evalModelDouble()`; expression `getModelDouble()`.
- Candidate filters: `withDoubleIsDoubleCreateFunctionNow()` and `withDoubleIsNotDoubleCreateFunctionNow()`; exact value `double(createFunction=now`.
- Candidate filters: `withDoubleIsUpdateFunctionNow()` and `withDoubleIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withDoubleIs`, `withDoubleIsNot`, `withDoubleGreaterThan`, `withDoubleGreaterThanOrEqualTo`, `withDoubleLessThan`, `withDoubleLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withDoubleBetween(lower, upper)`, `withDoubleIn(std::span<const double>)`, `withDoubleNotIn(std::span<const double>)`, `withDoubleIsNull()`, `withDoubleIsNotNull()`.
- Projection/order/group: `selectModelDouble()`, `orderByModelDoubleAsc()`, `orderByModelDoubleDesc()`, `groupByModelDouble()`.
- Group/count: `groupByModelDouble()`, `groupByModelDoubleAs(alias)`, `groupByModelDoubleWithFunction(alias, function)`, `countModelDouble()` and `countModelDoubleAs(alias)`.
- Scalar aggregate: `sumModelDouble()` uses alias `"sum_double"`; `sumModelDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelDouble()` uses alias `"avg_double"`; `avgModelDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelDouble()` uses alias `"min_double"`; `minModelDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelDouble()` uses alias `"max_double"`; `maxModelDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelDouble()` and `standardDeviationModelDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelDouble()` and `squareRootOfPopulationStandardDeviationModelDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelDouble()` and `sampleVarianceModelDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelDouble()` and `samplePopulationVarianceModelDoubleAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withDoubleIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withDoubleIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
