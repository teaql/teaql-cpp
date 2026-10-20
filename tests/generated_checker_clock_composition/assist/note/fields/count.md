# Field Assist: note.count

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `count()` / `updateCount(const std::int64_t&)`; changed `changedCount()`; evaluation `evalCount()`; expression `getCount()`.
- Candidate filters: `withCountIsInteger()` and `withCountIsNotInteger()`; exact value `integer()`.
- Scalar predicates: `withCountIs`, `withCountIsNot`, `withCountGreaterThan`, `withCountGreaterThanOrEqualTo`, `withCountLessThan`, `withCountLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withCountBetween(lower, upper)`, `withCountIn(std::span<const std::int64_t>)`, `withCountNotIn(std::span<const std::int64_t>)`, `withCountIsNull()`, `withCountIsNotNull()`.
- Projection/order/group: `selectCount()`, `orderByCountAsc()`, `orderByCountDesc()`, `groupByCount()`.
- Group/count: `groupByCount()`, `groupByCountAs(alias)`, `groupByCountWithFunction(alias, function)`, `countCount()` and `countCountAs(alias)`.
- Scalar aggregate: `sumCount()` uses alias `"sum_count"`; `sumCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCount()` uses alias `"avg_count"`; `avgCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCount()` uses alias `"min_count"`; `minCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCount()` uses alias `"max_count"`; `maxCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationCount()` and `standardDeviationCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationCount()` and `squareRootOfPopulationStandardDeviationCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceCount()` and `sampleVarianceCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceCount()` and `samplePopulationVarianceCountAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withCountIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withCountIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
