# Field Assist: record.choice

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `choice()` / `updateChoice(const Date&)`; changed `changedChoice()`; evaluation `evalChoice()`; expression `getChoice()`.
- Candidate filters: `withChoiceIsValue20261001()` and `withChoiceIsNotValue20261001()`; exact value `2026-10-01`.
- Candidate filters: `withChoiceIsValue20261002()` and `withChoiceIsNotValue20261002()`; exact value `2026-10-02`.
- Scalar predicates: `withChoiceIs`, `withChoiceIsNot`, `withChoiceGreaterThan`, `withChoiceGreaterThanOrEqualTo`, `withChoiceLessThan`, `withChoiceLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withChoiceBetween(lower, upper)`, `withChoiceIn(std::span<const Date>)`, `withChoiceNotIn(std::span<const Date>)`, `withChoiceIsNull()`, `withChoiceIsNotNull()`.
- Projection/order/group: `selectChoice()`, `orderByChoiceAsc()`, `orderByChoiceDesc()`, `groupByChoice()`.
- Group/count: `groupByChoice()`, `groupByChoiceAs(alias)`, `groupByChoiceWithFunction(alias, function)`, `countChoice()` and `countChoiceAs(alias)`.
- Scalar aggregate: `sumChoice()` uses alias `"sum_choice"`; `sumChoiceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgChoice()` uses alias `"avg_choice"`; `avgChoiceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minChoice()` uses alias `"min_choice"`; `minChoiceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxChoice()` uses alias `"max_choice"`; `maxChoiceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.

Null alias: `withChoiceIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withChoiceIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Date predicates: `withChoiceBefore(value)`, `withChoiceAfter(value)` and `withChoiceBetween(from, to)`.

[Model/action index](../../../ASSIST.md)
