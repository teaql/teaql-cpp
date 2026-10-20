# Field Assist: person.nickname

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `nickname()` / `updateNickname(const std::optional<String>&)`; changed `changedNickname()`; evaluation `evalNickname()`; expression `getNickname()`.
- Candidate filters: `whoseNicknameIsString()` and `whoseNicknameIsNotString()`; exact value `string()`.
- Scalar predicates: `whoseNicknameIs`, `whoseNicknameIsNot`, `whoseNicknameGreaterThan`, `whoseNicknameGreaterThanOrEqualTo`, `whoseNicknameLessThan`, `whoseNicknameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `whoseNicknameBetween(lower, upper)`, `whoseNicknameIn(std::span<const String>)`, `whoseNicknameNotIn(std::span<const String>)`, `whoseNicknameIsNull()`, `whoseNicknameIsNotNull()`.
- Projection/order/group: `selectNickname()`, `orderByNicknameAsc()`, `orderByNicknameDesc()`, `groupByNickname()`.
- Group/count: `groupByNickname()`, `groupByNicknameAs(alias)`, `groupByNicknameWithFunction(alias, function)`, `countNickname()` and `countNicknameAs(alias)`.
- Scalar aggregate: `sumNickname()` uses alias `"sum_nickname"`; `sumNicknameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNickname()` uses alias `"avg_nickname"`; `avgNicknameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNickname()` uses alias `"min_nickname"`; `minNicknameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNickname()` uses alias `"max_nickname"`; `maxNicknameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `whoseNicknameContains(text)`, `whoseNicknameStartsWith(text)`, `whoseNicknameEndsWith(text)`.

Null alias: `whoseNicknameIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseNicknameIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `whoseNicknameNotContains(value)`.

Text predicate: `whoseNicknameNotStartsWith(value)`.

Text predicate: `whoseNicknameNotEndsWith(value)`.

Text predicate: `whoseNicknameSoundsLike(value)`.

[Model/action index](../../../ASSIST.md)
