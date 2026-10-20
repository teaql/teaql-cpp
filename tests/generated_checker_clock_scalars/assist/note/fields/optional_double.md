# Field Assist: note.optional_double

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalDouble()` / `updateOptionalDouble(const std::optional<double>&)`; changed `changedOptionalDouble()`; evaluation `evalOptionalDouble()`; expression `getOptionalDouble()`.
- Candidate filters: `withOptionalDoubleIsDoubleCreateFunctionNow()` and `withOptionalDoubleIsNotDoubleCreateFunctionNow()`; exact value `double(createFunction=now`.
- Candidate filters: `withOptionalDoubleIsUpdateFunctionNow()` and `withOptionalDoubleIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalDoubleIs`, `withOptionalDoubleIsNot`, `withOptionalDoubleGreaterThan`, `withOptionalDoubleGreaterThanOrEqualTo`, `withOptionalDoubleLessThan`, `withOptionalDoubleLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withOptionalDoubleBetween(lower, upper)`, `withOptionalDoubleIn(std::span<const double>)`, `withOptionalDoubleNotIn(std::span<const double>)`, `withOptionalDoubleIsNull()`, `withOptionalDoubleIsNotNull()`.
- Projection/order/group: `selectOptionalDouble()`, `orderByOptionalDoubleAsc()`, `orderByOptionalDoubleDesc()`, `groupByOptionalDouble()`.
- Group/count: `groupByOptionalDouble()`, `groupByOptionalDoubleAs(alias)`, `groupByOptionalDoubleWithFunction(alias, function)`, `countOptionalDouble()` and `countOptionalDoubleAs(alias)`.
- Scalar aggregate: `sumOptionalDouble()` uses alias `"sum_optional_double"`; `sumOptionalDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDouble()` uses alias `"avg_optional_double"`; `avgOptionalDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDouble()` uses alias `"min_optional_double"`; `minOptionalDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDouble()` uses alias `"max_optional_double"`; `maxOptionalDoubleAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalDouble()` and `standardDeviationOptionalDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalDouble()` and `squareRootOfPopulationStandardDeviationOptionalDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalDouble()` and `sampleVarianceOptionalDoubleAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalDouble()` and `samplePopulationVarianceOptionalDoubleAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOptionalDoubleIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalDoubleIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
