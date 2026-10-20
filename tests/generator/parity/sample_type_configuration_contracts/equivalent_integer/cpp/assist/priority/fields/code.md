# Field Assist: priority.code

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `code()` / `updateCode(std::string_view)`; changed `changedCode()`; evaluation `evalCode()`; expression `getCode()`.
- Candidate filters: `withCodeIsBasiC()` and `withCodeIsNotBasiC()`; exact value `BASIC`.
- Candidate filters: `withCodeIsExtendeD()` and `withCodeIsNotExtendeD()`; exact value `EXTENDED`.
- Scalar predicates: `withCodeIs`, `withCodeIsNot`, `withCodeGreaterThan`, `withCodeGreaterThanOrEqualTo`, `withCodeLessThan`, `withCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withCodeBetween(lower, upper)`, `withCodeIn(std::span<const String>)`, `withCodeNotIn(std::span<const String>)`, `withCodeIsNull()`, `withCodeIsNotNull()`.
- Projection/order/group: `selectCode()`, `orderByCodeAsc()`, `orderByCodeDesc()`, `groupByCode()`.
- Group/count: `groupByCode()`, `groupByCodeAs(alias)`, `groupByCodeWithFunction(alias, function)`, `countCode()` and `countCodeAs(alias)`.
- Scalar aggregate: `sumCode()` uses alias `"sum_code"`; `sumCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCode()` uses alias `"avg_code"`; `avgCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCode()` uses alias `"min_code"`; `minCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCode()` uses alias `"max_code"`; `maxCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withCodeContaining(text)`, `withCodeStartingWith(text)`, `withCodeEndingWith(text)`.

Null alias: `withCodeIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withCodeIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withCodeNotContaining(value)`.

Text predicate: `withCodeNotStartingWith(value)`.

Text predicate: `withCodeNotEndingWith(value)`.

Text predicate: `withCodeSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
