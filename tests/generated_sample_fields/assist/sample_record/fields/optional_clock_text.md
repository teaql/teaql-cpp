# Field Assist: sample_record.optional_clock_text

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalClockText()` / `updateOptionalClockText(const std::optional<String>&)`; changed `changedOptionalClockText()`; evaluation `evalOptionalClockText()`; expression `getOptionalClockText()`.
- Scalar predicates: `withOptionalClockTextIs`, `withOptionalClockTextIsNot`, `withOptionalClockTextGreaterThan`, `withOptionalClockTextGreaterThanOrEqualTo`, `withOptionalClockTextLessThan`, `withOptionalClockTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalClockTextBetween(lower, upper)`, `withOptionalClockTextIn(std::span<const String>)`, `withOptionalClockTextNotIn(std::span<const String>)`, `withOptionalClockTextIsNull()`, `withOptionalClockTextIsNotNull()`.
- Projection/order/group: `selectOptionalClockText()`, `orderByOptionalClockTextAsc()`, `orderByOptionalClockTextDesc()`, `groupByOptionalClockText()`.
- Group/count: `groupByOptionalClockText()`, `groupByOptionalClockTextAs(alias)`, `groupByOptionalClockTextWithFunction(alias, function)`, `countOptionalClockText()` and `countOptionalClockTextAs(alias)`.
- Scalar aggregate: `sumOptionalClockText()` uses alias `"sum_optional_clock_text"`; `sumOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalClockText()` uses alias `"avg_optional_clock_text"`; `avgOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalClockText()` uses alias `"min_optional_clock_text"`; `minOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalClockText()` uses alias `"max_optional_clock_text"`; `maxOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalClockTextContaining(text)`, `withOptionalClockTextStartingWith(text)`, `withOptionalClockTextEndingWith(text)`.

Null alias: `withOptionalClockTextIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalClockTextIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withOptionalClockTextNotContaining(value)`.

Text predicate: `withOptionalClockTextNotStartingWith(value)`.

Text predicate: `withOptionalClockTextNotEndingWith(value)`.

Text predicate: `withOptionalClockTextSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
