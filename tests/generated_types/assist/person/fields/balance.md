# Field Assist: person.balance

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `balance()` / `updateBalance(const Decimal&)`; changed `changedBalance()`; evaluation `evalBalance()`; expression `getBalance()`.
- Candidate filters: `whoseBalanceIsNumber()` and `whoseBalanceIsNotNumber()`; exact value `number()`.
- Scalar predicates: `whoseBalanceIs`, `whoseBalanceIsNot`, `whoseBalanceGreaterThan`, `whoseBalanceGreaterThanOrEqualTo`, `whoseBalanceLessThan`, `whoseBalanceLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `whoseBalanceBetween(lower, upper)`, `whoseBalanceIn(std::span<const Decimal>)`, `whoseBalanceNotIn(std::span<const Decimal>)`, `whoseBalanceIsNull()`, `whoseBalanceIsNotNull()`.
- Projection/order/group: `selectBalance()`, `orderByBalanceAsc()`, `orderByBalanceDesc()`, `groupByBalance()`.
- Group/count: `groupByBalance()`, `groupByBalanceAs(alias)`, `groupByBalanceWithFunction(alias, function)`, `countBalance()` and `countBalanceAs(alias)`.
- Scalar aggregate: `sumBalance()` uses alias `"sum_balance"`; `sumBalanceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgBalance()` uses alias `"avg_balance"`; `avgBalanceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minBalance()` uses alias `"min_balance"`; `minBalanceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxBalance()` uses alias `"max_balance"`; `maxBalanceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationBalance()` and `standardDeviationBalanceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationBalance()` and `squareRootOfPopulationStandardDeviationBalanceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceBalance()` and `sampleVarianceBalanceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceBalance()` and `samplePopulationVarianceBalanceAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `whoseBalanceIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseBalanceIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
