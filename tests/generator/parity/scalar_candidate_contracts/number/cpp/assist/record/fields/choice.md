# Field Assist: record.choice

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `choice()` / `updateChoice(const Decimal&)`; changed `changedChoice()`; evaluation `evalChoice()`; expression `getChoice()`.
- Candidate filters: `withChoiceIsValue15()` and `withChoiceIsNotValue15()`; exact value `1.5`.
- Candidate filters: `withChoiceIsValue25()` and `withChoiceIsNotValue25()`; exact value `2.5`.
- Scalar predicates: `withChoiceIs`, `withChoiceIsNot`, `withChoiceGreaterThan`, `withChoiceGreaterThanOrEqualTo`, `withChoiceLessThan`, `withChoiceLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withChoiceBetween(lower, upper)`, `withChoiceIn(std::span<const Decimal>)`, `withChoiceNotIn(std::span<const Decimal>)`, `withChoiceIsNull()`, `withChoiceIsNotNull()`.
- Projection/order/group: `selectChoice()`, `orderByChoiceAsc()`, `orderByChoiceDesc()`, `groupByChoice()`.
- Group/count: `groupByChoice()`, `groupByChoiceAs(alias)`, `groupByChoiceWithFunction(alias, function)`, `countChoice()` and `countChoiceAs(alias)`.
- Scalar aggregate: `sumChoice()` uses alias `"sum_choice"`; `sumChoiceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgChoice()` uses alias `"avg_choice"`; `avgChoiceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minChoice()` uses alias `"min_choice"`; `minChoiceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxChoice()` uses alias `"max_choice"`; `maxChoiceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationChoice()` and `standardDeviationChoiceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationChoice()` and `squareRootOfPopulationStandardDeviationChoiceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceChoice()` and `sampleVarianceChoiceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceChoice()` and `samplePopulationVarianceChoiceAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withChoiceIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withChoiceIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
