# Field Assist: note.optional_integer

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalInteger()` / `updateOptionalInteger(const std::optional<std::int64_t>&)`; changed `changedOptionalInteger()`; evaluation `evalOptionalInteger()`; expression `getOptionalInteger()`.
- Candidate filters: `withOptionalIntegerIsIntegerCreateFunctionNow()` and `withOptionalIntegerIsNotIntegerCreateFunctionNow()`; exact value `integer(createFunction=now`.
- Candidate filters: `withOptionalIntegerIsUpdateFunctionNow()` and `withOptionalIntegerIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalIntegerIs`, `withOptionalIntegerIsNot`, `withOptionalIntegerGreaterThan`, `withOptionalIntegerGreaterThanOrEqualTo`, `withOptionalIntegerLessThan`, `withOptionalIntegerLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalIntegerBetween(lower, upper)`, `withOptionalIntegerIn(std::span<const std::int64_t>)`, `withOptionalIntegerNotIn(std::span<const std::int64_t>)`, `withOptionalIntegerIsNull()`, `withOptionalIntegerIsNotNull()`.
- Projection/order/group: `selectOptionalInteger()`, `orderByOptionalIntegerAsc()`, `orderByOptionalIntegerDesc()`, `groupByOptionalInteger()`.
- Group/count: `groupByOptionalInteger()`, `groupByOptionalIntegerAs(alias)`, `groupByOptionalIntegerWithFunction(alias, function)`, `countOptionalInteger()` and `countOptionalIntegerAs(alias)`.
- Scalar aggregate: `sumOptionalInteger()` uses alias `"sum_optional_integer"`; `sumOptionalIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalInteger()` uses alias `"avg_optional_integer"`; `avgOptionalIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalInteger()` uses alias `"min_optional_integer"`; `minOptionalIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalInteger()` uses alias `"max_optional_integer"`; `maxOptionalIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalInteger()` and `standardDeviationOptionalIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalInteger()` and `squareRootOfPopulationStandardDeviationOptionalIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalInteger()` and `sampleVarianceOptionalIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalInteger()` and `samplePopulationVarianceOptionalIntegerAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOptionalIntegerIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalIntegerIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
