# Field Assist: entry.optional_amount

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalAmount()` / `updateOptionalAmount(const std::optional<Decimal>&)`; changed `changedOptionalAmount()`; evaluation `evalOptionalAmount()`; expression `getOptionalAmount()`.
- Scalar predicates: `withOptionalAmountIs`, `withOptionalAmountIsNot`, `withOptionalAmountGreaterThan`, `withOptionalAmountGreaterThanOrEqualTo`, `withOptionalAmountLessThan`, `withOptionalAmountLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withOptionalAmountBetween(lower, upper)`, `withOptionalAmountIn(std::span<const Decimal>)`, `withOptionalAmountNotIn(std::span<const Decimal>)`, `withOptionalAmountIsNull()`, `withOptionalAmountIsNotNull()`.
- Projection/order/group: `selectOptionalAmount()`, `orderByOptionalAmountAsc()`, `orderByOptionalAmountDesc()`, `groupByOptionalAmount()`.
- Group/count: `groupByOptionalAmount()`, `groupByOptionalAmountAs(alias)`, `groupByOptionalAmountWithFunction(alias, function)`, `countOptionalAmount()` and `countOptionalAmountAs(alias)`.
- Scalar aggregate: `sumOptionalAmount()` uses alias `"sum_optional_amount"`; `sumOptionalAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalAmount()` uses alias `"avg_optional_amount"`; `avgOptionalAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalAmount()` uses alias `"min_optional_amount"`; `minOptionalAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalAmount()` uses alias `"max_optional_amount"`; `maxOptionalAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalAmount()` and `standardDeviationOptionalAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalAmount()` and `squareRootOfPopulationStandardDeviationOptionalAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalAmount()` and `sampleVarianceOptionalAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalAmount()` and `samplePopulationVarianceOptionalAmountAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOptionalAmountIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalAmountIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
