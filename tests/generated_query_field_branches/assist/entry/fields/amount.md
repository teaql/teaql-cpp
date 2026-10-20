# Field Assist: entry.amount

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `amount()` / `updateAmount(const Decimal&)`; changed `changedAmount()`; evaluation `evalAmount()`; expression `getAmount()`.
- Scalar predicates: `withAmountIs`, `withAmountIsNot`, `withAmountGreaterThan`, `withAmountGreaterThanOrEqualTo`, `withAmountLessThan`, `withAmountLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withAmountBetween(lower, upper)`, `withAmountIn(std::span<const Decimal>)`, `withAmountNotIn(std::span<const Decimal>)`, `withAmountIsNull()`, `withAmountIsNotNull()`.
- Projection/order/group: `selectAmount()`, `orderByAmountAsc()`, `orderByAmountDesc()`, `groupByAmount()`.
- Group/count: `groupByAmount()`, `groupByAmountAs(alias)`, `groupByAmountWithFunction(alias, function)`, `countAmount()` and `countAmountAs(alias)`.
- Scalar aggregate: `sumAmount()` uses alias `"sum_amount"`; `sumAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgAmount()` uses alias `"avg_amount"`; `avgAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minAmount()` uses alias `"min_amount"`; `minAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxAmount()` uses alias `"max_amount"`; `maxAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationAmount()` and `standardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationAmount()` and `squareRootOfPopulationStandardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceAmount()` and `sampleVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceAmount()` and `samplePopulationVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withAmountIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withAmountIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
