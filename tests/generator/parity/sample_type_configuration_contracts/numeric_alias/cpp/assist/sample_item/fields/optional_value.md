# Field Assist: sample_item.optional_value

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalValue()` / `updateOptionalValue(const std::optional<String>&)`; changed `changedOptionalValue()`; evaluation `evalOptionalValue()`; expression `getOptionalValue()`.
- Scalar predicates: `withOptionalValueIs`, `withOptionalValueIsNot`, `withOptionalValueGreaterThan`, `withOptionalValueGreaterThanOrEqualTo`, `withOptionalValueLessThan`, `withOptionalValueLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalValueBetween(lower, upper)`, `withOptionalValueIn(std::span<const String>)`, `withOptionalValueNotIn(std::span<const String>)`, `withOptionalValueIsNull()`, `withOptionalValueIsNotNull()`.
- Projection/order/group: `selectOptionalValue()`, `orderByOptionalValueAsc()`, `orderByOptionalValueDesc()`, `groupByOptionalValue()`.
- Group/count: `groupByOptionalValue()`, `groupByOptionalValueAs(alias)`, `groupByOptionalValueWithFunction(alias, function)`, `countOptionalValue()` and `countOptionalValueAs(alias)`.
- Scalar aggregate: `sumOptionalValue()` uses alias `"sum_optional_value"`; `sumOptionalValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalValue()` uses alias `"avg_optional_value"`; `avgOptionalValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalValue()` uses alias `"min_optional_value"`; `minOptionalValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalValue()` uses alias `"max_optional_value"`; `maxOptionalValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalValue()` and `standardDeviationOptionalValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalValue()` and `squareRootOfPopulationStandardDeviationOptionalValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalValue()` and `sampleVarianceOptionalValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalValue()` and `samplePopulationVarianceOptionalValueAs(alias)`; SQLite requires the corresponding SQL function.
- Text predicates: `withOptionalValueContaining(text)`, `withOptionalValueStartingWith(text)`, `withOptionalValueEndingWith(text)`.

Null alias: `withOptionalValueIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalValueIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withOptionalValueNotContaining(value)`.

Text predicate: `withOptionalValueNotStartingWith(value)`.

Text predicate: `withOptionalValueNotEndingWith(value)`.

Text predicate: `withOptionalValueSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
