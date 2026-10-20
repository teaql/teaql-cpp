# Field Assist: note.optional_code

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalCode()` / `updateOptionalCode(const std::optional<String>&)`; changed `changedOptionalCode()`; evaluation `evalOptionalCode()`; expression `getOptionalCode()`.
- Candidate filters: `withOptionalCodeIsUniStrCreateFunctionNow()` and `withOptionalCodeIsNotUniStrCreateFunctionNow()`; exact value `uniStr(createFunction=now`.
- Candidate filters: `withOptionalCodeIsUpdateFunctionNow()` and `withOptionalCodeIsNotUpdateFunctionNow()`; exact value `updateFunction=now)`.
- Scalar predicates: `withOptionalCodeIs`, `withOptionalCodeIsNot`, `withOptionalCodeGreaterThan`, `withOptionalCodeGreaterThanOrEqualTo`, `withOptionalCodeLessThan`, `withOptionalCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalCodeBetween(lower, upper)`, `withOptionalCodeIn(std::span<const String>)`, `withOptionalCodeNotIn(std::span<const String>)`, `withOptionalCodeIsNull()`, `withOptionalCodeIsNotNull()`.
- Projection/order/group: `selectOptionalCode()`, `orderByOptionalCodeAsc()`, `orderByOptionalCodeDesc()`, `groupByOptionalCode()`.
- Group/count: `groupByOptionalCode()`, `groupByOptionalCodeAs(alias)`, `groupByOptionalCodeWithFunction(alias, function)`, `countOptionalCode()` and `countOptionalCodeAs(alias)`.
- Scalar aggregate: `sumOptionalCode()` uses alias `"sum_optional_code"`; `sumOptionalCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalCode()` uses alias `"avg_optional_code"`; `avgOptionalCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalCode()` uses alias `"min_optional_code"`; `minOptionalCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalCode()` uses alias `"max_optional_code"`; `maxOptionalCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalCodeContaining(text)`, `withOptionalCodeStartingWith(text)`, `withOptionalCodeEndingWith(text)`.

Null alias: `withOptionalCodeIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalCodeIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withOptionalCodeNotContaining(value)`.

Text predicate: `withOptionalCodeNotStartingWith(value)`.

Text predicate: `withOptionalCodeNotEndingWith(value)`.

Text predicate: `withOptionalCodeSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
