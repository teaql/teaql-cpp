# Field Assist: sample_record.optional_count

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalCount()` / `updateOptionalCount(const std::optional<std::int64_t>&)`; changed `changedOptionalCount()`; evaluation `evalOptionalCount()`; expression `getOptionalCount()`.
- Scalar predicates: `withOptionalCountIs`, `withOptionalCountIsNot`, `withOptionalCountGreaterThan`, `withOptionalCountGreaterThanOrEqualTo`, `withOptionalCountLessThan`, `withOptionalCountLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalCountBetween(lower, upper)`, `withOptionalCountIn(std::span<const std::int64_t>)`, `withOptionalCountNotIn(std::span<const std::int64_t>)`, `withOptionalCountIsNull()`, `withOptionalCountIsNotNull()`.
- Projection/order/group: `selectOptionalCount()`, `orderByOptionalCountAsc()`, `orderByOptionalCountDesc()`, `groupByOptionalCount()`.
- Group/count: `groupByOptionalCount()`, `groupByOptionalCountAs(alias)`, `groupByOptionalCountWithFunction(alias, function)`, `countOptionalCount()` and `countOptionalCountAs(alias)`.
- Scalar aggregate: `sumOptionalCount()` uses alias `"sum_optional_count"`; `sumOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalCount()` uses alias `"avg_optional_count"`; `avgOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalCount()` uses alias `"min_optional_count"`; `minOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalCount()` uses alias `"max_optional_count"`; `maxOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalCount()` and `standardDeviationOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalCount()` and `squareRootOfPopulationStandardDeviationOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalCount()` and `sampleVarianceOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalCount()` and `samplePopulationVarianceOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOptionalCountIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalCountIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
