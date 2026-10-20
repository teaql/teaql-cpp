# Field Assist: entry.counter

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `counter()` / `updateCounter(const std::int64_t&)`; changed `changedCounter()`; evaluation `evalCounter()`; expression `getCounter()`.
- Scalar predicates: `withCounterIs`, `withCounterIsNot`, `withCounterGreaterThan`, `withCounterGreaterThanOrEqualTo`, `withCounterLessThan`, `withCounterLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withCounterBetween(lower, upper)`, `withCounterIn(std::span<const std::int64_t>)`, `withCounterNotIn(std::span<const std::int64_t>)`, `withCounterIsNull()`, `withCounterIsNotNull()`.
- Projection/order/group: `selectCounter()`, `orderByCounterAsc()`, `orderByCounterDesc()`, `groupByCounter()`.
- Group/count: `groupByCounter()`, `groupByCounterAs(alias)`, `groupByCounterWithFunction(alias, function)`, `countCounter()` and `countCounterAs(alias)`.
- Scalar aggregate: `sumCounter()` uses alias `"sum_counter"`; `sumCounterAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCounter()` uses alias `"avg_counter"`; `avgCounterAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCounter()` uses alias `"min_counter"`; `minCounterAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCounter()` uses alias `"max_counter"`; `maxCounterAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationCounter()` and `standardDeviationCounterAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationCounter()` and `squareRootOfPopulationStandardDeviationCounterAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceCounter()` and `sampleVarianceCounterAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceCounter()` and `samplePopulationVarianceCounterAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withCounterIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withCounterIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
