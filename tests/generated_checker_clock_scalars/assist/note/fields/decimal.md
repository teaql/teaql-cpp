# Field Assist: note.decimal

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `decimal()` / `updateDecimal(const Decimal&)`; changed `changedDecimal()`; evaluation `evalDecimal()`; expression `getDecimal()`.
- Candidate filters: `withDecimalIsNumberCreateFunctionNow()` and `withDecimalIsNotNumberCreateFunctionNow()`; exact value `number(createFunction=now`.
- Candidate filters: `withDecimalIsUpdateFunctionNow()` and `withDecimalIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withDecimalIs`, `withDecimalIsNot`, `withDecimalGreaterThan`, `withDecimalGreaterThanOrEqualTo`, `withDecimalLessThan`, `withDecimalLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withDecimalBetween(lower, upper)`, `withDecimalIn(std::span<const Decimal>)`, `withDecimalNotIn(std::span<const Decimal>)`, `withDecimalIsNull()`, `withDecimalIsNotNull()`.
- Projection/order/group: `selectDecimal()`, `orderByDecimalAsc()`, `orderByDecimalDesc()`, `groupByDecimal()`.
- Group/count: `groupByDecimal()`, `groupByDecimalAs(alias)`, `groupByDecimalWithFunction(alias, function)`, `countDecimal()` and `countDecimalAs(alias)`.
- Scalar aggregate: `sumDecimal()` uses alias `"sum_decimal"`; `sumDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDecimal()` uses alias `"avg_decimal"`; `avgDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDecimal()` uses alias `"min_decimal"`; `minDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDecimal()` uses alias `"max_decimal"`; `maxDecimalAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationDecimal()` and `standardDeviationDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationDecimal()` and `squareRootOfPopulationStandardDeviationDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceDecimal()` and `sampleVarianceDecimalAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceDecimal()` and `samplePopulationVarianceDecimalAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withDecimalIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withDecimalIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
