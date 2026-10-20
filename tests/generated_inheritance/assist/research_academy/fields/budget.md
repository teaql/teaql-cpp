# Field Assist: research_academy.budget

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `budget()` / `updateBudget(const Decimal&)`; changed `changedBudget()`; evaluation `evalBudget()`; expression `getBudget()`.
- Scalar predicates: `withBudgetIs`, `withBudgetIsNot`, `withBudgetGreaterThan`, `withBudgetGreaterThanOrEqualTo`, `withBudgetLessThan`, `withBudgetLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withBudgetBetween(lower, upper)`, `withBudgetIn(std::span<const Decimal>)`, `withBudgetNotIn(std::span<const Decimal>)`, `withBudgetIsNull()`, `withBudgetIsNotNull()`.
- Projection/order/group: `selectBudget()`, `orderByBudgetAsc()`, `orderByBudgetDesc()`, `groupByBudget()`.
- Group/count: `groupByBudget()`, `groupByBudgetAs(alias)`, `groupByBudgetWithFunction(alias, function)`, `countBudget()` and `countBudgetAs(alias)`.
- Scalar aggregate: `sumBudget()` uses alias `"sum_budget"`; `sumBudgetAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgBudget()` uses alias `"avg_budget"`; `avgBudgetAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minBudget()` uses alias `"min_budget"`; `minBudgetAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxBudget()` uses alias `"max_budget"`; `maxBudgetAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationBudget()` and `standardDeviationBudgetAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationBudget()` and `squareRootOfPopulationStandardDeviationBudgetAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceBudget()` and `sampleVarianceBudgetAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceBudget()` and `samplePopulationVarianceBudgetAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withBudgetIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withBudgetIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
