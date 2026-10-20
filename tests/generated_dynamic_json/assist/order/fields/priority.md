# Field Assist: order.priority

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `priority()` / `updatePriority(const std::int64_t&)`; changed `changedPriority()`; evaluation `evalPriority()`; expression `getPriority()`.
- Scalar predicates: `withPriorityIs`, `withPriorityIsNot`, `withPriorityGreaterThan`, `withPriorityGreaterThanOrEqualTo`, `withPriorityLessThan`, `withPriorityLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withPriorityBetween(lower, upper)`, `withPriorityIn(std::span<const std::int64_t>)`, `withPriorityNotIn(std::span<const std::int64_t>)`, `withPriorityIsNull()`, `withPriorityIsNotNull()`.
- Projection/order/group: `selectPriority()`, `orderByPriorityAsc()`, `orderByPriorityDesc()`, `groupByPriority()`.
- Group/count: `groupByPriority()`, `groupByPriorityAs(alias)`, `groupByPriorityWithFunction(alias, function)`, `countPriority()` and `countPriorityAs(alias)`.
- Scalar aggregate: `sumPriority()` uses alias `"sum_priority"`; `sumPriorityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPriority()` uses alias `"avg_priority"`; `avgPriorityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPriority()` uses alias `"min_priority"`; `minPriorityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPriority()` uses alias `"max_priority"`; `maxPriorityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationPriority()` and `standardDeviationPriorityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationPriority()` and `squareRootOfPopulationStandardDeviationPriorityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVariancePriority()` and `sampleVariancePriorityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVariancePriority()` and `samplePopulationVariancePriorityAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withPriorityIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withPriorityIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
