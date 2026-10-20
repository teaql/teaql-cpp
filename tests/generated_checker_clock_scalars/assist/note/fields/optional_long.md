# Field Assist: note.optional_long

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalLong()` / `updateOptionalLong(const std::optional<std::int64_t>&)`; changed `changedOptionalLong()`; evaluation `evalOptionalLong()`; expression `getOptionalLong()`.
- Candidate filters: `withOptionalLongIsLongCreateFunctionNow()` and `withOptionalLongIsNotLongCreateFunctionNow()`; exact value `long(createFunction=now`.
- Candidate filters: `withOptionalLongIsUpdateFunctionNow()` and `withOptionalLongIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalLongIs`, `withOptionalLongIsNot`, `withOptionalLongGreaterThan`, `withOptionalLongGreaterThanOrEqualTo`, `withOptionalLongLessThan`, `withOptionalLongLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalLongBetween(lower, upper)`, `withOptionalLongIn(std::span<const std::int64_t>)`, `withOptionalLongNotIn(std::span<const std::int64_t>)`, `withOptionalLongIsNull()`, `withOptionalLongIsNotNull()`.
- Projection/order/group: `selectOptionalLong()`, `orderByOptionalLongAsc()`, `orderByOptionalLongDesc()`, `groupByOptionalLong()`.
- Group/count: `groupByOptionalLong()`, `groupByOptionalLongAs(alias)`, `groupByOptionalLongWithFunction(alias, function)`, `countOptionalLong()` and `countOptionalLongAs(alias)`.
- Scalar aggregate: `sumOptionalLong()` uses alias `"sum_optional_long"`; `sumOptionalLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalLong()` uses alias `"avg_optional_long"`; `avgOptionalLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalLong()` uses alias `"min_optional_long"`; `minOptionalLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalLong()` uses alias `"max_optional_long"`; `maxOptionalLongAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalLong()` and `standardDeviationOptionalLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalLong()` and `squareRootOfPopulationStandardDeviationOptionalLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalLong()` and `sampleVarianceOptionalLongAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalLong()` and `samplePopulationVarianceOptionalLongAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOptionalLongIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalLongIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
