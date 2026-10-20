# Field Assist: record.type

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `type()` / `updateType(const std::int64_t&)`; changed `changedType()`; evaluation `evalType()`; expression `getType()`.
- Scalar predicates: `withTypeIs`, `withTypeIsNot`, `withTypeGreaterThan`, `withTypeGreaterThanOrEqualTo`, `withTypeLessThan`, `withTypeLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withTypeBetween(lower, upper)`, `withTypeIn(std::span<const std::int64_t>)`, `withTypeNotIn(std::span<const std::int64_t>)`, `withTypeIsNull()`, `withTypeIsNotNull()`.
- Projection/order/group: `selectType()`, `orderByTypeAsc()`, `orderByTypeDesc()`, `groupByType()`.
- Group/count: `groupByType()`, `groupByTypeAs(alias)`, `groupByTypeWithFunction(alias, function)`, `countType()` and `countTypeAs(alias)`.
- Scalar aggregate: `sumType()` uses alias `"sum_type_field"`; `sumTypeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgType()` uses alias `"avg_type_field"`; `avgTypeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minType()` uses alias `"min_type_field"`; `minTypeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxType()` uses alias `"max_type_field"`; `maxTypeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationType()` and `standardDeviationTypeAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationType()` and `squareRootOfPopulationStandardDeviationTypeAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceType()` and `sampleVarianceTypeAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceType()` and `samplePopulationVarianceTypeAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withTypeIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withTypeIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
