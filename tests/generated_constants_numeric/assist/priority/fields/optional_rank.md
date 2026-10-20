# Field Assist: priority.optional_rank

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalRank()` / `updateOptionalRank(const std::optional<std::int64_t>&)`; changed `changedOptionalRank()`; evaluation `evalOptionalRank()`; expression `getOptionalRank()`.
- Candidate filters: `withOptionalRankIsValue0()` and `withOptionalRankIsNotValue0()`; exact value `0`.
- Candidate filters: `withOptionalRankIsValue5()` and `withOptionalRankIsNotValue5()`; exact value `5`.
- Scalar predicates: `withOptionalRankIs`, `withOptionalRankIsNot`, `withOptionalRankGreaterThan`, `withOptionalRankGreaterThanOrEqualTo`, `withOptionalRankLessThan`, `withOptionalRankLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalRankBetween(lower, upper)`, `withOptionalRankIn(std::span<const std::int64_t>)`, `withOptionalRankNotIn(std::span<const std::int64_t>)`, `withOptionalRankIsNull()`, `withOptionalRankIsNotNull()`.
- Projection/order/group: `selectOptionalRank()`, `orderByOptionalRankAsc()`, `orderByOptionalRankDesc()`, `groupByOptionalRank()`.
- Group/count: `groupByOptionalRank()`, `groupByOptionalRankAs(alias)`, `groupByOptionalRankWithFunction(alias, function)`, `countOptionalRank()` and `countOptionalRankAs(alias)`.
- Scalar aggregate: `sumOptionalRank()` uses alias `"sum_optional_rank"`; `sumOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalRank()` uses alias `"avg_optional_rank"`; `avgOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalRank()` uses alias `"min_optional_rank"`; `minOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalRank()` uses alias `"max_optional_rank"`; `maxOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalRank()` and `standardDeviationOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalRank()` and `squareRootOfPopulationStandardDeviationOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalRank()` and `sampleVarianceOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalRank()` and `samplePopulationVarianceOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOptionalRankIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalRankIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
