# Field Assist: note.integer

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `integer()` / `updateInteger(const std::int64_t&)`; changed `changedInteger()`; evaluation `evalInteger()`; expression `getInteger()`.
- Candidate filters: `withIntegerIsIntegerCreateFunctionNow()` and `withIntegerIsNotIntegerCreateFunctionNow()`; exact value `integer(createFunction=now`.
- Candidate filters: `withIntegerIsUpdateFunctionNow()` and `withIntegerIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withIntegerIs`, `withIntegerIsNot`, `withIntegerGreaterThan`, `withIntegerGreaterThanOrEqualTo`, `withIntegerLessThan`, `withIntegerLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withIntegerBetween(lower, upper)`, `withIntegerIn(std::span<const std::int64_t>)`, `withIntegerNotIn(std::span<const std::int64_t>)`, `withIntegerIsNull()`, `withIntegerIsNotNull()`.
- Projection/order/group: `selectInteger()`, `orderByIntegerAsc()`, `orderByIntegerDesc()`, `groupByInteger()`.
- Group/count: `groupByInteger()`, `groupByIntegerAs(alias)`, `groupByIntegerWithFunction(alias, function)`, `countInteger()` and `countIntegerAs(alias)`.
- Scalar aggregate: `sumInteger()` uses alias `"sum_integer"`; `sumIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgInteger()` uses alias `"avg_integer"`; `avgIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minInteger()` uses alias `"min_integer"`; `minIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxInteger()` uses alias `"max_integer"`; `maxIntegerAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationInteger()` and `standardDeviationIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationInteger()` and `squareRootOfPopulationStandardDeviationIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceInteger()` and `sampleVarianceIntegerAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceInteger()` and `samplePopulationVarianceIntegerAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withIntegerIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withIntegerIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
