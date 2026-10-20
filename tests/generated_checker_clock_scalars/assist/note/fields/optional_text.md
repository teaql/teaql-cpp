# Field Assist: note.optional_text

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalText()` / `updateOptionalText(const std::optional<String>&)`; changed `changedOptionalText()`; evaluation `evalOptionalText()`; expression `getOptionalText()`.
- Candidate filters: `withOptionalTextIsStringCreateFunctionNow()` and `withOptionalTextIsNotStringCreateFunctionNow()`; exact value `string(createFunction=now`.
- Candidate filters: `withOptionalTextIsUpdateFunctionNow()` and `withOptionalTextIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalTextIs`, `withOptionalTextIsNot`, `withOptionalTextGreaterThan`, `withOptionalTextGreaterThanOrEqualTo`, `withOptionalTextLessThan`, `withOptionalTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalTextBetween(lower, upper)`, `withOptionalTextIn(std::span<const String>)`, `withOptionalTextNotIn(std::span<const String>)`, `withOptionalTextIsNull()`, `withOptionalTextIsNotNull()`.
- Projection/order/group: `selectOptionalText()`, `orderByOptionalTextAsc()`, `orderByOptionalTextDesc()`, `groupByOptionalText()`.
- Group/count: `groupByOptionalText()`, `groupByOptionalTextAs(alias)`, `groupByOptionalTextWithFunction(alias, function)`, `countOptionalText()` and `countOptionalTextAs(alias)`.
- Scalar aggregate: `sumOptionalText()` uses alias `"sum_optional_text"`; `sumOptionalTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalText()` uses alias `"avg_optional_text"`; `avgOptionalTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalText()` uses alias `"min_optional_text"`; `minOptionalTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalText()` uses alias `"max_optional_text"`; `maxOptionalTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalTextContaining(text)`, `withOptionalTextStartingWith(text)`, `withOptionalTextEndingWith(text)`.

Null alias: `withOptionalTextIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalTextIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withOptionalTextNotContaining(value)`.

Text predicate: `withOptionalTextNotStartingWith(value)`.

Text predicate: `withOptionalTextNotEndingWith(value)`.

Text predicate: `withOptionalTextSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
