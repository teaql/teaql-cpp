# Field Assist: note.optional_float

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalFloat()` / `updateOptionalFloat(const std::optional<float>&)`; changed `changedOptionalFloat()`; evaluation `evalOptionalFloat()`; expression `getOptionalFloat()`.
- Candidate filters: `withOptionalFloatIsFloatCreateFunctionNow()` and `withOptionalFloatIsNotFloatCreateFunctionNow()`; exact value `float(createFunction=now`.
- Candidate filters: `withOptionalFloatIsUpdateFunctionNow()` and `withOptionalFloatIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalFloatIs`, `withOptionalFloatIsNot`, `withOptionalFloatGreaterThan`, `withOptionalFloatGreaterThanOrEqualTo`, `withOptionalFloatLessThan`, `withOptionalFloatLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withOptionalFloatBetween(lower, upper)`, `withOptionalFloatIn(std::span<const float>)`, `withOptionalFloatNotIn(std::span<const float>)`, `withOptionalFloatIsNull()`, `withOptionalFloatIsNotNull()`.
- Projection/order/group: `selectOptionalFloat()`, `orderByOptionalFloatAsc()`, `orderByOptionalFloatDesc()`, `groupByOptionalFloat()`.
- Group/count: `groupByOptionalFloat()`, `groupByOptionalFloatAs(alias)`, `groupByOptionalFloatWithFunction(alias, function)`, `countOptionalFloat()` and `countOptionalFloatAs(alias)`.
- Scalar aggregate: `sumOptionalFloat()` uses alias `"sum_optional_float"`; `sumOptionalFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalFloat()` uses alias `"avg_optional_float"`; `avgOptionalFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalFloat()` uses alias `"min_optional_float"`; `minOptionalFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalFloat()` uses alias `"max_optional_float"`; `maxOptionalFloatAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalFloat()` and `standardDeviationOptionalFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalFloat()` and `squareRootOfPopulationStandardDeviationOptionalFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalFloat()` and `sampleVarianceOptionalFloatAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalFloat()` and `samplePopulationVarianceOptionalFloatAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOptionalFloatIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalFloatIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
