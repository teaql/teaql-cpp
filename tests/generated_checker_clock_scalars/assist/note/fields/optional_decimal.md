# Field Assist: note.optional_decimal

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalDecimal()` / `updateOptionalDecimal(const std::optional<Decimal>&)`; changed `changedOptionalDecimal()`; evaluation `evalOptionalDecimal()`; expression `getOptionalDecimal()`.
- Candidate filters: `withOptionalDecimalIsNumberCreateFunctionNow()` and `withOptionalDecimalIsNotNumberCreateFunctionNow()`; exact value `number(createFunction=now`.
- Candidate filters: `withOptionalDecimalIsUpdateFunctionNow()` and `withOptionalDecimalIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalDecimalIs`, `withOptionalDecimalIsNot`, `withOptionalDecimalGreaterThan`, `withOptionalDecimalGreaterThanOrEqualTo`, `withOptionalDecimalLessThan`, `withOptionalDecimalLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withOptionalDecimalBetween(lower, upper)`, `withOptionalDecimalIn(std::span<const Decimal>)`, `withOptionalDecimalNotIn(std::span<const Decimal>)`, `withOptionalDecimalIsNull()`, `withOptionalDecimalIsNotNull()`.
- Projection/order/group: `selectOptionalDecimal()`, `orderByOptionalDecimalAsc()`, `orderByOptionalDecimalDesc()`, `groupByOptionalDecimal()`.
- Group/count: `groupByOptionalDecimal()`, `groupByOptionalDecimalAs(alias)`, `groupByOptionalDecimalWithFunction(alias, function)`, `countOptionalDecimal()` and `countOptionalDecimalAs(alias)`.
- Scalar aggregate: `sumOptionalDecimal()` uses alias `"sum_optional_decimal"`; `sumOptionalDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDecimal()` uses alias `"avg_optional_decimal"`; `avgOptionalDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDecimal()` uses alias `"min_optional_decimal"`; `minOptionalDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDecimal()` uses alias `"max_optional_decimal"`; `maxOptionalDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalDecimal()` and `standardDeviationOptionalDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalDecimal()` and `squareRootOfPopulationStandardDeviationOptionalDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalDecimal()` and `sampleVarianceOptionalDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalDecimal()` and `samplePopulationVarianceOptionalDecimalAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOptionalDecimalIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalDecimalIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
