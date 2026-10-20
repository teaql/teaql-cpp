# Field Assist: sample_record.clock_text

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `clockText()` / `updateClockText(std::string_view)`; changed `changedClockText()`; evaluation `evalClockText()`; expression `getClockText()`.
- Scalar predicates: `withClockTextIs`, `withClockTextIsNot`, `withClockTextGreaterThan`, `withClockTextGreaterThanOrEqualTo`, `withClockTextLessThan`, `withClockTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withClockTextBetween(lower, upper)`, `withClockTextIn(std::span<const String>)`, `withClockTextNotIn(std::span<const String>)`, `withClockTextIsNull()`, `withClockTextIsNotNull()`.
- Projection/order/group: `selectClockText()`, `orderByClockTextAsc()`, `orderByClockTextDesc()`, `groupByClockText()`.
- Group/count: `groupByClockText()`, `groupByClockTextAs(alias)`, `groupByClockTextWithFunction(alias, function)`, `countClockText()` and `countClockTextAs(alias)`.
- Scalar aggregate: `sumClockText()` uses alias `"sum_clock_text"`; `sumClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgClockText()` uses alias `"avg_clock_text"`; `avgClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minClockText()` uses alias `"min_clock_text"`; `minClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxClockText()` uses alias `"max_clock_text"`; `maxClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withClockTextContaining(text)`, `withClockTextStartingWith(text)`, `withClockTextEndingWith(text)`.

Null alias: `withClockTextIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withClockTextIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withClockTextNotContaining(value)`.

Text predicate: `withClockTextNotStartingWith(value)`.

Text predicate: `withClockTextNotEndingWith(value)`.

Text predicate: `withClockTextSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
