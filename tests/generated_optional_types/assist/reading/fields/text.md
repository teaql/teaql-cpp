# Field Assist: reading.text

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `text()` / `updateText(const std::optional<String>&)`; changed `changedText()`; evaluation `evalText()`; expression `getText()`.
- Candidate filters: `withTextIsString()` and `withTextIsNotString()`; exact value `string()`.
- Scalar predicates: `withTextIs`, `withTextIsNot`, `withTextGreaterThan`, `withTextGreaterThanOrEqualTo`, `withTextLessThan`, `withTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withTextBetween(lower, upper)`, `withTextIn(std::span<const String>)`, `withTextNotIn(std::span<const String>)`, `withTextIsNull()`, `withTextIsNotNull()`.
- Projection/order/group: `selectText()`, `orderByTextAsc()`, `orderByTextDesc()`, `groupByText()`.
- Group/count: `groupByText()`, `groupByTextAs(alias)`, `groupByTextWithFunction(alias, function)`, `countText()` and `countTextAs(alias)`.
- Scalar aggregate: `sumText()` uses alias `"sum_text"`; `sumTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgText()` uses alias `"avg_text"`; `avgTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minText()` uses alias `"min_text"`; `minTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxText()` uses alias `"max_text"`; `maxTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withTextContaining(text)`, `withTextStartingWith(text)`, `withTextEndingWith(text)`.

Null alias: `withTextIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withTextIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withTextNotContaining(value)`.

Text predicate: `withTextNotStartingWith(value)`.

Text predicate: `withTextNotEndingWith(value)`.

Text predicate: `withTextSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
