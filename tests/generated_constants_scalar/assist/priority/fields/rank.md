# Field Assist: priority.rank

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `rank()` / `updateRank(const std::int64_t&)`; changed `changedRank()`; evaluation `evalRank()`; expression `getRank()`.
- Candidate filters: `withRankIsValue7()` and `withRankIsNotValue7()`; exact value `-7`.
- Candidate filters: `withRankIsValue9()` and `withRankIsNotValue9()`; exact value `9`.
- Scalar predicates: `withRankIs`, `withRankIsNot`, `withRankGreaterThan`, `withRankGreaterThanOrEqualTo`, `withRankLessThan`, `withRankLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withRankBetween(lower, upper)`, `withRankIn(std::span<const std::int64_t>)`, `withRankNotIn(std::span<const std::int64_t>)`, `withRankIsNull()`, `withRankIsNotNull()`.
- Projection/order/group: `selectRank()`, `orderByRankAsc()`, `orderByRankDesc()`, `groupByRank()`.
- Group/count: `groupByRank()`, `groupByRankAs(alias)`, `groupByRankWithFunction(alias, function)`, `countRank()` and `countRankAs(alias)`.
- Scalar aggregate: `sumRank()` uses alias `"sum_rank"`; `sumRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRank()` uses alias `"avg_rank"`; `avgRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRank()` uses alias `"min_rank"`; `minRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRank()` uses alias `"max_rank"`; `maxRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationRank()` and `standardDeviationRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationRank()` and `squareRootOfPopulationStandardDeviationRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceRank()` and `sampleVarianceRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceRank()` and `samplePopulationVarianceRankAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withRankIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withRankIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
