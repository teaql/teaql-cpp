# Field Assist: sample_item.value

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `value()` / `updateValue(const std::int64_t&)`; changed `changedValue()`; evaluation `evalValue()`; expression `getValue()`.
- Scalar predicates: `withValueIs`, `withValueIsNot`, `withValueGreaterThan`, `withValueGreaterThanOrEqualTo`, `withValueLessThan`, `withValueLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withValueBetween(lower, upper)`, `withValueIn(std::span<const std::int64_t>)`, `withValueNotIn(std::span<const std::int64_t>)`, `withValueIsNull()`, `withValueIsNotNull()`.
- Projection/order/group: `selectValue()`, `orderByValueAsc()`, `orderByValueDesc()`, `groupByValue()`.
- Group/count: `groupByValue()`, `groupByValueAs(alias)`, `groupByValueWithFunction(alias, function)`, `countValue()` and `countValueAs(alias)`.
- Scalar aggregate: `sumValue()` uses alias `"sum_value"`; `sumValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgValue()` uses alias `"avg_value"`; `avgValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minValue()` uses alias `"min_value"`; `minValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxValue()` uses alias `"max_value"`; `maxValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationValue()` and `standardDeviationValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationValue()` and `squareRootOfPopulationStandardDeviationValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceValue()` and `sampleVarianceValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceValue()` and `samplePopulationVarianceValueAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withValueIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withValueIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
