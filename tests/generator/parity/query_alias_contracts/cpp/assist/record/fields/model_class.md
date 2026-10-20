# Field Assist: record.class

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `modelClass()` / `updateModelClass(const Decimal&)`; changed `changedModelClass()`; evaluation `evalModelClass()`; expression `getModelClass()`.
- Scalar predicates: `withClassIs`, `withClassIsNot`, `withClassGreaterThan`, `withClassGreaterThanOrEqualTo`, `withClassLessThan`, `withClassLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withClassBetween(lower, upper)`, `withClassIn(std::span<const Decimal>)`, `withClassNotIn(std::span<const Decimal>)`, `withClassIsNull()`, `withClassIsNotNull()`.
- Projection/order/group: `selectModelClass()`, `orderByModelClassAsc()`, `orderByModelClassDesc()`, `groupByModelClass()`.
- Group/count: `groupByModelClass()`, `groupByModelClassAs(alias)`, `groupByModelClassWithFunction(alias, function)`, `countModelClass()` and `countModelClassAs(alias)`.
- Scalar aggregate: `sumModelClass()` uses alias `"sum_class"`; `sumModelClassAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelClass()` uses alias `"avg_class"`; `avgModelClassAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelClass()` uses alias `"min_class"`; `minModelClassAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelClass()` uses alias `"max_class"`; `maxModelClassAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelClass()` and `standardDeviationModelClassAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelClass()` and `squareRootOfPopulationStandardDeviationModelClassAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelClass()` and `sampleVarianceModelClassAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelClass()` and `samplePopulationVarianceModelClassAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withClassIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withClassIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
