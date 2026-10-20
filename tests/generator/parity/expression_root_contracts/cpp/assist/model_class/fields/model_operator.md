# Field Assist: class.operator

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `modelOperator()` / `updateModelOperator(const Decimal&)`; changed `changedModelOperator()`; evaluation `evalModelOperator()`; expression `getModelOperator()`.
- Scalar predicates: `withOperatorIs`, `withOperatorIsNot`, `withOperatorGreaterThan`, `withOperatorGreaterThanOrEqualTo`, `withOperatorLessThan`, `withOperatorLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withOperatorBetween(lower, upper)`, `withOperatorIn(std::span<const Decimal>)`, `withOperatorNotIn(std::span<const Decimal>)`, `withOperatorIsNull()`, `withOperatorIsNotNull()`.
- Projection/order/group: `selectModelOperator()`, `orderByModelOperatorAsc()`, `orderByModelOperatorDesc()`, `groupByModelOperator()`.
- Group/count: `groupByModelOperator()`, `groupByModelOperatorAs(alias)`, `groupByModelOperatorWithFunction(alias, function)`, `countModelOperator()` and `countModelOperatorAs(alias)`.
- Scalar aggregate: `sumModelOperator()` uses alias `"sum_operator"`; `sumModelOperatorAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgModelOperator()` uses alias `"avg_operator"`; `avgModelOperatorAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minModelOperator()` uses alias `"min_operator"`; `minModelOperatorAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxModelOperator()` uses alias `"max_operator"`; `maxModelOperatorAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationModelOperator()` and `standardDeviationModelOperatorAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationModelOperator()` and `squareRootOfPopulationStandardDeviationModelOperatorAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceModelOperator()` and `sampleVarianceModelOperatorAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceModelOperator()` and `samplePopulationVarianceModelOperatorAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withOperatorIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOperatorIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
