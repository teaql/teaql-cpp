# Field Assist: record.match

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `match()` / `updateMatch(const Decimal&)`; changed `changedMatch()`; evaluation `evalMatch()`; expression `getMatch()`.
- Scalar predicates: `withMatchIs`, `withMatchIsNot`, `withMatchGreaterThan`, `withMatchGreaterThanOrEqualTo`, `withMatchLessThan`, `withMatchLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withMatchBetween(lower, upper)`, `withMatchIn(std::span<const Decimal>)`, `withMatchNotIn(std::span<const Decimal>)`, `withMatchIsNull()`, `withMatchIsNotNull()`.
- Projection/order/group: `selectMatch()`, `orderByMatchAsc()`, `orderByMatchDesc()`, `groupByMatch()`.
- Group/count: `groupByMatch()`, `groupByMatchAs(alias)`, `groupByMatchWithFunction(alias, function)`, `countMatch()` and `countMatchAs(alias)`.
- Scalar aggregate: `sumMatch()` uses alias `"sum_match_field"`; `sumMatchAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgMatch()` uses alias `"avg_match_field"`; `avgMatchAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minMatch()` uses alias `"min_match_field"`; `minMatchAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxMatch()` uses alias `"max_match_field"`; `maxMatchAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationMatch()` and `standardDeviationMatchAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationMatch()` and `squareRootOfPopulationStandardDeviationMatchAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceMatch()` and `sampleVarianceMatchAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceMatch()` and `samplePopulationVarianceMatchAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withMatchIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withMatchIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
