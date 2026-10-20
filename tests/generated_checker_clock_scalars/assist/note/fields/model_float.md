# Field Assist: note.float

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `modelFloat()` / `updateModelFloat(const float&)`; changed `changedModelFloat()`; evaluation `evalModelFloat()`; expression `getModelFloat()`.
- Candidate filters: `withFloatIsFloatCreateFunctionNow()` and `withFloatIsNotFloatCreateFunctionNow()`; exact value `float(createFunction=now`.
- Candidate filters: `withFloatIsUpdateFunctionNow()` and `withFloatIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withFloatIs`, `withFloatIsNot`, `withFloatGreaterThan`, `withFloatGreaterThanOrEqualTo`, `withFloatLessThan`, `withFloatLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withFloatBetween(lower, upper)`, `withFloatIn(std::span<const float>)`, `withFloatNotIn(std::span<const float>)`, `withFloatIsNull()`, `withFloatIsNotNull()`.
- Projection/order/group: `selectModelFloat()`, `orderByModelFloatAsc()`, `orderByModelFloatDesc()`, `groupByModelFloat()`.
- Group/count: `groupByModelFloat()`, `groupByModelFloatAs(alias)`, `groupByModelFloatWithFunction(alias, function)`, `countModelFloat()` and `countModelFloatAs(alias)`.
- Scalar aggregate: `sumModelFloat()` uses alias `"sum_float"`; `sumModelFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelFloat()` uses alias `"avg_float"`; `avgModelFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelFloat()` uses alias `"min_float"`; `minModelFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelFloat()` uses alias `"max_float"`; `maxModelFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelFloat()` and `standardDeviationModelFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelFloat()` and `squareRootOfPopulationStandardDeviationModelFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelFloat()` and `sampleVarianceModelFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelFloat()` and `samplePopulationVarianceModelFloatAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withFloatIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withFloatIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
